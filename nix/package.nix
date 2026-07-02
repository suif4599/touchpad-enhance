{ stdenv, cmake
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

  src = ./..;

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
    runHook postInstall
  '';
}
