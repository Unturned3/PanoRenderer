#!/usr/bin/env bash

script_dir=$(dirname "$(readlink -f "$0")")
cd $script_dir

clang-format -i -style=file \
	*.h *.hpp *.cpp

