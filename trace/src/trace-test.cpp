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

QMap<int, QString> populateFileMap(const QString& workDir);


int trace_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
   // using namespace std;

    auto pConfig = std::make_unique<Config>();
    auto pTracker = std::make_unique<SceneTracker>(pConfig.get(), true);
    QString dirImages("/home/holger/counter/2017-09-18/opposite/6 - 30 truck 7m10s/track debug");
    if ( !QDir(dirImages).exists() ) {
        qDebug() << "directory does not exist:" << dirImages;
        qDebug() << "exit";
        return -1;
    }

    // get names of segmentation image files
    QMap<int, QString> inputFiles = populateFileMap(dirImages);
    if (inputFiles.size() == 0) {
        qDebug() << "file map empty";
        qDebug() << "exit";
        return -1;
    }

    // set config->roi to image size, update SceneTracker
    setRoiToImgSize(pConfig.get(), dirImages, inputFiles.first());
    pTracker.get()->update();

    // apply tracking algorithm, save tracking results in global variable
    if (!trackImages(dirImages, inputFiles, pTracker.get())) {
            qDebug() << "tracking not sucessful";
            qDebug() << "exit";
            return -1;
    }

    // search for state with two tracks
    // size_t nTracks = 0;
    g_itCurrent = g_trackStateMap.cbegin();
    while (g_itCurrent->second.front().m_tracks.size() < 2 && g_itCurrent != g_trackStateMap.end()) {
        // nTracks = g_itCurrent->second.front().m_tracks.size();
        ++g_itCurrent;
    }

    cv::Size dispSize(200,200);
    cv::Mat canvas(dispSize, CV_8UC3, black);

    cv::Size roi = g_itCurrent->second.front().m_roi;
    printIdsScaled(canvas, g_itCurrent->second.front().m_tracks, roi, true);
    printTracksScaled(canvas, g_itCurrent->second.front().m_tracks, roi, true);
    printOcclusionsScaled(canvas, g_itCurrent->second.front().m_occlusions, roi);

    cv::imshow("tracks", canvas);
    cv::waitKey(0);


    /*cout << endl << "Press <enter> to exit" << endl;
    std::stifring str;
    getline(cin, str);
    cout << str << endl;
    */

    qDebug() << "test finished";

    return 0;
}
