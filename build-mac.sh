#!/bin/sh
set -eu
cd "$(dirname "$0")"
command -v geode >/dev/null 2>&1 || { echo "Geode CLI is not installed or not on PATH."; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "CMake is not installed or not on PATH yet."; exit 1; }
geode build
