#include "stdafx.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/config.h" // includes tracker.h


///////////////////////////////////////////////////////////////////////////////
// TrackEntry 
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// Track 
///////////////////////////////////////////////////////////////////////////////

TEST_CASE( "#id001 getId", "[Track]" ) {
	Track track(1);
	REQUIRE(track.getId() == 1);
}


TEST_CASE( "#vel001 addTrackEntry: blob smaller than roi width", "[Track]" ) {
	Track track(1);
	int roiWidth = 100;
	int roiHeight = 100;
	cv::Size roi (roiWidth, roiHeight);

	int velocityLeft = -20;
	cv::Point blobOrigin(roiWidth+10, 20);
	cv::Size blobSize(40, 30);
	cv::Rect blob(blobOrigin, blobSize);

	SECTION("blob outside roi area is not added to track") {
		REQUIRE(false == track.addTrackEntry(blob, roi));
	}

	SECTION("same velocity, if blob is moved in steps from right to left") {
		blobOrigin.x = roiWidth-10;
		blob = cv::Rect(blobOrigin, blobSize);
		REQUIRE(true == track.addTrackEntry(blob, roi));
		for(int i=1; i<=6; ++i) {
			// move rect with constant velocity
			blobOrigin.x += velocityLeft;
			blob = cv::Rect(blobOrigin, blobSize);
			track.addTrackEntry(blob, roi);
			//std::cout << track.getActualEntry().rect() << std::endl;
			REQUIRE( velocityLeft == static_cast<int>(track.getVelocity().x) );
		}
	}
}


TEST_CASE("#vel002 addTrackEntry: blob covers entire roi width", "[Track]") {
	using namespace std;
	Track track(1);
	int roiWidth = 100;
	int roiHeight = 100;
	cv::Size roi (roiWidth, roiHeight);

	int velocityLeft = -20;
	cv::Point blobOrigin(10+abs(velocityLeft), 20);
	cv::Size blobSize = cv::Size(110, 30);
	cv::Rect blob(blobOrigin, blobSize);
		
	// two updates required in order to calculate velocity
	// 1st track entry
	REQUIRE(true == track.addTrackEntry(blob, roi));
	//cout << "blob:" << track.getActualEntry().rect() << " velocity:" << track.getVelocity() << endl;

	// 2nd
	blobOrigin.x += velocityLeft;
	blob = cv::Rect(blobOrigin, blobSize);
	REQUIRE(true == track.addTrackEntry(blob, roi));
	//cout << "blob:" << track.getActualEntry().rect() << " velocity:" << track.getVelocity() << endl;

	SECTION("average velocity keeps the same") {
		blobOrigin.x += velocityLeft;
		cv::Rect blob = cv::Rect(blobOrigin, blobSize);
		REQUIRE(true == track.addTrackEntry(blob, roi));
		//cout << "blob:" << track.getActualEntry().rect() << " velocity:" << track.getVelocity() << endl;
	}
}


TEST_CASE("#vel003 addSubstitute: compose from previous TrackEntry", "[Track]") {
	// track populated with two entries necessary in order to calculate velocity
	using namespace std;
	Track track(1);
	cv::Size roi(100,100);

	cv::Size blobSize(30,20);
	cv::Point blobOrigin(50,50);
	int velocity = 10;

	// 1st
	cv::Rect blob(blobOrigin, blobSize);
	track.addTrackEntry(blob, roi);

	// 2nd
	blobOrigin.x += velocity;
	blob = cv::Rect(blobOrigin, blobSize);
	track.addTrackEntry(blob, roi);
	//cout << track.getVelocity() << endl;

	SECTION("take average velocity to move substitute") {
		REQUIRE( true == track.addSubstitute(roi) );
		REQUIRE( velocity == static_cast<int>(track.getVelocity().x) );
		REQUIRE( blobSize == track.getActualEntry().rect().size() );
	}
}


TEST_CASE("#upd001 updateTrack: intersection", "[Track]") {
	// track populated with one entry
	using namespace std;
	Track track(1);
	cv::Size roi(100,100);

	cv::Size blobSize(10,10);
	cv::Point blobOrigin(50,50);

	// 1st
	cv::Rect blob(blobOrigin, blobSize);
	track.addTrackEntry(blob, roi);

	// create blob with 0.6 * area intersection of track entry
	//  -> move 4 pixels to right
	blobOrigin += cv::Point(4,0);
	blob = cv::Rect(blobOrigin, blobSize);
	list<cv::Rect> blobs;
	blobs.push_back(cv::Rect(blobOrigin, blobSize));
	//cout << "track:" << track.getActualEntry().rect() << " blob:" << blob << endl;

	SECTION("intersection large enough -> assign new blob") {
		double minIntersection = 0.5;
		track.updateTrackIntersect(blobs, roi, minIntersection);
		REQUIRE( blob == track.getActualEntry().rect() );
		REQUIRE( 2 == track.getHistory() );
		REQUIRE( 0 == blobs.size() );
	}

	SECTION("intersection too small -> blob will not be assigned") {
		double minIntersection = 0.7;
		track.updateTrackIntersect(blobs, roi, minIntersection);
		REQUIRE( 1 == track.getHistory() );
		REQUIRE( 1 == blobs.size() );
	}
}


