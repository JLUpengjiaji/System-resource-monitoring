#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>
#include <QPair>
#include <QDateTime>

#ifdef Q_OS_WIN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "iphlpapi.lib")

#ifndef IF_OPER_STATUS_UP
#define IF_OPER_STATUS_UP 1
#endif
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
    QPair<quint64, quint64> getNetworkStats();
    double calculateProcessCpu(qint64 pid);

#ifdef Q_OS_WIN
    PDH_HQUERY m_cpuQuery;
    PDH_HCOUNTER m_cpuCounter;
    quint64 m_lastIdleTime;
    quint64 m_lastKernelTime;
    quint64 m_lastUserTime;
    quint64 m_lastNetworkUpload;
    quint64 m_lastNetworkDownload;
    QMap<qint64, QPair<quint64, quint64>> m_processTimes;
#else
    long m_lastCpuTotal;
    long m_lastCpuIdle;
    QMap<QString, QPair<quint64, quint64>> m_interfaceStats;
    QMap<qint64, QPair<unsigned long, unsigned long>> m_processTimes;
#endif
    bool m_firstUpdate;
};

#endif // SYSTEMINFO_H
