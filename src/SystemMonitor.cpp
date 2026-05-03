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

    m_cpuLabel = new QLabel(tr("CPU: 0%"), this);
    m_cpuLabel->setMinimumWidth(100);

    m_memoryLabel = new QLabel(tr("Memory: 0%"), this);
    m_memoryLabel->setMinimumWidth(120);

    m_diskLabel = new QLabel(tr("Disk: 0%"), this);
    m_diskLabel->setMinimumWidth(120);

    m_networkLabel = new QLabel(tr("Net: ↑0 KB/s ↓0 KB/s"), this);
    m_networkLabel->setMinimumWidth(180);

    m_timeLabel = new QLabel(tr("Last update: --:--:--"), this);
    m_timeLabel->setMinimumWidth(150);
    m_timeLabel->setAlignment(Qt::AlignRight);

    m_statusBar->addWidget(m_cpuLabel);
    m_statusBar->addWidget(m_memoryLabel);
    m_statusBar->addWidget(m_diskLabel);
    m_statusBar->addWidget(m_networkLabel);
    m_statusBar->addPermanentWidget(m_timeLabel);
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
    m_cpuLabel = new QLabel(tr("0.0%"), this);
    m_cpuLabel->setFont(labelFont);
    m_cpuLabel->setAlignment(Qt::AlignRight);
    m_cpuLabel->setMinimumWidth(80);
    summaryLayout->addWidget(cpuTitleLabel, 0, 0);
    summaryLayout->addWidget(m_cpuLabel, 0, 1);

    QLabel* memoryTitleLabel = new QLabel(tr("Memory Usage:"), this);
    memoryTitleLabel->setFont(labelFont);
    m_memoryLabel = new QLabel(tr("0.0%"), this);
    m_memoryLabel->setFont(labelFont);
    m_memoryLabel->setAlignment(Qt::AlignRight);
    m_memoryLabel->setMinimumWidth(80);
    summaryLayout->addWidget(memoryTitleLabel, 0, 2);
    summaryLayout->addWidget(m_memoryLabel, 0, 3);

    QLabel* diskTitleLabel = new QLabel(tr("Disk Usage:"), this);
    diskTitleLabel->setFont(labelFont);
    m_diskLabel = new QLabel(tr("0.0%"), this);
    m_diskLabel->setFont(labelFont);
    m_diskLabel->setAlignment(Qt::AlignRight);
    m_diskLabel->setMinimumWidth(80);
    summaryLayout->addWidget(diskTitleLabel, 1, 0);
    summaryLayout->addWidget(m_diskLabel, 1, 1);

    QLabel* networkTitleLabel = new QLabel(tr("Network Speed:"), this);
    networkTitleLabel->setFont(labelFont);
    m_networkLabel = new QLabel(tr("↑ 0 KB/s  ↓ 0 KB/s"), this);
    m_networkLabel->setFont(labelFont);
    m_networkLabel->setAlignment(Qt::AlignRight);
    summaryLayout->addWidget(networkTitleLabel, 1, 2);
    summaryLayout->addWidget(m_networkLabel, 1, 3);

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
    QString cpuText = QString(tr("CPU: %1%")).arg(m_currentData.cpuUsage, 0, 'f', 1);
    m_cpuLabel->setText(cpuText);

    QString memoryText = QString(tr("Memory: %1%")).arg(m_currentData.memoryUsage, 0, 'f', 1);
    m_memoryLabel->setText(memoryText);

    QString diskText = QString(tr("Disk: %1%")).arg(m_currentData.diskUsage, 0, 'f', 1);
    m_diskLabel->setText(diskText);

    QString networkText = QString(tr("↑ %1  ↓ %2"))
        .arg(formatBytes(static_cast<quint64>(m_currentData.uploadSpeed * 1024)) + "/s")
        .arg(formatBytes(static_cast<quint64>(m_currentData.downloadSpeed * 1024)) + "/s");
    m_networkLabel->setText(networkText);

    if (m_currentData.cpuUsage > 50.0) {
        m_cpuLabel->setStyleSheet("color: orange; font-weight: bold;");
    } else if (m_currentData.cpuUsage > 80.0) {
        m_cpuLabel->setStyleSheet("color: red; font-weight: bold;");
    } else {
        m_cpuLabel->setStyleSheet("");
    }

    if (m_currentData.memoryUsage > 70.0) {
        m_memoryLabel->setStyleSheet("color: orange; font-weight: bold;");
    } else if (m_currentData.memoryUsage > 85.0) {
        m_memoryLabel->setStyleSheet("color: red; font-weight: bold;");
    } else {
        m_memoryLabel->setStyleSheet("");
    }

    QString timeText = tr("Last update: %1")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"));
    m_timeLabel->setText(timeText);
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
