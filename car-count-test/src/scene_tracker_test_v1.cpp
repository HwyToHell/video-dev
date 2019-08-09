#include "stdafx.h"
#include "../../car-count/include/config.h" // includes tracker.h
#include "../../car-count/include/frame_handler.h"

#if defined(__linux__)
    #include "../../utilities/inc/util-visual-trace.h"
#elif(_WIN32)
    #include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"    #pragma warning(disable: 4482) // MSVC10: enum nonstd extension
#endif



// enable visual trace
static bool g_traceScene = false;


//////////////////////////////////////////////////////////////////////////////
// Functions for setup - tear down ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

struct OppositeTracks {
	Track right;
	Track left;
};

/// create tracks that will collide at position x
/// create them steps before collision
/// \param[out] trackRight track moving right
/// \param[out] trackRight track moving left
OppositeTracks createTracksBeforeOcclusion(const cv::Size roi,  const int collisionX, const int stepsBeforeOcclusion,
	const cv::Size blobSize, const cv::Point velocityRight, const cv::Point velocityLeft) {
	
	// adjust collision point, if necessary
    int colXAct(collisionX);
	if (collisionX > roi.width) {
		std::cerr << "collision point: " << collisionX << "outside roi: " << roi.width << std::endl;
		colXAct = roi.width / 2;
		std::cerr << "setting to roi/2: " << colXAct << std::endl; 
	}

	// adjust blob height, if necessary
	cv::Size blobSizeAct(blobSize);
	if ((roi.height - blobSize.height - 10) < 0) {
		blobSizeAct.height = 10;
		std::cerr << "blob height adjusted to: " << blobSizeAct.height << std::endl;
	}
	
	cv::Point colAct(colXAct, roi.height - blobSize.height - 10);
	cv::Point orgRight = colAct - cv::Point(blobSizeAct.width, 0) - (velocityRight * stepsBeforeOcclusion);
	cv::Point orgLeft = colAct - (velocityLeft * stepsBeforeOcclusion);

	OppositeTracks tracks;
	tracks.right = createTrackAt(roi, orgRight, blobSizeAct, velocityRight, 1);
	tracks.left = createTrackAt(roi, orgLeft, blobSizeAct, velocityLeft, 2);

	return tracks;
}


//////////////////////////////////////////////////////////////////////////////
// Scene - ID handling
//////////////////////////////////////////////////////////////////////////////
TEST_CASE("#sce001 getTrackID, returnTrackID", "[Scene]") {
	// scene with no tracks
	using namespace std;
	Config config;
	Config* pConfig = &config;
	SceneTracker scene(pConfig);

	SECTION("trackID is pulled 9 times -> no track id available anymore") {
		for (int i = 0; i < 9; ++i)
            scene.nextTrackID();
		REQUIRE(scene.nextTrackID() == 0);

		SECTION("trackID returned -> is returned 9 times") {
            for (size_t n = 1; n < 10; ++n) {
                REQUIRE(scene.returnTrackID(n));
				//cout << "returned id[" << i << "]: " << i << endl;
			}
			REQUIRE(scene.returnTrackID(10) == false);
			REQUIRE(scene.returnTrackID(0) == false);
		}
	}
}



