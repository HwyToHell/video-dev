#include "stdafx.h"
#include "../car-count/include/config.h" // includes tracker.h

using namespace std;
using namespace cv;

//////////////////////////////////////////////////////////////////////////////
// ID handling
//////////////////////////////////////////////////////////////////////////////
SCENARIO("get and return trackIDs", "[SceneTracker]") {
	GIVEN("scene with no tracks") {
		Config config;
		Config* pConfig = &config;
		SceneTracker scene(pConfig);
		WHEN("trackID is pulled 9 times") {
			for (int i = 0; i < 9; ++i)
                scene.nextTrackID();
			THEN("next trackid is 0")
				REQUIRE(scene.nextTrackID() == 0);
			AND_THEN("trackID is returned 9 times") {
				for (int i = 1; i < 10; ++i) {
					REQUIRE(scene.returnTrackID(i));
					//cout << "returned id[" << i << "]: " << i << endl;
				}
				REQUIRE(scene.returnTrackID(10) == false);
				REQUIRE(scene.returnTrackID(0) == false);
			}
		}
	}
}



//////////////////////////////////////////////////////////////////////////////
// update logic (number of tracks, confidence)
//////////////////////////////////////////////////////////////////////////////

SCENARIO("update tracks", "[SceneTracker]") {
	GIVEN("SceneTracker with two Tracks and nUpdates") {
		Config config;
		Config* pConfig = &config;
		SceneTracker scene(pConfig);

		int nUpdates = 9;
		int truck_width = stoi(pConfig->getParam("truck_width_min"));
		int truck_height = stoi(pConfig->getParam("truck_height_min"));
		int truck_vel_x = -10; // moves from right to left
		int truck_track_length = abs(nUpdates * truck_vel_x);
		int truck_org_x = truck_track_length; // x will end at 0 after nUpdates
		
		// count_pos = 1/2 movement range truck (largest range)
		int count_pos_x = (truck_track_length + truck_width) / 2; 

		int car_width = truck_width -1;
		int car_height = truck_height -1;
		int car_vel_x = 8;// moves from left to right
        //int car_track_length = abs(nUpdates * car_vel_x);
		int car_org_x = 0;

		int max_confidence = stoi(pConfig->getParam("track_max_confidence"));
		int max_n_tracks = stoi(pConfig->getParam("max_n_of_tracks"));

		// update count pos in SceneTracker.mClassify
		pConfig->setParam("count_pos_x", to_string( (long long)(count_pos_x) ));
		scene.update();

		list<TrackEntry> blobs;
        list<Track>* tracks;
		for (int i = 0; i <= nUpdates; ++i) {
			// car
			blobs.push_back(TrackEntry(car_org_x + i * car_vel_x, 0, car_width, car_height));

			// truck
			blobs.push_back(TrackEntry(truck_org_x + i * truck_vel_x, 0, truck_width, truck_height));

			tracks = scene.updateTracks(blobs);
		}

		WHEN("nUpdates are complete") {
			THEN("two tracks with high confidence are present") {
                REQUIRE(tracks->size() == 2);
	
                list<Track>::iterator iTrack = tracks->begin();
                while (iTrack != tracks->end()) {
					REQUIRE(iTrack->getConfidence() == max_confidence);
					++iTrack;
				}
			}
		}

		WHEN("9 unrelated blobs are introduced, all trackIDs will be assigned") {
			int org_x = truck_org_x + 100; // distance large enough to be unassigned
			blobs.clear();
			for (int i = 0; i <= nUpdates; ++i) {
				blobs.push_back(TrackEntry(org_x + i * 100, 0));
			}
			tracks = scene.updateTracks(blobs);

			THEN("no trackID is available, \n"
				"after update with no blobs, trackIDs are available again") { 
				REQUIRE(scene.nextTrackID() == 0);
                REQUIRE(tracks->size() == max_n_tracks);
				
				// update with no blobs -> trackID available
				blobs.clear();
				tracks = scene.updateTracks(blobs);
				REQUIRE(scene.nextTrackID() > 0);
                REQUIRE(tracks->size() == 2);
			}
		}

		WHEN("vehicles are counted") {
			CountResults cr;
			cr = scene.countVehicles(1);
				
			THEN("one car moves right, one truck moves left") {
				REQUIRE(cr.carRight == 1);
				REQUIRE(cr.carLeft == 0);
				REQUIRE(cr.truckRight == 0);
				REQUIRE(cr.truckLeft == 1);
			}
		}
	} // end GIVEN("SceneTracker with two Tracks and nUpdates")
} // end SCENARIO("update tracks", "[SceneTracker]")


//////////////////////////////////////////////////////////////////////////////
// count and classify
//////////////////////////////////////////////////////////////////////////////

// creates a track with high confidence that ends 1/2 velocity step before count pos
//  on the left or on the right, depending on direction,
//  so that next updateTracks() will move over count pos
list<Track>* createTrackBeforeCountPos(SceneTracker& scene, int vehicle_width, int vehicle_height,
	int count_pos_x, int velocity_x) {

	list<TrackEntry> blobs;
    list<Track>* tracks;
	int nUpdates = 5;

	int track_length = abs(nUpdates * velocity_x);
	int centroid_x = vehicle_width / 2;

	int org_x; 	// after nUpdates org_x will end at count_pos - velocity_x / 2
	if (signBit(velocity_x)) { // moves to left
		org_x = count_pos_x + track_length - centroid_x - velocity_x / 2; 
	}
	else { // moves to right
		org_x = count_pos_x - track_length - centroid_x - velocity_x / 2;
	}
	
	for (int i = 0; i <= nUpdates; ++i) {
		// truck
		blobs.push_back(TrackEntry(org_x + i * velocity_x, 0, vehicle_width, vehicle_height));

		tracks = scene.updateTracks(blobs);
	}
    return tracks;
}

