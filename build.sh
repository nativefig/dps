#!/bin/bash

set -e

g++ -std=c++11 -Wall -Wextra -Werror -c dps.cpp -o dps.o
g++ -std=c++11 dps.o -o dps