//////////////////////////////////////////////////////////////////////////////
// Scene - update logic (assgin, combine, delete tracks)
//////////////////////////////////////////////////////////////////////////////
TEST_CASE("#sce002 assignBlobs", "[Scene]") {
// config with two tracks
	using namespace std;
	cv::Size roi(100,100);
	Config config;	
	Config* pConfig = &config;
	config.setParam("roi_width", to_string(static_cast<long long>(roi.width)));
	config.setParam("roi_height", to_string(static_cast<long long>(roi.height)));
	SceneTracker scene(pConfig);
	//cout << config.getParam("roi_height") << endl;

	cv::Size blobSize(40,10);
	cv::Point blobOriginLeft(0, 0);
	cv::Rect blobLeft(blobOriginLeft, blobSize);

	cv::Point blobOriginRight(roi.width - blobSize.width, 0);
	cv::Rect blobRight(blobOriginRight, blobSize);
	
	list<cv::Rect> blobs;
	blobs.push_back(blobLeft);
	blobs.push_back(blobRight);

	// two new tracks are created, each with one history element 
	list<Track>* pTracks = scene.assignBlobs(blobs);
	REQUIRE( 2 == pTracks->size() );
	REQUIRE( 1 == pTracks->front().getHistory() );
	REQUIRE( 1 == pTracks->back().getHistory() );

	SECTION("add two related blobs -> two tracks with two history elements") {
		cv::Point velocity(10,0);
		// moving to right
		blobOriginLeft += velocity;
		blobLeft = cv::Rect(blobOriginLeft, blobSize);
		// moving to left
		blobOriginRight -= velocity;
		blobRight = cv::Rect(blobOriginRight, blobSize);
		// push to list
		blobs.push_back(blobLeft);
		blobs.push_back(blobRight);
		// after update: two history elements
		pTracks = scene.assignBlobs(blobs);
		REQUIRE( 2 == pTracks->size() );
		REQUIRE( 2 == pTracks->front().getHistory() );
		REQUIRE( 2 == pTracks->back().getHistory() );
	}

	SECTION("add two un-related blobs -> two additional tracks will be created") {
		cv::Point velocityUnrelated(0,30);
		// moving to right
		blobOriginLeft += velocityUnrelated;
		blobLeft = cv::Rect(blobOriginLeft, blobSize);
		// moving to left
		blobOriginRight += velocityUnrelated;
		blobRight = cv::Rect(blobOriginRight, blobSize);
		// push to list
		blobs.push_back(blobLeft);
		blobs.push_back(blobRight);

	/* TRACE
	if (g_traceScene) {
		cv::Mat canvas(roi, CV_8UC3, black);
		printTrack(canvas, pTracks->front(), green);
		printTrack(canvas, pTracks->back(), yellow);
		printBlobs(canvas, blobs);
		cv::imshow("un-related blobs", canvas);
		breakEscContinueEnter();
	}
	*/// END_TRACE

		// after update: four history elements
		pTracks = scene.assignBlobs(blobs);
		REQUIRE( 4 == pTracks->size() );
	}
}


// combine tracks
	// two tracks: same direction + intersection -> combine, take the one with longer history
	// two tracks: opposite direction + intersection -> don't combine
