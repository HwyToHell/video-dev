#include "../inc/sql_trace.h"

#include <list>
#include <string>
#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>

#include "../../car-count/include/tracker.h"
#include "../../utilities/inc/util-visual-trace.h"

int main(int argc, const char* argv[]) {

    cv::Size roi(100,100);
    cv::Size blob(30,20);
    std::list<Track> trackList;
    trackList.push_back(createTrackAt(roi, cv::Point(70,20), blob, cv::Point(5,0)));
    trackList.push_back(createTrackAt(roi, cv::Point(20,50), blob, cv::Point(-10,0)));

    // check work dir
    namespace fs = boost::filesystem;
    std::string wd ="/home/holger/counter";
    fs::path workDir(wd);
    std::cout << "work dir: " << workDir.native() << std::endl;
    if (!fs::exists(workDir)) {
        std::cerr << "directory does not exist: " << workDir.native() << std::endl;
        throw "path does not exist";
    }

    // video file
    std::string vf = "traffic320x240.avi";
    fs::path videoFilePath = workDir;
    videoFilePath /= vf;
    std::cout << "video file: " << videoFilePath.native() << std::endl;
    if (!fs::exists(videoFilePath)) {
        std::cerr << "directory does not exist: " << videoFilePath.native() << std::endl;
        throw "path does not exist";
    }

    // db file
    std::string dbFile = "track.sqlite";

    SqlTrace trace(workDir.native(), dbFile, "tracks");
    //SqlTrace trace(wd, dbFile, "tracks");
    bool success = trace.createTable();
    success = trace.insertTrackState(1, &trackList);


    return 0;
}
