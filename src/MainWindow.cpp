#include "MainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QGridLayout>
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

#include <algorithm>
#include <filesystem>

#include "BenchmarkRunner.hpp"
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

    auto* benchmarkBox = new QGroupBox(QStringLiteral("Benchmark (Short Synthetic Test)"), this);
    auto* benchmarkLayout = new QVBoxLayout;

    auto* benchmarkButtons = new QHBoxLayout;
    runBaselineButton_ = new QPushButton(QStringLiteral("Run Baseline Benchmark"), this);
    runCurrentButton_ = new QPushButton(QStringLiteral("Run Current Benchmark"), this);
    benchmarkButtons->addWidget(runBaselineButton_);
    benchmarkButtons->addWidget(runCurrentButton_);
    benchmarkLayout->addLayout(benchmarkButtons);

    auto* grid = new QGridLayout;
    grid->addWidget(new QLabel(QStringLiteral("CPU Baseline:"), this), 0, 0);
    cpuBaselineLabel_ = new QLabel(QStringLiteral("N/A"), this);
    grid->addWidget(cpuBaselineLabel_, 0, 1);
    grid->addWidget(new QLabel(QStringLiteral("CPU Current:"), this), 1, 0);
    cpuCurrentLabel_ = new QLabel(QStringLiteral("N/A"), this);
    grid->addWidget(cpuCurrentLabel_, 1, 1);
    grid->addWidget(new QLabel(QStringLiteral("CPU Expected:"), this), 2, 0);
    cpuExpectedLabel_ = new QLabel(QStringLiteral("N/A"), this);
    grid->addWidget(cpuExpectedLabel_, 2, 1);

    grid->addWidget(new QLabel(QStringLiteral("GPU Baseline:"), this), 0, 2);
    gpuBaselineLabel_ = new QLabel(QStringLiteral("N/A"), this);
    grid->addWidget(gpuBaselineLabel_, 0, 3);
    grid->addWidget(new QLabel(QStringLiteral("GPU Current:"), this), 1, 2);
    gpuCurrentLabel_ = new QLabel(QStringLiteral("N/A"), this);
    grid->addWidget(gpuCurrentLabel_, 1, 3);
    grid->addWidget(new QLabel(QStringLiteral("GPU Expected:"), this), 2, 2);
    gpuExpectedLabel_ = new QLabel(QStringLiteral("N/A"), this);
    grid->addWidget(gpuExpectedLabel_, 2, 3);

    benchmarkLayout->addLayout(grid);
    benchmarkBox->setLayout(benchmarkLayout);
    mainLayout->addWidget(benchmarkBox);

    central->setLayout(mainLayout);
    setCentralWidget(central);
    statusBar()->showMessage(QStringLiteral("Initializing..."));

    connect(cpuList_, &QListWidget::currentRowChanged, this, &MainWindow::HandleCpuSelection);
    connect(gpuList_, &QListWidget::currentRowChanged, this, &MainWindow::HandleGpuSelection);
    connect(applyCpuButton_, &QPushButton::clicked, this, &MainWindow::ApplyCpuTarget);
    connect(applyGpuButton_, &QPushButton::clicked, this, &MainWindow::ApplyGpuTarget);
    connect(restoreButton_, &QPushButton::clicked, this, &MainWindow::RestoreDefaults);
    connect(runBaselineButton_, &QPushButton::clicked, this, &MainWindow::RunBaselineBenchmark);
    connect(runCurrentButton_, &QPushButton::clicked, this, &MainWindow::RunCurrentBenchmark);

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
    state_.cpuNominalFrequencyMHz = state_.engine.CpuNominalFrequencyMHz();
    state_.gpuNominalClockMHz = state_.engine.GpuNominalFrequencyMHz();
    state_.gpuNominalPowerWatts = state_.engine.GpuNominalPowerWatts();
    state_.initialized = true;

    PopulateLists();
    UpdateSnapshotLabel();
    UpdateButtonStates();
    UpdateBenchmarkLabels();
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
    UpdateBenchmarkLabels();
}

void MainWindow::HandleGpuSelection(int row) {
    if (row >= 0 && row < static_cast<int>(state_.gpuOptions.size())) {
        state_.selectedGpu = state_.gpuOptions[static_cast<size_t>(row)];
    } else {
        state_.selectedGpu.reset();
    }
    UpdateButtonStates();
    UpdateBenchmarkLabels();
}

void MainWindow::ApplyCpuTarget() {
    if (!state_.selectedCpu) {
        UpdateStatus(QStringLiteral("Select a CPU target first"));
        return;
    }
    if (state_.selectedCpu->requiresConfirmation) {
        const auto label = QString::fromStdString(state_.selectedCpu->label);
        if (!ConfirmHighImpact(label)) {
            UpdateStatus(QStringLiteral("Action cancelled by user"));
            return;
        }
    }
    auto result = state_.throttler.ApplyCpuTarget(*state_.selectedCpu);
    UpdateStatus(QString::fromWCharArray(result.message.c_str()));
}

