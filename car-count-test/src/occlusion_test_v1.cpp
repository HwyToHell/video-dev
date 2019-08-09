#include "stdafx.h"
#include "../../car-count/include/config.h" // includes tracker.h
#include "../../car-count/include/tracker.h"
#include "../../car-count/include/frame_handler.h"

#if defined(__linux__)
    #include "../../utilities/inc/util-visual-trace.h"
#elif(_WIN32)
    #include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"    #pragma warning(disable: 4482) // MSVC10: enum nonstd extension
#endif


// enable visual trace
static bool g_traceOcclusion = false; // static == internal linkage (file)

//////////////////////////////////////////////////////////////////////////////
// Occlusion
//////////////////////////////////////////////////////////////////////////////
TEST_CASE("#occ001 id (set at construction)", "[Occlusion]") {
	cv::Size roi(100,100);
	cv::Size blobSize(30,20);
	cv::Point blobOrgRight(10,70);
	cv::Point blobOrgLeft(60,70);

	// track entry must be added after track construction, otherwise getActualEntry will fail
	Track trackRight(1);
	trackRight.addTrackEntry(cv::Rect(blobOrgRight, blobSize), roi);
	Track trackLeft(2);
	trackLeft.addTrackEntry(cv::Rect(blobOrgLeft, blobSize), roi);
	
	// construct without id
	Occlusion occ0(roi, &trackLeft, &trackRight, 5);
	REQUIRE( 0 == occ0.id() );

	// construct with id
	Occlusion occ1(roi, &trackLeft, &trackRight, 5, 1);
	REQUIRE( 1 == occ1.id() );
}


TEST_CASE("#occ002 setId", "[Occlusion]") {
	cv::Size roi(100,100);
	cv::Size blobSize(30,20);
	cv::Point blobOrgRight(10,70);
	cv::Point blobOrgLeft(60,70);

	// track entry must be added after track construction, otherwise getActualEntry will fail
	Track trackRight(1);
	trackRight.addTrackEntry(cv::Rect(blobOrgRight, blobSize), roi);
	Track trackLeft(2);
	trackLeft.addTrackEntry(cv::Rect(blobOrgLeft, blobSize), roi);
	
	// construct without id
	Occlusion occ(roi, &trackLeft, &trackRight, 5);
	REQUIRE( 0 == occ.id() );

	// set id
	occ.setId(1);
	REQUIRE( 1 == occ.id() );
}


TEST_CASE("#occ005 updateRect", "[Occlusion]") {
	cv::Size roi(100,100);
	cv::Size blobSize(30,20);
	cv::Point blobOrgRight(10,70);
	cv::Point blobOrgLeft(60,70);

	SECTION("construct with velocity == 0") {
		cv::Point velocityZero(0,0);
		
		// track with track entries needed in order to calculate occlusion rect
		Track trackRight = createTrackAt(roi, blobOrgRight, blobSize, velocityZero, 1);
		Track trackLeft = createTrackAt(roi, blobOrgLeft, blobSize, velocityZero, 1);
		cv::Rect nextRight = trackRight.getActualEntry().rect() 
			+ static_cast<cv::Point>(trackRight.getVelocity());;
		cv::Rect nextLeft = trackLeft.getActualEntry().rect()
			+ static_cast<cv::Point>(trackLeft.getVelocity());

		Occlusion occ(roi, &trackLeft, &trackRight, 5);
		REQUIRE( (nextRight | nextLeft) == occ.rect() );
	}

	SECTION("construct with velocity <> 0") {
		cv::Point velocityRight(6,0);
		cv::Point velocityLeft(-5,0);

		// track with track entries needed in order to calculate occlusion rect
		Track trackRight = createTrackAt(roi, blobOrgRight, blobSize, velocityRight, 1);
		Track trackLeft = createTrackAt(roi, blobOrgLeft, blobSize, velocityLeft, 1);
		cv::Rect nextRight = trackRight.getActualEntry().rect() 
			+ static_cast<cv::Point>(trackRight.getVelocity());;
		cv::Rect nextLeft = trackLeft.getActualEntry().rect()
			+ static_cast<cv::Point>(trackLeft.getVelocity());

		Occlusion occ(roi, &trackLeft, &trackRight, 5);
		REQUIRE( (nextRight | nextLeft) == occ.rect() );

		SECTION("do another update") {
			std::list<cv::Rect> blobs;
			blobs.push_back(nextRight);
			blobs.push_back(nextLeft);
			trackRight.updateTrackIntersect(blobs, roi);
			trackLeft.updateTrackIntersect(blobs, roi);
			REQUIRE( 0 == blobs.size() );

			// calc next update entries
			nextRight = trackRight.getActualEntry().rect() 
				+ static_cast<cv::Point>(trackRight.getVelocity());;
			nextLeft = trackLeft.getActualEntry().rect()
				+ static_cast<cv::Point>(trackLeft.getVelocity());

			REQUIRE( (nextRight | nextLeft) == occ.updateRect() );
		}
	}
} // TEST_CASE #occ005