TEST_CASE("#sce003 combineTracks", "[Scene]") {
	// start with one long track moving to right (4 history elements)
	using namespace std;
	cv::Size roi(100,100);
	cv::Size blobSize(40,10);
	int nUpdates = 3;
	int velocity = 10;

	int xLongEnd = roi.width - blobSize.width - 2 * velocity;
	cv::Point orgLongBegin(xLongEnd - nUpdates * velocity, 0);
	cv::Rect rcLong(orgLongBegin, blobSize);
	Track trackLong(1);
	trackLong.addTrackEntry(rcLong, roi);
	for (int i=1; i<=nUpdates; ++i) {
		rcLong += cv::Point(velocity, 0);
		trackLong.addTrackEntry(rcLong, roi);
	}
	//cout << "long:" << rcLong << "history: " << trackLong.getHistory() << endl;

	SECTION("tracks with same direction and intersection-> combine, keep the one with longer history") {
		// second track shorter (3 history elements) but intersecting 75% at las entry
		int nUpdates = 2;
		int xShortEnd = xLongEnd - 5;
		cv::Point orgShortBegin(xShortEnd - nUpdates * velocity, 0);
		cv::Rect rcShort(orgShortBegin, blobSize);
		Track trackShort(2);
		trackShort.addTrackEntry(rcShort, roi);
		for (int i=1; i<=nUpdates; ++i) {
			rcShort += cv::Point(velocity, 0);
			trackShort.addTrackEntry(rcShort, roi);
		}
		//cout << "short:" << rcShort << "history: " << trackShort.getHistory() << endl;

		// put short and long track in list
		list<Track> tracks;
		tracks.push_back(trackShort);
		tracks.push_back(trackLong);
		REQUIRE( 3 == tracks.front().getHistory() );
		REQUIRE( 4 == tracks.back().getHistory() );

		// short track will be marked for deletion
		// long resulting gets last element of short track assigned and increases history to 5
		list<Track>* pTracks = combineTracks(tracks, roi);
		REQUIRE( true == pTracks->front().isMarkedForDelete() );
		REQUIRE( rcShort == pTracks->back().getActualEntry().rect() );
		REQUIRE( 5 == pTracks->back().getHistory() );
	}

	SECTION("tracks with opposite direction and intersection-> don't combine") {
		// second track shorter (3 history elements) but intersecting 75% at las entry
		int nUpdates = 2;
		int velocity = -10;
		int xShortEnd = xLongEnd - 5;
		cv::Point orgShortBegin(xShortEnd - nUpdates * velocity, 0);
		cv::Rect rcShort(orgShortBegin, blobSize);
		Track trackShort(2);
		trackShort.addTrackEntry(rcShort, roi);
		for (int i=1; i<=nUpdates; ++i) {
			rcShort += cv::Point(velocity, 0);
			trackShort.addTrackEntry(rcShort, roi);
		}
		//cout << "short:" << rcShort << "history: " << trackShort.getHistory() << endl;

		// put short and long track in list
		list<Track> tracks;
		tracks.push_back(trackShort);
		tracks.push_back(trackLong);
		REQUIRE( 3 == tracks.front().getHistory() );
		REQUIRE( 4 == tracks.back().getHistory() );

		// short track will be marked for deletion
		// long resulting gets last element of short track assigned and increases history to 5
		list<Track>* pTracks = combineTracks(tracks, roi);
		REQUIRE( false == pTracks->front().isMarkedForDelete() );
		REQUIRE( rcShort != pTracks->back().getActualEntry().rect() );
		REQUIRE( 4 == pTracks->back().getHistory() );
	}
}



TEST_CASE("#sce004 deleteMarkedTracks", "[Scene]") {
// config with two tracks (non intersecting), one marked for deletion
	using namespace std;
	Config config;	
	Config* pConfig = &config;
	config.setParam("roi_width", "100");
	config.setParam("roi_height", "100");
	SceneTracker scene(pConfig);

	cv::Size blobSize(40,10);
	cv::Point velocity(10,0);
	int nUpdates = 3;

	cv::Rect blobTop(cv::Point(0,0), blobSize);
	cv::Rect blobBottom(cv::Point(0,50), blobSize);
	
	list<cv::Rect> blobs;
    list<Track>* pTracks = nullptr;
	for (int i=1; i<=nUpdates; ++i) {
		blobTop += velocity;
		blobBottom += velocity;
		blobs.push_back(blobTop);
		blobs.push_back(blobBottom);
		pTracks = scene.assignBlobs(blobs);
		REQUIRE( 0 == blobs.size() );
	}
	// cout << "top:" << pTracks->front().getActualEntry().rect() << " bottom: " << pTracks->back().getActualEntry().rect() << endl;

	// mark track #1 for delete
	pTracks->front().markForDeletion();
	REQUIRE( true == pTracks->front().isMarkedForDelete() );
	// ID #1 will be returned and become available
	REQUIRE( 1 == pTracks->front().getId() );

	SECTION("marked track will be deleted, track ID returned") {
		pTracks = scene.deleteMarkedTracks();
		REQUIRE( 1 == pTracks->size() );
		REQUIRE( 1 == scene.nextTrackID() );
	}

}


TEST_CASE("#sce005 deleteOcclusionWithMarkedTracks", "[Scene]") {
// create occlusion
// go through occlusion
// delete occlusion with marked tracks
}


