========================================================================
    CarCount Application
========================================================================


car counter using opencv
- processes web cam input
- segments motion
- tracks observations and counts cars

# ToDo
set up git in ~/app-dev/video-dev
msvc: change includes: #include "../../car-count/include/config.h"
instead of: #include "../car-count/include/config.h"
and set project location accordingly

# change history
[2018-07-24]
- car-counter debugging in MSVC IDE: specify 't' as Command Argument
- car-counter-test: main.cpp options 'not using precompiled headers'


# directory tree
|--video-dev
    |	contains .git, README.md  
    |
    |--car-count
    |	|--include
    |	|--src
    |	|--msvc-car-count
    |	|    contains: project files, build directories (debug, release)
    |	|--qtcr-car-count
    |	|    contains: qt creator project files
    |	|--debug-car-count
    |	|    contains: build files (x64 architecture)
    |
    |--car-count-test: as above for test cases
    |
    |--cpp:
	  private libraries (developmental), used in above projects
