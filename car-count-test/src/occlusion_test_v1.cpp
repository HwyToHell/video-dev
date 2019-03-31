#include "stdafx.h"
#include "../../car-count/include/config.h" // includes tracker.h
#include "../../car-count/include/tracker.h"


/// create track at blob position with given velocity
Track createTrackAt(const cv::Size roi, const cv::Point blobPos, const cv::Size blobSize, const cv::Point velocity, const size_t id) {
	cv::Rect blobAct(blobPos, blobSize);
	cv::Rect blobPrev = blobAct - velocity;
	Track track(id);
	track.addTrackEntry(blobPrev, roi);
	track.addTrackEntry(blobAct, roi);
	return track;
}


/// create occlusion at position x
/// \param[out] trackRight track moving right
/// \param[out] trackRight track moving left
Occlusion createOcclusionAt(Track& trackRight, Track& trackLeft, const cv::Size roi,  const int collisionX, const cv::Size blobSize, const cv::Point velocityRight, const cv::Point velocityLeft) {
	
	// adjust collision point, if necessary
	size_t colXAct(collisionX);
	if (collisionX > roi.width) {
		std::cerr << "collision point: " << collisionX << "outside roi: " << roi.width << std::endl;
		colXAct = roi.width / 2;
		std::cerr << "taking roi/2: " << colXAct << std::endl; 
	}

	// adjust blob height, if necessary
	cv::Size blobSizeAct(blobSize);
	if ((roi.height - blobSize.height - 10) < 0) {
		blobSizeAct.height = 10;
		std::cerr << "blob height adjusted to: " << blobSizeAct.height << std::endl;
	}
	
	cv::Point colAct(colXAct, roi.height - blobSize.height - 10);
	cv::Point orgRight(colAct.x - blobSizeAct.width, colAct.y);
	cv::Point orgLeft(colAct);
	trackRight = createTrackAt(roi, orgRight, blobSizeAct, velocityRight, 1);
	trackLeft = createTrackAt(roi, orgLeft, blobSizeAct, velocityLeft, 2);

	return Occlusion(roi, &trackRight, &trackLeft, 5);
}





//////////////////////////////////////////////////////////////////////////////
// Occlusion
//////////////////////////////////////////////////////////////////////////////
TEST_CASE("#id001 id (set at construction)", "[Occlusion]") {
	cv::Size roi(100,100);
	Track trackRight(1);
	Track trackLeft(2);
	
	// construct without id
	Occlusion occ0(roi, &trackLeft, &trackRight, 5);
	REQUIRE( 0 == occ0.id() );

	// construct with id
	Occlusion occ1(roi, &trackLeft, &trackRight, 5, 1);
	REQUIRE( 1 == occ1.id() );
}


TEST_CASE("#id002 setId", "[Occlusion]") {
	cv::Size roi(100,100);
	Track trackRight(1);
	Track trackLeft(2);
	
	// construct without id
	Occlusion occ(roi, &trackLeft, &trackRight, 5);
	REQUIRE( 0 == occ.id() );

	// set id
	occ.setId(1);
	REQUIRE( 1 == occ.id() );
}



TEST_CASE("#ass001 assignBlobs", "[Occlusion]") {

	// setOcclusion based on two occluding tracks
	cv::Size roi(100,100);
	cv::Size blobSize(30,20);
	Track trackRight;
	Track trackLeft;
	cv::Point velocityRight(6,0);
	cv::Point velocityLeft(-5,0);
	int collisionPointX = 50;

	Occlusion occ = createOcclusionAt(trackRight, trackLeft, roi, collisionPointX, blobSize, velocityRight, velocityLeft);


	// SECTION 0 blobs

	// SECTION 1 blob

	// SECTION 2 blobs

	// SECTION 3 blobs
}








