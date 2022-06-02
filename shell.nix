{ pkgs ? import <nixpkgs> {} }:

with pkgs;

let
  version = "0.52.0";

  libstdcpp-preload-for-unvanquished-nacl = stdenv.mkDerivation {
    name = "libstdcpp-preload-for-unvanquished-nacl";
    buildCommand = ''
      mkdir $out/etc -p
      echo ${gcc.cc.lib}/lib/libstdc++.so.6 > $out/etc/ld-nix.so.preload
    '';
    propagatedBuildInputs = [ gcc.cc.lib ];
  };

  fhsEnv = buildFHSUserEnv {
    name = "unvanquished-fhs-wrapper";
    targetPkgs = pkgs: [ libstdcpp-preload-for-unvanquished-nacl ];
    runScript = "./run-wrapper";
  };

  unvanquished-assets = stdenv.mkDerivation {
    pname = "unvanquished-assets";
    inherit version;
    src = ./download-paks;

    outputHash = "sha256:1fy85cjnjk9rrqkhgx5701inff2yv14hnxglzx3209c553gn31n7";
    outputHashMode = "recursive";
    nativeBuildInputs = [ aria2 cacert ];
    buildCommand = "bash $src $out"; # FIXME
  };

  deps = [
    libGL zlib ncurses geoip lua5 pkg-config meson
    nettle curl SDL2 freetype glew openal
    libopus opusfile libogg libvorbis libjpeg libwebp libpng
  ];

in
  mkShell {
    buildInputs = deps;
    # nativeBuildInputs is usually what you want -- tools you need to run
    nativeBuildInputs = [
      cmake ninja gdb ccache ccacheWrapper valgrind
      python2 python27Packages.jinja2 python27Packages.pyyaml # CBSE
      binutils-unwrapped fhsEnv libstdcpp-preload-for-unvanquished-nacl
    ];
    shellHook = ''
        export PAKPATH=${unvanquished-assets}
        export _ASAN=${gcc.cc.lib}/lib/libasan.so
        export PATH=${ccacheWrapper}/bin:$PATH
    '';
  }