TEST_CASE("#occ003 assignBlobs", "[Occlusion]") {

	// setOcclusion based on two occluding tracks
	cv::Size roi(100,100);
	cv::Size blobSize(30,20);
	Track trackRight;
	Track trackLeft;
	cv::Point velocityRight(5,0);
	cv::Point velocityLeft(-5,0);
	int collisionPointX = 50;

	Occlusion occ = createOcclusionAt(trackRight, trackLeft, roi, collisionPointX, blobSize, velocityRight, velocityLeft);
	REQUIRE(2 == trackRight.getConfidence());
	REQUIRE(2 == trackLeft.getConfidence());

	// TRACE
	if (g_traceOcclusion) {
		cv::Mat canvas(roi, CV_8UC3, black);
		printTrack(canvas, trackRight, green);
		printTrack(canvas, trackLeft, yellow);
        printOcclusion(canvas, occ.rect(), magenta);
		cv::imshow("tracks", canvas);
		breakEscContinueEnter();
	}
	// END_TRACE

	std::list<cv::Rect> blobs;

	SECTION("0 blobs -> calc substitute and decrease confidence") {
		// generate substitute for comparing
		cv::Rect rightRect(occ.movingRight()->getActualEntry().rect());
		cv::Rect leftRect(occ.movingLeft()->getActualEntry().rect());
		rightRect += velocityRight;
		leftRect += velocityLeft;

		// decrease confidence 2 -> 1
        // TODO DELETE const Track* trackRight = occ.movingRight();
        // TODO DELETE const Track* trackLeft = occ.movingLeft();
		occ.assignBlobs(blobs);
		REQUIRE(rightRect == occ.movingRight()->getActualEntry().rect());
		REQUIRE(leftRect == occ.movingLeft()->getActualEntry().rect());
		REQUIRE(1 == occ.movingRight()->getConfidence());
		REQUIRE(1 == occ.movingLeft()->getConfidence());
	}


	SECTION("1 blob ->  calc substitutes for both occluded tracks") {
		// add one blob to blobs list
		cv::Rect oneBlob = trackRight.getActualEntry().rect() + velocityRight;
		blobs.push_back(oneBlob);

		// generate substitute for comparing
		cv::Rect rightRect(occ.movingRight()->getActualEntry().rect());
		cv::Rect leftRect(occ.movingLeft()->getActualEntry().rect());
		rightRect += velocityRight;
		leftRect += velocityLeft;
		
		// keep confidence 2
		occ.assignBlobs(blobs);
		REQUIRE(rightRect == occ.movingRight()->getActualEntry().rect());
		REQUIRE(leftRect == occ.movingLeft()->getActualEntry().rect());
		REQUIRE(2 == occ.movingRight()->getConfidence());
		REQUIRE(2 == occ.movingLeft()->getConfidence());
	}

	SECTION("2 blobs -> regular track update") {
		// add two blobs to blobs list (same position, but smaller)
		// blob width must be smaller, so that there are two separate blobs
		cv::Size blobSize(25,10);
		cv::Rect blobRight(trackRight.getActualEntry().rect().tl(), blobSize);
		cv::Rect blobLeft(trackLeft.getActualEntry().rect().tl(), blobSize);
	
		// no intersection -> separate blobs
		REQUIRE( 0 == (blobLeft & blobRight).area() );
		blobs.push_back(blobRight);
		blobs.push_back(blobLeft);

		// blobs deleted, increase confidence 2 -> 3
		occ.assignBlobs(blobs);
		REQUIRE(0 == blobs.size());
		REQUIRE(blobRight == occ.movingRight()->getActualEntry().rect());
		REQUIRE(blobLeft == occ.movingLeft()->getActualEntry().rect());
		REQUIRE(3 == occ.movingRight()->getConfidence());
		REQUIRE(3 == occ.movingLeft()->getConfidence());
	}

	SECTION("3 blobs -> regular track update, discard not matching blob") {
		// add three blobs to blobs list
		// two smaller ones (see section before) and one no matching blob
		cv::Size blobSize(25,10);
		cv::Rect blobRight(trackRight.getActualEntry().rect().tl(), blobSize);
		cv::Rect blobLeft(trackLeft.getActualEntry().rect().tl(), blobSize);
		cv::Point offsetRight(3,0);
		blobRight += offsetRight;

		// no matching blob -> will not be assigned
		// moving to right, small blob, at left edge of occlusion
		cv::Rect blobNoMatch(trackRight.getActualEntry().rect().tl(), cv::Size(2,blobSize.height));
		
		// no intersection -> separate blobs
		REQUIRE( 0 == (blobLeft & blobRight).area() );
		blobs.push_back(blobRight);
		blobs.push_back(blobLeft);
		blobs.push_back(blobNoMatch);

		// TRACE
		if (g_traceOcclusion) {
			cv::Mat canvas(roi, CV_8UC3, black);
			printBlobs(canvas, blobs);
			cv::imshow("blobs 3", canvas);
			breakEscContinueEnter();
		}
		// END_TRACE
		
		// 2 blobs deleted, 1 blob left, increase confidence 2 -> 3	
		occ.assignBlobs(blobs);
		REQUIRE( 1 == blobs.size() );
		REQUIRE( blobRight == occ.movingRight()->getActualEntry().rect() );
		REQUIRE( blobLeft == occ.movingLeft()->getActualEntry().rect() );
		REQUIRE( 3 == occ.movingRight()->getConfidence() );
		REQUIRE( 3 == occ.movingLeft()->getConfidence() );
	}

}  // TEST_CASE #occ003


