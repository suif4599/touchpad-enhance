# Touchpad Enhance 中文版

[English Version](./README.md)

一个Linux工具，模拟华为压力触控板的部分功能，特别是左右边缘的亮度和音量控制功能。

## 功能特性

- **边缘控制**：在左侧边缘滑动调节屏幕亮度，右侧边缘调节音量。
- **自动检测**：自动检测并使用第一个可用的触摸板设备。
- **手动设备选择**：为具有多个触摸板的多系统提供手动指定触摸板设备的选项。
- **可配置**：可调节边缘宽度、滚动阈值和轴反转。
- **Systemd集成**：作为后台服务运行，实现无缝操作。

## 兼容性

本方案基于Linux输入子系统标准实现，适用于以下情况：

- 现代Linux系统（通常为内核版本≥3.0）
- 支持EV_ABS（绝对坐标）事件的触摸板设备
- 已正确配置为输入子系统设备的触摸板
- 使用libinput或synaptics驱动的触摸板（常见于大多数笔记本）

### 不适用的情况

- 旧版Linux系统（内核版本<3.0）
- 仅支持相对坐标（EV_REL）的触摸板
- 未正确实现Linux输入子系统标准的定制触摸板
- 某些嵌入式Linux设备（可能使用不同的输入处理方式）

## 安装

### 前置要求

- CMake（版本3.10或更高）
- C++17兼容编译器
- 安装和运行服务需要root权限

### 构建和安装

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

这将：
- 编译应用程序
- 将二进制文件安装到`/usr/local/bin/`
- 安装并启用systemd服务
- 自动启动服务

### 卸载

```bash
cd build
sudo make uninstall
```

## 配置

应用程序可以通过CMake选项进行配置：

- `INVERTX`：反转X轴（默认：OFF）
- `INVERTY`：反转Y轴（默认：OFF）
- `TOUCHPAD_DEVICE`：指定触摸板设备文件（例如，/dev/input/event5），用于具有多个触摸板的系统（默认：自动检测）

反转Y轴和手动设备选择的示例：

```bash
cmake -DINVERTY=ON -DTOUCHPAD_DEVICE=/dev/input/event5 ..
```

可以在`config.h`中修改其他参数：

- `EDGE_WIDTH`：边缘区域宽度占触摸板总宽度的比例（默认：0.05）
- `SCROLL_THRESHOLD`：触发滚动动作的最小垂直移动量（默认：0.2）
- `SCROLL_STEP`：每次滚动动作的移动步长（默认：0.05）

## 使用方法

安装后，服务会在后台自动运行。不需要用户交互。

- **亮度控制**：触摸触摸板左侧边缘并垂直滑动
- **音量控制**：触摸触摸板右侧边缘并垂直滑动

滑动方向（上/下）可以使用`INVERTY`选项反转。

## 故障排除

- 确保服务正在运行：`systemctl status touchpad-enhance`
- 检查日志：`journalctl -u touchpad-enhance`
- 验证触摸板设备：`ls /dev/input/event*`
- 对于多个触摸板，使用`TOUCHPAD_DEVICE`选项指定设备
- 如果出现权限问题，请使用root权限测试

## 贡献

欢迎贡献！请随时提交问题和拉取请求。

## 许可证

本项目采用GNU通用公共许可证 v3.0 - 详情请见[LICENSE](./LICENSE)文件。
