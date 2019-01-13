#include "stdafx.h"
#include "../include/config.h"

#include <cctype> // isdigit()
#if defined (_WIN32)
#include <io.h> // _access()
#pragma warning(disable: 4996)
#else
#include <unistd.h>
#endif


using namespace std;

// observer.h
void updateObserver(Observer* pObserver) {
	pObserver->update();
}

Parameter::Parameter(std::string name, std::string type, std::string value) : mName(name), mType(type), mValue(value) {}

std::string Parameter::getName() const { return mName; }

std::string Parameter::getType() const { return mType; }

std::string Parameter::getValue() const { return mValue; }

bool Parameter::setValue(std::string& value) {
	mValue = value;
	return true;
}

class Param_eq : public unary_function<Parameter, bool> {
	std::string mName;
public: 
	Param_eq (const std::string& name) : mName(name) {}
	bool operator() (const Parameter& par) const { 
		return (mName == par.getName());
	}
};


//////////////////////////////////////////////////////////////////////////////
// parameter names used by application ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/*const char* configParams[] = {
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
	*/

//////////////////////////////////////////////////////////////////////////////
// Config ////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

Config::Config() :	m_appPath(std::string()),
					m_configFilePath(std::string()),
					m_configTableName(std::string()),
					m_dbHandle(nullptr),
					m_quiet(false),
					m_paramList(std::list<Parameter>()),
					m_videoFilePath(std::string()) {
	init();
}

Config::~Config() {
	if (m_dbHandle) {
		int rc = sqlite3_close(m_dbHandle);
		if (rc != SQLITE_OK)
			cerr << "Config destructor: data base was not closed correctly" << endl;
	}
}


std::string Config::getParam(std::string name) {
	using namespace	std;
	list<Parameter>::iterator iParam = find_if(m_paramList.begin(), m_paramList.end(), Param_eq(name));
	if (iParam == m_paramList.end()) {
		cerr << "getParam: parameter not in config: " << name <<endl;
		return string("");
	}
	else {
		return (iParam->getValue());
	}
}

bool Config::init() {
	using namespace std;
	// create parameter list with standard values
	// must be done before setAppPath, as m_appPath is saved in parameter list
	if (!populateStdParams()) {
		cerr << "init: cannot create parameter list" << endl;
		return false;
	}

	// set application path underneath /home
	if (!setAppPath("counter")) {
		cerr << "init: cannot create application path" << endl;
		return false;
	}

	// set path to config file and table name
	setConfigProps(m_appPath, "config.sqlite", "config");
		
	return true;
}


bool Config::insertParam(Parameter param) {
	m_paramList.push_back(param);
	return true;
}

// load parameters from sqlite db
// if parameter row does not exist in db, create one by using standard value
bool Config::loadParamsFromDb() {
	using namespace	std;
	
	// check valid object state
	if (!m_dbHandle) {
		cerr << "loadParamsFromDb: data base not open yet" << endl;
		return false;
	}
	if (m_paramList.empty()) {
		cerr << "loadParamsFromDb: parameter list not initialized yet" << endl;
		return false;
	}
	if (m_configTableName.empty()) {
		cerr << "loadParamsFromDb: db table not specified" << endl;
		return false;
	}

	bool success = false;
	stringstream ss;
	string sqlStmt;
	string answer;

	// wrap in transaction
	sqlStmt = "BEGIN TRANSACTION;";
	if (!queryDbSingle(sqlStmt, answer)) {
		cerr << "loadParamsFromDb: begin transaction failed" << endl;
		return false;
	}
	
	// check, if table exists
	ss.str("");
	ss << "create table if not exists " << m_configTableName << " (name text, type text, value text);";
	sqlStmt = ss.str();
	success = queryDbSingle(sqlStmt, answer);

	list<Parameter>::iterator iParam = m_paramList.begin();
	while (iParam != m_paramList.end())
	{
		// read parameter
		ss.str("");
		ss << "select value from " << m_configTableName << " where name=" << "'" << iParam->getName() << "';"; 
		string sqlRead = ss.str();
		success = queryDbSingle(sqlRead, answer);

		if (success && !answer.empty()) // use parameter value from db
			iParam->setValue(answer);
		else // use standard parameter value and insert new record into db
		{
			ss.str("");
			ss << "insert into " << m_configTableName << " values ('" << iParam->getName() << "', '" << iParam->getType() << "', '" << iParam->getValue() << "');";
			string sqlinsert = ss.str();
			answer = iParam->getValue();
			success =  queryDbSingle(sqlinsert, answer);
		}
		++iParam;
	}

		// end transaction
	sqlStmt = "END TRANSACTION;";
	if (!queryDbSingle(sqlStmt, answer)) {
		cerr << "loadParamsFromDb: end transaction failed" << endl;
		return false;
	}

	return success;
}


