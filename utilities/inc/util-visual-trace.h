#pragma once
#include <map>

#include "../../car-count/include/tracker.h"

/// functions for visual tracing of blob assignment to track and scene tracker
/// for testing purposes and troubleshooting
/// includes fcns for key handling in highgui windows


//////////////////////////////////////////////////////////////////////////////
// Types /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
enum Key {
	arrowUp		= 2490368,
	arrowDown	= 2621440,
	enter		= 13,
	escape		= 27,
	space		= 32,
};

// TODO define as class with show function -> allows for template examineTimeSeries<T>
typedef std::vector<std::list<cv::Rect>> BlobTimeSeries;
typedef std::vector<std::list<Track>> TrackTimeSeries;
typedef std::vector<std::list<TrackState>> TrackStateVec;
typedef std::map<long long, std::list<TrackState>> TrackStateMap;

struct MovingBlob {
	cv::Point	origin;
	cv::Size	size;
	cv::Point	velocity;
	MovingBlob(const cv::Point& origin_, const cv::Size& size_, const cv::Point& velocity_)
        : origin(origin_), size(size_), velocity(velocity_) {}
};


//////////////////////////////////////////////////////////////////////////////
// Global Vars ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
extern TrackStateVec g_trackState;
extern size_t g_idx;
extern TrackStateMap g_trackStateMap;
extern std::map<long long, std::list<TrackState>>::const_iterator g_itCurrent;



//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// key handling for highgui windows
bool breakEscContinueEnter();

/// create occlusion at position x
/// \param[out] trackRight track moving right
/// \param[out] trackRight track moving left
Occlusion createOcclusionAt(Track& trackRight, Track& trackLeft, cv::Size roi,  int collisionX, cv::Size blobSize, cv::Point velocityRight, cv::Point velocityLeft);

/// create track from constant velocity
Track createTrack(cv::Size roi, cv::Rect end, cv::Point velocity, int steps, size_t id = 0);

/// create track at blob position with given velocity
Track createTrackAt(cv::Size roi, cv::Point blobPos, cv::Size blobSize, cv::Point velocity, size_t id = 0);

/// keyboard interface to move through recorded blobs
/// and visualize them in highgui windows with showBlobsAt
bool examineBlobTimeSeries(const BlobTimeSeries& blobTmSer, cv::Size roi);

/// keyboard interface to move through recorded track states
/// and visualize them in highgui windows with showTrackStateAt
bool examineTrackState(const TrackStateVec trackState, cv::Size roi);

/// move blobs through entire roi
BlobTimeSeries moveBlobsThroughRoi(const cv::Size& roi, const MovingBlob& right, const MovingBlob& left);

/// move occlusion through roi
/// starting at collsion point + gap
/// \return vector of list of blobs (cv::Rect)
/// \param[in] roi size of observed frame area
/// \param[in] right moving blob (size, velocity)
/// \param[in] left moving blob (size, velocity)
/// \param[in] collisionX collision point
/// \param[in] gapStartX gap between blobs at start
BlobTimeSeries moveOcclusionThroughRoi(cv::Size roi, MovingBlob right, MovingBlob left, unsigned int collisionX = 0, unsigned int gapStartX = 0);

/// print single blob rect on existing canvas
void printBlob(cv::Mat& canvas, cv::Rect blob, cv::Scalar color);

/// print list of blobs on existing canvas
void printBlobs(cv::Mat& canvas, const std::list<cv::Rect>& blobs);

/// at trace time step idx: print list of blobs on canvas
void printBlobsAt(cv::Mat& canvas, const BlobTimeSeries& timeSeries, const size_t idx);

/// print frame index in upper left corner of existing canvas
void printIndex(cv::Mat& canvas, size_t index);

/// print single occlusion rect on existing canvas
void printOcclusion(cv::Mat& canvas, const Occlusion& occlusion, cv::Scalar color);

/// print list of occlusions
void printOcclusions(cv::Mat& canvas, const std::list<Occlusion>& occlusions);

/// print single track on existing canvas
void printTrack(cv::Mat& canvas, const Track& track, cv::Scalar color);

/// print list of tracks
void printTracks(cv::Mat& canvas, const std::list<Track>& tracks, bool withPrevious = false);

/// at trace time step idx: print list of tracks on canvas
void printTracksAt(cv::Mat& canvas, const TrackTimeSeries& timeSeries, size_t idxUnchecked);

/// print ID, confidence, lenght and velocity of list of tracks
void printTrackInfo(cv::Mat& canvas, const std::list<Track>& tracks);

/// at trace time step idx: print track info (ID, confidence, length, velocity) on canvas
void printTrackInfoAt(cv::Mat& canvas, const TrackTimeSeries& timeSeries, size_t idxUnchecked);

/// print help for stepping through time series with keys in highgui window 
void printUsageHelp();

/// at trace time step idx: show highgui windows for all recorded track states
bool showBlobsAt(const BlobTimeSeries& blobTmSer, cv::Size roi, size_t idxUnchecked);

/// create highgui window and show assignment of blobs to single track
void showBlobAssignment(std::string winName, const Track& track, cv::Rect blob, cv::Size roi, size_t id = 0);

/// at trace time step idx: show highgui windows for all recorded track states
bool showTrackStateAt(const TrackStateVec& state, cv::Size roi, size_t idxUnchecked);
