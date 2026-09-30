{
    description = "hyprliquid";

    inputs =
    {
        hyprland.url    = "github:hyprwm/Hyprland";
        nixpkgs.follows = "hyprland/nixpkgs";
        systems.follows = "hyprland/systems";
    };

    outputs = { self, nixpkgs, systems, ... }:
    let
        forAllSystems = nixpkgs.lib.genAttrs (import systems);

        makeDerivation = system:
        let
            pkgs     = nixpkgs.legacyPackages.${system};
            hyprland = pkgs.hyprland;
        in hyprland.stdenv.mkDerivation
        {
            pname = "hyprliquid";
            version = "0.2.1";
            src = self;
            nativeBuildInputs = with pkgs;
            [
                cmake
                pkg-config
            ];
            buildInputs =
            [
                hyprland.dev
                pkgs.stb
            ] ++ hyprland.buildInputs;
            cmakeFlags =
            [
                "-DCMAKE_BUILD_TYPE=Release"
            ];

            meta = with nixpkgs.lib;
            {
                description = "Liquid Glass, Acrylic, Mica and Aero material effects for Hyprland";
                homepage    = "https://github.com/zaregototsukai/hyprliquid";
                license     = licenses.bsd3;
                platforms   = platforms.linux;
            };
        };
    in
    {
        packages = forAllSystems (system: rec
        {
            default = makeDerivation system;
            hyprliquid = default;
        });

        homeManagerModules.default = { config, lib, pkgs, ... }:
        let
            cfg = config.wayland.windowManager.hyprland.hyprliquid;
            system = pkgs.stdenv.hostPlatform.system;
        in
        {
            options.wayland.windowManager.hyprland.hyprliquid.enable = lib.mkEnableOption "hyprliquid";
            config = lib.mkIf cfg.enable
            {
                wayland.windowManager.hyprland.plugins = [ self.packages.${system}.hyprliquid ];
            };
        };
    };
}
