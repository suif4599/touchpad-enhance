# Touchpad Enhance 中文版

[English Version](./README.md)

一个Linux工具，模拟华为压力触控板的部分功能，特别是左右边缘的亮度和音量控制功能。

守护进程会以独占方式 grab 触摸板（`EVIOCGRAB`），把它镜像到一个 uinput 设备上，并通过这个镜像转发事件——因此你的桌面行为与原来完全一致，只是多了边缘滑动手势。另一个独立的 uinput 键盘设备负责发送模拟的亮度/音量按键事件。`touchpad-enhance-ctl` 客户端通过抽象命名空间 Unix 域套接字与守护进程通信，可以在不重启服务的情况下在四种运行状态之间切换。

## 功能特性

- **边缘控制**：在左侧边缘滑动调节屏幕亮度，右侧边缘调节音量。
- **Grab + 转发**：独占 grab 原始触摸板，通过 uinput 镜像设备重新发出事件，因此守护进程可以选择性地抑制触发手势的事件。
- **四种运行状态**（`active` / `inactive` / `disable-touchpad` / `control-only`），可用 `touchpad-enhance-ctl` 实时切换。
- **自动检测**：自动检测并使用第一个可用的触摸板设备。
- **手动设备选择**：为具有多个触摸板的系统提供手动指定触摸板设备的选项。
- **可配置**：可调节边缘宽度、滚动阈值和轴反转。
- **Systemd 集成**：作为后台服务运行，实现无缝操作。
- **NixOS Flake**：原生支持 flake + NixOS 模块。

## 兼容性

本方案基于 Linux 输入子系统标准实现，适用于以下情况：

- 现代 Linux 系统（通常为内核版本 ≥ 3.0）
- 支持 EV_ABS（绝对坐标）事件的触摸板设备
- 已正确配置为输入子系统设备的触摸板
- 使用 libinput 或 synaptics 驱动的触摸板（常见于大多数笔记本）

### 不适用的情况

- 旧版 Linux 系统（内核版本 < 3.0）
- 仅支持相对坐标（EV_REL）的触摸板
- 未正确实现 Linux 输入子系统标准的定制触摸板
- 某些嵌入式 Linux 设备（可能使用不同的输入处理方式）

## 运行状态

守护进程处于以下四种状态之一。`touchpad-enhance-ctl <状态>` 在运行时切换；守护进程的 `initialState`（NixOS 选项或 CLI 参数）设置启动默认值。

| 状态 | 边缘滑动手势 | 其他事件转发 | 适用场景 |
|---|---|---|---|
| `active`（默认） | 是 | 是 | 正常使用 |
| `inactive` | 否 | 是 | 关闭手势，触摸板仍正常工作 |
| `disable-touchpad` | 否 | 否 | 完全禁用触摸板 |
| `control-only` | 是 | 否 | 手势生效但光标完全不动 |

## 安装

### NixOS（推荐）

在你的系统 flake 的 inputs 中添加本 flake：

```nix
# flake.nix
inputs.touchpad-enhance = {
  url = "path:/path/to/touchpad-enhance";
  inputs.nixpkgs.follows = "nixpkgs";
};
```

然后导入模块并启用服务：

```nix
# configuration.nix 或你 flake 中的某个模块
{ inputs, ... }: {
  imports = [ inputs.touchpad-enhance.nixosModules.default ];

  services.touchpad-enhance = {
    enable = true;
    edgeWidth = 0.04;        # 视为边缘的触摸板宽度比例
    scrollThreshold = 0.2;   # 触发滚动的最小滑动距离
    scrollStep = 0.05;       # 每次滚动的步长
    initialState = "active"; # active | inactive | disable-touchpad | control-only
    # touchpadDevice = "/dev/input/event5";  # 仅当自动检测选错设备时设置
  };
}
```

这会配置 systemd 服务（以 root 身份运行，因为 `EVIOCGRAB` 和 `/dev/uinput` 需要特权），并把 `touchpad-enhance` 和 `touchpad-enhance-ctl` 都加入 `PATH`。

所有配置项也都作为 NixOS 选项暴露——修改任何一个都会触发以新的编译时常量重新构建包。

### CMake（其他发行版）

#### 前置要求

- CMake（版本 3.10 或更高）
- C++17 兼容编译器
- 安装和运行服务需要 root 权限

#### 构建和安装

```bash
cmake -B build
cmake --build build
sudo cmake --install build
```

这将：

- 编译 `touchpad-enhance` 和 `touchpad-enhance-ctl`
- 将二进制文件安装到 `/usr/local/bin/`
- 安装、启用并启动 systemd 服务

#### 卸载

```bash
sudo cmake --build build --target uninstall
```

## 配置

### 边缘 / 滚动行为

下面的 NixOS 模块选项和 CMake 变量调整的是同一组编译时常量，根据你的安装方式选择对应的接口即可。

| NixOS 选项 | CMake 变量 | 默认值 | 含义 |
|---|---|---|---|
| `edgeWidth` | `EDGE_WIDTH` | `0.05` | 边缘区域宽度占触摸板总宽度的比例 |
| `scrollThreshold` | `SCROLL_THRESHOLD` | `0.2` | 触发滚动动作的最小垂直移动量 |
| `scrollStep` | `SCROLL_STEP` | `0.05` | 每次后续滚动动作的移动步长 |
| `invertX` | `INVERTX` | `false`/`OFF` | 交换亮度和音量对应的边缘 |
| `invertY` | `INVERTY` | `false`/`OFF` | 交换上滑和下滑 |
| `touchpadDevice` | `TOUCHPAD_DEVICE` | `""`/自动 | 指定具体的 `/dev/input/eventN` |

反转 Y 轴 + 手动指定设备的 CMake 示例：

```bash
cmake -B build -DINVERTY=ON -DTOUCHPAD_DEVICE=/dev/input/event5
```

## 使用方法

安装后，服务会在后台自动运行。

- **亮度控制**：触摸触摸板左侧边缘并垂直滑动。
- **音量控制**：触摸触摸板右侧边缘并垂直滑动。

在运行时切换状态：

```bash
touchpad-enhance-ctl get                 # 查看当前状态
touchpad-enhance-ctl active              # 开启手势，触摸板正常透传
touchpad-enhance-ctl inactive            # 关闭手势，触摸板正常透传
touchpad-enhance-ctl control-only        # 开启手势，光标冻结
touchpad-enhance-ctl disable-touchpad    # 全部静默
```

滑动方向（上/下）可以通过 `INVERTY` / `invertY` 反转。

## 故障排除

- 确保服务正在运行：`systemctl status touchpad-enhance`
- 检查日志：`journalctl -u touchpad-enhance`
- 验证触摸板设备：`ls /dev/input/event*`
- 对于多个触摸板，使用 `TOUCHPAD_DEVICE` / `touchpadDevice` 指定设备
- 检查 libinput 从守护进程设备收到的事件：`sudo libinput debug-events` —— 应当看到一个 `Touchpad` 设备和一个 `Keyboard` 设备（"Touchpad Enhancer Hotkeys"）。如果 libinput 把触摸板设备识别为 `Tablet`，边缘滑动的光标行为会出错。

## 贡献

欢迎贡献！请随时提交 issue 和 PR。

## 许可证

本项目采用 GNU 通用公共许可证 v3.0 —— 详情请见 [LICENSE](./LICENSE) 文件。
