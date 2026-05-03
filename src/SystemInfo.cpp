#include "SystemInfo.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QThread>
#include <QDateTime>
#include <limits>

#ifdef Q_OS_WIN

#ifndef __IF_TYPE_SOFTWARE_LOOPBACK
#define IF_TYPE_SOFTWARE_LOOPBACK 24
#endif

#ifndef __IF_OPER_STATUS_UP
#define IF_OPER_STATUS_UP 1
#endif

#endif

SystemInfo::SystemInfo(QObject *parent)
    : QObject(parent)
    , m_firstUpdate(true)
#ifdef Q_OS_WIN
    , m_prevIdleTime(0)
    , m_prevKernelTime(0)
    , m_prevUserTime(0)
    , m_prevSystemTotal(0)
    , m_prevNetworkUpload(0)
    , m_prevNetworkDownload(0)
    , m_prevNetworkTime(0)
    , m_processorCount(0)
    , m_firstCpuUpdate(true)
    , m_firstNetworkUpdate(true)
#else
    , m_lastCpuTotal(0)
    , m_lastCpuIdle(0)
#endif
{
#ifdef Q_OS_WIN
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_processorCount = sysInfo.dwNumberOfProcessors;
#endif
}

SystemInfo::~SystemInfo()
{
}

#ifdef Q_OS_WIN
quint64 SystemInfo::fileTimeToUInt64(const FILETIME& ft)
{
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}
#endif

SystemData SystemInfo::getSystemData()
{
    QMutexLocker locker(&m_dataMutex);
    
    SystemData data;
    data.timestamp = QDateTime::currentMSecsSinceEpoch();

    data.cpuUsage = getCpuUsage();

    auto memory = getMemoryInfo();
    data.memoryTotal = memory.first;
    data.memoryUsed = memory.second;
    data.memoryUsage = data.memoryTotal > 0 ? (static_cast<double>(data.memoryUsed) / data.memoryTotal) * 100.0 : 0.0;

    auto disk = getDiskInfo();
    data.diskTotal = disk.first;
    data.diskUsed = disk.second;
    data.diskUsage = data.diskTotal > 0 ? (static_cast<double>(data.diskUsed) / data.diskTotal) * 100.0 : 0.0;

    auto networkSpeeds = getNetworkSpeeds();
    data.uploadSpeed = networkSpeeds.first;
    data.downloadSpeed = networkSpeeds.second;

    data.processes = getProcessList();

    m_firstUpdate = false;

    return data;
}

double SystemInfo::getCpuUsage()
{
#ifdef Q_OS_WIN
    FILETIME idleTime, kernelTime, userTime;
    
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return 0.0;
    }

    quint64 currIdle = fileTimeToUInt64(idleTime);
    quint64 currKernel = fileTimeToUInt64(kernelTime);
    quint64 currUser = fileTimeToUInt64(userTime);

    quint64 currSystemTotal = currKernel + currUser;

    if (m_firstCpuUpdate) {
        m_prevIdleTime = currIdle;
        m_prevKernelTime = currKernel;
        m_prevUserTime = currUser;
        m_prevSystemTotal = currSystemTotal;
        m_firstCpuUpdate = false;
        return 0.0;
    }

    quint64 systemDiff = currSystemTotal - m_prevSystemTotal;
    quint64 idleDiff = currIdle - m_prevIdleTime;

    if (systemDiff <= 0) {
        return 0.0;
    }

    double cpuUsage = 100.0 * (1.0 - static_cast<double>(idleDiff) / static_cast<double>(systemDiff));

    m_prevIdleTime = currIdle;
    m_prevKernelTime = currKernel;
    m_prevUserTime = currUser;
    m_prevSystemTotal = currSystemTotal;

    return qMax(0.0, qMin(100.0, cpuUsage));
#else
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0.0;
    }

    QString line = file.readLine();
    file.close();

    QRegExp rx("cpu\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)");
    if (rx.indexIn(line) != -1) {
        long user = rx.cap(1).toLong();
        long nice = rx.cap(2).toLong();
        long system = rx.cap(3).toLong();
        long idle = rx.cap(4).toLong();
        long iowait = rx.cap(5).toLong();
        long irq = rx.cap(6).toLong();
        long softirq = rx.cap(7).toLong();

        long total = user + nice + system + idle + iowait + irq + softirq;
        long totalDiff = total - m_lastCpuTotal;
        long idleDiff = idle - m_lastCpuIdle;

        m_lastCpuTotal = total;
        m_lastCpuIdle = idle;

        if (totalDiff > 0) {
            double usage = 100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff);
            return qMax(0.0, qMin(100.0, usage));
        }
    }
    return 0.0;
#endif
}

QPair<quint64, quint64> SystemInfo::getMemoryInfo()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (GlobalMemoryStatusEx(&memInfo)) {
        quint64 total = memInfo.ullTotalPhys;
        quint64 used = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
        return qMakePair(total, used);
    }
    return qMakePair(0ULL, 0ULL);