TEST_CASE("#upd002 updateTrack: multiple blobs", "[Track]") {
	// track populated with one entry
	// will be moved to left and to right in test sections
	using namespace std;
	Track track(1);
	cv::Size roi(100,100);
	cv::Size blobSize(50,50);
	cv::Point blobOrigin(10,0);
	// 1st
	cv::Rect blob(blobOrigin, blobSize);
	track.addTrackEntry(blob, roi);
	double minIntersection = 0.5;

	SECTION("track moves to left -> take leftmost blob") {
		// assign large track entry
		cv::Point velocity(-10,0);
		blobOrigin += velocity;
		blob = cv::Rect(blobOrigin, blobSize);
		track.addTrackEntry(blob, roi);
		REQUIRE( blob == track.getActualEntry().rect() );

		// generate 2 small blobs fitting into area of last track entry
		cv::Rect leftMost(0,0,10,10);
		cv::Rect rightMost(40,0,10,10);
		list<cv::Rect> blobs;
		blobs.push_back(rightMost);	
		blobs.push_back(leftMost);

		// leftmost blob will be assigned
		track.updateTrackIntersect(blobs, roi, minIntersection);
		REQUIRE( leftMost == track.getActualEntry().rect() );
	}

	SECTION("track moves to right -> take rightmost blob") {
		// assign large track entry
		cv::Point velocity(10,0);
		blobOrigin += velocity;
		blob = cv::Rect(blobOrigin, blobSize);
		track.addTrackEntry(blob, roi);
		REQUIRE( blob == track.getActualEntry().rect() );

		// generate 2 small blobs fitting into area of last track entry
		cv::Rect leftMost(20,0,10,10);
		cv::Rect rightMost(40,0,10,10);
		list<cv::Rect> blobs;
		blobs.push_back(leftMost);
		blobs.push_back(rightMost);	
		
		// rightmost blob will be assigned
		track.updateTrackIntersect(blobs, roi, minIntersection);
		REQUIRE( rightMost == track.getActualEntry().rect() );
	}
}


TEST_CASE("#upd003 updateTrack: confidence in- and decrease", "[Track]") {
	// track populated with one entry
	using namespace std;
	Track track(1);
	cv::Size roi(100,100);
	cv::Size blobSize(10,10);
	cv::Point blobOrigin(0,0);
	// 1st
	cv::Rect blob(blobOrigin, blobSize);
	track.addTrackEntry(blob, roi);
	double minIntersection = 0.5;
	int maxConfidence = 2;
	// empty blob list
	list<cv::Rect> blobs;

	SECTION("increase confidence after matching blob has been assigned") {
		blobs.push_back(blob);
		track.updateTrackIntersect(blobs, roi, minIntersection, maxConfidence);
		REQUIRE( blob == track.getActualEntry().rect() );
		REQUIRE( 1 == track.getConfidence() );

		SECTION("another update: increase confidence again") {
			blobs.push_back(blob);
			track.updateTrackIntersect(blobs, roi, minIntersection, maxConfidence);
			REQUIRE( blob == track.getActualEntry().rect() );
			REQUIRE( 2 == track.getConfidence() );

			SECTION("another update: confidence saturates") {
				blobs.push_back(blob);
				track.updateTrackIntersect(blobs, roi, minIntersection, maxConfidence);
				REQUIRE( blob == track.getActualEntry().rect() );
				REQUIRE( 2 == track.getConfidence() );
			}
		}
	}

	SECTION("decrease confidence if no matching blob could be assigned") {
		// track with full confidence
		blobs.push_back(blob);
		track.updateTrackIntersect(blobs, roi, minIntersection, maxConfidence);
		blobs.push_back(blob);
		track.updateTrackIntersect(blobs, roi, minIntersection, maxConfidence);
		REQUIRE( 2 == track.getConfidence() );

		// no matching blob
		cv::Point blobOriginNoMatch = blobOrigin + cv::Point(blobSize.width, 0);
		cv::Rect blobNoMatch(blobOriginNoMatch, blobSize);
		//cout << "matching blob:" << blob << " not matching blob:" << blobNoMatch << endl;

		SECTION("update with no matching blob: decrease confidence") {
			blobs.push_back(blobNoMatch);
			// update track adds substitute at velocity(0,0)
			track.updateTrackIntersect(blobs, roi, minIntersection, maxConfidence);
			REQUIRE( blob == track.getActualEntry().rect() );
			REQUIRE( 1 == track.getConfidence() );

			SECTION("another update: confidence drops to zero, mark track for deletion") {
				blobs.push_back(blobNoMatch);
				// update track adds substitute at velocity(0,0)
				track.updateTrackIntersect(blobs, roi, minIntersection, maxConfidence);
				REQUIRE( blob == track.getActualEntry().rect() );
				REQUIRE( 0 == track.getConfidence() );
				REQUIRE( true == track.isMarkedForDelete() );
			}
		}
	}
}