bool Config::populateStdParams() {
	m_paramList.clear();

	/* TODO set standard parameters */
	// TODO parameter for capFileName
	// capFile, capDevice, framesize, framerate
	m_paramList.push_back(Parameter("application_path", "string", "D:\\Users\\Holger\\counter"));
	m_paramList.push_back(Parameter("video_file", "string", "traffic640x480.avi"));
	m_paramList.push_back(Parameter("is_video_from_cam", "bool", "false"));

	m_paramList.push_back(Parameter("cam_device_ID", "int", "0"));
	m_paramList.push_back(Parameter("cam_resolution_ID", "int", "0"));
	m_paramList.push_back(Parameter("cam_fps", "int", "10"));

	m_paramList.push_back(Parameter("frame_size_x", "int", "320"));
	m_paramList.push_back(Parameter("frame_size_y", "int", "240"));
	m_paramList.push_back(Parameter("inset_height", "int", "64"));
	m_paramList.push_back(Parameter("inset_file", "string", "inset4.png"));
	
	// region of interest
	m_paramList.push_back(Parameter("roi_x", "int", "80"));
	m_paramList.push_back(Parameter("roi_y", "int", "80"));
	m_paramList.push_back(Parameter("roi_width", "int", "200"));
	m_paramList.push_back(Parameter("roi_height", "int", "200"));
	// blob assignment
	m_paramList.push_back(Parameter("blob_area_min", "int", "200"));
	m_paramList.push_back(Parameter("blob_area_max", "int", "20000"));
	// track assignment
	m_paramList.push_back(Parameter("track_max_confidence", "int", "4"));
	m_paramList.push_back(Parameter("track_max_deviation", "double", "80"));
	m_paramList.push_back(Parameter("track_max_distance", "double", "30"));
	// scene tracker
	m_paramList.push_back(Parameter("max_n_of_tracks", "int", "9")); // maxNoIDs
	// counting
	m_paramList.push_back(Parameter("count_confidence", "int", "3"));
	m_paramList.push_back(Parameter("count_pos_x", "int", "90")); // counting position (within roi)
	m_paramList.push_back(Parameter("count_track_length", "int", "20")); // min track length for counting
	// classification
	m_paramList.push_back(Parameter("truck_width_min", "int", "55")); // classified as truck, if larger 60
	m_paramList.push_back(Parameter("truck_height_min", "int", "26")); // classified as truck, if larger 28
	return true;
}


