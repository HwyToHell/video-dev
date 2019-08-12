#include <iostream>
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>
#include <QApplication>
#include <QDebug>
#include <QDir>

#include "../../video-dev/car-count/include/config.h"
#include "../../video-dev/car-count/include/frame_handler.h"
#include "../../video-dev/car-count/include/tracker.h"
#include "../../utilities/inc/util-visual-trace.h"
#include "../inc/trackimages.h"


void adjustFrameSizeParams(Config& conf, FrameHandler& fh);
bool openCapSource(Config& conf, FrameHandler& fh);

int trc_tst_main(int argc, const char* argv[]) {
    ProgramOptions cmdLineOpts(argc, argv, "i:qr:v:");

    // file from command line or file chooser
    auto pConfig = std::make_unique<Config>();

    // read config file (sqlite db)
    if (!pConfig->readConfigFile()) {
        pConfig->saveConfigToFile();
    }

    // parse command line options
    if (!pConfig->readCmdLine(cmdLineOpts)) {
        std::cerr << "error parsing command line options" << std::endl;
        return EXIT_FAILURE;
    }

    auto pFrameHandler = std::make_unique<FrameHandler>(pConfig.get());
    pConfig->attach(pFrameHandler.get());

    // capSource must be open in order to know frame size
    if (!openCapSource(*pConfig, *pFrameHandler)) {
        std::cerr << "error opening capture source" << std::endl;
        return EXIT_FAILURE;
    }

    // recalcFrameSizeDependentParams, if different frame size
    adjustFrameSizeParams(*pConfig, *pFrameHandler);
    pConfig->saveConfigToFile();

    auto pTracker = std::make_unique<SceneTracker>(pConfig.get(), true);
    pConfig->attach(pTracker.get());

    // create inset
    std::string insetImgPath = pConfig->getParam("application_path");
    appendDirToPath(insetImgPath, pConfig->getParam("inset_file"));
    Inset inset = pFrameHandler->createInset(insetImgPath);
    //inset.putCount(cr);

    while(true)
    {
        if (!pFrameHandler->segmentFrame()) {
            std::cerr << "frame segmentation failed" << std::endl;
            break;
        }

    std::list<Track>* pTracks = pTracker->updateTracks(pFrameHandler->calcBBoxes());
    pFrameHandler->showFrame(pTracks, inset);

        if (cv::waitKey(10) == 27) 	{
            std::cout << "ESC pressed -> end video processing" << std::endl;
            //cv::imwrite("frame.jpg", frame);
            break;
        }
    }

    qDebug() << "test finished";
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
