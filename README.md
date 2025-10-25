# QtWebSocketProtobuf

QtWebSocketProtobuf 是一个基于 Qt WebSockets 和 Protocol Buffers 的 WebSocket 通信库，旨在提供高可扩展性和高性能的 WebSocket 通信解决方案。该库的主要特点是将消息处理与 WebSocket 通信解耦，同时提供线程化的服务端以实现更高的性能和并发处理能力。

## 特性

- **自定义消息支持**：支持自定义消息，无需手动序列化和反序列化
- **会话管理**：完整的 WebSocket 会话生命周期管理
- **自动重连**：支持自动重连机制，确保连接稳定性
- **高性能服务端**：提供线程安全的线程化服务端，支持高并发连接和消息处理
- **跨平台支持**：支持 Windows、Linux 等主流平台

## 依赖

- C++11+
- Qt 5.12+
- Protocol Buffers 3.12+ (通过git submodule提供)
- CMake 3.22+

## 构建

### 使用CMake构建和安装

```bash
# 1. 初始化submodule
git submodule update --init --recursive

# 2. 创建构建目录并编译
mkdir -p build && cd build
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(nproc)
sudo make install
```

## 使用方法

### 在CMake项目中使用

```cmake
find_package(QtWebSocketProtobuf REQUIRED)
target_link_libraries(your_target QtWebSocketProtobuf::QtWebSocketProtobuf)
```

详见 [examples](examples/) 目录中的完整示例程序：

### 基础示例 ([examples/basic/](examples/basic/))
- [标准服务端](examples/basic/server.cpp) - 适用于中小规模并发连接
- [线程化服务端](examples/basic/threadedserver.cpp) - 高性能服务端，支持高并发
- [客户端](examples/basic/client.cpp) - 完整的客户端实现

### 性能测试示例 ([examples/latencytest/](examples/latencytest/))
- [延迟测试标准服务端](examples/latencytest/latencyserver.cpp) - 标准服务端延迟测试
- [延迟测试线程化服务端](examples/latencytest/latencythreadedserver.cpp) - 线程化服务端延迟测试
- [延迟测试客户端](examples/latencytest/latencyclient.cpp) - 延迟测试客户端

## 性能测试

### 延迟测试结果

使用线程化服务端与标准服务端进行对比测试：

**测试环境：**
- 客户端：Windows 平台
- 服务端：Ubuntu 22.04 虚拟机
- 网络：局域网

**测试条件：**
- 单客户端连接（Windows → Ubuntu虚拟机）
- 测试间隔：100ms (10次/秒)
- 数据大小：400字节
- 样本数量：1000个
- 测试时长：100秒

**测试结果：**
| 测试项目 | 标准服务端 | 线程化服务端 | 差异 |
|---------|-----------|-------------|------|
| **平均RTT** | 2863.20μs | 2800.03μs | -2.2% |
| **最小RTT** | 1098μs | 1102μs | +0.4% |
| **最大RTT** | 24818μs | 32714μs | +31.8% |
| **Jitter** | 764.65μs | 737.42μs | -3.6% |
| **P50 (中位数)** | 2617.00μs | 2609.50μs | -0.3% |
| **P95 (95%分位数)** | 4360.00μs | 4228.20μs | -3.0% |
| **P99 (99%分位数)** | 6678.45μs | 9287.38μs | +39.1% |

**分析：**
- 在跨平台单客户端低频率测试场景下，两种服务端的平均延迟性能相近
- 网络延迟占主导地位，服务端处理差异相对较小
- 线程化服务端的P99延迟略高，体现了线程切换的开销
- 线程化服务端的优势主要体现在高并发、多客户端场景下

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。