void MainWindow::ApplyGpuTarget() {
    if (!state_.selectedGpu) {
        UpdateStatus(QStringLiteral("Select a GPU target first"));
        return;
    }
    if (state_.selectedGpu->requiresConfirmation) {
        const auto label = QString::fromStdString(state_.selectedGpu->label);
        if (!ConfirmHighImpact(label)) {
            UpdateStatus(QStringLiteral("Action cancelled by user"));
            return;
        }
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

void MainWindow::RunBaselineBenchmark() {
    RunBenchmark(true);
}

void MainWindow::RunCurrentBenchmark() {
    RunBenchmark(false);
}

void MainWindow::RunBenchmark(bool baseline) {
    BenchmarkRunner runner;
    QApplication::setOverrideCursor(Qt::BusyCursor);
    UpdateStatus(baseline ? QStringLiteral("Running baseline benchmark...")
                          : QStringLiteral("Running current benchmark..."));
    const auto report = runner.Run(state_.snapshot);
    QApplication::restoreOverrideCursor();

    if (baseline) {
        state_.benchmark.baselineCpu = report.cpu;
        state_.benchmark.baselineGpu = report.gpu;
    } else {
        state_.benchmark.currentCpu = report.cpu;
        state_.benchmark.currentGpu = report.gpu;
    }
    UpdateBenchmarkLabels();
    UpdateStatus(QStringLiteral("Benchmark complete"));
}

QString MainWindow::FormatScoreLabel(const std::optional<BenchmarkResultData>& data) const {
    if (!data) {
        return QStringLiteral("N/A");
    }
    return QString::number(data->score, 'f', 2) + QStringLiteral(" ") +
           QString::fromStdString(data->unit);
}

void MainWindow::UpdateBenchmarkLabels() {
    cpuBaselineLabel_->setText(FormatScoreLabel(state_.benchmark.baselineCpu));
    cpuBaselineLabel_->setToolTip(state_.benchmark.baselineCpu
                                      ? QString::fromStdString(state_.benchmark.baselineCpu->details)
                                      : QString());
    cpuCurrentLabel_->setText(FormatScoreLabel(state_.benchmark.currentCpu));
    cpuCurrentLabel_->setToolTip(state_.benchmark.currentCpu
                                     ? QString::fromStdString(state_.benchmark.currentCpu->details)
                                     : QString());

    auto cpuExpected = ComputeExpectedCpuScore();
    if (cpuExpected && state_.benchmark.baselineCpu) {
        cpuExpectedLabel_->setText(QString::number(*cpuExpected, 'f', 2) + QStringLiteral(" ") +
                                   QString::fromStdString(state_.benchmark.baselineCpu->unit));
    } else {
        cpuExpectedLabel_->setText(QStringLiteral("N/A"));
    }

    gpuBaselineLabel_->setText(FormatScoreLabel(state_.benchmark.baselineGpu));
    gpuBaselineLabel_->setToolTip(state_.benchmark.baselineGpu
                                      ? QString::fromStdString(state_.benchmark.baselineGpu->details)
                                      : QString());
    gpuCurrentLabel_->setText(FormatScoreLabel(state_.benchmark.currentGpu));
    gpuCurrentLabel_->setToolTip(state_.benchmark.currentGpu
                                     ? QString::fromStdString(state_.benchmark.currentGpu->details)
                                     : QString());

    auto gpuExpected = ComputeExpectedGpuScore();
    if (gpuExpected && state_.benchmark.baselineGpu) {
        gpuExpectedLabel_->setText(QString::number(*gpuExpected, 'f', 2) + QStringLiteral(" ") +
                                   QString::fromStdString(state_.benchmark.baselineGpu->unit));
    } else {
        gpuExpectedLabel_->setText(QStringLiteral("N/A"));
    }
}

std::optional<double> MainWindow::ComputeExpectedCpuScore() const {
    if (!state_.benchmark.baselineCpu || !state_.selectedCpu ||
        state_.benchmark.baselineCpu->score <= 0.0) {
        return std::nullopt;
    }
    const double base = state_.benchmark.baselineCpu->score;
    const double percent = state_.selectedCpu->maxPercent > 0
                               ? static_cast<double>(state_.selectedCpu->maxPercent) / 100.0
                               : 1.0;
    double freqFactor = 1.0;
    if (state_.cpuNominalFrequencyMHz > 0 && state_.selectedCpu->maxFrequencyMHz > 0) {
        freqFactor = static_cast<double>(state_.selectedCpu->maxFrequencyMHz) /
                     static_cast<double>(state_.cpuNominalFrequencyMHz);
    }
    const double factor = std::clamp(std::min(percent, freqFactor), 0.05, 1.0);
    return base * factor;
}

std::optional<double> MainWindow::ComputeExpectedGpuScore() const {
    if (!state_.benchmark.baselineGpu || !state_.selectedGpu ||
        state_.benchmark.baselineGpu->score <= 0.0) {
        return std::nullopt;
    }
    double freqFactor = 1.0;
    if (state_.gpuNominalClockMHz > 0 && state_.selectedGpu->maxFrequencyMHz > 0) {
        freqFactor = static_cast<double>(state_.selectedGpu->maxFrequencyMHz) /
                     static_cast<double>(state_.gpuNominalClockMHz);
    }
    double powerFactor = 1.0;
    if (state_.gpuNominalPowerWatts > 0 && state_.selectedGpu->powerLimitWatts > 0) {
        powerFactor = static_cast<double>(state_.selectedGpu->powerLimitWatts) /
                      static_cast<double>(state_.gpuNominalPowerWatts);
    }
    const double factor = std::clamp(std::min(freqFactor, powerFactor), 0.05, 1.0);
    return state_.benchmark.baselineGpu->score * factor;
}

bool MainWindow::ConfirmHighImpact(const QString& targetLabel) const {
    const QString text = tr(
        "Applying \"%1\" will enforce an aggressive limit that may impact stability or cooling.\n\n"
        "Proceed only if you understand the risks. All responsibility lies with the operator.\n\n"
        "Continue?")
        .arg(targetLabel);
    const auto choice = QMessageBox::warning(
        const_cast<MainWindow*>(this),
        tr("High Impact Throttle"),
        text,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    return choice == QMessageBox::Yes;
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