bool Config::queryDbSingle(const std::string& sql, std::string& value) {
	bool success = false;
	sqlite3_stmt *stmt;
	// empty value indicate error
	value.clear();

	int rc = sqlite3_prepare_v2(m_dbHandle, sql.c_str(), -1, &stmt, 0);
	if (rc == SQLITE_OK)
	{
		int step = SQLITE_ERROR;
		int nRow = 0; 
		do 
		{
			step = sqlite3_step(stmt);
			
			switch (step) {
			case SQLITE_ROW: 
				{
					// one result expected: take first row only and discard others
					if (nRow == 0)
					{
						int nCol = sqlite3_column_count(stmt);
						nCol = 0; // one result expected: take first column only
						if (sqlite3_column_type(stmt, nCol) == SQLITE_NULL) 
							cerr << __LINE__ << " NULL value in table" << endl;
						else
							value = (const char*)sqlite3_column_text(stmt, nCol);
					}
				}
				break;
			case SQLITE_DONE: break;
			default: 
				cerr << __LINE__ << "Error executing step statement" << endl;
				break;
			}
			++nRow;
		} while (step != SQLITE_DONE);
		
		rc = sqlite3_finalize(stmt);			
		if (rc == SQLITE_OK)
			success = true;
		else
			success = false;
	} 

	else // sqlite3_prepare != OK
	{
		cerr << "SQL error: " << sqlite3_errmsg(m_dbHandle) << endl;
		rc = sqlite3_finalize(stmt);
		success = false;
	}
	return success;
}


// command line arguments
//  i(nput):		cam ID (single digit number) or file name,
//					if empty take standard cam
//  q(uiet)			quiet mode (take standard arguments from config file
//  r(ate):			cam fps
//  v(ideo size):	cam resolution ID (single digit number)
// 1st: perform lexical checks
// 2nd: update config data (will be saved to data base later)
// physical test are done in other submodules, e.g. file existence will be checked 
bool Config::readCmdLine(ProgramOptions programOptions) {
	using namespace	std;
	bool isVideoSourceValid = false;

	// 1st lexical checks, argument by argument
	// i: video input from cam or file
	if (programOptions.exists('i')) { 
		string videoInput = programOptions.getOptArg('i');
		// check if single digit -> cam
		// check if file exists -> error message
		if (videoInput.size() == 0) // use std cam
			videoInput.append("0");
		
		if (videoInput.size() == 1) { // cam ID
			string camID = videoInput.substr(0, 1);
			
			if (isdigit(camID.at(0))) { // cam ID must be int digit
				setParam("cam_device_ID", camID);
				setParam("is_video_from_cam", "true");
				isVideoSourceValid = true;
			}
			else {
				cerr << "invalid argument: camera ID option '-i " << camID << "'" << endl;
				cerr << "camera ID must be integer within range 0 ... 9" << endl;

				return false;
			}
		}

		else { // video file name
			setParam("video_file", videoInput);
			setParam("is_video_from_cam", "false");
			isVideoSourceValid = true;
		}
	} // end if (programOptions.exists('i'))

	//  q: quiet mode = take standard arguments, don't query for user input
	if (programOptions.exists('q')) {
		m_quiet = true;
	} else { 
		m_quiet = false;
	}

	// r: frame rate in fps
	if (programOptions.exists('r')) {
		string frameRate = programOptions.getOptArg('r');
		if (isInteger(frameRate)) {
			setParam("cam_fps", frameRate);
		}
		else {
			cerr << "invalid argument: frame rate option '-r " << frameRate << "'" << endl;
			cerr << "frame rate must be integer" << endl;
			return false;
		}
	} // end if (programOptions.exists('r'))

	// TODO change lexical check for single digit
	// v: video size
	if (programOptions.exists('v')) {
		string resolutionID = programOptions.getOptArg('v');
		// check if single digit
		// check if file exists -> error message
		if (resolutionID.size() == 1)  { // OK, must be one digit
			if (isdigit(resolutionID.at(0))) { // OK, must be int digit 
				setParam("cam_resolution_ID", resolutionID);
			} else {
				cerr << "invalid argument: resolution ID option '-v " << resolutionID << "'" << endl;
				cerr << "resolution ID must be integer within range 0 ... 9" << endl;
				return false;
			}
		} else {
				cerr << "invalid argument: resolution ID option '-v " << resolutionID << "'" << endl;
				cerr << "resolution ID must be integer within range 0 ... 9" << endl;
				return false;
		}
		return true;
	} // end if (programOptions.exists('v'))

	// not quiet, no input specified -> printHelp, exit
	if (!isVideoSourceValid && !m_quiet) {
		printCommandOptions();
		return false;
	}
	
	return true;
	// TODO move save to config file after adjustFrameSizeDependentParameters
	// 2nd: save to config file
	/*
		if (saveConfigToFile()) {
			return true;
		} else { // not able to save parameter changes from cmd line to config file
			cerr << "readCmdLine: error saving config to db file" << endl;
			return false;
		}
		*/
}


