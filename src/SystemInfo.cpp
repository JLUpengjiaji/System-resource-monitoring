#include "SystemInfo.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QThread>

SystemInfo::SystemInfo(QObject *parent)
    : QObject(parent)
    , m_firstUpdate(true)
#ifdef Q_OS_WIN
    , m_lastIdleTime(0)
    , m_lastKernelTime(0)
    , m_lastUserTime(0)
    , m_lastNetworkUpload(0)
    , m_lastNetworkDownload(0)
#else
    , m_lastCpuTotal(0)
    , m_lastCpuIdle(0)
#endif
{
#ifdef Q_OS_WIN
    if (PdhOpenQuery(NULL, 0, &m_cpuQuery) == ERROR_SUCCESS) {
        PdhAddEnglishCounter(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuCounter);
        PdhCollectQueryData(m_cpuQuery);
    }
#endif
}

SystemInfo::~SystemInfo()
{
#ifdef Q_OS_WIN
    if (m_cpuQuery) {
        PdhCloseQuery(m_cpuQuery);
    }
#endif
}

SystemData SystemInfo::getSystemData()
{
    SystemData data;

    data.cpuUsage = getCpuUsage();

    auto memory = getMemoryInfo();
    data.memoryTotal = memory.first;
    data.memoryUsed = memory.second;
    data.memoryUsage = data.memoryTotal > 0 ? (static_cast<double>(data.memoryUsed) / data.memoryTotal) * 100.0 : 0.0;

    auto disk = getDiskInfo();
    data.diskTotal = disk.first;
    data.diskUsed = disk.second;
    data.diskUsage = data.diskTotal > 0 ? (static_cast<double>(data.diskUsed) / data.diskTotal) * 100.0 : 0.0;

    auto network = getNetworkStats();
    if (!m_firstUpdate) {
        data.uploadSpeed = (network.first - m_lastNetworkUpload) / 1024.0;
        data.downloadSpeed = (network.second - m_lastNetworkDownload) / 1024.0;
    } else {
        data.uploadSpeed = 0;
        data.downloadSpeed = 0;
    }
    data.networkUpload = network.first;
    data.networkDownload = network.second;

#ifdef Q_OS_WIN
    m_lastNetworkUpload = network.first;
    m_lastNetworkDownload = network.second;
#else
    m_lastCpuTotal = 0;
    m_lastCpuIdle = 0;
#endif
    m_firstUpdate = false;

    data.processes = getProcessList();

    return data;
}

double SystemInfo::getCpuUsage()
{
#ifdef Q_OS_WIN
    if (m_cpuQuery) {
        PDH_FMT_COUNTERVALUE counterVal;
        PdhCollectQueryData(m_cpuQuery);
        if (PdhGetFormattedCounterValue(m_cpuCounter, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS) {
            return counterVal.doubleValue;
        }
    }
    return 0.0;
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
            return 100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff);
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

QPair<quint64, quint64> SystemInfo::getNetworkStats()
{
    quint64 totalUpload = 0;
    quint64 totalDownload = 0;

#ifdef Q_OS_WIN
    MIB_IFTABLE* ifTable;
    DWORD dwSize = 0;

    if (GetIfTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        ifTable = (MIB_IFTABLE*)malloc(dwSize);
        if (ifTable != NULL) {
            if (GetIfTable(ifTable, &dwSize, FALSE) == NO_ERROR) {
                for (DWORD i = 0; i < ifTable->dwNumEntries; i++) {
                    MIB_IFROW& row = ifTable->table[i];
                    if (row.dwType != IF_TYPE_SOFTWARE_LOOPBACK && 
                        row.dwOperStatus == IF_OPER_STATUS_UP) {
                        totalUpload += row.dwOutOctets;
                        totalDownload += row.dwInOctets;
                    }
                }
            }
            free(ifTable);
        }
    }
#else
    struct ifaddrs* ifAddrStruct = NULL;
    struct ifaddrs* ifa = NULL;

    if (getifaddrs(&ifAddrStruct) == 0) {
        for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_PACKET && 
                strcmp(ifa->ifa_name, "lo") != 0) {
                struct rtnl_link_stats* stats = (struct rtnl_link_stats*)ifa->ifa_data;
                if (stats) {
                    totalUpload += stats->tx_bytes;
                    totalDownload += stats->rx_bytes;
                }
            }
        }
        freeifaddrs(ifAddrStruct);
    }
#endif

    return qMakePair(totalUpload, totalDownload);
}

