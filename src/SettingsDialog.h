#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include "ConfigManager.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    void setConfig(const AlertConfig& config);
    AlertConfig getConfig() const;

private slots:
    void onCpuSliderChanged(int value);
    void onMemorySliderChanged(int value);
    void onCpuSpinBoxChanged(int value);
    void onMemorySpinBoxChanged(int value);
    void onRefreshIntervalChanged(int value);

private:
    void setupUI();
    void updateLabels();

    QGroupBox* m_cpuGroup;
    QGroupBox* m_memoryGroup;
    QGroupBox* m_generalGroup;

    QCheckBox* m_cpuAlertEnabled;
    QSlider* m_cpuThresholdSlider;
    QSpinBox* m_cpuThresholdSpinBox;
    QLabel* m_cpuValueLabel;

    QCheckBox* m_memoryAlertEnabled;
    QSlider* m_memoryThresholdSlider;
    QSpinBox* m_memoryThresholdSpinBox;
    QLabel* m_memoryValueLabel;

    QSpinBox* m_refreshIntervalSpinBox;
    QLabel* m_refreshIntervalLabel;

    QDialogButtonBox* m_buttonBox;

    AlertConfig m_config;
    bool m_updatingUI;
};

#endif // SETTINGSDIALOG_H
