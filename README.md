
# H1CarCount Application
========================================================================

car counter using opencv
- processes web cam input
- segments motion
- tracks observations and counts cars
- simple classification (length, height)
- experimental: using neural networks (cnn)

change history
[2018-09-22]
- separated code directory from IDE project directory
- video-dev
   |-car-count
   |  |-include
   |  |-src
   |  |-msvc-car-count (windows, visual studio 10) 
   |  |  (msvc-car-count.sln, build directories, ...)
   |  |-qtcr-car-count (linux x64, gcc)
   |  |-raspi-car-count (cross x64 -> arm, gcc toolchain)
   |-car-count-test (test cases for car-count)
   |-cnn (experimental with neural networks)
   |-cpp (own lib files)
   
- 

[2018-02-03]
- test cases for vehicle moved to tracker_refactor

[2018-07-24]
- car-counter debugging in MSVC IDE: specify 't' as Command Argument
- car-counter-test: main.cpp options 'not using precompiled headers'



