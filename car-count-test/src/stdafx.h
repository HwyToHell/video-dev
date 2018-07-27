// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#if defined (_WIN32)
#include <direct.h> // for _mkdir, _getcwd
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <cmath>
#include <iostream>
#include <list>
#include <string>

#include <catch.hpp>
#include <opencv2/opencv.hpp>
#include <sqlite3.h>
