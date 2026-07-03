{ config, lib, pkgs, ... }:

let
  cfg = config.services.touchpad-enhance;

  pkg = pkgs.callPackage ./package.nix {
    inherit (cfg) invertX invertY touchpadDevice edgeWidth scrollThreshold scrollStep;
  };
in {
  options.services.touchpad-enhance = {
    enable = lib.mkEnableOption "Touchpad Enhance service";

    invertX = lib.mkOption {
      type = lib.types.bool;
      default = false;
      description = ''
        Invert the X axis. Swaps which edge controls brightness vs volume.
      '';
    };

    invertY = lib.mkOption {
      type = lib.types.bool;
      default = false;
      description = ''
        Invert the Y axis. Swaps swipe-up vs swipe-down direction.
      '';
    };

    touchpadDevice = lib.mkOption {
      type = lib.types.str;
      default = "";
      example = "/dev/input/event5";
      description = ''
        Touchpad device file to use. Leave empty to auto-detect.
        Set explicitly when multiple touchpads are present.
      '';
    };

    edgeWidth = lib.mkOption {
      type = lib.types.float;
      default = 0.05;
      description = ''
        Width of the edge region as a fraction of total touchpad width.
      '';
    };

    scrollThreshold = lib.mkOption {
      type = lib.types.float;
      default = 0.2;
      description = ''
        Minimal vertical movement (as a fraction of touchpad height)
        required to trigger a scroll action.
      '';
    };

    scrollStep = lib.mkOption {
      type = lib.types.float;
      default = 0.05;
      description = ''
        Movement step (as a fraction of touchpad height) for each
        emulated scroll event after the threshold is reached.
      '';
    };

    initialState = lib.mkOption {
      type = lib.types.enum [ "active" "inactive" "disable-touchpad" ];
      default = "active";
      description = ''
        State the daemon starts in. Use `touchpad-enhance-ctl` to switch
        states at runtime:
          - `active`: edge-swipe is processed and all other events forwarded.
          - `inactive`: every event is forwarded verbatim, no processing.
          - `disable-touchpad`: the (grabbed) touchpad is silenced entirely.
      '';
    };
  };

  config = lib.mkIf cfg.enable {
    environment.systemPackages = [ pkg ];

    systemd.services.touchpad-enhance = {
      description = "Touchpad Enhance - edge swipe for brightness and volume";
      documentation = [ "https://github.com/suif4599/touchpad-enhance" ];
      after = [ "multi-user.target" ];
      wantedBy = [ "multi-user.target" ];

      serviceConfig = {
        Type = "simple";
        User = "root";
        Group = "root";
        ExecStart = "${pkg}/bin/touchpad-enhance ${cfg.initialState}";
        Restart = "always";
        RestartSec = 3;
        StandardOutput = "journal";
        StandardError = "journal";
      };
    };
  };
}
