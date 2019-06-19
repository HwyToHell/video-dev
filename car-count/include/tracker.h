#pragma once
#include <opencv2/opencv.hpp>


#if defined(__linux__)
#include "/home/holger/app-dev/cpp/inc/observer.h"
#include "/home/holger/app-dev/cpp/inc/id_pool.h"
#include "../include/recorder.h"
#elif(_WIN32)
    #if !defined(__GNUG__) && (_MSC_VER == 1600)
    #pragma warning(disable: 4482) // MSVC10: enum nonstd extension
    #endif
#include "../../cpp/inc/observer.h"
#include "../../cpp/inc/id_pool.h"
#include "recorder.h"
#endif

// forward decls
class Config;
class Track;


//////////////////////////////////////////////////////////////////////////////
// Occlusion /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// occlusion = overlapping tracks
class Occlusion {
public:
				Occlusion(cv::Size roi, Track* movingRight, Track* movingLeft, int steps, size_t id = 0);		
	void		assignBlobs(std::list<cv::Rect>& blobs);
	bool		hasPassed() const;
	size_t		id() const;
	bool		isMarkedForDelete() const;
	Track*		movingLeft() const;
	Track*		movingRight() const;
	cv::Rect	rect() const;
	int			remainingUpdateSteps();
	void		setId(size_t id);
	cv::Rect	updateRect();
	
private:
	bool		m_hasPassed;
	size_t		m_id;
	bool		m_isMarkedForDelete;
	Track*		m_movingLeft;
	Track*		m_movingRight;
	cv::Rect	m_rect;
	int			m_remainingUpdateSteps;
	cv::Size	m_roiSize;
};


//////////////////////////////////////////////////////////////////////////////
// Occlusion Id List /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// collection of occlusions
class OcclusionIdList {
public:
	typedef std::list<Occlusion>::const_iterator	IterOcclusionConst;
	typedef std::list<Occlusion>::iterator			IterOcclusion;
								OcclusionIdList(size_t maxIdCount);
	bool						add(Occlusion& occlusion);
	void						assignBlobs(std::list<cv::Rect>& blobs);
	std::list<Occlusion>*		getList();
	bool						isOcclusion();
	IterOcclusion				remove(IterOcclusion iOcclusion);
private:
	std::list<Occlusion>		m_occlusions;
	IdGen						m_occlusionIds;
};


//////////////////////////////////////////////////////////////////////////////
// Track Entry ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// representation for a blob (detected geometric moving object) in the track vector
class TrackEntry {
public:
	// TODO DELETE TrackEntry(int x = 0, int y = 0, int width = 100, int height = 50);
	TrackEntry(cv::Rect rect, cv::Point2d velocity);
	
    cv::Point2i centroid() const;
    int height() const;
    cv::Rect rect() const;
	cv::Point2i velocity() const;
    int width() const;

    bool isClose(const TrackEntry& teCompare, const int maxDist);
    bool isSizeSimilar(const TrackEntry& teCompare, const double maxDeviation) const;
private:
	cv::Rect	m_bbox;
	cv::Point2i m_centroid;
	cv::Point2d m_velocity;
};


//////////////////////////////////////////////////////////////////////////////
// Track /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// a time sequence of the same shape (track), with the history of the shape occurances
class Track {
public:
	enum Direction {none, left, right};		// C++11: enum class Direction {left, right};

	/// construct track with initial track entry
	/// \param[in] blob initial track entry
	/// \param[in] id track ID
    Track(size_t id = 0);
	
	/// adds motion detection to track
	/// blob is clipped, if outside roi area (non-const parameter)
    bool addTrackEntry(const cv::Rect& blob, const cv::Size roi);

	/// adds substitute motion detection, extrapolating from prevoius size and velocity
	bool addSubstitute(cv::Size roi);

	// TODO delete
	void checkPosAndDir(); 

	/// returns motion object at current time (t)
	const TrackEntry& getActualEntry() const;

	/// returns track confidence (as measure of reliability)
	int getConfidence() const;

	/// returns history size
    size_t getHistory() const;

	/// returns track ID
    size_t getId() const;

	/// returns track length
	double getLength() const;

	/// returns motion object from previous update step (t-1)
	const TrackEntry& getPreviousEntry() const;

	/// returns velocity of movin object
	cv::Point2d getVelocity() const;

	/// returns true, if track has been counted already
	bool isCounted();

	/// returns direction to which track leaves the roi area
	Direction leavingRoiTo();

	/// returns true, if track has been marked for deletion in next update step
	bool isMarkedForDelete() const;

	/// returns true, if track is occluded by another track
	bool isOccluded();

	/// returns true, if track velocity in x direction has changed sign
	bool isReversingX(const double backlash = 0.5);

	/// mark track for deletion
	void markForDeletion();

	/// set counted status
	void setCounted(bool state);

	/// set flag if track is about leaving roi area
	/// (e.g. track moves to right and touches right border of roi)
	Direction setLeavingRoiFlag(cv::Size roi);

	/// set occluded status
	void setOccluded(bool state);

	/// update track depending on size and distance of motion detection
	
	/// update track based on blob intersection
	/// solves tracking error #1
	/// \param[in] blobs new unassigned motion detections
	/// \param[in] minInter necessary percentage of intersection between track
	///  and new detected blob (0 ... 1 = 100% of new detected blob)
	/// \param[in] maxConf maximum value for confidence of track
	void updateTrackIntersect(std::list<cv::Rect>& blobs, cv::Size roi, double minInter = 0.5, int maxConf = 2);

