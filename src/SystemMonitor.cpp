#include "SystemMonitor.h"
#include <QCloseEvent>
#include <QMessageBox>
#include <QSplitter>
#include <QDateTime>
#include <QFont>
#include <QPalette>

SystemMonitor::SystemMonitor(QWidget *parent)
    : QMainWindow(parent)
    , m_systemInfo(new SystemInfo(this))
    , m_configManager(new ConfigManager(this))
    , m_settingsDialog(new SettingsDialog(this))
    , m_refreshTimer(new QTimer(this))
    , m_cpuAlertTriggered(false)
    , m_memoryAlertTriggered(false)
{
    setWindowTitle(tr("System Resource Monitor"));
    setMinimumSize(900, 600);
    resize(1024, 768);

    setupUI();
    setupMenuBar();
    setupStatusBar();

    loadConfiguration();

    connect(m_refreshTimer, &QTimer::timeout, this, &SystemMonitor::onRefreshData);
    m_refreshTimer->start(m_alertConfig.refreshInterval);

    onRefreshData();
}

SystemMonitor::~SystemMonitor()
{
}

void SystemMonitor::setupUI()
{
    m_centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(false);
    m_tabWidget->setMovable(false);

    createOverviewTab();
    createProcessTab();

    mainLayout->addWidget(m_tabWidget, 1);
    setCentralWidget(m_centralWidget);
}

void SystemMonitor::setupMenuBar()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* settingsAction = new QAction(tr("&Settings..."), this);
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &SystemMonitor::onSettingsClicked);
    fileMenu->addAction(settingsAction);

    fileMenu->addSeparator();

    QAction* exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &SystemMonitor::onExitClicked);
    fileMenu->addAction(exitAction);

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction* aboutAction = new QAction(tr("&About"), this);
    connect(aboutAction, &QAction::triggered, this, &SystemMonitor::onAboutClicked);
    helpMenu->addAction(aboutAction);
}

void SystemMonitor::setupStatusBar()
{
    m_statusBar = statusBar();
    m_statusBar->setSizeGripEnabled(true);

    m_statusCpuLabel = new QLabel(tr("CPU: 0%"), this);
    m_statusCpuLabel->setMinimumWidth(100);

    m_statusMemoryLabel = new QLabel(tr("Memory: 0%"), this);
    m_statusMemoryLabel->setMinimumWidth(120);

    m_statusDiskLabel = new QLabel(tr("Disk: 0%"), this);
    m_statusDiskLabel->setMinimumWidth(120);

    m_statusNetworkLabel = new QLabel(tr("Net: ↑0 KB/s ↓0 KB/s"), this);
    m_statusNetworkLabel->setMinimumWidth(180);

    m_statusTimeLabel = new QLabel(tr("Last update: --:--:--"), this);
    m_statusTimeLabel->setMinimumWidth(150);
    m_statusTimeLabel->setAlignment(Qt::AlignRight);

    m_statusBar->addWidget(m_statusCpuLabel);
    m_statusBar->addWidget(m_statusMemoryLabel);
    m_statusBar->addWidget(m_statusDiskLabel);
    m_statusBar->addWidget(m_statusNetworkLabel);
    m_statusBar->addPermanentWidget(m_statusTimeLabel);
}

