{
  description = "Touchpad Enhance - edge swipe on the touchpad for brightness and volume control";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: let
    systems = [ "x86_64-linux" "aarch64-linux" ];
    forAllSystems = nixpkgs.lib.genAttrs systems;
  in {
    packages = forAllSystems (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      default = pkgs.callPackage ./nix/package.nix { };
      touchpad-enhance = pkgs.callPackage ./nix/package.nix { };
    });

    nixosModules = {
      default = import ./nix/module.nix;
      touchpad-enhance = import ./nix/module.nix;
    };
  };
}