// save all parameters to sqlite db
// in: full path to config file
// if empty string (default), use m_configFilePath
bool Config::readConfigFile(std::string configFilePath) {
	using namespace std;

	if (configFilePath.empty()) {
		configFilePath = m_configFilePath;
	}

	if (!isFileExist(configFilePath)) {
		cerr << "readConfigFile: '" << configFilePath << "' does not exist" << endl;
		return false;
	}

	// db already open?
	if (!m_dbHandle) {
		int rc = sqlite3_open(configFilePath.c_str(), &m_dbHandle);
		if (rc != SQLITE_OK) {
			cerr << "readConfigFile: error opening " << configFilePath << endl;
			cerr << "message: " << sqlite3_errmsg(m_dbHandle) << endl;
			sqlite3_close(m_dbHandle);
			m_dbHandle = nullptr;
			return false;
		}
	}

	// read all parameters from config db file
	if (!loadParamsFromDb()) {
		cerr << "readConfigFile: cannot read parameters from config database" << endl;
		return false;
	}

	// set app path and save in parameter list
	// note: cannot be done in setAppPath(), as function is called before reading config file
	if (!setParam("application_path", m_appPath)) {
		cerr << "readConfigFile: cannot save application path in parameter list" << endl;
		return false;
	}

	// close db
	int rc = sqlite3_close(m_dbHandle);
	if (rc == SQLITE_OK) {
		m_dbHandle = nullptr;
	} else {
		cerr << "readConfigFile: error closing " << configFilePath << endl;
		return false;
	}

	// all functions returned successful
	return true;
}

// save all parameters to sqlite db 
// in: full path to config file
// if empty string (default), use m_configFilePath
bool Config::saveConfigToFile(std::string configFilePath) {
	using namespace std;

	if (configFilePath.empty()) {
		configFilePath = m_configFilePath;
	}

	// validate object state
	// db already open?
	if (!m_dbHandle) {
		int rc = sqlite3_open(configFilePath.c_str(), &m_dbHandle);
		if (rc != SQLITE_OK) {
			cerr << "saveConfigToFile: error opening " << configFilePath << endl;
			cerr << "message: " << sqlite3_errmsg(m_dbHandle) << endl;
			sqlite3_close(m_dbHandle);
			m_dbHandle = nullptr;
			return false;
		}
	}

	// table name specified
	if (m_configTableName.empty()) {
		cerr << "saveConfigToFile: db table not specified" << endl;
		return false;
	}

	// wrap in transaction
	string answer;
	string sqlStmt = "BEGIN TRANSACTION;";
	if (!queryDbSingle(sqlStmt, answer)) {
		cerr << "saveConfigToFile: begin transaction failed" << endl;
		return false;
	}

	// create table, if not exist
	stringstream ss;
	ss << "create table if not exists " << m_configTableName << " (name text, type text, value text);";
	sqlStmt = ss.str();
	if (!queryDbSingle(sqlStmt, answer)) {
		cerr << "saveConfigToFile: db table does not exist, cannot create it" << endl;
		return false;
	}
	
	// read all parameters and insert into db
	// if it does not exist -> insert
	// otherwise            -> update
    bool success = false;
    list<Parameter>::iterator iParam = m_paramList.begin();
	while (iParam != m_paramList.end())	{
		ss.str("");
		ss << "select 1 from " << m_configTableName << " where name='" << iParam->getName() << "';";
		sqlStmt = ss.str();
        success = queryDbSingle(sqlStmt, answer);

		// check, if parameter row exists in db
		ss.str("");
		if (stob(answer)) {// row does exist -> update
			ss << "update " << m_configTableName << " set value='" << iParam->getValue() << "' " 
				<< "where name='" << iParam->getName() << "';"; 
		} else { // row does not exist -> insert
			ss << "insert into " << m_configTableName << " values ('" << iParam->getName() << 
				"', '" << iParam->getType() << "', '" << iParam->getValue() << "');";
		}
		sqlStmt = ss.str();
		success = queryDbSingle(sqlStmt, answer);
		++iParam;
	}

    if  (!success) {
        cerr << "saveConfigToFile: writing parameters to db failed" << endl;
    }

	// end transaction
	sqlStmt = "END TRANSACTION;";
	if (!queryDbSingle(sqlStmt, answer)) {
		cerr << "saveConfigToFile: end transaction failed" << endl;
		return false;
	}

	// close db
	int rc = sqlite3_close(m_dbHandle);
	if (rc == SQLITE_OK) {
		m_dbHandle = nullptr;
	} else {
		cerr << "saveConfigFile: error closing " << configFilePath << endl;
		return false;
	}

	return true;
}



