# simple.nix

{ pkgs ? import <nixpkgs> {} }:


#pkgs.boost.override { enableShared = false; enabledStatic = true; }

let
  my-python = pkgs.python3;
  python-with-my-packages = my-python.withPackages (p: with p; [
    pyarrow
    pybind11
    black
    # pre-commit
    pytest
    isort
    cassandra-driver
    pandas
    tabulate
    jupyter
    pympler
    pytest-benchmark
    wheel
    build
  ]);
  #boost = pkgs.boost.override { enableShared = false; enabledStatic = true; };
  
in
pkgs.mkShell {
  buildInputs = [
    pkgs.gccStdenv
    pkgs.gbenchmark
    pkgs.boost
    pkgs.arrow-cpp
    pkgs.cmake
    pkgs.gcc
    pkgs.protobuf
    pkgs.boost
    pkgs.libxcrypt
    pkgs.libkrb5
    pkgs.clang-tools
    python-with-my-packages
  ];

}



