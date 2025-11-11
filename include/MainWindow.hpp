#pragma once

#include <filesystem>

#include <QMainWindow>

#include "AppState.hpp"

class QListWidget;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void HandleCpuSelection(int row);
    void HandleGpuSelection(int row);
    void ApplyCpuTarget();
    void ApplyGpuTarget();
    void RestoreDefaults();
    void RunBaselineBenchmark();
    void RunCurrentBenchmark();

private:
    void InitializeState();
    void PopulateLists();
    void UpdateSnapshotLabel();
    void UpdateStatus(const QString& text);
    void UpdateButtonStates();
    void RunBenchmark(bool baseline);
    void UpdateBenchmarkLabels();
    std::optional<double> ComputeExpectedCpuScore() const;
    std::optional<double> ComputeExpectedGpuScore() const;
    QString FormatScoreLabel(const std::optional<BenchmarkResultData>& data) const;
    bool ConfirmHighImpact(const QString& targetLabel) const;
    std::filesystem::path ResolveProfilesPath() const;

    AppState state_;
    QListWidget* cpuList_ = nullptr;
    QListWidget* gpuList_ = nullptr;
    QLabel* snapshotLabel_ = nullptr;
    QPushButton* applyCpuButton_ = nullptr;
    QPushButton* applyGpuButton_ = nullptr;
    QPushButton* restoreButton_ = nullptr;
    QPushButton* runBaselineButton_ = nullptr;
    QPushButton* runCurrentButton_ = nullptr;
    QLabel* cpuBaselineLabel_ = nullptr;
    QLabel* cpuCurrentLabel_ = nullptr;
    QLabel* cpuExpectedLabel_ = nullptr;
    QLabel* gpuBaselineLabel_ = nullptr;
    QLabel* gpuCurrentLabel_ = nullptr;
    QLabel* gpuExpectedLabel_ = nullptr;
};
