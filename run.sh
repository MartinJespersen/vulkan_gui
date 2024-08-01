#!/bin/bash
set -e

pushd ./shaders
./compile.sh
popd
make clean
make
./VulkanTest