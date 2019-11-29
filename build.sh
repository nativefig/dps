#!/bin/bash

set -e

g++ -std=c++11 -g -Wall -Wextra -Werror -c dps.cpp -o dps.o
g++ -std=c++11 -g dps.o -o dps
