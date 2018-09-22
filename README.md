
# H1 CarCount Application
========================================================================

# Car counter using opencv
- processes web cam input
- segments motion
- tracks observations and counts cars
- simple classification (length, height)
- experimental: using neural networks (cnn)

# Directory tree
- video-dev (.git, README.md)  
   |  
   |-car-count (TODO.md)  
   |  |-include  
   |  |-src
   |  |-msvc-car-count (windows, visual studio 10) 
   |  |  (msvc-car-count.sln, build directories, ...)
   |  |-qtcr-car-count (linux x64, gcc)
   |  |-raspi-car-count (cross x64 -> arm, gcc toolchain)
   |
   |-car-count-test (test cases for car-count)
   |
   |-cnn (experimental classification with neural networks)
   |
   |-cpp (developmental private libs, used in above projects)
   
# TODO
msvc: change includes: #include "../../car-count/include/config.h"
instead of: #include "../car-count/include/config.h"
and set project location accordingly

# Change history
[2018-09-22]
- separated code directory from IDE project directory

[2018-02-03]
- test cases for vehicle moved to tracker_refactor

[2018-07-24]
- car-counter debugging in MSVC IDE: specify 't' as Command Argument
- car-counter-test: main.cpp options 'not using precompiled headers'
