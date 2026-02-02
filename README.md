# Linux System Monitor

A lightweight, terminal-based system monitoring tool for Linux written in C++. It provides real-time statistics about system resources including CPU, Memory, Disk I/O, Network, and Processes.

## Features

- **System Information**: Uptime, Load Average, Task states.
- **CPU Usage**: Real-time total CPU utilization percentage.
- **Memory Usage**: Total, Used, and Free memory statistics.
- **Disk I/O**: Read/Write counts and sectors for available block devices.
- **Network Stats**: Receive/Transmit data volume and packet counts for network interfaces.
- **Process Monitoring**: Top 5 memory-consuming processes.

## Requirements

- Linux OS
- C++ Compiler (g++ or clang++)
- CMake (version 3.10 or higher)
- Make

## Build Instructions

1. **Clone the repository** (if you haven't already):
   ```bash
   git clone https://github.com/AbeginnerFLM/System_Monitor.git
   cd System_Monitor
   ```

2. **Create a build directory:**
   ```bash
   mkdir -p build
   cd build
   ```

3. **Configure the project with CMake:**
   ```bash
   cmake ..
   ```

4. **Compile:**
   ```bash
   make
   ```

## Usage

After compiling, run the executable from the `build` directory:

```bash
./system_monitor
```

The monitor refreshes every second. Press `Ctrl+C` to exit the application.

## Project Structure

- `src/`: Source files (`main.cpp`, `Collectors.cpp`, etc.)
- `include/`: Header files (`Collectors.h`, `CollectorFactory.h`, etc.)
- `CMakeLists.txt`: CMake build configuration.
