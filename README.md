# Touchpad Enhance

[中文版本](./README_CN.md)

A Linux utility that emulates some features of Huawei's pressure-sensitive touchpad, specifically the brightness and volume control functions on the left and right edges.

## Features

- **Edge-based Control**: Swipe on the left edge to adjust screen brightness, right edge for volume control.
- **Grab + Forward**: The daemon grabs the touchpad exclusively and forwards every event to a uinput mirror device, so it can intercept edge swipes without leaking them to userspace.
- **Three-State Machine**: Switch between `active`, `inactive`, and `disable-touchpad` at runtime via IPC — no restart needed.
- **`touchpad-enhance-ctl`**: A small CLI that talks to the running daemon over an abstract Unix-domain socket.
- **Automatic Detection**: Automatically detects and uses the first available touchpad device.
- **Manual Device Selection**: Option to manually specify touchpad device for systems with multiple touchpads.
- **Configurable**: Adjustable edge width, scroll thresholds, and axis inversion.
- **Systemd / NixOS Integration**: Ships as a systemd service or a NixOS module via the provided flake.

## How it works

The daemon runs as root and performs three jobs:

1. **Grab** the real touchpad with `EVIOCGRAB` so no other process sees the raw events.
2. **Mirror** the touchpad onto a uinput device with identical capabilities (event types, key/abs/rel/msc bits, abs ranges). libinput picks this device up as the touchpad.
3. **Decide per touch** whether the gesture is an edge swipe:
   - If yes and a scroll threshold is reached → emit `KEY_VOLUME*` / `KEY_BRIGHTNESS*` on a separate keyboard-classified uinput device and **drop** the buffered events.
   - Otherwise → **replay** the buffered events so the cursor responds normally.

A third uinput device (keyboard-only) carries the emulated hotkeys. Keeping it separate from the touchpad mirror is what makes both libinput's classification (touchpad = relative motion, keyboard = multimedia keys) and the desktop's hotkey handling work correctly.

## States

| State              | Behavior                                                                  |
|--------------------|---------------------------------------------------------------------------|
| `active`           | Edge-swipe processing on; all non-captured events forwarded. Default.     |
| `inactive`         | Forward every event verbatim; skip all edge detection.                    |
| `disable-touchpad` | Drop everything; the touchpad is effectively silenced.                    |

State is held in the daemon and read on every event. Transitions are immediate — switching to `inactive` mid-touch flushes any buffered events so libinput doesn't see a half-completed touch.

## Compatibility

This solution is based on the Linux input subsystem standard and is suitable for:

- Modern Linux systems (typically kernel version ≥ 3.0, uinput ≥ 4.5 for `UI_ABS_SETUP`)
- Touchpad devices that support EV_ABS (absolute coordinates) events
- Touchpads correctly configured as input subsystem devices
- Touchpads using libinput or synaptics drivers (common in most laptops)

### Incompatible Cases

- Older Linux systems (kernel version < 3.0)
- Touchpads that only support relative coordinates (EV_REL)
- Custom touchpads that do not properly implement the Linux input subsystem standard
- Some embedded Linux devices (may use different input handling methods)

## Installation

### Option A: NixOS (recommended)

Add the flake to your system flake's inputs:

```nix
inputs.touchpad-enhance = {
  url = "path:/home/you/src/touchpad-enhance";
  inputs.nixpkgs.follows = "nixpkgs";
};
```

Then import the module and enable the service:

```nix
{ inputs, ... }: {
  imports = [ inputs.touchpad-enhance.nixosModules.default ];

  services.touchpad-enhance = {
    enable = true;
    edgeWidth = 0.04;        # fraction of touchpad width classified as edge
    scrollThreshold = 0.2;   # vertical travel needed to fire one scroll
    scrollStep = 0.05;       # travel per subsequent scroll event
    initialState = "active"; # active | inactive | disable-touchpad
    # invertX = false;
    # invertY = false;
    # touchpadDevice = "/dev/input/event5";  # only if auto-detect picks the wrong one
  };
}
```