TEST_CASE("#occ006 hasPassed", "[Occlusion]") {
	// setOcclusion based on two occluding tracks
	cv::Size roi(100,100);
	cv::Size blobSize(30,20);
	Track trackRight;
	Track trackLeft;
	cv::Point velocityRight(6,0);
	cv::Point velocityLeft(-5,0);
	int collisionPointX = 50;

	// update sequence based on remaining occluded update steps
	Occlusion occ = createOcclusionAt(trackRight, trackLeft, roi, collisionPointX, blobSize, velocityRight, velocityLeft);
	int iSteps = remainingOccludedUpdateSteps(trackLeft, trackRight);
	cv::Rect blobRight = trackRight.getActualEntry().rect();
	cv::Rect blobLeft = trackLeft.getActualEntry().rect();
	std::list<cv::Rect> blobs;

	SECTION("one step before passing") {
		for (int i = 0; i < iSteps - 1; ++i) {
			blobRight += velocityRight;
			blobLeft += velocityLeft;
			blobs.push_back(blobRight);
			blobs.push_back(blobLeft);
			occ.assignBlobs(blobs);
			REQUIRE( 0 == blobs.size() );
		}
		REQUIRE( false == occ.hasPassed() );

			// TRACE
			if (g_traceOcclusion) {
				cv::Mat canvas(roi, CV_8UC3, black);
				printTrack(canvas, trackRight, green);
				printTrack(canvas, trackLeft, yellow);
                printOcclusion(canvas, occ.rect(), magenta);
				cv::imshow("tracks", canvas);
				breakEscContinueEnter();
			}
			// END_TRACE
			
		SECTION("one step after passing") {
			blobRight += velocityRight;
			blobLeft += velocityLeft;
			blobs.push_back(blobRight);
			blobs.push_back(blobLeft);
			occ.assignBlobs(blobs);
			REQUIRE( 0 == blobs.size() );
			REQUIRE( true == occ.hasPassed() );
		}


	}
} // TEST_CASE #occ006