// set path to application directory 
// create, if it does not exist
bool Config::setAppPath(std::string appDir) {
	using namespace std;
	m_appPath = m_homePath = getHomePath();
	
	if (m_homePath.empty()) {
		cerr << "setAppPath: no home path set" << endl;
		return false;
	} 
	
	// set app path and save in parameter list
	appendDirToPath(m_appPath, appDir);
	if (!setParam("application_path", m_appPath)) {
		cerr << "setAppPath: cannot save application path in parameter list" << endl;
		return false;
	}

	// if appPath does not exist, create it
	if (!isFileExist(m_appPath)) {
		if (!makePath(m_appPath)) {
			cerr << "setAppPath: cannot create path: " << m_appPath << endl;
			return false;
		}
	}

	return true;
}	

// set path to config file and table name within config database
void Config::setConfigProps(std::string configDirPath, std::string configFileName, std::string configTable) {
	// set name and location of config file
	// note: existence of config file is checked in readConfigFile()
	m_configFilePath = configDirPath;
	appendDirToPath(m_configFilePath, configFileName);

	// table name for config data
	m_configTableName = configTable;
	return;
}


bool Config::setParam(std::string name, std::string value) {
	list<Parameter>::iterator iParam = find_if(m_paramList.begin(), m_paramList.end(),
		Param_eq(name));
	if (iParam == m_paramList.end())
		return false;
	iParam->setValue(value);
	return true;
}


// Directory manipulation functions
std::string& appendDirToPath(std::string& path, const std::string& dir) {
	#if defined (_WIN32)
		char delim = '\\';
	#elif defined (__linux__)
		char delim = '/';
	#else
		throw "unsupported OS";
	#endif
	
	// append delimiter, if missing 
	if (path.back() != delim && dir.front() != delim)
		path += delim;
	path += dir;

	return path;
}


std::string getHomePath() {
	using namespace	std;
	std::string home(std::string(""));
	
	#if defined (_WIN32)
		char *pHomeDrive = nullptr, *pHomePath = nullptr;
		pHomeDrive = getenv("HOMEDRIVE");
		if (pHomeDrive == nullptr) {
			cerr << "getHomePath: no home drive set in $env" << endl;
			std::string("");
		} else {
			home += pHomeDrive;
		}
		
		pHomePath = getenv("HOMEPATH");
		if (pHomePath == nullptr) {
			cerr << "getHomePath: no home path set in $env" << endl;
			return std::string("");
		} else {
			home = appendDirToPath(home, std::string(pHomePath));
		}

	#elif defined (__linux__)
        char *pHomePath = nullptr;
		pHomePath = getenv("HOME");
        if (pHomePath ==0) {
			cerr << "getHomePath: no home path set in $env" << endl;
			return std::string("");
		} else {
            home += pHomePath;
			home += '/';
		}
	#else
		throw "unsupported OS";
	#endif
	
	return home;
}