#else
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        quint64 total = info.totalram * info.mem_unit;
        quint64 free = info.freeram * info.mem_unit;
        quint64 buffers = info.bufferram * info.mem_unit;

        quint64 used = total - free - buffers;
        return qMakePair(total, used);
    }
    return qMakePair(0ULL, 0ULL);
#endif
}

QPair<quint64, quint64> SystemInfo::getDiskInfo()
{
#ifdef Q_OS_WIN
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    
    if (GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        quint64 total = totalNumberOfBytes.QuadPart;
        quint64 used = total - totalNumberOfFreeBytes.QuadPart;
        return qMakePair(total, used);
    }
    return qMakePair(0ULL, 0ULL);
#else
    struct statvfs info;
    if (statvfs("/", &info) == 0) {
        quint64 total = info.f_blocks * info.f_frsize;
        quint64 free = info.f_bfree * info.f_frsize;
        quint64 used = total - free;
        return qMakePair(total, used);
    }
    return qMakePair(0ULL, 0ULL);
#endif
}

QPair<double, double> SystemInfo::getNetworkSpeeds()
{
#ifdef Q_OS_WIN
    quint64 currUpload = 0;
    quint64 currDownload = 0;

    PMIB_IF_TABLE2 pIfTable = nullptr;
    NETIO_STATUS status;

    status = GetIfTable2(&pIfTable);
    if (status != NO_ERROR || pIfTable == nullptr) {
        return qMakePair(0.0, 0.0);
    }

    for (ULONG i = 0; i < pIfTable->NumEntries; i++) {
        MIB_IF_ROW2& row = pIfTable->Table[i];
        
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK) {
            continue;
        }
        
        if (row.OperStatus != IfOperStatusUp) {
            continue;
        }

        if (row.InOctets == 0 && row.OutOctets == 0) {
            continue;
        }

        currUpload += row.OutOctets;
        currDownload += row.InOctets;
    }

    FreeMibTable(pIfTable);

    qint64 currTime = QDateTime::currentMSecsSinceEpoch();

    if (m_firstNetworkUpdate) {
        m_prevNetworkUpload = currUpload;
        m_prevNetworkDownload = currDownload;
        m_prevNetworkTime = currTime;
        m_firstNetworkUpdate = false;
        return qMakePair(0.0, 0.0);
    }

    qint64 timeDiff = currTime - m_prevNetworkTime;
    
    if (timeDiff <= 0) {
        return qMakePair(0.0, 0.0);
    }

    double uploadSpeed = 0.0;
    double downloadSpeed = 0.0;

    const quint64 COUNTER_THRESHOLD = 1000000000000ULL;

    if (currUpload >= m_prevNetworkUpload) {
        uploadSpeed = static_cast<double>(currUpload - m_prevNetworkUpload) / timeDiff * 1000.0 / 1024.0;
    } else if (m_prevNetworkUpload - currUpload > COUNTER_THRESHOLD) {
        quint64 wrapAround = (std::numeric_limits<quint64>::max)() - m_prevNetworkUpload + currUpload + 1;
        uploadSpeed = static_cast<double>(wrapAround) / timeDiff * 1000.0 / 1024.0;
    }

    if (currDownload >= m_prevNetworkDownload) {
        downloadSpeed = static_cast<double>(currDownload - m_prevNetworkDownload) / timeDiff * 1000.0 / 1024.0;
    } else if (m_prevNetworkDownload - currDownload > COUNTER_THRESHOLD) {
        quint64 wrapAround = (std::numeric_limits<quint64>::max)() - m_prevNetworkDownload + currDownload + 1;
        downloadSpeed = static_cast<double>(wrapAround) / timeDiff * 1000.0 / 1024.0;
    }

    m_prevNetworkUpload = currUpload;
    m_prevNetworkDownload = currDownload;
    m_prevNetworkTime = currTime;

    return qMakePair(qMax(0.0, uploadSpeed), qMax(0.0, downloadSpeed));
