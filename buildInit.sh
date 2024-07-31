#!/bin/bash

gcc build/build.c -o bin/build -Wall -Wextra -flto -O3 -fPIE

./bin/build
