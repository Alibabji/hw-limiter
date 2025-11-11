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

private:
    void InitializeState();
    void PopulateLists();
    void UpdateSnapshotLabel();
    void UpdateStatus(const QString& text);
    void UpdateButtonStates();
    std::filesystem::path ResolveProfilesPath() const;

    AppState state_;
    QListWidget* cpuList_ = nullptr;
    QListWidget* gpuList_ = nullptr;
    QLabel* snapshotLabel_ = nullptr;
    QPushButton* applyCpuButton_ = nullptr;
    QPushButton* applyGpuButton_ = nullptr;
    QPushButton* restoreButton_ = nullptr;
};

