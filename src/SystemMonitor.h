#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QTimer>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QSplitter>

#include "SystemInfo.h"
#include "ConfigManager.h"
#include "TrendChart.h"
#include "ProcessList.h"
#include "SettingsDialog.h"

class SystemMonitor : public QMainWindow
{
    Q_OBJECT

public:
    explicit SystemMonitor(QWidget *parent = nullptr);
    ~SystemMonitor();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onRefreshData();
    void onSettingsClicked();
    void onExitClicked();
    void onAboutClicked();
    void checkAlerts(const SystemData& data);

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void createOverviewTab();
    void createProcessTab();
    void loadConfiguration();
    void saveConfiguration();
    void updateStatusBar();
    QString formatBytes(quint64 bytes);

    SystemInfo* m_systemInfo;
    ConfigManager* m_configManager;
    SettingsDialog* m_settingsDialog;
    QTimer* m_refreshTimer;

    QWidget* m_centralWidget;
    QTabWidget* m_tabWidget;
    QStatusBar* m_statusBar;

    QLabel* m_cpuLabel;
    QLabel* m_memoryLabel;
    QLabel* m_diskLabel;
    QLabel* m_networkLabel;
    QLabel* m_timeLabel;

    TrendChart* m_cpuChart;
    TrendChart* m_memoryChart;
    TrendChart* m_uploadChart;
    TrendChart* m_downloadChart;

    ProcessList* m_processList;

    AlertConfig m_alertConfig;
    SystemData m_currentData;

    bool m_cpuAlertTriggered;
    bool m_memoryAlertTriggered;
};

#endif // SYSTEMMONITOR_H
