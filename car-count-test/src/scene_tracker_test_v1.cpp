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
