{
  description = "C++ DevShell";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
      };
      llvm = pkgs.llvmPackages_18;
    in
    {
      devShells.${system}.default =
        pkgs.mkShell.override
          {
            stdenv = pkgs.clang18Stdenv;
          }
          {
            packages = with pkgs; [
              gnumake
              llvm.lldb
              llvm.libcxx
              llvm.libcxxStdenv
              valgrind
              clang-tools
              cppcheck
              bear
              doctest
            ];

            shellHook = ''
              export CXXFLAGS="-stdlib=libc++"
              export LDFLAGS="-stdlib=libc++ -lc++abi"
            '';
          };

    };
}