bool makeDir(const std::string& dir) {
	#if defined (_WIN32)
		std::wstring wDir;
		wDir.assign(dir.begin(), dir.end());

		if (CreateDirectory(wDir.c_str(), 0))
			return true;
		DWORD error = GetLastError();
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			return true;
		else {
			if (GetLastError() == ERROR_INVALID_NAME)
				std::cerr << "makeDir: invalid directory name: " << dir << std::endl;
			else
				std::cerr << "makeDir: last error code: " << GetLastError() << std::endl;
			return false;
		}
	#elif defined (__linux__)
        // TODO use mkdir from <sys/stat.h>
        mode_t permission = S_IRWXU | S_IRWXG | S_IRWXO;
        int status = mkdir(dir.c_str(), permission);
        if (status == 0) {
            return true;
        } else {
            if (errno == EEXIST) {
                return true;
            } else {
                if (errno == EACCES)
                    cerr << "permission denied" << endl;
            return false;
            }
        }
	#else
		throw "unsupported OS";
	#endif
}


// make full path
bool makePath(std::string path) {
	#if defined (_WIN32)
		char delim = '\\';
	#elif defined (__linux__)
		char delim = '/';
	#else
		throw "unsupported OS";
	#endif
	bool success = true;
	size_t pos = 0;

	// path must end with delim character
	if (path.back() != delim)
		path += delim;

	pos = path.find_first_of(delim);
	while (success == true && pos != std::string::npos)
	{
		if (pos != 0)
			success = makeDir(path.substr(0, pos));
		++pos;
		pos = path.find_first_of(delim, pos);
	}
	return success;
}


bool isFileExist(const std::string& path) {
	#if defined (_WIN32)
	if (_access(path.c_str(), 0) == 0) {
		return true;
	} else {
		return false;
		// errno: EACCES == acess denied
		//        ENOENT == file name or path not found
		//        EINVAL == invalid parameter
	}
	#elif defined (__linux__)
        // TODO use access from <unistd.h>
    int status = access(path.c_str(), F_OK);
    if (status == 0) {
        return true;
    } else {
        return false;
    }
	#else
		throw "unsupported OS";
	#endif
}


// String conversion functions
bool isInteger(const std::string& str) {
	std::string::const_iterator iString = str.begin();
	while (iString != str.end() && isdigit(*iString))
		++iString;
	return !str.empty() && iString == str.end();
}

// converts string to bool
// true:      "1", "true",  "TRUE"
// false: "", "0", "false", "FALSE"
bool stob(std::string str) {
	transform(str.begin(), str.end(), str.begin(), ::tolower);
	if (str.empty())
		return false;
	else if (str == "true" || str == "1")
		return true;
	else if (str == "false" || str == "0")
		return false;
	else
		throw "bad bool string";
}

// command line arguments
//  i(nput):		cam ID (single digit number) or file name,
//					if empty take standard cam
//  q(uiet)			quiet mode (take standard arguments from config file
//  r(ate):			cam fps
//  v(ideo size):	cam resolution ID (single digit number)
void printCommandOptions() {
	using namespace std;
	cout << "usage: video [options]" << endl;
	cout << endl;
	cout << "options:" << endl;
	cout << "-i                 specify video input, either" << endl;
	cout << "-i cam_ID            camera -or-" << endl;
	cout << "-i file              file" << endl;
	cout << "-q                 quiet mode (yes to all questions)" << endl;
	cout << "-r fps             set camera frame rate" << endl;
	cout << "-v resolution_ID   set camera resolution ID" << endl; 
	return;
}
