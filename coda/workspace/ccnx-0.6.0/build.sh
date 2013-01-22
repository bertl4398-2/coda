#!/bin/bash
make clean
./configure
make MORE_LDLIBS="-pie -lm"
