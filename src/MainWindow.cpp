#include "MainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>

#include <filesystem>

#include "HardwareInfo.hpp"
#include "ProfileEngine.hpp"
#include "ProfileLoader.hpp"

namespace {

QString JoinGpuNames(const std::vector<GpuInfo>& gpus) {
    if (gpus.empty()) {
        return QStringLiteral("No discrete GPU detected");
    }
    QStringList names;
    for (const auto& gpu : gpus) {
        names << QString::fromWCharArray(gpu.name.c_str());
    }
    return names.join(QStringLiteral(" • "));
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Hardware Limiter"));
    resize(820, 520);

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout;

    snapshotLabel_ = new QLabel(QStringLiteral("Detecting hardware..."), this);
    snapshotLabel_->setWordWrap(true);
    mainLayout->addWidget(snapshotLabel_);

    auto* listsLayout = new QHBoxLayout;
    listsLayout->setSpacing(16);

    auto* cpuBox = new QGroupBox(QStringLiteral("CPU Downgrade Targets"), this);
    auto* cpuLayout = new QVBoxLayout;
    cpuList_ = new QListWidget(this);
    cpuLayout->addWidget(cpuList_);
    auto* cpuButtonRow = new QHBoxLayout;
    applyCpuButton_ = new QPushButton(QStringLiteral("Apply CPU Target"), this);
    cpuButtonRow->addStretch();
    cpuButtonRow->addWidget(applyCpuButton_);
    cpuLayout->addLayout(cpuButtonRow);
    cpuBox->setLayout(cpuLayout);
    listsLayout->addWidget(cpuBox);

    auto* gpuBox = new QGroupBox(QStringLiteral("GPU Downgrade Targets"), this);
    auto* gpuLayout = new QVBoxLayout;
    gpuList_ = new QListWidget(this);
    gpuLayout->addWidget(gpuList_);
    auto* gpuButtonRow = new QHBoxLayout;
    applyGpuButton_ = new QPushButton(QStringLiteral("Apply GPU Target"), this);
    gpuButtonRow->addStretch();
    gpuButtonRow->addWidget(applyGpuButton_);
    gpuLayout->addLayout(gpuButtonRow);
    gpuBox->setLayout(gpuLayout);
    listsLayout->addWidget(gpuBox);

    mainLayout->addLayout(listsLayout);

    auto* restoreRow = new QHBoxLayout;
    restoreRow->addStretch();
    restoreButton_ = new QPushButton(QStringLiteral("Restore Defaults"), this);
    restoreRow->addWidget(restoreButton_);
    mainLayout->addLayout(restoreRow);

    central->setLayout(mainLayout);
    setCentralWidget(central);
    statusBar()->showMessage(QStringLiteral("Initializing..."));

    connect(cpuList_, &QListWidget::currentRowChanged, this, &MainWindow::HandleCpuSelection);
    connect(gpuList_, &QListWidget::currentRowChanged, this, &MainWindow::HandleGpuSelection);
    connect(applyCpuButton_, &QPushButton::clicked, this, &MainWindow::ApplyCpuTarget);
    connect(applyGpuButton_, &QPushButton::clicked, this, &MainWindow::ApplyGpuTarget);
    connect(restoreButton_, &QPushButton::clicked, this, &MainWindow::RestoreDefaults);

    InitializeState();
}

void MainWindow::InitializeState() {
    HardwareInfoService infoService;
    state_.snapshot = infoService.QueryHardware();

    ProfileLoader loader;
    std::filesystem::path profilePath = ResolveProfilesPath();
    try {
        state_.profiles = loader.LoadFromFile(profilePath);
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, QStringLiteral("Hardware Limiter"),
                              QStringLiteral("Failed to load profiles:\n%1").arg(ex.what()));
        cpuList_->setEnabled(false);
        gpuList_->setEnabled(false);
        applyCpuButton_->setEnabled(false);
        applyGpuButton_->setEnabled(false);
        restoreButton_->setEnabled(false);
        UpdateStatus(QStringLiteral("Profile load failed"));
        return;
    }

