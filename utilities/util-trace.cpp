#include "stdafx.h"

#include "../../cpp/inc/id_pool.h"
#include "../../cpp/inc/pick_list.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/frame_handler.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"

// TODO define as class with show function -> allows for template examineTimeSeries<T>
typedef std::vector<std::list<cv::Rect>> BlobTimeSeries;
typedef std::vector<std::list<Track>> TrackTimeSeries;
typedef std::vector<std::list<TrackState>> TrackStateVec;

// glob vars for temporary tracing
TrackStateVec	g_trackState;
size_t			g_idx = 0;