TEST_CASE("#sce006 deleteReversingTracks", "[Scene]") {
// config with two tracks, #1 reversing, #2 not reversing
	using namespace std;
	Config config;	
	Config* pConfig = &config;
	config.setParam("roi_width", "100");
	config.setParam("roi_height", "100");
	SceneTracker scene(pConfig);

	cv::Size size(40,10);
	cv::Point origin(0,0);
	cv::Point velocity(10,0);
	cv::Rect reversing(origin, size);
	cv::Rect monotonic(origin + cv::Point(0,50), size);

	list<cv::Rect> blobs;
	list<Track>* pTracks;

	// 1st and 2nd with velocity 10
	for (int i=1; i<=2; ++i) {
		reversing += velocity;
		monotonic += velocity;
		blobs.push_back(reversing);
		blobs.push_back(monotonic);
		scene.assignBlobs(blobs);
		//cout << reversing << endl;
	}

	// 3rd reversing -12, 3rd monotonic +10
	reversing -= (velocity*12/10);
	monotonic += velocity;
	blobs.push_back(reversing);
	blobs.push_back(monotonic);
	pTracks = scene.assignBlobs(blobs);
	//cout << reversing << endl;
	
	SECTION("delete reversing track and create new one from last track entry") {
		// first track: #1 before deleting
		REQUIRE( 1 == pTracks->front().getId() );
		REQUIRE( 2 == pTracks->size() );
		cv::Rect lastTrackEntry = pTracks->front().getActualEntry().rect();

		// first track: appended as #3 after deleting, matches last track entry
		pTracks = scene.deleteReversingTracks();
		REQUIRE( 3 == pTracks->back().getId());
		REQUIRE( 2 == pTracks->size() );
		REQUIRE( lastTrackEntry == pTracks->back().getActualEntry().rect() );
	}
}


TEST_CASE("#sce007 isDirectionOpposite", "[Scene]") {
	// two tracks moving into opposite direction
	using namespace std;
	Track right(1);
	Track left(2);		
	cv::Size roi(100,100);

	cv::Size size(20,10);
	cv::Point orgRight(0,0);
	cv::Point orgLeft(80,0);
	cv::Point velocity(5,0);
	int nUpdates = 3;
	
	cv::Rect rcRight(orgRight, size);
	cv::Rect rcLeft(orgLeft, size);
	for (int i=1; i<=nUpdates; ++i) {
		rcRight += velocity;
		rcLeft -= velocity;
		right.addTrackEntry(rcRight, roi);
		left.addTrackEntry(rcLeft, roi);
	}
	//cout << "velocity right: " << right.getVelocity() << " velocity left: " << left.getVelocity() << endl;

	SECTION("direction opposite, velocity outside backlash -> is opposite") {
		double backlash = 0.5;
		REQUIRE( true == isDirectionOpposite(right, left, backlash) );
	}

	SECTION("direction opposite, velocity within backlash -> not opposite") {
		double backlash = 10;
		REQUIRE( false == isDirectionOpposite(right, left, backlash) );
	}
}


TEST_CASE("#sce008 isNextUpdateOccluded", "[Scene]") {
	// two tracks moving into opposite direction
	using namespace std;
	Track right(1);
	Track left(2);		
	cv::Size roi(200,200);
	cv::Size size(30,10);
	cv::Point velocity(10,0);

	// distance after next update = 0,5 * velocity -> another update step necessary
	int distCurr = (velocity.x * 3) + 1;
	int nUpdates = 3;

	// final origin after nUpdates
	cv::Point finLeft(100,0);
	int finRightX = finLeft.x - distCurr - size.width;
	cv::Point finRight(finRightX,0);

	// origin before nUpdates
	cv::Point orgLeft = finLeft + nUpdates * velocity;
	cv::Point orgRight= finRight - nUpdates * velocity;
	//cout << "left org: " << orgLeft << " fin: " << finLeft << endl;
	//cout << "right org: " << orgRight << " fin: " << finRight << endl;	
	
	cv::Rect rcRight(orgRight, size);
	cv::Rect rcLeft(orgLeft, size);
	for (int i=1; i<=nUpdates; ++i) {
		rcRight += velocity;
		rcLeft -= velocity;
		right.addTrackEntry(rcRight, roi);
		left.addTrackEntry(rcLeft, roi);
	}
	
	SECTION("distance after next update step larger than half velocity -> not occluded") {
		//cout << right.getActualEntry().rect() << endl;
		//cout << left.getActualEntry().rect() << endl;
		REQUIRE( false == isNextUpdateOccluded(left, right) );
	}

	SECTION("distance after next update step smaller than half velocity -> is occluded") {
		// do another update with track velocity
		rcRight += velocity;
		rcLeft -= velocity;
		right.addTrackEntry(rcRight, roi);
		left.addTrackEntry(rcLeft, roi);
		//cout << right.getActualEntry().rect() << endl;
		//cout << left.getActualEntry().rect() << endl;

		REQUIRE( true == isNextUpdateOccluded(left, right) );
	}
}


