#!/bin/sh
exec nix shell nixpkgs/nixos-20.09#SDL2.dev nixpkgs/nixos-20.09#pythonPackages.compiledb nixpkgs/nixos-20.09#clang-tools
# use `compiledb make` instead of `make`