void SystemMonitor::createOverviewTab()
{
    QWidget* overviewWidget = new QWidget(this);
    QVBoxLayout* overviewLayout = new QVBoxLayout(overviewWidget);
    overviewLayout->setContentsMargins(5, 5, 5, 5);
    overviewLayout->setSpacing(10);

    QGroupBox* summaryGroup = new QGroupBox(tr("System Overview"), this);
    QGridLayout* summaryLayout = new QGridLayout(summaryGroup);
    summaryLayout->setContentsMargins(10, 10, 10, 10);
    summaryLayout->setSpacing(15);

    QFont labelFont;
    labelFont.setBold(true);
    labelFont.setPointSize(11);

    QLabel* cpuTitleLabel = new QLabel(tr("CPU Usage:"), this);
    cpuTitleLabel->setFont(labelFont);
    m_overviewCpuLabel = new QLabel(tr("0.0%"), this);
    m_overviewCpuLabel->setFont(labelFont);
    m_overviewCpuLabel->setAlignment(Qt::AlignRight);
    m_overviewCpuLabel->setMinimumWidth(80);
    summaryLayout->addWidget(cpuTitleLabel, 0, 0);
    summaryLayout->addWidget(m_overviewCpuLabel, 0, 1);

    QLabel* memoryTitleLabel = new QLabel(tr("Memory Usage:"), this);
    memoryTitleLabel->setFont(labelFont);
    m_overviewMemoryLabel = new QLabel(tr("0.0%"), this);
    m_overviewMemoryLabel->setFont(labelFont);
    m_overviewMemoryLabel->setAlignment(Qt::AlignRight);
    m_overviewMemoryLabel->setMinimumWidth(80);
    summaryLayout->addWidget(memoryTitleLabel, 0, 2);
    summaryLayout->addWidget(m_overviewMemoryLabel, 0, 3);

    QLabel* diskTitleLabel = new QLabel(tr("Disk Usage:"), this);
    diskTitleLabel->setFont(labelFont);
    m_overviewDiskLabel = new QLabel(tr("0.0%"), this);
    m_overviewDiskLabel->setFont(labelFont);
    m_overviewDiskLabel->setAlignment(Qt::AlignRight);
    m_overviewDiskLabel->setMinimumWidth(80);
    summaryLayout->addWidget(diskTitleLabel, 1, 0);
    summaryLayout->addWidget(m_overviewDiskLabel, 1, 1);

    QLabel* networkTitleLabel = new QLabel(tr("Network Speed:"), this);
    networkTitleLabel->setFont(labelFont);
    m_overviewNetworkLabel = new QLabel(tr("↑ 0 KB/s  ↓ 0 KB/s"), this);
    m_overviewNetworkLabel->setFont(labelFont);
    m_overviewNetworkLabel->setAlignment(Qt::AlignRight);
    summaryLayout->addWidget(networkTitleLabel, 1, 2);
    summaryLayout->addWidget(m_overviewNetworkLabel, 1, 3);

    overviewLayout->addWidget(summaryGroup);

    QGroupBox* chartsGroup = new QGroupBox(tr("Trend Charts (Last 60 Seconds)"), this);
    QGridLayout* chartsLayout = new QGridLayout(chartsGroup);
    chartsLayout->setContentsMargins(10, 10, 10, 10);
    chartsLayout->setSpacing(10);

    m_cpuChart = new TrendChart(TrendChart::CPUChart, this);
    m_memoryChart = new TrendChart(TrendChart::MemoryChart, this);
    m_uploadChart = new TrendChart(TrendChart::NetworkUploadChart, this);
    m_downloadChart = new TrendChart(TrendChart::NetworkDownloadChart, this);

    chartsLayout->addWidget(m_cpuChart, 0, 0);
    chartsLayout->addWidget(m_memoryChart, 0, 1);
    chartsLayout->addWidget(m_uploadChart, 1, 0);
    chartsLayout->addWidget(m_downloadChart, 1, 1);

    overviewLayout->addWidget(chartsGroup, 1);

    m_tabWidget->addTab(overviewWidget, tr("Overview"));
}

void SystemMonitor::createProcessTab()
{
    QWidget* processWidget = new QWidget(this);
    QVBoxLayout* processLayout = new QVBoxLayout(processWidget);
    processLayout->setContentsMargins(5, 5, 5, 5);
    processLayout->setSpacing(5);

    m_processList = new ProcessList(this);
    processLayout->addWidget(m_processList, 1);

    m_tabWidget->addTab(processWidget, tr("Processes"));
}

void SystemMonitor::loadConfiguration()
{
    m_configManager->loadConfig();
    m_alertConfig = m_configManager->getAlertConfig();
    m_settingsDialog->setConfig(m_alertConfig);
}

void SystemMonitor::saveConfiguration()
{
    m_configManager->setAlertConfig(m_alertConfig);
    m_configManager->saveConfig();
}

void SystemMonitor::onRefreshData()
{
    m_currentData = m_systemInfo->getSystemData();

    m_cpuChart->addDataPoint(m_currentData.cpuUsage);
    m_memoryChart->addDataPoint(m_currentData.memoryUsage);
    m_uploadChart->addDataPoint(m_currentData.uploadSpeed);
    m_downloadChart->addDataPoint(m_currentData.downloadSpeed);

    m_processList->updateProcessList(m_currentData.processes);

    updateStatusBar();
    checkAlerts(m_currentData);
}

