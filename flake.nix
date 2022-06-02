{
  inputs = { nixpkgs.url = "github:nixos/nixpkgs/nixos-22.05"; };

  outputs = { self, nixpkgs }:
    let pkgs = nixpkgs.legacyPackages.x86_64-linux;
    in {
      devShell.x86_64-linux =
        import ./shell.nix { inherit pkgs; };
   };
}

