# Touchpad Enhance

[中文版本](./README_CN.md)

A Linux utility that emulates some features of Huawei's pressure-sensitive touchpad, specifically the brightness and volume control functions on the left and right edges.

## Features

- **Edge-based Control**: Swipe on the left edge to adjust screen brightness, right edge for volume control.
- **Automatic Detection**: Automatically detects and uses the first available touchpad device.
- **Manual Device Selection**: Option to manually specify touchpad device for systems with multiple touchpads.
- **Configurable**: Adjustable edge width, scroll thresholds, and axis inversion.
- **Systemd Integration**: Runs as a background service for seamless operation.

## Compatibility

This solution is based on the Linux input subsystem standard and is suitable for:

- Modern Linux systems (typically kernel version ≥ 3.0)
- Touchpad devices that support EV_ABS (absolute coordinates) events
- Touchpads correctly configured as input subsystem devices
- Touchpads using libinput or synaptics drivers (common in most laptops)

### Incompatible Cases

- Older Linux systems (kernel version < 3.0)
- Touchpads that only support relative coordinates (EV_REL)
- Custom touchpads that do not properly implement the Linux input subsystem standard
- Some embedded Linux devices (may use different input handling methods)

## Installation

### Prerequisites

- CMake (version 3.10 or higher)
- C++17 compatible compiler
- Root privileges for installation and running the service

### Build and Install

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

This will:
- Compile the application
- Install the binary to `/usr/local/bin/`
- Install and enable the systemd service
- Start the service automatically

### Uninstall

```bash
cd build
sudo make uninstall
```

## Configuration

The application can be configured through CMake options:

- `INVERTX`: Invert X axis (default: OFF)
- `INVERTY`: Invert Y axis (default: OFF)
- `TOUCHPAD_DEVICE`: Specify touchpad device file (e.g., /dev/input/event5) for systems with multiple touchpads (default: auto-detect)

Example with inverted Y axis and manual device selection:

```bash
cmake -DINVERTY=ON -DTOUCHPAD_DEVICE=/dev/input/event5 ..
```

Additional parameters can be modified in `config.h`:

- `EDGE_WIDTH`: Width of the edge region as a fraction of total touchpad width (default: 0.05)
- `SCROLL_THRESHOLD`: Minimal vertical movement to trigger a scroll action (default: 0.2)
- `SCROLL_STEP`: Movement step for each scroll action (default: 0.05)

## Usage

Once installed, the service runs automatically in the background. No user interaction is required.

- **Brightness Control**: Touch and swipe vertically on the left edge of the touchpad
- **Volume Control**: Touch and swipe vertically on the right edge of the touchpad

The direction of swiping (up/down) can be inverted using the `INVERTY` option.

## Troubleshooting

- Ensure the service is running: `systemctl status touchpad-enhance`
- Check logs: `journalctl -u touchpad-enhance`
- Verify touchpad device: `ls /dev/input/event*`
- For multiple touchpads, specify the device using `TOUCHPAD_DEVICE` option
- Test with root privileges if permission issues occur

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](./LICENSE) file for details.