    state_.engine.Refresh(state_.snapshot, state_.profiles);
    state_.cpuOptions = state_.engine.CpuOptions();
    state_.gpuOptions = state_.engine.GpuOptions();
    state_.initialized = true;

    PopulateLists();
    UpdateSnapshotLabel();
    UpdateButtonStates();
    UpdateStatus(QStringLiteral("Ready"));
}

void MainWindow::PopulateLists() {
    cpuList_->clear();
    for (const auto& option : state_.cpuOptions) {
        cpuList_->addItem(QString::fromStdString(option.label));
    }
    gpuList_->clear();
    for (const auto& option : state_.gpuOptions) {
        gpuList_->addItem(QString::fromStdString(option.label));
    }
}

void MainWindow::UpdateSnapshotLabel() {
    const auto& cpu = state_.snapshot.cpu;
    const QString cpuName = cpu.name.empty() ? QStringLiteral("Unknown CPU")
                                             : QString::fromStdString(cpu.name);
    QString text =
        QStringLiteral("<b>CPU:</b> %1 (%2 cores / %3 threads)")
            .arg(cpuName)
            .arg(cpu.physicalCores)
            .arg(cpu.logicalCores);
    text += QStringLiteral("<br/><b>GPU:</b> %1").arg(JoinGpuNames(state_.snapshot.gpus));
    snapshotLabel_->setText(text);
}

void MainWindow::HandleCpuSelection(int row) {
    if (row >= 0 && row < static_cast<int>(state_.cpuOptions.size())) {
        state_.selectedCpu = state_.cpuOptions[static_cast<size_t>(row)];
    } else {
        state_.selectedCpu.reset();
    }
    UpdateButtonStates();
}

void MainWindow::HandleGpuSelection(int row) {
    if (row >= 0 && row < static_cast<int>(state_.gpuOptions.size())) {
        state_.selectedGpu = state_.gpuOptions[static_cast<size_t>(row)];
    } else {
        state_.selectedGpu.reset();
    }
    UpdateButtonStates();
}

void MainWindow::ApplyCpuTarget() {
    if (!state_.selectedCpu) {
        UpdateStatus(QStringLiteral("Select a CPU target first"));
        return;
    }
    auto result = state_.throttler.ApplyCpuTarget(*state_.selectedCpu);
    UpdateStatus(QString::fromWCharArray(result.message.c_str()));
}

void MainWindow::ApplyGpuTarget() {
    if (!state_.selectedGpu) {
        UpdateStatus(QStringLiteral("Select a GPU target first"));
        return;
    }
    auto result = state_.throttler.ApplyGpuTarget(*state_.selectedGpu);
    UpdateStatus(QString::fromWCharArray(result.message.c_str()));
}

void MainWindow::RestoreDefaults() {
    auto result = state_.throttler.RestoreDefaults();
    UpdateStatus(QString::fromWCharArray(result.message.c_str()));
}

void MainWindow::UpdateStatus(const QString& text) {
    if (text.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Ready"));
    } else {
        statusBar()->showMessage(text);
    }
}

void MainWindow::UpdateButtonStates() {
    const bool hasCpu = state_.selectedCpu.has_value();
    const bool hasGpu = state_.selectedGpu.has_value();
    applyCpuButton_->setEnabled(hasCpu);
    applyGpuButton_->setEnabled(hasGpu);
    restoreButton_->setEnabled(state_.initialized);
}

std::filesystem::path MainWindow::ResolveProfilesPath() const {
    std::filesystem::path path;
    const QString appDir = QCoreApplication::applicationDirPath();
#ifdef _WIN32
    path = std::filesystem::path(appDir.toStdWString()) / "profiles.json";
#else
    path = std::filesystem::path(appDir.toStdString()) / "profiles.json";
#endif
    if (std::filesystem::exists(path)) {
        return path;
    }
    std::filesystem::path fallback = std::filesystem::current_path() / "resources" / "profiles.json";
    return fallback;
}