This wires up the systemd service and puts `touchpad-enhance-ctl` on `PATH`. See the **Configuration** section below for the full option list.

### Option B: CMake (manual)

#### Prerequisites

- CMake (version 3.10 or higher)
- C++17 compatible compiler
- Root privileges for installation and running the service

#### Build and Install

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

This will:

- Compile `touchpad-enhance` (the daemon) and `touchpad-enhance-ctl` (the IPC client)
- Install both binaries to `/usr/local/bin/`
- Install, enable, and start the systemd service

#### Uninstall

```bash
cd build
sudo make uninstall
```

## Configuration

All compile-time knobs are CMake cache variables. They can be set on the command line or via the NixOS module options (which feed into the same flags).

| CMake flag         | NixOS option      | Default | Meaning                                                            |
|--------------------|-------------------|---------|--------------------------------------------------------------------|
| `INVERTX`          | `invertX`         | `OFF`   | Swap which edge controls brightness vs volume.                     |
| `INVERTY`          | `invertY`         | `OFF`   | Invert swipe up vs swipe down.                                     |
| `TOUCHPAD_DEVICE`  | `touchpadDevice`  | `""`    | Pin a specific `/dev/input/eventN`. Empty = auto-detect.           |
| `EDGE_WIDTH`       | `edgeWidth`       | `0.05`  | Edge region as a fraction of total width.                          |
| `SCROLL_THRESHOLD` | `scrollThreshold` | `0.2`   | Minimal vertical travel (fraction of height) to fire one scroll.   |
| `SCROLL_STEP`      | `scrollStep`      | `0.05`  | Subsequent travel per emitted scroll event.                        |

CMake example:

```bash
cmake -DINVERTY=ON -DTOUCHPAD_DEVICE=/dev/input/event5 -DEDGE_WIDTH=0.04 ..
```

The initial daemon state is also configurable at startup (no recompile needed) by passing it as the first CLI argument:

```bash
touchpad-enhance inactive      # or: active | disable-touchpad
```

The NixOS module passes `services.touchpad-enhance.initialState` here automatically.

## Usage

Once installed, the service runs automatically in the background.

- **Brightness**: touch and swipe vertically on the left edge.
- **Volume**: touch and swipe vertically on the right edge.
- Swipe direction can be inverted via `INVERTY`.

### Runtime state control

Use `touchpad-enhance-ctl` to query or change the daemon's state without restarting it:

```bash
touchpad-enhance-ctl get                    # print current state
touchpad-enhance-ctl active                 # full edge-swipe processing
touchpad-enhance-ctl inactive               # passthrough only
touchpad-enhance-ctl disable-touchpad       # silence the touchpad entirely
```

The CLI exits `0` on `OK` replies, `1` on daemon errors, and `2` on usage errors — friendly for keybindings and scripts.

Example: a window-manager keybind that toggles the touchpad off while typing:

```sh
if [ "$(touchpad-enhance-ctl get)" = "OK active" ]; then
  touchpad-enhance-ctl disable-touchpad
else
  touchpad-enhance-ctl active
fi
```

## Troubleshooting

- Ensure the service is running: `systemctl status touchpad-enhance`
- Check logs: `journalctl -u touchpad-enhance`
- Verify the touchpad device: `ls /dev/input/event*`
- Confirm the uinput devices exist and are classified correctly: `libinput debug-events` should show a `Touchpad` and a `Keyboard` from this daemon
- For multiple touchpads, specify the device via `TOUCHPAD_DEVICE` / `touchpadDevice`
- If volume/brightness keys do nothing despite the daemon running, the most common cause is the trigger device not being on a separate keyboard uinput — make sure you are on a build after the two-device refactor
- Test with root privileges if permission issues occur

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](./LICENSE) file for details.
