#include "stdafx.h"
#include <fstream>
#include "../car-count/include/frame_handler.h"

using namespace std;


SCENARIO("#frm001 FrameHandler::FrameHandler", "[Frame]") {
	GIVEN("frameHandler with std params") {
		;
	}
}
			
SCENARIO("#frm002 FrameHandler::adjustFrameSizeDependentParams", "[Frame]") {
	GIVEN("dependent parameter list, new video size") {
		Config config;
		
		// create map with test values
		typedef pair<const string, int> TPairStr;
		map<TPairStr::first_type, TPairStr::second_type> depParams;
		
		const int old_size_x = 320;
		const int old_size_y = 240;
		depParams.insert(TPairStr("framesize_x", old_size_x));
		depParams.insert(TPairStr("framesize_y", old_size_y));
		int new_size_x = 640;
		int new_size_y = 480;
		
		const int intToAdjust = 100;		
		depParams.insert(TPairStr("roi_x", intToAdjust));
		depParams.insert(TPairStr("roi_width", intToAdjust));
		depParams.insert(TPairStr("roi_y", intToAdjust));
		depParams.insert(TPairStr("roi_height", intToAdjust));
		depParams.insert(TPairStr("inset_height", intToAdjust));

		depParams.insert(TPairStr("blob_area_min", intToAdjust));
		depParams.insert(TPairStr("blob_area_max", intToAdjust));		
		depParams.insert(TPairStr("track_max_distance", intToAdjust));

		depParams.insert(TPairStr("count_pos_x", intToAdjust));
		depParams.insert(TPairStr("count_track_length", intToAdjust));
		depParams.insert(TPairStr("truck_width_min", intToAdjust));		
		depParams.insert(TPairStr("truck_height_min", intToAdjust));

		// copy test values to config
		class SetCfgParam {
			Config* m_cfg;
		public:
			SetCfgParam(Config* cfg) : m_cfg(cfg) {}
			void operator()(TPairStr& param) {
				m_cfg->setParam(param.first, to_string((long long)param.second));
			}
		};
		for_each(depParams.begin(), depParams.end(), SetCfgParam(&config));

		// set up frame_handler
		FrameHandler frameHandler(&config);
		
		WHEN("parameters are adjusted") {
			frameHandler.adjustFrameSizeDependentParams(new_size_x, new_size_y);

			THEN("getParam() shows adjusted values") {
				// dependent on x
				REQUIRE( depParams.at("roi_x") * new_size_x / old_size_x 
					== stoi(config.getParam("roi_x")) );
				REQUIRE( depParams.at("roi_width") * new_size_x / old_size_x 
					== stoi(config.getParam("roi_width")) );
				REQUIRE( depParams.at("count_pos_x") * new_size_x / old_size_x 
					== stoi(config.getParam("count_pos_x")) );
				REQUIRE( depParams.at("count_track_length") * new_size_x / old_size_x 
					== stoi(config.getParam("count_track_length")) );
				REQUIRE( depParams.at("truck_width_min") * new_size_x / old_size_x 
					== stoi(config.getParam("truck_width_min")) );
				
				// dependent on y
				REQUIRE( depParams.at("roi_y") * new_size_y / old_size_y 
					== stoi(config.getParam("roi_y")) );
				REQUIRE( depParams.at("roi_height") * new_size_y / old_size_y 
					== stoi(config.getParam("roi_height")) );
				REQUIRE( depParams.at("inset_height") * new_size_y / old_size_y 
					== stoi(config.getParam("inset_height")) );
				REQUIRE( depParams.at("truck_height_min") * new_size_y / old_size_y 
					== stoi(config.getParam("truck_height_min")) );

				// dependent on (x + y) / 2
				REQUIRE( depParams.at("track_max_distance") 
					* (new_size_x /  old_size_x + new_size_y / old_size_y) / 2
					== stoi(config.getParam("track_max_distance")) );

				// dependent on x * y
				REQUIRE( depParams.at("blob_area_min") 
					* new_size_x /  old_size_x * new_size_y / old_size_y
					== stoi(config.getParam("blob_area_min")) );
				REQUIRE( depParams.at("blob_area_max") 
					* new_size_x /  old_size_x * new_size_y / old_size_y
					== stoi(config.getParam("blob_area_max")) );

				//new video size
				REQUIRE( new_size_x == stoi(config.getParam("frame_size_x")) );
				REQUIRE( new_size_y == stoi(config.getParam("frame_size_y")) );
			}
			
		}


	} 
} // end SCENARIO("adjust video size dependent parameters", "[Config]")

SCENARIO("#frm003 FrameHandler::locateVideoFilePath", "[Frame]") {
	GIVEN("frameHandler with std params") {
		Config config;
		FrameHandler frameHandler(&config);

		WHEN("test file is created in current dir") {
			string fileName("testfile_can_be_deleted.txt");
			ofstream outfile(fileName);
			outfile << "test file only" << endl;
			outfile.close();

			THEN("file is successfully located in current dir") {
				#if defined (_WIN32)
				char bufPath[_MAX_PATH];
				_getcwd(bufPath, _MAX_PATH);
				#elif defined (__linux__)
				char bufPath[PATH_MAX];
				getcwd(bufPath, PATH_MAX);
				#endif
				string filePath(bufPath);
				appendDirToPath(filePath, fileName);

				string locatedPath = frameHandler.locateFilePath(fileName);
				REQUIRE(filePath == locatedPath);
				// delete test file
				REQUIRE(remove(filePath.c_str()) == 0);
			}
		}

		WHEN("test file is created in application path") {
			string appPath = config.getParam("application_path");
			REQUIRE(true == isFileExist(appPath));
			string fileName("testfile_app_path.txt");
			appendDirToPath(appPath, fileName);

			ofstream outfile(appPath);
			outfile << "test file only" << endl;
			outfile.close();
			
			THEN("file is successfully located in application path") {
				string locatedPath = frameHandler.locateFilePath(fileName);
				REQUIRE(appPath == locatedPath);
				// delete test file
				REQUIRE(remove(appPath.c_str()) == 0);
			}
		}
	}
}


SCENARIO("#frm004 FrameHandler::initCam, initFileReader, getFrameSize", "[Frame]") {
	GIVEN("frameHandler with std params") {
		Config config;
		FrameHandler frameHandler(&config);
		
/*		WHEN("internal cam is initialized with default resolution ID") {
			REQUIRE(true == frameHandler.initCam(0, 0));
			THEN("frame size is 320 x 240") {
				cv::Size2d frameSize = frameHandler.getFrameSize();
				REQUIRE(frameSize.width == 320);
				REQUIRE(frameSize.height == 240);
			}
		}
*/
        #if defined (_WIN32)
        string videoFilePath("D:\\Users\\Holger\\counter\\traffic320x240.avi");
        #elif defined (__linux__)
        string videoFilePath("/home/holger/counter/traffic320x240.avi");
        #endif

		REQUIRE(isFileExist(videoFilePath));

		WHEN("file reader is initialized with valid video file") {
			REQUIRE(true == frameHandler.initFileReader(videoFilePath));
			THEN("frame size is 320 x 240") {
				cv::Size2d frameSize = frameHandler.getFrameSize();
				REQUIRE(frameSize.width == 320);
				REQUIRE(frameSize.height == 240);
			}
		}
	}
}
