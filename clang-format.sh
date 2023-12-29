#!/usr/bin/env bash

script_dir=$(dirname "$(readlink -f "$0")")
cd $script_dir

clang-format -i \
	--style='{BasedOnStyle: WebKit, ColumnLimit: 80}' \
	*.h *.hpp *.cpp \

