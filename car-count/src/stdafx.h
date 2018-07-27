// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#if defined (_WIN32)
#include <conio.h>
#include <direct.h>		// for _mkdir, _getcwd
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <assert.h>		// for vibe
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <list>
#include <string>
#include <time.h>		// for vibe

#include <opencv2/opencv.hpp>
#include <sqlite3.h>