TEST_CASE("#sce009 remainingOccludedUpdateSteps", "[Scene]") {
	// two tracks moving into opposite direction, next step in occlusion
	using namespace std;
	Track right(1);
	Track left(2);		
	cv::Size roi(200,200);
	cv::Size size(30,10);
	cv::Point velocity(10,0);

	// distance after next update = 0,5 * velocity -> another update step necessary
	int distCurr = velocity.x;
	int nUpdates = 3;

	// final origin after nUpdates
	cv::Point finLeft(100,0);
	int finRightX = finLeft.x - distCurr - size.width;
	cv::Point finRight(finRightX,0);

	// origin before nUpdates
	cv::Point orgLeft = finLeft + nUpdates * velocity;
	cv::Point orgRight= finRight - nUpdates * velocity;
	//cout << "left org: " << orgLeft << " fin: " << finLeft << endl;
	//cout << "right org: " << orgRight << " fin: " << finRight << endl;	
	
	cv::Rect rcRight(orgRight, size);
	cv::Rect rcLeft(orgLeft, size);
	for (int i=1; i<=nUpdates; ++i) {
		rcRight += velocity;
		rcLeft -= velocity;
		right.addTrackEntry(rcRight, roi);
		left.addTrackEntry(rcLeft, roi);
	}
	//cout << right.getActualEntry().rect() << endl;
	//cout << left.getActualEntry().rect() << endl;

	SECTION("distance after next update step smaller than half velocity -> is occluded") {
		// steps in occlusion: (2 * size + distCurr) / (2 * velocity)
		int steps = static_cast<int>( ceil((2 * size.width + distCurr) / static_cast<double>( (2 * velocity.x) )) );
		//cout << right.getActualEntry().rect() << endl;
		//cout << left.getActualEntry().rect() << endl;

		REQUIRE( steps == remainingOccludedUpdateSteps(left, right) );
	}
}