//////////////////////////////////////////////////////////////////////////////
// Occlusion ID List
//////////////////////////////////////////////////////////////////////////////
TEST_CASE("#ocl001 add, remove occlusion", "[Occlusion]") {
	// set scene params
	cv::Size roi(200,200);
	cv::Size blobSize(10,5);
	cv::Point velocityRight(4,0);
	cv::Point velocityLeft(-3,0);
	OcclusionIdList ocl(2); // size limit == 2
	Track trackRight;
	Track trackLeft;

	// before adding occlusions -> isOcclusion == false
	REQUIRE( false == ocl.isOcclusion() );

	SECTION("add occlusions") {
		// add occlusion 1
		Occlusion occ1 = createOcclusionAt(trackRight, trackLeft, roi, 50, blobSize, velocityRight, velocityLeft);
		REQUIRE( true == ocl.add(occ1) );
		REQUIRE( true == ocl.isOcclusion() );
		REQUIRE( 1 == ocl.getList()->size() );
	
		// add occlusion 2 (at size limit)
		Occlusion occ2 = createOcclusionAt(trackRight, trackLeft, roi, 100, blobSize, velocityRight, velocityLeft);
		REQUIRE( true == ocl.add(occ2) );
		REQUIRE( true == ocl.isOcclusion() );
		REQUIRE( 2 == ocl.getList()->size() );
		
		// occlusion 3 cannot be added
		Occlusion occ3 = createOcclusionAt(trackRight, trackLeft, roi, 150, blobSize, velocityRight, velocityLeft);
		REQUIRE( false == ocl.add(occ3) );
		REQUIRE( true == ocl.isOcclusion() );
		REQUIRE( 2 == ocl.getList()->size() );

		SECTION("remove occlusions") {
			OcclusionIdList::IterOcclusion iOcc = ocl.getList()->begin();

			// remove first
			iOcc = ocl.remove(iOcc);
			REQUIRE( true == ocl.isOcclusion() );
			REQUIRE( 1 == ocl.getList()->size() );

			// check if ID has been freed by trying to add 1 more occlusion
			// add occlusion 3 now (at size limit)
			Occlusion occ3 = createOcclusionAt(trackRight, trackLeft, roi, 150, blobSize, velocityRight, velocityLeft);
			REQUIRE( true == ocl.add(occ3) );
			REQUIRE( true == ocl.isOcclusion() );
			REQUIRE( 2 == ocl.getList()->size() );

			// occlusion 4 cannot be added
			Occlusion occ4 = createOcclusionAt(trackRight, trackLeft, roi, 50, blobSize, velocityRight, velocityLeft);
			REQUIRE( false == ocl.add(occ4) );
			REQUIRE( true == ocl.isOcclusion() );
			REQUIRE( 2 == ocl.getList()->size() );
		}
	}
} // TEST_CASE #ocl001


TEST_CASE("#ocl002 assignBlobs", "[Occlusion]") {
	// set up 2 occlusions
	cv::Size roi(200,200);
	cv::Size blobSize(10,5);
	cv::Point velocityRight(4,0);
	cv::Point velocityLeft(-3,0);
	Track trackRightOcc1;
	Track trackLeftOcc1;
	Track trackRightOcc2;
	Track trackLeftOcc2;
	OcclusionIdList ocl(2); // size limit == 2


	// add occlusion 1 and 2 to ocl (at size limit)
	Occlusion occ1 = createOcclusionAt(trackRightOcc1, trackLeftOcc1, roi, 50, blobSize, velocityRight, velocityLeft);
	Occlusion occ2 = createOcclusionAt(trackRightOcc2, trackLeftOcc2, roi, 150, blobSize, velocityRight, velocityLeft);
	ocl.add(occ1);
	ocl.add(occ2);

	// create blobs for next assigBlobs call
	std::list<cv::Rect> blobs;
	struct TrackTail {
		cv::Rect right;
		cv::Rect left;
	};
	std::list<TrackTail> trackTailList;
	OcclusionIdList::IterOcclusion iOcc = ocl.getList()->begin();
	while (iOcc != ocl.getList()->end()) {
		cv::Rect blobRight = iOcc->movingRight()->getActualEntry().rect();
		cv::Rect blobLeft = iOcc->movingLeft()->getActualEntry().rect();
		blobRight += velocityRight;
		blobLeft += velocityLeft;
		TrackTail tt;
		tt.right = blobRight;
		tt.left = blobLeft;
		trackTailList.push_back(tt);
		blobs.push_back(blobRight);
		blobs.push_back(blobLeft);
		++iOcc;
	}
	REQUIRE( 4 == blobs.size() );

			// TRACE
			if (g_traceOcclusion) {
				cv::Mat canvas(roi, CV_8UC3, black);
				printTrack(canvas, trackRightOcc1, green);
				printTrack(canvas, trackLeftOcc1, yellow);
				//printOcclusion(canvas, occ, magenta);
				printBlobs(canvas, blobs);
				cv::imshow("tracks", canvas);
				breakEscContinueEnter();
			}
			// END_TRACE

	// test assignments for last occlusion in list
	ocl.assignBlobs(blobs);
	REQUIRE( 0 == blobs.size() );
	iOcc = ocl.getList()->begin();
	std::list<TrackTail>::const_iterator iTT = trackTailList.begin();
	while (iOcc != ocl.getList()->end()) {
		REQUIRE( iTT->right == iOcc->movingRight()->getActualEntry().rect() );
		REQUIRE( iTT->left == iOcc->movingLeft()->getActualEntry().rect() );
		++iOcc;
		++iTT;
	}

} // TEST_CASE #ocl002

	
