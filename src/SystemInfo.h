#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include <QPair>
#include <QDateTime>
#include <QMutex>
#include <QElapsedTimer>

#ifdef Q_OS_WIN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <tlhelp32.h>
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "pdh.lib")

typedef struct _PROCESS_TIME_INFO {
    quint64 kernelTime;
    quint64 userTime;
    quint64 totalTime;
} PROCESS_TIME_INFO;

typedef struct _NETWORK_COUNTER_INFO {
    HCOUNTER hCounterUpload;
    HCOUNTER hCounterDownload;
    QString interfaceName;
} NETWORK_COUNTER_INFO;

#else
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <proc/readproc.h>
#include <net/if.h>
#include <ifaddrs.h>
#endif

struct ProcessInfo
{
    QString name;
    qint64 pid;
    double cpuUsage;
    quint64 memoryUsage;
    QDateTime startTime;
};

struct SystemData
{
    double cpuUsage;
    quint64 memoryTotal;
    quint64 memoryUsed;
    double memoryUsage;
    quint64 diskTotal;
    quint64 diskUsed;
    double diskUsage;
    quint64 networkUpload;
    quint64 networkDownload;
    double uploadSpeed;
    double downloadSpeed;
    QVector<ProcessInfo> processes;
    qint64 timestamp;
};

class SystemInfo : public QObject
{
    Q_OBJECT

public:
    explicit SystemInfo(QObject *parent = nullptr);
    ~SystemInfo();

    SystemData getSystemData();
    QVector<ProcessInfo> getProcessList();

private:
    double getCpuUsage();
    QPair<quint64, quint64> getMemoryInfo();
    QPair<quint64, quint64> getDiskInfo();
    QPair<double, double> getNetworkSpeeds();
    double calculateProcessCpu(qint64 pid, quint64 kernelTime, quint64 userTime);
    void updateSystemTimes();

#ifdef Q_OS_WIN
    quint64 fileTimeToUInt64(const FILETIME& ft);
    bool initializeNetworkCounters();
    void cleanupNetworkCounters();
    QVector<QString> getActiveNetworkInterfaces();
    
    quint64 m_prevIdleTime;
    quint64 m_prevKernelTime;
    quint64 m_prevUserTime;
    quint64 m_prevSystemTotal;
    
    QMap<qint64, PROCESS_TIME_INFO> m_prevProcessTimes;
    int m_processorCount;
    
    bool m_firstCpuUpdate;
    bool m_firstNetworkUpdate;
    HQUERY m_pdhQuery;
    QVector<NETWORK_COUNTER_INFO> m_networkCounters;
#else
    long m_lastCpuTotal;
    long m_lastCpuIdle;
    QMap<QString, QPair<quint64, quint64>> m_interfaceStats;
    QMap<qint64, QPair<unsigned long, unsigned long>> m_processTimes;
#endif
    bool m_firstUpdate;
    QMutex m_dataMutex;
};

#endif // SYSTEMINFO_H