TEST_CASE("#sce010 setOcclusion", "[Scene]") {
	// empty config, two tracks (#1 moving to right, #2 moving to left)
	// will be assigned in SceneTracker::updateIntersect() steps
	using namespace std;
	Config config;	
	Config* pConfig = &config;
	config.setParam("roi_width", "200");
	config.setParam("roi_height", "200");
	SceneTracker scene(pConfig);

	// two blobs with velocityX distance to next update
	cv::Size size(30,20);
	cv::Point origin(0,0);
	cv::Point velocity(10,0);
	int distCurr = velocity.x;
	int nUpdates = 3;
	int roiWidth = stoi(config.getParam("roi_width"));
	
	// final origin of blobs after nUpdates
	cv::Point finLeft(roiWidth/2,0);
	int finRightX = finLeft.x - distCurr - size.width;
	cv::Point finRight(finRightX,0);

	// origin before nUpdates
	cv::Point orgLeft = finLeft + nUpdates * velocity;
	cv::Point orgRight= finRight - nUpdates * velocity;

	// nUpdates of SceneTracker::updateIntersect()
	cv::Rect rcRight(orgRight, size);
	cv::Rect rcLeft(orgLeft, size);
	list<cv::Rect> blobs;
    list<Track>* pTracks = nullptr;
	for (int i=1; i<=nUpdates; ++i) {
		rcRight += velocity;
		blobs.push_back(rcRight);
		rcLeft -= velocity;
		blobs.push_back(rcLeft);
		pTracks = scene.assignBlobs(blobs);
	}
	//cout << "moving right: " << pTracks->front().getActualEntry().rect() << "  velocity: " << pTracks->front().getVelocity() << endl;
	//cout << "moving left:  " << pTracks->back().getActualEntry().rect()	<< " velocity: " << pTracks->back().getVelocity() << endl;

	SECTION("next step will be occluded -> set occlusion structure and isOccluded bit for both tracks") {
		const list<Occlusion>* pOcclusion = scene.setOcclusion();
		REQUIRE( 1 == pOcclusion->size() );
		REQUIRE( true == pTracks->front().isOccluded() );
		REQUIRE( true == pTracks->back().isOccluded() );

			SECTION("setOcclusion only once (reset in updateTracks)") {
				// occlusion list contains still one element
				// and it will be deleted after nRemainingUpdateSteps (decremented in updateTracks)
				const list<Occlusion>* pOcclusion = scene.setOcclusion();
				REQUIRE( 1 == pOcclusion->size() );

				// find track #1 and #2 by ID
				list<Track>::iterator it;
				it = find_if(pTracks->begin(), pTracks->end(), TrackID_eq(1));
				REQUIRE( true == it->isOccluded() );
				it = find_if(pTracks->begin(), pTracks->end(), TrackID_eq(2));
				REQUIRE( true == it->isOccluded() );
		}
	}
}


TEST_CASE("#sce011 calcSubstitute", "[Scene]") {
	// two tracks moving into opposite direction
	using namespace std;
	Track right(1);
	Track left(2);		
	cv::Size roi(100,100);

	cv::Size size(20,10);
	cv::Point orgRight(0,0);
	cv::Point orgLeft(80,0);
	cv::Point velocity(5,0);
	int nUpdates = 3;
	
	cv::Rect rcRight(orgRight, size);
	cv::Rect rcLeft(orgLeft, size);
	for (int i=1; i<=nUpdates; ++i) {
		rcRight += velocity;
		rcLeft -= velocity;
		right.addTrackEntry(rcRight, roi);
		left.addTrackEntry(rcLeft, roi);
	}
	//cout << "velocity right: " << right.getVelocity() << " velocity left: " << left.getVelocity() << endl;

	SECTION("substitute = track entry + velocity") {
		cv::Rect rightSubstitute = right.getActualEntry().rect() + velocity;
		REQUIRE( rightSubstitute == calcSubstitute(right) );
		cv::Rect leftSubstitute = left.getActualEntry().rect() - velocity;
		REQUIRE( leftSubstitute == calcSubstitute(left) );
	}
}


