# door
  Transport Door Chime Program for Raspberry Pi

  sudo apt-get install libasound2-dev libfftw3-dev libsndfile1-dev
  gcc -o door -lasound -lm -lfftw3 -lsndfile door.c

  Check capture device:
   arecord -l
  Change capture input level:
   alsamixer

  ./door hw:1 &

