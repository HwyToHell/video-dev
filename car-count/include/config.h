#pragma once
#include <sqlite3.h>
#include "tracker.h"

#if defined(__linux__)
#include "/home/holger/app-dev/cpp/inc/observer.h"
#include "/home/holger/app-dev/cpp/inc/program_options.h"
#elif(_WIN32)
#include "../../cpp/inc/observer.h"
#include "../../cpp/inc/program_options.h"
#endif


class Parameter {
private:
	std::string mName;
	std::string mType;
	std::string mValue;
public:
	Parameter(std::string name = "name", std::string type = "type", std::string value = "value");
	std::string getName() const;
	std::string getType() const;
	std::string getValue() const;
	bool setValue(std::string& value);
};

// parameters used by application
// extern const char* configParams[25];
// static == internal linkage 
// -> each module uses separate const var, but with same const value
// -> embracing with anonymous namespace {} would also work

const char* const configParams[] = {
	"application_path",
	"video_file",
	"is_video_from_cam",
	"cam_device_ID",
	"cam_resolution_ID",
	"cam_fps",
	"frame_size_x",
	"frame_size_y",
	"inset_height",
	"inset_file",
	"roi_x",
	"roi_y",
	"roi_width",
	"roi_height",
	"blob_area_min",
	"blob_area_max",
	"track_max_confidence",
	"track_max_deviation",
	"track_max_distance",
	"max_n_of_tracks",
	"count_confidence",
	"count_pos_x", 
	"count_track_length",
	"truck_width_min",
	"truck_height_min" };

/// config: hold application parameters, command line parsing,
/// read from / write to sqlite config file,
/// automatic update of subscribed observers
class Config : public Subject {
public:
	// TODO second constructor with config file name specified
	Config();
	~Config();
	std::string getParam(std::string name);
	bool		insertParam(Parameter param);
	bool		readCmdLine(ProgramOptions po);
	bool		readConfigFile(std::string configFilePath = ""); 
	bool		saveConfigToFile(std::string configFilePath = "");
	bool		setParam(std::string name, std::string value);

private:
	std::string				m_appPath;
	std::string				m_configFilePath;
	std::string				m_configTableName;
	sqlite3*				m_dbHandle;
	std::string				m_homePath;
	bool					m_quiet;
	std::list<Parameter>	m_paramList;
	std::string				m_videoFilePath; // TODO delete, as implemented in FrameHandler

	// init: set path to application and con-fig file,
	// create parameter list (with std values)
	bool					init(); 
	bool					loadParamsFromDb();
	bool					populateStdParams();
	bool					queryDbSingle(const std::string& sql, std::string& value);
	bool					setAppPath(std::string appDir = "counter");
	void					setConfigProps(	std::string configDirPath, 
											std::string configFileName = "config.sqlite",
											std::string configTable = "config");
};

//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Help
void printCommandOptions();

// Config manipulation
/// set roi size in config
bool setRoiToConfig(Config* pConfig, cv::Size roi);

// Directory manipulation functions
std::string		getHomePath();
std::string&	appendDirToPath(std::string& path, const std::string& dir);
bool			isFileExist(const std::string& path);
bool			makeDir(const std::string& dir);
bool			makePath(std::string path);

// String conversion functions
bool			isInteger(const std::string& str);
bool			stob(std::string str); // string to bool
