#!/bin/sh

exec nix shell	nixpkgs/nixos-20.09#pythonPackages.compiledb \
		nixpkgs/nixos-20.09#clang-tools \
		nixpkgs/nixos-20.09#SDL2.dev \
		nixpkgs/nixos-20.09#SDL2_image \
		nixpkgs/nixos-20.09#pkg-config \
		nixpkgs/nixos-20.09#glew.dev

# set -x PKG_CONFIG_PATH (nix eval --raw nixpkgs/nixos-20.09#glew.dev.outPath)/lib/pkgconfig $PKG_CONFIG_PATH
# set -x PKG_CONFIG_PATH (nix eval --raw nixpkgs/nixos-20.09#SDL2_image.outPath)/lib/pkgconfig $PKG_CONFIG_PATH
# set -x PKG_CONFIG_PATH (nix eval --raw nixpkgs/nixos-20.09#SDL2.dev.outPath)/lib/pkgconfig $PKG_CONFIG_PATH

# use `compiledb make` instead of `make`
