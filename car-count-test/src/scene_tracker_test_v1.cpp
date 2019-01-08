#include "stdafx.h"
#include "../../car-count/include/config.h" // includes tracker.h

using namespace std;
using namespace cv;

//////////////////////////////////////////////////////////////////////////////
// ID handling
//////////////////////////////////////////////////////////////////////////////
TEST_CASE("#id001 getTrackID, returnTrackID", "[Scene]") {
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
			for (int i = 1; i < 10; ++i) {
				REQUIRE(scene.returnTrackID(i));
				//cout << "returned id[" << i << "]: " << i << endl;
			}
			REQUIRE(scene.returnTrackID(10) == false);
			REQUIRE(scene.returnTrackID(0) == false);
		}
	}
}



//////////////////////////////////////////////////////////////////////////////
// update logic (assgin, combine, delete tracks)
//////////////////////////////////////////////////////////////////////////////
TEST_CASE("#ass001 assignBlobs", "[SCENE]") {
// config with two tracks
	using namespace std;
	Config config;	
	Config* pConfig = &config;
	config.setParam("roi_width", "100");
	config.setParam("roi_height", "100");
	SceneTracker scene(pConfig);
	//cout << config.getParam("roi_height") << endl;

	cv::Size blobSize(40,10);
	int velocityX = 10;

	cv::Point blobOriginLeft(0, 0);
	cv::Rect blobLeft(blobOriginLeft, blobSize);

	int roiWidth = stoi(config.getParam("roi_width"));
	cv::Point blobOriginRight(roiWidth - blobSize.width, 0);
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
		// moving to right
		blobOriginLeft += cv::Point(velocityX, 0);
		blobLeft = cv::Rect(blobOriginLeft, blobSize);
		// moving to left
		blobOriginRight -= cv::Point(velocityX, 0);
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
		// moving to right
		blobOriginLeft += cv::Point(velocityX, 30);
		blobLeft = cv::Rect(blobOriginLeft, blobSize);
		// moving to left
		blobOriginRight -= cv::Point(velocityX, 30);
		blobRight = cv::Rect(blobOriginRight, blobSize);
		// push to list
		blobs.push_back(blobLeft);
		blobs.push_back(blobRight);
		// after update: four history elements
		pTracks = scene.assignBlobs(blobs);
		REQUIRE( 4 == pTracks->size() );
	}
}


// combine tracks
	// two tracks: same direction + intersection -> combine, take the one with longer history
	// two tracks: opposite direction + intersection -> don't combine
TEST_CASE("#com001 combineTracks", "[SCENE]") {
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



TEST_CASE("#del001 deleteMarkedTracks", "[SCENE]") {
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
	list<Track>* pTracks;
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


TEST_CASE("#del002 deleteReversingTracks", "[SCENE]") {
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


TEST_CASE("#occ001 isDirectionOpposite", "[SCENE]") {
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


TEST_CASE("#occ002 isNextUpdateOccluded", "[SCENE]") {
	// two tracks moving into opposite direction
	using namespace std;
	Track right(1);
	Track left(2);		
	cv::Size roi(200,200);
	cv::Size size(30,10);
	cv::Point velocity(10,0);

	// distance after next update = 0,5 * velocity -> another update step necessary
	int distNextUpd = (velocity.x * 3) + 1;
	int nUpdates = 3;

	// final origin after nUpdates
	cv::Point finLeft(100,0);
	int finRightX = finLeft.x - distNextUpd - size.width;
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