#!/bin/sh

# sudo apt-get install libfftw3-dev libsndfile1-dev

gcc -g -O3 -o door -lasound door.c -lm -lfftw3 -lsndfile

# ./alsa-record-example hw:0
