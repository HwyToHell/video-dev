/// TODO
[2018-05-21]
adjust test cases:
logic moved from Config -> FrameHandler
- adjustFrameSizeDependent Parameters
- locateVideoFilePath 
set up test cases:
FrameHandler
- initCam
- initFileReader
- getFrameSize
[2018-04-15]
- cmd line args: -q quiet flag (automatically takes pre-selected items in pick list)
- implement changes in cmd line arg parsing (see ppt in red)
- feed back changed frame size to config
[2018-02-16]
- calcFrameSizeDependendParams
- try catch for "invalid argument exception" by stoi
[2018-02-13]
OK - Config::readCmdLine
[2018-02-03]
- Track::checkPosAndDir
- parameters (to be specified in Config)
	int count_pos (depends on frame_size)
	int count_track_length (depends on frame_size)
	cv::Point2i classify_truck_size (depends on frame_size)
[2017-01-28]
OK - Track::isClose --> TrackEntry
OK - TrackEntry::mVelocity --> Track
- Track --> avgHeight, avgWidth (similar to avgVelocity)
[2017-12-30]
Track
- define struct for parameters (maxDist, maxConfidence, trafficFlow)
OK - passing parameters when constructing track
OK - hold parameters in SceneTracker (update with observer pattern)
[2017-12-27]
Config::init
- move workDir logic to here
- store workPath in ParamList
Config
- read from / write to config only at startup and closedown
FrameHandler
- norm parameters as function of framesize: roi, blob_area, track_max_distance, group_distance 
Subject
- get rid of getDouble, getString -> getParam 
- double: stod(getParam)
- int: stoi(getParam)

Observer.h
- funktionsobjekt UpdateObserver einführen (als Ersatz für Funktionszeiger updateObserver in Config.cpp)

[2017-12-26]
FrameHandler::openCapSource
define path to video file as config date

rename: 
frame_handling.h, *.cpp -> frame_handler.h, *.cpp
tracking.h 	-> tracker.h
scene.cpp, vehicle.cpp, track.cpp -> tracker.cpp
Scene -> Tracker

[2017-11-02]
done: create sqlite3/bin /inc /lib /src for std setup (comparable to linux)
done: create all params in Config::Init
setup test cases

