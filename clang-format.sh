#!/usr/bin/env bash

clang-format -i \
	--style='{BasedOnStyle: WebKit, ColumnLimit: 80}' \
	*.h *.hpp *.cpp \