#else
    quint64 totalUpload = 0;
    quint64 totalDownload = 0;

    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;

    if (getifaddrs(&ifAddrStruct) != 0) {
        return qMakePair(0.0, 0.0);
    }

    qint64 currTime = QDateTime::currentMSecsSinceEpoch();

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family != AF_PACKET) {
            continue;
        }

        if (strcmp(ifa->ifa_name, "lo") == 0) {
            continue;
        }

        struct rtnl_link_stats* stats = static_cast<struct rtnl_link_stats*>(ifa->ifa_data);
        if (stats) {
            totalUpload += stats->tx_bytes;
            totalDownload += stats->rx_bytes;
        }
    }

    freeifaddrs(ifAddrStruct);

    QString totalKey = "total";
    if (!m_interfaceStats.contains(totalKey)) {
        m_interfaceStats[totalKey] = qMakePair(totalUpload, totalDownload);
        return qMakePair(0.0, 0.0);
    }

    auto& prev = m_interfaceStats[totalKey];
    qint64 timeDiff = 1000;

    double uploadSpeed = 0.0;
    double downloadSpeed = 0.0;

    if (totalUpload >= prev.first) {
        uploadSpeed = static_cast<double>(totalUpload - prev.first) / timeDiff * 1000.0 / 1024.0;
    }

    if (totalDownload >= prev.second) {
        downloadSpeed = static_cast<double>(totalDownload - prev.second) / timeDiff * 1000.0 / 1024.0;
    }

    m_interfaceStats[totalKey] = qMakePair(totalUpload, totalDownload);

    return qMakePair(qMax(0.0, uploadSpeed), qMax(0.0, downloadSpeed));
#endif
}

QVector<ProcessInfo> SystemInfo::getProcessList()
{
    QVector<ProcessInfo> processes;

#ifdef Q_OS_WIN
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    QMap<qint64, PROCESS_TIME_INFO> currProcessTimes;

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            ProcessInfo info;
            info.name = QString::fromWCharArray(pe32.szExeFile);
            info.pid = pe32.th32ProcessID;
            info.cpuUsage = 0.0;
            info.memoryUsage = 0;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, info.pid);
            if (hProcess != nullptr) {
                PROCESS_MEMORY_COUNTERS_EX pmc;
                if (GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
                    info.memoryUsage = pmc.WorkingSetSize;
                }

                FILETIME creationTime, exitTime, kernelTime, userTime;
                if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
                    quint64 kTime = fileTimeToUInt64(kernelTime);
                    quint64 uTime = fileTimeToUInt64(userTime);
                    quint64 tTime = kTime + uTime;

                    PROCESS_TIME_INFO timeInfo;
                    timeInfo.kernelTime = kTime;
                    timeInfo.userTime = uTime;
                    timeInfo.totalTime = tTime;
                    currProcessTimes[info.pid] = timeInfo;

                    info.cpuUsage = calculateProcessCpu(info.pid, kTime, uTime);

                    ULARGE_INTEGER createTime;
                    createTime.LowPart = creationTime.dwLowDateTime;
                    createTime.HighPart = creationTime.dwHighDateTime;

                    QDateTime dt;
                    dt.setMSecsSinceEpoch((createTime.QuadPart - 116444736000000000ULL) / 10000);
                    info.startTime = dt;
                }

                CloseHandle(hProcess);
            }

            if (info.name.isEmpty()) {
                info.name = tr("[System Process]");
            }

            processes.append(info);

        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    m_prevProcessTimes = currProcessTimes;
#else
    proc_t** procList = readproctab(PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLMEM);
    if (!procList) {
        return processes;
    }

    for (int i = 0; procList[i]; i++) {
        proc_t* p = procList[i];

        ProcessInfo info;
        info.name = QString(p->cmd);
        if (info.name.isEmpty()) {
            info.name = tr("[Unknown]");
        }
        info.pid = p->tid;
        info.memoryUsage = static_cast<quint64>(p->rss) * sysconf(_SC_PAGESIZE);
        
        if (p->start_time != 0) {
            info.startTime = QDateTime::currentDateTime().addSecs(-(time(nullptr) - p->start_time / sysconf(_SC_CLK_TCK)));
        }

        info.cpuUsage = 0.0;
        processes.append(info);
    }

    freeproc(procList);
#endif

    return processes;
}

double SystemInfo::calculateProcessCpu(qint64 pid, quint64 kernelTime, quint64 userTime)
{
#ifdef Q_OS_WIN
    if (!m_prevProcessTimes.contains(pid)) {
        return 0.0;
    }

    PROCESS_TIME_INFO prevInfo = m_prevProcessTimes[pid];
    quint64 prevTotal = prevInfo.totalTime;
    quint64 currTotal = kernelTime + userTime;

    FILETIME idleTime, kernelTimeSys, userTimeSys;
    if (!GetSystemTimes(&idleTime, &kernelTimeSys, &userTimeSys)) {
        return 0.0;
    }

    quint64 currIdle = fileTimeToUInt64(idleTime);
    quint64 currKernel = fileTimeToUInt64(kernelTimeSys);
    quint64 currUser = fileTimeToUInt64(userTimeSys);
    quint64 currSystemTotal = currKernel + currUser;

    quint64 processDiff = currTotal - prevTotal;
    quint64 systemDiff = currSystemTotal - m_prevSystemTotal;

    if (systemDiff <= 0) {
        return 0.0;
    }

    double cpuUsage = (static_cast<double>(processDiff) / systemDiff) * 100.0;

    cpuUsage = qMax(0.0, qMin(100.0 * m_processorCount, cpuUsage));

    return cpuUsage;
#else
    return 0.0;
#endif
}
