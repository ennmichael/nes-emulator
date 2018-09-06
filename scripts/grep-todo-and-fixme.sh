#!/usr/bin/env bash

grep -n TODO src/*.cpp src/*.h tests/*.cpp
grep -n FIXME src/*.cpp src/*.h tests/*.cpp