SCENARIO("count vehicles", "[SceneTracker]") {
	GIVEN("SceneTracker with one assigned track") {
		Config config;
		Config* pConfig = &config;
		SceneTracker scene(pConfig);
		CountResults cr;

		int truck_width = stoi(pConfig->getParam("truck_width_min"));
		int truck_height = stoi(pConfig->getParam("truck_height_min"));
		int count_pos_x = stoi(pConfig->getParam("count_pos_x")); 
		int count_track_length = stoi(pConfig->getParam("count_track_length"));

        WHEN("vehicle has positive velocity and\n"
			"is smaller than min_truck in width and height") {
			int vehicle_width = truck_width - 1;
			int vehicle_height = truck_height - 1;
			int velocity_x = 10; // moves from left to right
            list<Track>* tracks = createTrackBeforeCountPos(scene, vehicle_width,
				vehicle_height, count_pos_x, velocity_x);
		
			// count pos not reached yet, but track length sufficient
			cr = scene.countVehicles();
			REQUIRE(cr.carRight == 0);
            REQUIRE(tracks->front().getLength() > count_track_length);

			THEN("carRight is counted") {
				list<TrackEntry> blobs;
                Rect vehicle = tracks->front().getActualEntry().rect();
				vehicle.x += velocity_x;

				// one more updateTracks() and count pos is passed over
				blobs.push_back(TrackEntry(vehicle));
				tracks = scene.updateTracks(blobs);
				cr = scene.countVehicles();
				
				REQUIRE(cr.carRight == 1);
				REQUIRE(cr.carLeft == 0);
				REQUIRE(cr.truckRight == 0);
				REQUIRE(cr.truckLeft == 0);
			}
		}

		WHEN("vehicle has negative velocity and\n"
			"is smaller than truck in width and height") {
			int vehicle_width = truck_width - 1;
			int vehicle_height = truck_height - 1;
			int velocity_x = -10; // moves from right to left
            list<Track>* tracks = createTrackBeforeCountPos(scene, vehicle_width,
				vehicle_height, count_pos_x, velocity_x);
		
			// count pos not reached yet, but track length sufficient
			cr = scene.countVehicles();
			REQUIRE(cr.carRight == 0);
            REQUIRE(tracks->front().getLength() > count_track_length);

			THEN("carRight is counted") {
				list<TrackEntry> blobs;
                Rect vehicle = tracks->front().getActualEntry().rect();
				vehicle.x += velocity_x;

				// one more updateTracks() and count pos is passed over
				blobs.push_back(TrackEntry(vehicle));
				tracks = scene.updateTracks(blobs);
				cr = scene.countVehicles();
				
				REQUIRE(cr.carRight == 0);
				REQUIRE(cr.carLeft == 1);
				REQUIRE(cr.truckRight == 0);
				REQUIRE(cr.truckLeft == 0);
			}
		}

		WHEN("vehicle has positive velocity and\n"
			"width is smaller than truck but\n"
			"height is larger than truck") {
			int vehicle_width = truck_width - 1;
			int vehicle_height = truck_height + 5;
			int velocity_x = 10; // moves from right to left
            list<Track>* tracks = createTrackBeforeCountPos(scene, vehicle_width,
				vehicle_height, count_pos_x, velocity_x);
		
			// count pos not reached yet, but track length sufficient
			cr = scene.countVehicles();
			REQUIRE(cr.carRight == 0);
            REQUIRE(tracks->front().getLength() > count_track_length);

			THEN("carRight is counted") {
				list<TrackEntry> blobs;
                Rect vehicle = tracks->front().getActualEntry().rect();
				vehicle.x += velocity_x;

				// one more updateTracks() and count pos is passed over
				blobs.push_back(TrackEntry(vehicle));
				tracks = scene.updateTracks(blobs);
				cr = scene.countVehicles();
				
				REQUIRE(cr.carRight == 1);
				REQUIRE(cr.carLeft == 0);
				REQUIRE(cr.truckRight == 0);
				REQUIRE(cr.truckLeft == 0);
			}
		}

		WHEN("vehicle has positive velocity and\n"
			"width and height is larger than truck but\n"
			"height is larger than truck") {
			int vehicle_width = truck_width - 1;
			int vehicle_height = truck_height + 5;
			int velocity_x = 10; // moves from right to left
            list<Track>* tracks = createTrackBeforeCountPos(scene, vehicle_width,
				vehicle_height, count_pos_x, velocity_x);
		
			// count pos not reached yet, but track length sufficient
			cr = scene.countVehicles();
			REQUIRE(cr.carRight == 0);
            REQUIRE(tracks->front().getLength() > count_track_length);

			THEN("carRight is counted") {
				list<TrackEntry> blobs;
                Rect vehicle = tracks->front().getActualEntry().rect();
				vehicle.x += velocity_x;

				// one more updateTracks() and count pos is passed over
				blobs.push_back(TrackEntry(vehicle));
				tracks = scene.updateTracks(blobs);
				cr = scene.countVehicles();
				
				REQUIRE(cr.carRight == 1);
				REQUIRE(cr.carLeft == 0);
				REQUIRE(cr.truckRight == 0);
				REQUIRE(cr.truckLeft == 0);
			}
		}
	} // end GIVEN("SceneTracker with one assigned track")
} // end SCENARIO("count vehicles", "[SceneTracker]")
