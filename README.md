# Touchpad Enhance

[中文版本](./README_CN.md)

A Linux utility that emulates some features of Huawei's pressure-sensitive touchpad, specifically the brightness and volume control functions on the left and right edges.

The daemon grabs the touchpad exclusively (`EVIOCGRAB`), mirrors it onto a uinput device, and forwards events through that mirror — so your desktop behaves exactly as before, plus the edge-swipe gestures. A separate uinput keyboard device delivers the emulated brightness/volume key events. A `touchpad-enhance-ctl` client talks to the daemon over an abstract Unix-domain socket and can switch between four runtime states without restarting the service.

## Features

- **Edge-based Control**: Swipe on the left edge to adjust screen brightness, right edge for volume control.
- **Grab + Forward**: The original touchpad is grabbed exclusively and re-emitted through a uinput mirror device, so the daemon can selectively suppress events that trigger gestures.
- **Four Runtime States** (`active` / `inactive` / `disable-touchpad` / `control-only`), switchable live via `touchpad-enhance-ctl`.
- **Automatic Detection**: Automatically detects and uses the first available touchpad device.
- **Manual Device Selection**: Option to manually specify touchpad device for systems with multiple touchpads.
- **Configurable**: Adjustable edge width, scroll thresholds, and axis inversion.
- **Systemd Integration**: Runs as a background service for seamless operation.
- **NixOS Flake**: First-class flake + NixOS module.

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

## Runtime States

The daemon holds one of four states. `touchpad-enhance-ctl <state>` switches between them at runtime; the daemon's `initialState` (NixOS option or CLI argument) sets the startup default.

| State | Edge-swipe gestures | Other events forwarded | Use case |
|---|---|---|---|
| `active` (default) | Yes | Yes | Normal operation |
| `inactive` | No | Yes | Disable gestures, touchpad still works normally |
| `disable-touchpad` | No | No | Fully silence the touchpad |
| `control-only` | Yes | No | Gestures work but the cursor never moves |

## Installation

### NixOS (recommended)

Add the flake to your system flake's inputs:

```nix
# flake.nix
inputs.touchpad-enhance = {
  url = "path:/path/to/touchpad-enhance";
  inputs.nixpkgs.follows = "nixpkgs";
};
```

Then import the module and enable the service:

```nix
# configuration.nix / a module in your flake
{ inputs, ... }: {
  imports = [ inputs.touchpad-enhance.nixosModules.default ];

  services.touchpad-enhance = {
    enable = true;
    edgeWidth = 0.04;        # fraction of touchpad width treated as edge
    scrollThreshold = 0.2;   # minimal swipe distance to trigger
    scrollStep = 0.05;       # scroll step
    initialState = "active"; # active | inactive | disable-touchpad | control-only
    # touchpadDevice = "/dev/input/event5";  # only if auto-detect picks the wrong one
  };
}
```

This wires up the systemd service (running as root, since `EVIOCGRAB` and `/dev/uinput` require it) and puts both `touchpad-enhance` and `touchpad-enhance-ctl` on `PATH`.

All options are also exposed as NixOS options — changing any of them rebuilds the package with the new compile-time constants.

### CMake (other distros)

#### Prerequisites

- CMake (version 3.10 or higher)
- C++17 compatible compiler
- Root privileges for installation and running the service

#### Build and Install

```bash
cmake -B build
cmake --build build
sudo cmake --install build
```

This will:

- Compile both `touchpad-enhance` and `touchpad-enhance-ctl`
- Install the binaries to `/usr/local/bin/`
- Install, enable, and start the systemd service

#### Uninstall

```bash
sudo cmake --build build --target uninstall
```

## Configuration

### Edge / scroll behavior

Both the NixOS module options and the CMake variables below adjust the same compile-time constants. Pick the interface that matches how you installed.

| NixOS option | CMake variable | Default | Meaning |
|---|---|---|---|
| `edgeWidth` | `EDGE_WIDTH` | `0.05` | Width of the edge region, as a fraction of total touchpad width |
| `scrollThreshold` | `SCROLL_THRESHOLD` | `0.2` | Minimal vertical movement to trigger a scroll action |
| `scrollStep` | `SCROLL_STEP` | `0.05` | Movement step for each subsequent scroll action |
| `invertX` | `INVERTX` | `false`/`OFF` | Swap which edge is brightness vs. volume |
| `invertY` | `INVERTY` | `false`/`OFF` | Swap swipe-up vs. swipe-down |
| `touchpadDevice` | `TOUCHPAD_DEVICE` | `""`/auto | Force a specific `/dev/input/eventN` |

CMake example with inverted Y axis and manual device selection:

```bash
cmake -B build -DINVERTY=ON -DTOUCHPAD_DEVICE=/dev/input/event5
```

## Usage

Once installed, the service runs automatically in the background.

- **Brightness Control**: Touch and swipe vertically on the left edge of the touchpad.
- **Volume Control**: Touch and swipe vertically on the right edge of the touchpad.

Switch states at runtime:

```bash
touchpad-enhance-ctl get                 # show current state
touchpad-enhance-ctl active              # gestures on, touchpad passed through
touchpad-enhance-ctl inactive            # gestures off, touchpad passed through
touchpad-enhance-ctl control-only        # gestures on, cursor frozen
touchpad-enhance-ctl disable-touchpad    # everything silenced
```

The direction of swiping (up/down) can be inverted via `INVERTY` / `invertY`.

## Troubleshooting

- Ensure the service is running: `systemctl status touchpad-enhance`
- Check logs: `journalctl -u touchpad-enhance`
- Verify the touchpad device: `ls /dev/input/event*`
- For multiple touchpads, specify the device via `TOUCHPAD_DEVICE` / `touchpadDevice`
- Inspect what libinput sees from the daemon's devices: `sudo libinput debug-events` — you should see one `Touchpad` device and one `Keyboard` device ("Touchpad Enhancer Hotkeys"). If libinput classifies the touchpad device as a `Tablet`, edge-swipe cursor behavior will be wrong.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](./LICENSE) file for details.