TEST_CASE("#sce012 adjustSubstPos", "[Scene]") {
	// two track entries moving into opposite direction
	using namespace std;
	cv::Size rightSize(30,20);
	cv::Size leftSize(40,25);
	cv::Point velocity(5,0);

	SECTION("moving right >>> <<< moving left") {
		cv::Point rightOrg(10,0); // br=(40,20)
		cv::Point leftOrg(50,0);  // br=(90,25)
		cv::Rect movesRight(rightOrg,rightSize);
		cv::Rect movesLeft(leftOrg,leftSize);
		cv::Rect blob = movesRight | movesLeft;
		// blob: wh=[80,25] tl=(10,0)
		//cout << "blob: " << blob << endl;
		SECTION("shift rects to left (-velocity)") {
			cv::Rect shiftRight = movesRight - velocity;
			cv::Rect shiftLeft = movesLeft - velocity;
			adjustSubstPos(blob, shiftRight, shiftLeft);
		}
		SECTION("shift rects to right (+velocity)") {
			cv::Rect shiftRight = movesRight + velocity;
			cv::Rect shiftLeft = movesLeft + velocity;
			adjustSubstPos(blob, shiftRight, shiftLeft);
			REQUIRE( movesRight == shiftRight);
			REQUIRE( movesLeft == shiftLeft);
		}
	}

	SECTION("<<< moving left  moving right >>>") {
		cv::Point rightOrg(60,0); // br=(90,20)
		cv::Point leftOrg(10,0);  // br=(50,25)
		cv::Rect movesRight(rightOrg,rightSize);
		cv::Rect movesLeft(leftOrg,leftSize);
		cv::Rect blob = movesRight | movesLeft;
		// blob: wh=[80,25] tl=(10,0)
		//cout << "blob: " << blob << endl;	
		SECTION("shift rects to left (-velocity)") {
			cv::Rect shiftRight = movesRight - velocity;
			cv::Rect shiftLeft = movesLeft - velocity;
			adjustSubstPos(blob, shiftRight, shiftLeft);
			REQUIRE( movesRight == shiftRight);
			REQUIRE( movesLeft == shiftLeft);
		}
		SECTION("shift rects to right (+velocity)") {
			cv::Rect shiftRight = movesRight + velocity;
			cv::Rect shiftLeft = movesLeft + velocity;
			adjustSubstPos(blob, shiftRight, shiftLeft);
			REQUIRE( movesRight == shiftRight);
			REQUIRE( movesLeft == shiftLeft);
			//cout << shiftRight << endl;
			//cout << movesRight << endl;
		}
	}
}