double SystemInfo::calculateProcessCpu(qint64 pid)
{
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL) {
        return 0.0;
    }

    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (!GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
        CloseHandle(hProcess);
        return 0.0;
    }

    ULARGE_INTEGER kernel, user;
    kernel.LowPart = kernelTime.dwLowDateTime;
    kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime;
    user.HighPart = userTime.dwHighDateTime;

    quint64 totalTime = kernel.QuadPart + user.QuadPart;

    FILETIME idleTime, systemKernelTime, systemUserTime;
    GetSystemTimes(&idleTime, &systemKernelTime, &systemUserTime);

    ULARGE_INTEGER sysIdle, sysKernel, sysUser;
    sysIdle.LowPart = idleTime.dwLowDateTime;
    sysIdle.HighPart = idleTime.dwHighDateTime;
    sysKernel.LowPart = systemKernelTime.dwLowDateTime;
    sysKernel.HighPart = systemKernelTime.dwHighDateTime;
    sysUser.LowPart = systemUserTime.dwLowDateTime;
    sysUser.HighPart = systemUserTime.dwHighDateTime;

    quint64 systemTotal = sysKernel.QuadPart + sysUser.QuadPart - sysIdle.QuadPart;

    double cpuUsage = 0.0;
    if (m_processTimes.contains(pid)) {
        auto& prev = m_processTimes[pid];
        quint64 processDiff = totalTime - prev.first;
        quint64 systemDiff = systemTotal - prev.second;

        if (systemDiff > 0) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            cpuUsage = (static_cast<double>(processDiff) / systemDiff) * 100.0 * sysInfo.dwNumberOfProcessors;
        }
    }

    m_processTimes[pid] = qMakePair(totalTime, systemTotal);
    CloseHandle(hProcess);

    return qMin(cpuUsage, 100.0);
#else
    QString procPath = QString("/proc/%1/stat").arg(pid);
    QFile file(procPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0.0;
    }

    QString line = file.readLine();
    file.close();

    QStringList parts = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() < 22) {
        return 0.0;
    }

    unsigned long utime = parts[13].toULong();
    unsigned long stime = parts[14].toULong();
    unsigned long cutime = parts[15].toULong();
    unsigned long cstime = parts[16].toULong();

    unsigned long total = utime + stime + cutime + cstime;

    double cpuUsage = 0.0;
    if (m_processTimes.contains(pid)) {
        auto& prev = m_processTimes[pid];
        unsigned long processDiff = total - prev.first;
        unsigned long totalDiff = 0;

        QFile statFile("/proc/stat");
        if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString statLine = statFile.readLine();
            statFile.close();

            QStringList statParts = statLine.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
            if (statParts.size() >= 8) {
                unsigned long user = statParts[1].toULong();
                unsigned long nice = statParts[2].toULong();
                unsigned long system = statParts[3].toULong();
                unsigned long idle = statParts[4].toULong();
                unsigned long iowait = statParts[5].toULong();
                unsigned long irq = statParts[6].toULong();
                unsigned long softirq = statParts[7].toULong();

                unsigned long currentTotal = user + nice + system + idle + iowait + irq + softirq;
                totalDiff = currentTotal - prev.second;
            }
        }

        if (totalDiff > 0) {
            cpuUsage = (static_cast<double>(processDiff) / totalDiff) * 100.0;
        }
    }

    m_processTimes[pid] = qMakePair(total, 0UL);
    return qMin(cpuUsage, 100.0);
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

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            ProcessInfo info;
            info.name = QString::fromWCharArray(pe32.szExeFile);
            info.pid = pe32.th32ProcessID;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, info.pid);
            if (hProcess != NULL) {
                PROCESS_MEMORY_COUNTERS_EX pmc;
                if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                    info.memoryUsage = pmc.WorkingSetSize;
                }

                FILETIME creationTime, exitTime, kernelTime, userTime;
                if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
                    ULARGE_INTEGER createTime;
                    createTime.LowPart = creationTime.dwLowDateTime;
                    createTime.HighPart = creationTime.dwHighDateTime;

                    QDateTime dt;
                    dt.setMSecsSinceEpoch((createTime.QuadPart - 116444736000000000ULL) / 10000);
                    info.startTime = dt;
                }

                CloseHandle(hProcess);
            }

            info.cpuUsage = calculateProcessCpu(info.pid);
            processes.append(info);

        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
#else
    proc_t** procList = readproctab(PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLMEM);
    if (!procList) {
        return processes;
    }

    for (int i = 0; procList[i]; i++) {
        proc_t* p = procList[i];

        ProcessInfo info;
        info.name = QString(p->cmd);
        info.pid = p->tid;
        info.memoryUsage = static_cast<quint64>(p->rss) * sysconf(_SC_PAGESIZE);
        
        if (p->start_time != 0) {
            info.startTime = QDateTime::currentDateTime().addSecs(-(time(NULL) - p->start_time / sysconf(_SC_CLK_TCK)));
        }

        info.cpuUsage = calculateProcessCpu(info.pid);
        processes.append(info);
    }

    freeproc(procList);
#endif

    return processes;
}
