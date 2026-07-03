# Touchpad Enhance 中文版

[English Version](./README.md)

一个 Linux 工具，模拟华为压力触控板的部分功能，特别是左右边缘的亮度和音量控制功能。

## 功能特性

- **边缘控制**：在左侧边缘滑动调节屏幕亮度，右侧边缘调节音量。
- **独占抓取 + 转发**：守护进程独占抓取触摸板，并把所有事件转发到一个具备相同能力的 uinput 镜像设备上，从而拦截边缘滑动而不污染用户态。
- **三态状态机**：运行时通过 IPC 在 `active`、`inactive`、`disable-touchpad` 之间切换，无需重启。
- **`touchpad-enhance-ctl`**：通过抽象 Unix 域套接字与守护进程通信的小工具。
- **自动检测**：自动检测并使用第一个可用的触摸板设备。
- **手动设备选择**：为多触摸板系统提供手动指定设备的选项。
- **可配置**：可调节边缘宽度、滚动阈值和坐标轴反转。
- **Systemd / NixOS 集成**：以 systemd 服务的形式运行，或通过随附 flake 作为 NixOS 模块使用。

## 工作原理

守护进程以 root 运行，做三件事：

1. 用 `EVIOCGRAB` **独占抓取**真实触摸板，使其他进程都看不到原始事件。
2. 把触摸板的能力（事件类型、key/abs/rel/msc 位、abs 范围）**完整镜像**到一个 uinput 设备上。libinput 会把它识别为触摸板。
3. 对每一次触摸**判断**是否为边缘滑动：
   - 如果是且达到了滚动阈值 → 在另一个独立、仅键盘类型的 uinput 设备上发出 `KEY_VOLUME*` / `KEY_BRIGHTNESS*`，并**丢弃**已缓冲的事件。
   - 否则 → **回放**缓冲事件，让光标按正常方式响应。

第三个 uinput 设备（纯键盘）专门承载模拟的热键。把它和触摸板镜像**分开**是让 libinput 正确分类（触摸板 = 相对运动，键盘 = 多媒体键）以及让桌面环境正确处理热键的关键。

## 运行状态

| 状态                | 行为                                                       |
|---------------------|------------------------------------------------------------|
| `active`            | 处理边缘滑动；未被捕获的事件全部转发。默认值。             |
| `inactive`          | 原样转发所有事件，跳过所有边缘检测逻辑。                   |
| `disable-touchpad`  | 丢弃所有事件，触摸板事实上被禁用。                         |

状态保存在守护进程中，每个事件到来时都会读取。切换是立即生效的——在触摸过程中切到 `inactive` 会先把已缓冲的事件回放出来，避免 libinput 看到半截触摸序列。

## 兼容性

本方案基于 Linux 输入子系统标准实现，适用于以下情况：

- 现代 Linux 系统（通常内核版本 ≥ 3.0；uinput ≥ 4.5 才能使用 `UI_ABS_SETUP`）
- 支持 EV_ABS（绝对坐标）事件的触摸板设备
- 已正确配置为输入子系统设备的触摸板
- 使用 libinput 或 synaptics 驱动的触摸板（常见于大多数笔记本）

### 不适用的情况

- 旧版 Linux 系统（内核版本 < 3.0）
- 仅支持相对坐标（EV_REL）的触摸板
- 未正确实现 Linux 输入子系统标准的定制触摸板
- 某些嵌入式 Linux 设备（可能使用不同的输入处理方式）

## 安装

### 方式 A：NixOS（推荐）

把本 flake 加进系统 flake 的 inputs：

```nix
inputs.touchpad-enhance = {
  url = "path:/home/you/src/touchpad-enhance";
  inputs.nixpkgs.follows = "nixpkgs";
};
```

然后引入模块并启用服务：

```nix
{ inputs, ... }: {
  imports = [ inputs.touchpad-enhance.nixosModules.default ];

  services.touchpad-enhance = {
    enable = true;
    edgeWidth = 0.04;        # 边缘区域占触摸板总宽度的比例
    scrollThreshold = 0.2;   # 触发一次滚动所需的最小垂直位移
    scrollStep = 0.05;       # 后续每次滚动事件的位移步长
    initialState = "active"; # active | inactive | disable-touchpad
    # invertX = false;
    # invertY = false;
    # touchpadDevice = "/dev/input/event5";  # 仅当自动检测选错时才需要
  };
}
```

