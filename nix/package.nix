{ stdenv, lib, cmake
, invertX ? false
, invertY ? false
, touchpadDevice ? ""
, edgeWidth ? 0.05
, scrollThreshold ? 0.2
, scrollStep ? 0.05
}:

stdenv.mkDerivation {
  pname = "touchpad-enhance";
  version = "1.0.0";

  # Use lib.fileset so a stray local build/ (or any other untracked dir) does
  # not leak into the derivation. Otherwise a stale CMakeCache.txt from a
  # local cmake run poisons nix's configurePhase.
  src = lib.fileset.toSource {
    root = ./..;
    fileset = lib.fileset.gitTracked ./..;
  };

  nativeBuildInputs = [ cmake ];

  cmakeFlags = [
    "-DINVERTX=${if invertX then "ON" else "OFF"}"
    "-DINVERTY=${if invertY then "ON" else "OFF"}"
    "-DTOUCHPAD_DEVICE=${touchpadDevice}"
    "-DEDGE_WIDTH=${toString edgeWidth}"
    "-DSCROLL_THRESHOLD=${toString scrollThreshold}"
    "-DSCROLL_STEP=${toString scrollStep}"
  ];

  installPhase = ''
    runHook preInstall
    install -Dm755 touchpad-enhance $out/bin/touchpad-enhance
    install -Dm755 touchpad-enhance-ctl $out/bin/touchpad-enhance-ctl
    runHook postInstall
  '';
}
