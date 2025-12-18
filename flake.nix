{
  description = "Lunar Telescope devshell (deterministic local-first preflight)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system);
    in
    {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              # Core toolchain
              gcc
              gnumake
              pkg-config

              # C deps (for WITH_JSONC=1 / CI-equivalent preflight)
              json_c
              json_c.dev

              # Optional accelerators/tooling (for CI-equivalent preflight)
              python3
              rustc
              cargo

              # Local-first strict tooling (IDE/CI parity)
              clang-tools # clang-format/clang-tidy
              cppcheck
              shellcheck
              shfmt
              jq
              curl
              gitleaks
            ];

            shellHook = ''
              # Ensure pkg-config can find json-c (its .pc lives in the dev output).
              export PKG_CONFIG_PATH="${pkgs.json_c.dev}/lib/pkgconfig''${PKG_CONFIG_PATH:+:}$PKG_CONFIG_PATH"

              echo "Lunar Telescope devshell ready."
              echo "  - Recommended: make preflight-ci WITH_RUST=1 WITH_JSONC=1"
              echo "  - Baseline:     make preflight-baseline"
            '';
          };
        });
    };
}