这会自动配置 systemd 服务，并把 `touchpad-enhance-ctl` 放到 `PATH` 中。完整选项见下方**配置**一节。

### 方式 B：CMake（手动）

#### 前置要求

- CMake（版本 3.10 或更高）
- C++17 兼容编译器
- 安装和运行服务需要 root 权限

#### 构建和安装

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

这将：

- 编译 `touchpad-enhance`（守护进程）和 `touchpad-enhance-ctl`（IPC 客户端）
- 将两个二进制文件安装到 `/usr/local/bin/`
- 安装、启用并启动 systemd 服务

#### 卸载

```bash
cd build
sudo make uninstall
```

## 配置

所有编译期参数都是 CMake cache 变量。可以在命令行设置，也可以通过 NixOS 模块选项设置（最终也走同一套 cmake 标志）。

| CMake 标志          | NixOS 选项        | 默认值  | 含义                                              |
|---------------------|-------------------|---------|---------------------------------------------------|
| `INVERTX`           | `invertX`         | `OFF`   | 交换左右边缘分别控制的功能（亮度/音量）。         |
| `INVERTY`           | `invertY`         | `OFF`   | 反转上滑/下滑。                                   |
| `TOUCHPAD_DEVICE`   | `touchpadDevice`  | `""`    | 指定具体的 `/dev/input/eventN`。空 = 自动检测。   |
| `EDGE_WIDTH`        | `edgeWidth`       | `0.05`  | 边缘区域占触摸板总宽度的比例。                    |
| `SCROLL_THRESHOLD`  | `scrollThreshold` | `0.2`   | 触发一次滚动的最小垂直位移（占总高度的比例）。    |
| `SCROLL_STEP`       | `scrollStep`      | `0.05`  | 之后每发出一次滚动事件所需的位移。                |

CMake 示例：

```bash
cmake -DINVERTY=ON -DTOUCHPAD_DEVICE=/dev/input/event5 -DEDGE_WIDTH=0.04 ..
```

守护进程的**初始状态**也可以在启动时通过第一个命令行参数指定，无需重新编译：

```bash
touchpad-enhance inactive      # 或：active | disable-touchpad
```

NixOS 模块会自动把 `services.touchpad-enhance.initialState` 传到这里。

## 使用方法

安装后，服务会在后台自动运行。

- **亮度控制**：触摸触摸板左侧边缘并垂直滑动。
- **音量控制**：触摸触摸板右侧边缘并垂直滑动。
- 滑动方向可用 `INVERTY` 反转。

### 运行时状态切换

用 `touchpad-enhance-ctl` 可以在不重启守护进程的前提下查询或切换状态：

```bash
touchpad-enhance-ctl get                    # 打印当前状态
touchpad-enhance-ctl active                 # 完整的边缘滑动处理
touchpad-enhance-ctl inactive               # 仅原样转发
touchpad-enhance-ctl disable-touchpad       # 完全屏蔽触摸板
```

CLI 在收到 `OK` 时退出码为 `0`，守护进程错误为 `1`，参数错误为 `2`——适合绑定到快捷键或脚本里。

示例：打字时临时关闭触摸板的窗口管理器快捷键脚本：

```sh
if [ "$(touchpad-enhance-ctl get)" = "OK active" ]; then
  touchpad-enhance-ctl disable-touchpad
else
  touchpad-enhance-ctl active
fi
```

## 故障排除

- 确保服务正在运行：`systemctl status touchpad-enhance`
- 检查日志：`journalctl -u touchpad-enhance`
- 验证触摸板设备：`ls /dev/input/event*`
- 确认 uinput 设备存在且分类正确：`libinput debug-events` 应当能看到来自本守护进程的一个 `Touchpad` 和一个 `Keyboard`
- 多触摸板系统请用 `TOUCHPAD_DEVICE` / `touchpadDevice` 明确指定
- 如果守护进程在跑但音量/亮度键完全不起作用，最常见的原因是触发设备没有放在独立的键盘 uinput 上——请确保使用的是双设备重构之后的版本
- 出现权限问题时请用 root 权限测试

## 贡献

欢迎贡献！请随时提交 issue 和 PR。

## 许可证

本项目采用 GNU 通用公共许可证 v3.0 —— 详情请见 [LICENSE](./LICENSE) 文件。
