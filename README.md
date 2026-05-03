# System Resource Monitor

一个使用 C++17 + Qt5 Widgets 开发的桌面版系统资源监控工具。

## 功能特性

- **实时监控**：每秒刷新一次系统资源数据
- **CPU 监控**：显示 CPU 使用率及 60 秒趋势图
- **内存监控**：显示内存占用率及 60 秒趋势图
- **磁盘监控**：显示磁盘空间使用情况
- **网络监控**：显示上传/下载速度及 60 秒趋势图
- **进程列表**：显示所有进程的 PID、CPU 占用和内存占用
  - 支持按列排序
  - 支持关键词搜索过滤
- **告警功能**：可设置 CPU、内存告警阈值，超过后弹窗提醒
- **配置保存**：用户配置自动保存到本地 JSON 文件

## 目录结构

```
System-resource-monitoring-1/
├── CMakeLists.txt       # CMake 构建配置文件
├── README.md            # 项目说明文档
└── src/                 # 源代码目录
    ├── main.cpp         # 程序入口
    ├── SystemMonitor.h  # 主窗口头文件
    ├── SystemMonitor.cpp # 主窗口实现
    ├── SystemInfo.h     # 系统信息获取模块头文件
    ├── SystemInfo.cpp   # 系统信息获取模块实现
    ├── ConfigManager.h  # 配置管理模块头文件
    ├── ConfigManager.cpp # 配置管理模块实现
    ├── TrendChart.h     # 自定义折线图控件头文件
    ├── TrendChart.cpp   # 自定义折线图控件实现
    ├── ProcessList.h    # 进程列表模块头文件
    ├── ProcessList.cpp  # 进程列表模块实现
    ├── SettingsDialog.h # 设置对话框头文件
    └── SettingsDialog.cpp # 设置对话框实现
```

## 依赖要求

### 编译环境
- **CMake**: 3.16+
- **C++ 标准**: C++17
- **Qt5**: 5.12+ (仅需要 Qt5::Core 和 Qt5::Widgets)

### Windows 平台额外依赖
- Windows SDK (包含 pdh.dll, psapi.dll, iphlpapi.dll)
- Visual Studio 2017+ 或 MinGW

### Linux 平台额外依赖
- libprocps-dev (进程信息获取)
- GCC 7+ 或 Clang 5+

## 安装依赖

### Windows (使用 Qt 安装器)
1. 下载并安装 Qt 5.12+ (https://www.qt.io/download)
2. 选择安装组件时，确保包含：
   - Qt 5.12.x (MSVC 2017/2019 或 MinGW)
   - Qt Creator (可选)
   - CMake

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake qt5-default libqt5widgets5 libqt5core5a libprocps-dev
```

### Fedora/RHEL
```bash
sudo dnf install gcc-c++ cmake qt5-qtbase-devel procps-ng-devel
```

## 编译步骤

### 方法一：使用命令行 (推荐)

1. **创建构建目录**
```bash
mkdir build
cd build
```

2. **配置 CMake**
```bash
# Windows (MSVC)
cmake .. -G "Visual Studio 16 2019" -A x64

# Windows (MinGW)
cmake .. -G "MinGW Makefiles"

# Linux
cmake .. -DCMAKE_BUILD_TYPE=Release
```

3. **编译项目**
```bash
# Windows (MSVC)
cmake --build . --config Release

# Windows (MinGW) 或 Linux
cmake --build .
```

### 方法二：使用 Qt Creator

1. 打开 Qt Creator
2. 选择 "文件" -> "打开文件或项目"
3. 选择项目根目录下的 `CMakeLists.txt`
4. 选择合适的 Qt Kit (Qt 5.12+)
5. 点击 "配置项目"
6. 点击左侧工具栏的运行按钮 (绿色三角形)

## 运行程序

编译成功后，可执行文件位于：
- Windows: `build/bin/Release/SystemMonitor.exe`
- Linux: `build/bin/SystemMonitor`

### Windows 运行注意事项

如果使用动态链接的 Qt，需要将 Qt 的 DLL 文件复制到可执行文件目录，或者确保 Qt 的 bin 目录在 PATH 环境变量中。

需要的 Qt DLL (以 Qt 5.15.2 MSVC 为例):
- Qt5Core.dll
- Qt5Gui.dll
- Qt5Widgets.dll

以及平台插件：
- platforms/qwindows.dll

## 配置文件

程序配置自动保存到 JSON 文件，位置如下：
- Windows: `C:/Users/<用户名>/AppData/Local/OpenSource/System Resource Monitor/config.json`
- Linux: `~/.config/OpenSource/System Resource Monitor/config.json`

配置文件内容示例：
```json
{
    "cpuAlertEnabled": true,
    "cpuThreshold": 80.0,
    "memoryAlertEnabled": true,
    "memoryThreshold": 85.0,
    "refreshInterval": 1000
}
```

## 使用说明

### 主界面 (Overview Tab)
- **系统概览**：显示当前 CPU、内存、磁盘、网络的实时数据
- **趋势图表**：
  - CPU 使用率趋势图 (最近 60 秒)
  - 内存使用率趋势图 (最近 60 秒)
  - 上传速度趋势图 (最近 60 秒)
  - 下载速度趋势图 (最近 60 秒)

### 进程列表 (Processes Tab)
- 显示所有运行中的进程
- 列说明：
  - Process Name: 进程名称
  - PID: 进程 ID
  - CPU (%): CPU 使用率
  - Memory: 内存占用量
- 功能：
  - 点击表头按列排序
  - 在搜索框输入关键词过滤进程

### 设置 (File -> Settings)
- **刷新间隔**：设置数据刷新频率 (500ms - 5000ms)
- **CPU 告警**：启用/禁用 CPU 告警，设置阈值 (1% - 100%)
- **内存告警**：启用/禁用内存告警，设置阈值 (1% - 100%)

### 告警功能
当 CPU 或内存使用率超过设定阈值时：
- 弹出警告对话框
- 状态栏对应数值变为橙色或红色高亮

## 技术细节

### 系统信息获取

**Windows 平台**:
- 使用 Performance Data Helper (PDH) API 获取 CPU 使用率
- 使用 GlobalMemoryStatusEx 获取内存信息
- 使用 GetDiskFreeSpaceEx 获取磁盘信息
- 使用 IP Helper API 获取网络统计
- 使用 ToolHelp API 枚举进程

**Linux 平台**:
- 读取 `/proc/stat` 获取 CPU 信息
- 读取 `/proc/meminfo` 获取内存信息
- 使用 statvfs 获取磁盘信息
- 读取 `/proc/net/dev` 或使用 getifaddrs 获取网络信息
- 使用 /proc 文件系统枚举进程

### 自定义图表控件
- 继承自 QWidget
- 使用 QPainter 绘制折线图
- 支持 60 秒历史数据展示
- 自适应 Y 轴范围 (网络图表)
- 渐变填充和动画效果

## 已知问题与限制

1. **Windows 进程 CPU 计算**：首次获取进程 CPU 使用率可能为 0，需要等待下一次刷新
2. **网络接口**：默认统计所有非回环接口的总流量
3. **Linux 依赖**：需要 libprocps 库
4. **权限**：部分系统信息可能需要管理员/root 权限才能获取

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！

## 联系方式

如有问题或建议，请通过 GitHub Issues 联系。
