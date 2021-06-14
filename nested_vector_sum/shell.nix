{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.clangStdenv,
  cmdlineSrc ? pkgs.fetchFromGitHub {
    owner  = "deepsea-inria";
    repo   = "cmdline";
    rev    = "67b01773169de11bf04253347dd1087e5863a874";
    sha256 = "1bzmxdmnp7kn6arv3cy0h4a6xk03y7wdg0mdkayqv5qsisppazmg";
  },
  cmdline ? import "${cmdlineSrc}/script/default.nix" {},
  mcslSrc ? pkgs.fetchFromGitHub {
    owner  = "mikerainey";
    repo   = "mcsl";
    rev    = "32a64e3764bcf20076b7bbb3cacc6caa3884a12a";
    sha256 = "016q1jdc05559sc2sbx72a3zi37v6iw6sqa2wv7n6fvxscdf748r";
  },
  mcsl ? import "${mcslSrc}/nix/default.nix" {}
}:

stdenv.mkDerivation {
  name = "clang-nix-shell";
  buildInputs = [ mcsl ];
  shellHook =
  ''
    export MCSL_PATH="${mcsl}/include"
    export CMDLINE_PATH="${cmdline}/include"
  '';
}
  
