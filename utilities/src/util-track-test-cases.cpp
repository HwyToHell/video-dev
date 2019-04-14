#include "stdafx.h"

#include "../../../cpp/inc/pick_list.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/config.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/frame_handler.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"
#include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"

// status variables for visual tracking
extern TrackStateVec g_trackState;
extern size_t g_idx;

// util-random
void printRandomValues();

//////////////////////////////////////////////////////////////////////////////
// Local functions ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////



int main(int argc, char* argv[]) {
	using namespace std;
	// visual debugging of assignment process in occlusion
	// set up scene
	IdGen occIDs(6);
	cv::Size roi(100, 100);
	Config config;
	Config* pConfig = &config;
	setRoiToConfig(pConfig, roi);
	SceneTracker scene(pConfig); 
	SceneTracker* pScene = &scene;
	config.attach(pScene);

	// create blobs for occlusion
	cv::Size rightSize(60,20);
	cv::Point rightVelocity(5,0);
	MovingBlob right(cv::Point(0,0), rightSize, rightVelocity);

	cv::Size leftSize(30,20);
	cv::Point leftVelocity(-6,0);
	MovingBlob left(cv::Point(0,0), leftSize, leftVelocity);

	BlobTimeSeries blobTmSer = moveOcclusionThroughRoi(roi, right, left, 20, 20);
	// visualize blobs for debugging purposes
	//examineBlobTimeSeries(blobTmSer, roi);
	//return 0;

	
	//cv::Mat canvas(roi, CV_8UC3, black);

	// loop through blobList
	for (g_idx = 0; g_idx < blobTmSer.size(); ++g_idx) {
		//cout << endl << "new idx: " << g_idx << endl;

		// copy blob list of current time step to avoid deletion
		std::list<cv::Rect> currentBlobs(blobTmSer[g_idx]);
		std::list<Track>* pTracks =  scene.updateTracks(currentBlobs, g_idx);
		//showTrackStateAt(g_trackState, roi, g_idx);
		//if (breakEscContinueEnter()) break;

	}

	examineTrackState(g_trackState, roi);

	waitForEnter();
	return 0;
}