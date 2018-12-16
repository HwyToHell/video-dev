#include "stdafx.h"

#include "../include/config.h"
#include "../include/frame_handler.h"
#include "../include/tracker.h"
#include "../include/recorder.h"
#include "../../cpp/inc/program_options.h"


using namespace std;

void adjustFrameSizeParams(Config& conf, FrameHandler& fh);
bool openCapSource(Config& conf, FrameHandler& fh);


int main(int argc, const char* argv[]) {
	// TODO define region of interest --> TODO: select graphical -or- detect by optical flow
	
	// command line arguments
	//  i(nput):		cam ID (single digit number) or file name,
	//					if empty take standard cam
	//  q(uiet)			quiet mode (take standard arguments from config file
	//  r(ate):			cam fps
	//  v(ideo size):	cam resolution ID (single digit number)

    const char* av[] = {
        argv[0],
        "-i",
        "traffic640x480.avi" };
        //"traffic320x240.avi" };

	int ac = sizeof(av) / sizeof(av[0]);

	int nArgs = 0;
    const char** argStrings = nullptr;
	// 't' indicates test for debugging in IDE
	// (project properties -> debugging -> command arguments)
	if ((argc > 1) && (*argv[1] == 't')) {
		nArgs = ac;
		argStrings = av;
	// otherwize use standard command line arguments
	} else {
		nArgs = argc;
		argStrings = argv;
	}
		
	ProgramOptions cmdLineOpts(nArgs, argStrings, "i:qr:v:");
	//ProgramOptions cmdLineOpts(argc, argv, "i:qr:v:");

	// create config, initialize standard parameter list, set path to config file
	Config config;
	Config* pConfig = &config;

	// TODO fcn: bool readConfigOptions(Config&);
	// read config file (sqlite db)
	if (!config.readConfigFile()) {
        config.saveConfigToFile();
    }

	// parse command line options
	if (!config.readCmdLine(cmdLineOpts)) {
		std::cerr << "error parsing command line options" << std::endl;
		return EXIT_FAILURE;
	}
	
	// frame reading, segmentation, drawing fcn
	FrameHandler frameHandler(pConfig); 
	FrameHandler* pFrameHandler = &frameHandler;
	config.attach(pFrameHandler);

	// capSource must be open in order to know frame size
	if (!openCapSource(config, frameHandler)) {
		std::cerr << "error opening capture source" << std::endl;
		return EXIT_FAILURE;
	}
	
	// recalcFrameSizeDependentParams, if different frame size
	adjustFrameSizeParams(config, frameHandler);
	config.saveConfigToFile();

	// collection of tracks and vehicles, generated by motion detection in frame_handler
	SceneTracker scene(pConfig); 
	SceneTracker* pScene = &scene;
	config.attach(pScene);

	// counting vehicles 
	// TODO save results in db, using CountRecorder class
	CountResults cr;

	// create inset
	string insetImgPath = config.getParam("application_path");
	appendDirToPath(insetImgPath, config.getParam("inset_file"));
	Inset inset = frameHandler.createInset(insetImgPath);
	inset.putCount(cr);

	list<TrackEntry> bboxList; // debug only, delete after
    list<Track>* pTracks;


	while(true)
	{
		
		if (!frameHandler.segmentFrame()) {
			cerr << "frame segmentation failed" << endl;
			break;
		}

		bboxList = frameHandler.calcBBoxes();
        pTracks = scene.updateTracks(bboxList);

		// DEBUG
		scene.inspect(frameHandler.getFrameCount());

		cr += scene.countVehicles(frameHandler.getFrameCount());
		inset.putCount(cr);

        frameHandler.showFrame(pTracks, inset);

		//frameHandler.writeFrame();

		if (cv::waitKey(10) == 27) 	{
			cout << "ESC pressed -> end video processing" << endl;
			//cv::imwrite("frame.jpg", frame);
			break;
		}
	}

	CountRecorder recorder(cr);
	recorder.printResults();

	cv::waitKey(0);
	return 0;
}


void adjustFrameSizeParams(Config& conf, FrameHandler& fh) {
	int widthNew = static_cast<int>(fh.getFrameSize().width);
	int heightNew = static_cast<int>(fh.getFrameSize().height);
	int widthActual = stoi(conf.getParam("frame_size_x"));
	int heightActual = stoi(conf.getParam("frame_size_y"));
	if ( widthNew != widthActual || heightNew != heightActual ) 
		 fh.adjustFrameSizeDependentParams(widthNew, heightNew);
	return;
}

bool openCapSource(Config& conf, FrameHandler& fh) {
	bool isFromCam = stob(conf.getParam("is_video_from_cam"));
	bool success = false;
	
	if (isFromCam) {
		// initCam
		int camID = stoi(conf.getParam("cam_device_ID"));
		int camResolutionID = stoi(conf.getParam("cam_resolution_ID"));
		success = fh.initCam(camID, camResolutionID);
	} else {
		// init video file
		std::string videoFileName = conf.getParam("video_file");
		std::string videoFilePath = fh.locateFilePath(videoFileName);
		success = fh.initFileReader(videoFilePath);
	}

	if (success) {
		return true;
	} else {
		std::cerr << "video device cannot be opened" << std::endl;
		return false;
	}
}
