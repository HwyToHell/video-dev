#include "stdafx.h"

#include "../../../cpp/inc/id_pool.h"
#include "../../../cpp/inc/pick_list.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/config.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/frame_handler.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"
#include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"

// status variables for visual tracking
extern TrackStateVec g_trackState;
extern size_t g_idx;

//////////////////////////////////////////////////////////////////////////////
// Local functions ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
BlobTimeSeries moveThroughRoi(const cv::Size& roi, const MovingBlob& right, const MovingBlob& left);



//////////////////////////////////////////////////////////////////////////////
// Implementation ////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
cv::Rect clipAtRoi(cv::Rect rec, cv::Size roi) {
	cv::Rect rcRoi(cv::Point(0,0), roi);
	return rec & rcRoi;
}


/// create occlusion at collsionX
BlobTimeSeries createOcclusion(const cv::Size& roi, const cv::Size& blob, unsigned int collisionX = 0) {
	// collision point
	int colX = (collisionX == 0) ? roi.width / 2 : collisionX;
	colX = (colX > roi.width) ? roi.width : colX;
	cv::Point colOrgLeft(colX,70);
	cv::Point colOrgRight(colX-blob.width, 70);
	
	// gap between blobs at beginning of movement
	int minGapX(20);

	// velocities
	cv::Point veloLeft(-5,0);
	cv::Point veloRight(6,0);

	// update steps
	int nUpdates = ceil (static_cast<double>(minGapX) / (abs(veloLeft.x) + abs(veloRight.x)) );

	// origin for blob moves to left
	cv::Point startOrgLeft = colOrgLeft - nUpdates * veloLeft;
	
	// origin for blob moves to right		
	cv::Point startOrgRight = colOrgRight - nUpdates * veloRight; // - cv::Point(blob.width,0);
	
	MovingBlob right(startOrgRight, blob, veloRight);
	MovingBlob left(startOrgLeft, blob, veloLeft);
	BlobTimeSeries blobTimeSeries = moveThroughRoi(roi, right, left);

	return blobTimeSeries;
}


/// create track from constant velocity
Track createTrack(cv::Size roi, cv::Rect end, cv::Point velocity, int steps, int id = 0) {
	Track track;
	cv::Rect rc(end.tl() - velocity * (steps - 1), end.size());
	for (int i = 0; i < steps; ++i) {
		// addTrackEntry modifies rc, if out of roi, thus copy needed
		cv::Rect rcAdd = rc; 
		track.addTrackEntry(rcAdd, roi);
		rc += velocity;
	}
	return track;
}


/// move blobs through entire roi
BlobTimeSeries moveThroughRoi(const cv::Size& roi, const MovingBlob& right, const MovingBlob& left) {
	using namespace std;
	vector<list<cv::Rect>> timeSeries;

	// check velocity direction
	if (right.velocity.x <= 0) {
		cout << "velocity to right must be greater than 0" << endl;
		return timeSeries;
	}
	if (left.velocity.x >= 0) {
		cout << "velocity to left must be less than 0" << endl;
		return timeSeries;
	}

	// check blob inside roi
	cv::Rect roiRect(cv::Point(0,0), roi);
	cv::Rect rightRect(right.origin, right.size);
	if ( (roiRect & rightRect).empty() ) {
		cout << "blob right: " << rightRect << " outside roi" << endl;
		return timeSeries;
	}
	cv::Rect leftRect(left.origin, left.size);
	if ( (roiRect & leftRect).empty() ) {
		cout << "blob left: " << leftRect << " outside roi" << endl;
		return timeSeries;
	}

	// update steps to go through roi in x coord
	int nRight = (roi.width - right.origin.x) / right.velocity.x;
	int nLeft = (left.origin.x + left.size.width) / std::abs(left.velocity.x);
	int n = nRight > nLeft ? nRight : nLeft;

	// push blobs to list for all update steps
	for (; n >=0; --n) {
		list<cv::Rect> blobs;

		// combine rects, if intersection
		// clip rects, if outside roi
		cv::Rect intersection = rightRect & leftRect;
		if ( intersection.empty() ) {
			blobs.push_back(clipAtRoi(rightRect, roi));
			blobs.push_back(clipAtRoi(leftRect, roi));
		} else {
			blobs.push_back(clipAtRoi(rightRect | leftRect, roi));
		}
		
		timeSeries.push_back(blobs);
		rightRect += right.velocity;
		leftRect += left.velocity;
	}
	return timeSeries;
}




int main(int argc, char* argv[]) {
	using namespace std;
	typedef std::list<Occlusion> Occlusions;

	// visual debugging of assignment process in occlusion
	// create two blobs moving in opposite direction
	IdGen occIDs(6);
	cv::Size roi(100, 100);
	cv::Size sizeBlob(30,20);

	// set up scene
	Config config;
	Config* pConfig = &config;
	setRoiToConfig(pConfig, roi);
	SceneTracker scene(pConfig); 
	SceneTracker* pScene = &scene;
	config.attach(pScene);

	// test occlusion creation
	//waitForEnter();
	//return 0;

	// create blobs for occlusion
	BlobTimeSeries blobTmSer = createOcclusion(roi, sizeBlob, 20);
	// visualize blobs for debugging purposes
	//examineBlobTimeSeries(blobTmSer, roi);
	//return 0;

	
	cv::Mat canvas(roi, CV_8UC3, black);

	// loop through blobList
	for (g_idx = 0; g_idx < blobTmSer.size(); ++g_idx) {
		cout << endl << "new idx: " << g_idx << endl;

		// copy blob list of current time step to avoid deletion
		std::list<cv::Rect> currentBlobs(blobTmSer[g_idx]);
		std::list<Track>* pTracks =  scene.updateTracks(currentBlobs, g_idx);
		showTrackStateAt(g_trackState, roi, g_idx);
		if (breakEscContinueEnter())
			break;

	}

	examineTrackState(g_trackState, roi);

	waitForEnter();
	return 0;
}