	/// update track that has just passed occlusion area
	bool updateTrackPassedOcclusion(std::list<cv::Rect>& blobs, cv::Size roi, int maxConf);

private:
	cv::Point2d				m_avgVelocity;
	int						m_confidence;
	bool					m_counted;
	std::vector<TrackEntry> m_history; // dimension: time
    size_t					m_id;
	bool					m_isMarkedForDelete;
    bool					m_isOccluded;
	Direction				m_leavingRoiTo;
	cv::Point2d				m_prevAvgVelocity;

	cv::Point2d& updateAverageVelocity(cv::Size roi);
};


/// find matching tracks by ID
class TrackID_eq : public std::unary_function<Track, bool> {
public: 
    TrackID_eq (const size_t id) : m_ID(id) {}
	inline bool operator() (const Track& track) const { 
		return (m_ID == track.getId());
	}
private:
    size_t m_ID;
};


//////////////////////////////////////////////////////////////////////////////
// Scene Tracker /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// collection of all tracks, updates them based on the moving blobs from new frame:
/// create new tracks for unassigned blobs, delete orphaned tracks
/// evaluate counting criteria (track length, track confidence)
class SceneTracker : public Observer {
public:
	/// construct tracker
	/// \param[in] pConfig pointer to configuration object, containing changeable parameters
	SceneTracker(Config* pConfig);

	/// assigns blobs to existing tracks with overlapping area,
	/// creates new tracks for non-matching blobs
	std::list<Track>* assignBlobs(std::list<cv::Rect>& blobs);

	/// assign blobs in occlusion area
	// TODO DELETE std::list<Track>* assignBlobsInOcclusion(Occlusion& occlusion, std::list<cv::Rect>& blobs);

	// TODO reserved for later implementation of data base 
	void attachCountRecorder(CountRecorder* pRecorder);

	/// simple classifcation of vehicles based on length and size
	CountResults countVehicles(int frameCnt = 0);

	/// marked tracks are deleted
	std::list<Track>* deleteMarkedTracks();
	
	/// delete occlusions with tracks that are marked for deletion
	const std::list<Occlusion>* deleteOcclusionsWithMarkedTracks();

	/// reversing tracks are deleted,
	/// a new track is created from last track entry
	std::list<Track>* deleteReversingTracks();

	/// at least two tracks overlap, based on overlap regions
	bool isOverlappingTracks();

	/// returns ID for new track, if max number of tracks not exceeded
    size_t nextTrackID();

	/// return pointer to list of overlapping tracks
	const std::list<Occlusion>* occlusionList();

	// TODO delete
	void printVehicles();

	/// releases ID for deleted track
    bool returnTrackID(size_t id);

	// TODO make private in final version
	/// set isOccluded flag for each occluded track, calculates occlusion rectangle
	const std::list<Occlusion>* setOcclusion();

	friend void showTracks(std::string winName, const SceneTracker& scene);

	/// returns motion objects that need to be classified by DNN
	std::vector<TrackEntry>triggerDNNClassif();

	/// update observer's (SceneTracker) parameters from subject (Config)
	void update();

	/// updates tracks with new motion objects (new intersection method)
	/// returns pointer to updated track list
    std::list<Track>* updateTracks(std::list<cv::Rect>& blobs, long long frameCnt = 0);

	// DEBUG
	void inspect(int frameCnt);
	// END DEBUG
private:
	// changeable parameters (at run-time)
	ClassifyVehicle		m_classify;
	cv::Size			m_roiSize;
	int					m_maxConfidence;
	double				m_maxDeviation;
	double				m_maxDist;
	unsigned int		m_maxNoIDs;

	// variables
	CountRecorder*		m_recorder; 
	OcclusionIdList		m_occlusions;
	std::list<Track>	m_tracks;
    std::list<size_t>   m_trackIDs;
};


struct TrackState {
public:
    TrackState(std::string name, cv::Size roi, std::list<cv::Rect> blobs, const std::list<Occlusion>* occlusions, std::list<Track> tracks) :
      m_name(name), m_roi(roi), m_blobs(blobs), m_occlusions(*occlusions), m_tracks(tracks) {}
	std::string				m_name;
    cv::Size                m_roi;
	std::list<cv::Rect>		m_blobs;
	std::list<Occlusion>	m_occlusions;
	std::list<Track>		m_tracks;
};


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// math helper functions
double euclideanDist(const cv::Point& pt1, const cv::Point& pt2);
double round(double number); // not necessary in C++11
inline bool signBit(double x) {return (x < 0) ? true : false;} // use std::signbit in C++11

/// adjust position of substitute track entry based on blob edges (occluded tracks)
void adjustSubstPos(const cv::Rect& blob, cv::Rect& rcRight, cv::Rect& rcLeft);

/// create substitute track entry by using track velocity
cv::Rect calcSubstitute(const Track& track);

/// clip rect if it exeeds roi
cv::Rect clipAtRoi(cv::Rect rec, cv::Size roi);

/// combine tracks that have
///	same direction and area intersection
/// assign smaller tracks to longer ones
std::list<Track>* combineTracks(std::list<Track>& tracks, cv::Size roi);

/// velocityX of two tracks has different sign
bool isDirectionOpposite(Track& track, Track& trackCompare, const double backlash);

/// distance after next update step (with average velocity) shorter than half velocity 
bool isNextUpdateOccluded(const Track& mvLeft, const Track& mvRight);

/// return rectangle while tracks are occluded
cv::Rect occludedArea(Track& mvLeft, Track& mvRight, int updateSteps);

// DEBUG
void printRect(cv::Rect& rect);
void printTrackRect(Track& track);
// END_DEBUG

// DEBUG
void printVelocity(Track& track);
// END_DEBUG

/// steps in occlusion, considering average velocities
int remainingOccludedUpdateSteps(const Track& mvLeft, const Track& mvRight);