TEST_CASE("#sce015 updateTracks", "[Scene]") {
	// set up config, velocity, scene with 2 tracks
	// empty config, create scene with two tracks (#1 moving to right, #2 moving to left)
	// config with two tracks
	using namespace std;
	cv::Size roi(200,200);
	Config config;	
	Config* pConfig = &config;
	config.setParam("roi_width", to_string(static_cast<long long>(roi.width)));
	config.setParam("roi_height", to_string(static_cast<long long>(roi.height)));
	SceneTracker scene(pConfig);
	//cout << config.getParam("roi_height") << endl;

	// create tracks that will collide after <steps> updates
	cv::Size blobSize(30,20);
	cv::Point velocityRight(5,0);
	cv::Point velocityLeft(-4,0);
	int collision = 100;
	int stepsBeforeOcclusion = 2;

	OppositeTracks tracks = createTracksBeforeOcclusion(roi, collision, stepsBeforeOcclusion , blobSize, velocityRight, velocityLeft);
	list<cv::Rect> blobs;

	// TRACE
	if (g_traceScene) {
		cv::Mat canvas(roi, CV_8UC3, black);
		printTrack(canvas, tracks.right, green);
		printTrack(canvas, tracks.left, yellow);
        //printOcclusion(canvas, occ, magenta);
		printBlobs(canvas, blobs);
		cv::imshow("tracks", canvas);
		breakEscContinueEnter();
	}
	// END_TRACE

	SECTION("assign blobs before occlusion") {
		// next updates -> two detached blobs
		cv::Rect blobRight = tracks.right.getActualEntry().rect();
		cv::Rect blobLeft = tracks.left.getActualEntry().rect();
		for (int i=1; i <= stepsBeforeOcclusion; ++i) {
			blobRight += velocityRight;
			blobLeft += velocityLeft;
			blobs.push_back(blobRight);
			blobs.push_back(blobLeft);
			list<Track>* pTracks = scene.updateTracks(blobs, i);

			// all blobs have been assigned
			REQUIRE( 0 == blobs.size() );
			REQUIRE( blobRight == pTracks->front().getActualEntry().rect() );
			REQUIRE( blobLeft == pTracks->back().getActualEntry().rect() );
		}

		// occlusion has been created
		const list<Occlusion>* occlusions = scene.occlusionList();
		REQUIRE( 1 == occlusions->size() );
	
		SECTION("assign blobs in occlusion") {
			// next update -> create occlusion (blobs still detached)
			// no occlusion before update
			//cout << "left: " << occlusions->front().movingLeft()->getActualEntry().rect() << endl;
			//cout << "right: " << occlusions->front().movingRight()->getActualEntry().rect() << endl;

			int stepsInOcclusion = remainingOccludedUpdateSteps( *(occlusions->front().movingLeft()), *(occlusions->front().movingRight()) );

			for (int i=1; i < stepsInOcclusion; ++i) {
				blobRight += velocityRight;
				blobLeft += velocityLeft;
				
				// blobs do intersect
				REQUIRE( (blobRight & blobLeft).area() > 0);
				cv::Rect blobIntersect = blobRight | blobLeft;

				blobs.push_back(blobIntersect);
				list<Track>* pTracks = scene.updateTracks(blobs, i);
                (void)pTracks;

				// all blobs have been assigned
				REQUIRE( 0 == blobs.size() );
				const list<Occlusion>* occlusions = scene.occlusionList();
				REQUIRE( 1 == occlusions->size() );
				REQUIRE( false == occlusions->front().hasPassed() );
			}
				// TRACE
				if (g_traceScene) {
					cv::Mat canvas(roi, CV_8UC3, black);
					printTrack(canvas, *(occlusions->front().movingRight()), green);
					printTrack(canvas, *(occlusions->front().movingLeft()), yellow);
                    printOcclusion(canvas, occlusions->front().rect(), magenta);
					printBlobs(canvas, blobs);
					cv::imshow("sce#015 tracks", canvas);
					breakEscContinueEnter();
				}
				// END_TRACE

			SECTION("occlusion has passed -> normal assignment") {
				blobRight += velocityRight;
				blobLeft += velocityLeft;

				// blobs do not intersect
				REQUIRE( 0 == (blobRight & blobLeft).area() );
				blobs.push_back(blobRight);
				blobs.push_back(blobLeft);	
				list<Track>* pTracks = scene.updateTracks(blobs, 0);
				
				// all blobs have been assigned
				REQUIRE( 0 == blobs.size() );
				REQUIRE( blobRight == pTracks->front().getActualEntry().rect() );
				REQUIRE( blobLeft == pTracks->back().getActualEntry().rect() );

				// occlusion has passed
				const list<Occlusion>* occlusions = scene.occlusionList();
				REQUIRE( true == occlusions->front().hasPassed() );
		

				SECTION("occlusion is marked for deletion -> normal assignment") {
					blobRight += velocityRight;
					blobLeft += velocityLeft;

					// blobs do not intersect
					REQUIRE( 0 == (blobRight & blobLeft).area() );
					blobs.push_back(blobRight);
					blobs.push_back(blobLeft);	
					list<Track>* pTracks = scene.updateTracks(blobs, 0);
				
					// all blobs have been assigned
					REQUIRE( 0 == blobs.size() );
					REQUIRE( blobRight == pTracks->front().getActualEntry().rect() );
					REQUIRE( blobLeft == pTracks->back().getActualEntry().rect() );

					// occlusion is marked for deletion
					REQUIRE( true == occlusions->front().isMarkedForDelete() );

					SECTION("next update -> delete occlusion") {
						blobRight += velocityRight;
						blobLeft += velocityLeft;

						// blobs do not intersect
						REQUIRE( 0 == (blobRight & blobLeft).area() );
						blobs.push_back(blobRight);
						blobs.push_back(blobLeft);	
						list<Track>* pTracks = scene.updateTracks(blobs, 0);
				
						// all blobs have been assigned
						REQUIRE( 0 == blobs.size() );
						REQUIRE( blobRight == pTracks->front().getActualEntry().rect() );
						REQUIRE( blobLeft == pTracks->back().getActualEntry().rect() );

						// occlusion has been deleted
						const list<Occlusion>* occlusions = scene.occlusionList(); 
						REQUIRE( 0 == occlusions->size() );
					}
				}
			} // SECTION("occlusion has passed -> normal assignment")
		} // SECTION("assign blobs in occlusion") {
	} // SECTION("assign blobs before occlusion") {
} // TEST_CASE #sce015