void SystemMonitor::updateStatusBar()
{
    QString cpuValue = QString("%1%").arg(m_currentData.cpuUsage, 0, 'f', 1);
    QString memoryValue = QString("%1%").arg(m_currentData.memoryUsage, 0, 'f', 1);
    QString diskValue = QString("%1%").arg(m_currentData.diskUsage, 0, 'f', 1);
    QString networkText = QString(tr("↑ %1  ↓ %2"))
        .arg(formatBytes(static_cast<quint64>(m_currentData.uploadSpeed * 1024)) + "/s")
        .arg(formatBytes(static_cast<quint64>(m_currentData.downloadSpeed * 1024)) + "/s");

    QString statusCpuText = QString(tr("CPU: %1")).arg(cpuValue);
    QString statusMemoryText = QString(tr("Memory: %1")).arg(memoryValue);
    QString statusDiskText = QString(tr("Disk: %1")).arg(diskValue);

    m_statusCpuLabel->setText(statusCpuText);
    m_statusMemoryLabel->setText(statusMemoryText);
    m_statusDiskLabel->setText(statusDiskText);
    m_statusNetworkLabel->setText(networkText);

    m_overviewCpuLabel->setText(cpuValue);
    m_overviewMemoryLabel->setText(memoryValue);
    m_overviewDiskLabel->setText(diskValue);
    m_overviewNetworkLabel->setText(networkText);

    QString cpuStyle = "";
    QString memoryStyle = "";

    if (m_currentData.cpuUsage > 50.0) {
        cpuStyle = "color: orange; font-weight: bold;";
    } else if (m_currentData.cpuUsage > 80.0) {
        cpuStyle = "color: red; font-weight: bold;";
    }

    if (m_currentData.memoryUsage > 70.0) {
        memoryStyle = "color: orange; font-weight: bold;";
    } else if (m_currentData.memoryUsage > 85.0) {
        memoryStyle = "color: red; font-weight: bold;";
    }

    m_statusCpuLabel->setStyleSheet(cpuStyle);
    m_overviewCpuLabel->setStyleSheet(cpuStyle);

    m_statusMemoryLabel->setStyleSheet(memoryStyle);
    m_overviewMemoryLabel->setStyleSheet(memoryStyle);

    QString timeText = tr("Last update: %1")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"));
    m_statusTimeLabel->setText(timeText);
}

QString SystemMonitor::formatBytes(quint64 bytes)
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;

    if (bytes >= GB) {
        return QString("%1 GB").arg(bytes / GB, 0, 'f', 2);
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(bytes / MB, 0, 'f', 2);
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(bytes / KB, 0, 'f', 1);
    } else {
        return QString("%1 B").arg(bytes);
    }
}

void SystemMonitor::checkAlerts(const SystemData& data)
{
    if (m_alertConfig.cpuAlertEnabled) {
        if (data.cpuUsage >= m_alertConfig.cpuThreshold) {
            if (!m_cpuAlertTriggered) {
                m_cpuAlertTriggered = true;
                QMessageBox::warning(this,
                    tr("CPU Usage Alert"),
                    QString(tr("CPU usage has exceeded the threshold of %1%\n"
                              "Current CPU usage: %2%"))
                        .arg(m_alertConfig.cpuThreshold)
                        .arg(data.cpuUsage, 0, 'f', 1),
                    QMessageBox::Ok);
            }
        } else {
            m_cpuAlertTriggered = false;
        }
    }

    if (m_alertConfig.memoryAlertEnabled) {
        if (data.memoryUsage >= m_alertConfig.memoryThreshold) {
            if (!m_memoryAlertTriggered) {
                m_memoryAlertTriggered = true;
                QMessageBox::warning(this,
                    tr("Memory Usage Alert"),
                    QString(tr("Memory usage has exceeded the threshold of %1%\n"
                              "Current memory usage: %2%"))
                        .arg(m_alertConfig.memoryThreshold)
                        .arg(data.memoryUsage, 0, 'f', 1),
                    QMessageBox::Ok);
            }
        } else {
            m_memoryAlertTriggered = false;
        }
    }
}

void SystemMonitor::onSettingsClicked()
{
    m_settingsDialog->setConfig(m_alertConfig);

    if (m_settingsDialog->exec() == QDialog::Accepted) {
        AlertConfig newConfig = m_settingsDialog->getConfig();

        if (newConfig.refreshInterval != m_alertConfig.refreshInterval) {
            m_refreshTimer->setInterval(newConfig.refreshInterval);
        }

        m_alertConfig = newConfig;
        saveConfiguration();
    }
}

void SystemMonitor::onExitClicked()
{
    close();
}

void SystemMonitor::onAboutClicked()
{
    QMessageBox::about(this,
        tr("About System Monitor"),
        tr("System Resource Monitor v1.0.0\n\n"
           "A lightweight desktop application for monitoring system resources.\n\n"
           "Features:\n"
           "- Real-time CPU usage monitoring\n"
           "- Memory usage tracking\n"
           "- Disk space information\n"
           "- Network upload/download speed\n"
           "- Process list with sorting and search\n"
           "- Custom alert thresholds\n"
           "- 60-second trend charts\n\n"
           "Built with Qt5 Widgets and C++17\n"
           "Configuration file: %1")
            .arg(m_configManager->getConfigFilePath()));
}

void SystemMonitor::closeEvent(QCloseEvent* event)
{
    saveConfiguration();
    m_refreshTimer->stop();
    QMainWindow::closeEvent(event);
}
