dirhash-cpp ([v0.4.6](https://github.com/kusumi/dirhash-cpp/releases/tag/v0.4.6))
========

## About

+ Recursively walk directory trees and print message digest of regular files.

+ C++ version of [https://github.com/kusumi/dirhash-rs](https://github.com/kusumi/dirhash-rs).

## Supported platforms

Unix-likes in general

## Requirements

C++20

## Build

    $ make

## Usage

    $ ./build/src/dirhash-cpp -h
    Usage: ./build/src/dirhash-cpp [options] <paths>
    Options:
      --hash_algo - Hash algorithm to use (default "sha256")
      --hash_verify - Message digest to verify in hex string
      --hash_only - Do not print file paths
      --ignore_dot - Ignore entries start with .
      --ignore_dot_dir - Ignore directories start with .
      --ignore_dot_file - Ignore files start with .
      --ignore_symlink - Ignore symbolic links
      --follow_symlink - Follow symbolic links unless directory
      --abs - Print file paths in absolute path
      --swap - Print file path first in each line
      --sort - Print sorted file paths
      --squash - Print squashed message digest instead of per file
      --verbose - Enable verbose print
      --debug - Enable debug mode
      -v, --version - Print version and exit
      -h, --help - Print usage and exit
