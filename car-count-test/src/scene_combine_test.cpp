#include "stdafx.h"
#include "../../VideoProcessing/include/Tracking.h"

using namespace std;
using namespace cv;

void prnTrack(Track& track);

struct TwoTracks
{
	Track& track1;
	Track& track2;
	TwoTracks(Track& tr1, Track& tr2) : track1(tr1), track2(tr2) {};
	TwoTracks& operator= (const TwoTracks& tSource)
	{
		track1 = tSource.track1;
		track2 = tSource.track2;
		return *this;
	}
};

// Generate two tracks combined in a struct to compare with
//  origin:   start (upper left corner) of inital rectangle
//  velocity: velocity vector the rectangle will be moved at 
TwoTracks multiUpdateScene(Scene& scene, Point2i origin1, Point2i origin2, Point2d velocity1, Point2d velocity2)
{
	scene.mTracks.clear();
	int height = 100, width = 100;
	std::list<TrackEntry> blobs;
	for(size_t i = 0; i < 7; ++i)
	{
		blobs.push_back(TrackEntry(origin1.x + (int)(i*velocity1.x), origin1.y + (int)(i*velocity1.y),  width, height));
		blobs.push_back(TrackEntry(origin2.x + (int)(i*velocity2.x), origin2.y + (int)(i*velocity2.y),  width, height));
		scene.updateTracks(blobs);
	}
	TwoTracks twoTracks(scene.mTracks.front(), scene.mTracks.back());
	return twoTracks;
}



SCENARIO("Check on combining vehicles", "[Scene]")
{
	GIVEN("5 Tracks with 6 updated history elements")
	{
		// Introduce scene for track-update from blobs
		// 4 tracks in scene with 7 updated history elements
		Scene scene;
		list<TrackEntry> blobs;
		for (int i=0; i<7; ++i)
		{
			blobs.push_back(TrackEntry(i*10, 0, 100, 50));		// [1] initial track
			blobs.push_back(TrackEntry(120+i*11, 0, 100, 20));	// [1] combined (close and similar velocity )
			blobs.push_back(TrackEntry(300+i*11, 0, 100, 20));	//  [2] NOT close but similar velocity (+50 pixel difference)
			blobs.push_back(TrackEntry(50+i*10, 40, 80, 30));	// [1] close and similar velocity
			blobs.push_back(TrackEntry(80+i*22, 50, 100, 20));	//  [3] close but velocity NOT similar (+100% difference)
			scene.updateTracks(blobs);
		}
		for_each(scene.mTracks.begin(), scene.mTracks.end(), prnTrack);
		
		WHEN("3 tracks are close and have similar velocity")
		{
			THEN("3 tracks are combined, 2 stay separate")
			{
				scene.combineTracks();
				REQUIRE(scene.mVehicles.size() == 3);
				list<Vehicle>::iterator ve = scene.mVehicles.begin();
				REQUIRE(ve->getBbox() == cv::Rect(60, 0, 226, 70));		// [1]
				++ve;
				REQUIRE(ve->getBbox() == cv::Rect(366, 0, 100, 20));	// [2]
				++ve;
				REQUIRE(ve->getBbox() == cv::Rect(212, 50, 100, 20));	// [3]
			}
		}
		// scene.PrintVehicles();
	}
}


SCENARIO("Only moving tracks will be combined, not very slow moving ones", "[Scene]")
{
	GIVEN("scene with 2 Tracks with 6 updated history elements")
	{
		// 	minL2NormVelocity = 0.5;
		Scene scene;
		Point2i org1 = Point2i(100, 100);
		Point2i org2 = Point2i(200, 100);
		Point2d vel1 = Point2d(1, 0.2);

		WHEN("L2 norm of velocity vector < 0.5")
		{
			Point2d vel2 = Point2d(0.3, 0.1); // L2 norm = 0.2; cos(phi) = 1
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			scene.combineTracks();
			THEN("slow moving track is not considered for combination to vehicle")
				REQUIRE(scene.mVehicles.front().getBbox().width == 100);
		}

		WHEN("L2 norm of velocity vector > 0.5")
		{
			Point2d vel2 = Point2d(1, 0.2); // L2 norm = 1.01; cos(phi) = 0.98
			TwoTracks tracks(multiUpdateScene(scene, org1, org2, vel1, vel2));
			scene.combineTracks();
			THEN("track moves fast enough and is combined")
				REQUIRE(scene.mVehicles.front().getBbox().width > 100);
		}

	}
}


SCENARIO("Check on returning idxContour for each vehicle", "[Scene]")
{
	GIVEN("5 Tracks with 6 updated history elements")
	{
		// Introduce scene for track-update from blobs
		// 4 tracks in scene with 7 updated history elements
		Scene scene;
		list<TrackEntry> blobs;
		for (int i=0; i<7; ++i)
		{
			blobs.push_back(TrackEntry(i*10, 0, 100, 50, 1));		// vehicle: 1, idxContour: 1
			blobs.push_back(TrackEntry(120+i*11, 0, 100, 20, 3));	// vehicle: 1, idxContour: 3
			blobs.push_back(TrackEntry(300+i*11, 0, 100, 20, 4));	// vehicle: 2, idxContour: 4
			blobs.push_back(TrackEntry(50+i*10, 40, 80, 30, 7));	// vehicle: 1, idxContour: 7
			blobs.push_back(TrackEntry(80+i*22, 50, 100, 20, 9));	// vehicle: 3, idxContour: 9
			scene.updateTracks(blobs);
		}
		
		WHEN("tracks are combined to vehicles")
		{
			THEN("the appropriate vector for the contour indices is returned")
			{
				scene.combineTracks();
				list<Vehicle>::iterator iVehicles = scene.mVehicles.begin();
				// vehicle 1: [1, 3, 7]
				REQUIRE(iVehicles->mContourIndices.at(0) == 1);
				REQUIRE(iVehicles->mContourIndices.at(1) == 3);			
				REQUIRE(iVehicles->mContourIndices.at(2) == 7);
				++iVehicles;
				// vehicle 2: [4]
				REQUIRE(iVehicles->mContourIndices.at(0) == 4);
				++iVehicles;
				// vehicle 3: [9]
				REQUIRE(iVehicles->mContourIndices.at(0) == 9);
			}
		}
		// scene.PrintVehicles();
	}
}


void prnTrack(Track& track)
{
	cout << "ID:" << track.getId() << " bbox: " << track.getActualEntry().mBbox;
	/// ToDo: insert padding
	cout << " velocity: " << track.getVelocity() << endl;
}