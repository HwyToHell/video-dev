#pragma once

#if defined (_WIN32)
#pragma warning(disable: 4482) // MSVC10: enum nonstd extension
#include "../../cpp/inc/observer.h"
#include "../../cpp/inc/id_pool.h"
#include "../include/recorder.h"
#else
#include "../../cpp/inc/observer.h"
#include "../../cpp/inc/id_pool.h"
#include "../include/recorder.h"
#endif

// helper functions
double euclideanDist(const cv::Point& pt1, const cv::Point& pt2);
double round(double number); // not necessary in C++11
inline bool signBit(double x) {return (x < 0) ? true : false;} // use std signbit in C++11


/// configuration class for changeable parameters, declared in config.h
class Config;

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


/// a time sequence of the same shape (track), with the history of the shape occurances
class Track {
public:
	enum Direction {none, left, right};		// C++11: enum class Direction {left, right};

	/// construct track with initial track entry
	/// \param[in] blob initial track entry
	/// \param[in] id track ID
    Track(int id = 0);

	/// assignment operator
	Track& operator= (const Track& source);
	
	/// adds motion detection to track
	/// blob is clipped, if outside roi area (non-const parameter)
	bool addTrackEntry(cv::Rect& blob, const cv::Size roi);

	/// adds substitute motion detection, extrapolating from prevoius size and velocity
	bool addSubstitute(cv::Size roi);

	// TODO delete
	void checkPosAndDir(); 

	/// returns motion object at current time (t)
	const TrackEntry& getActualEntry() const;

	/// returns track confidence (as measure of reliability)
	int getConfidence() const;

	/// returns history size
	int getHistory() const;

	/// returns track ID
	int getId() const;

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
	bool isMarkedForDelete();

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
	/// \param[in] blobs new unassigned motion detections
	/// \param[in] maxConf maximum value for confidence of track
	/// \param[in] maxDeviation maximum allowed deviation in % of width and height for new motion detection
	/// \param[in] maxDist maximum distance in pixels of new motion detection
	void updateTrack(std::list<cv::Rect>& blobs, cv::Size roi, int maxConf, 
		double maxDeviation, double maxDist);
	
	/// update track based on blob intersection
	/// solves tracking error #1
	/// \param[in] blobs new unassigned motion detections
	/// \param[in] minInter necessary percentage of intersection between track
	///  and new detected blob (0 ... 1 = 100% of new detected blob)
	/// \param[in] maxConf maximum value for confidence of track
	void updateTrackIntersect(std::list<cv::Rect>& blobs, cv::Size roi, double minInter = 0.5, int maxConf = 2);

private:
	cv::Point2d				m_avgVelocity;
	int						m_confidence;
	bool					m_counted;
	std::vector<TrackEntry> m_history; // dimension: time
	int						m_id;
	bool					m_isMarkedForDelete;
	Direction				m_leavingRoiTo;
	bool					m_isOccluded;
	cv::Point2d				m_prevAvgVelocity;

	cv::Point2d& updateAverageVelocity(cv::Size roi);
};


/// find matching tracks by ID
class TrackID_eq : public std::unary_function<Track, bool> {
public: 
	TrackID_eq (const int id) : m_ID(id) {}
	inline bool operator() (const Track& track) const { 
		return (m_ID == track.getId());
	}
private:
	int m_ID;
};


/// overlapping tracks = occlusion
class Occlusion {
// with own id counter
public:
				Occlusion(IdGen* pIdGen, cv::Size roi, Track* movingRight, Track* movingLeft, int steps);
				~Occlusion(); // free ID
	
	void		assignBlobs(std::list<cv::Rect>& blobs);
	size_t		id();
	bool		hasPassed();
	Track*		movingLeft();
	Track*		movingRight();
	cv::Rect	rect();
	int			remainingUpdateSteps();
	void		setId(size_t id);
	cv::Rect	updateRect();

private:
	size_t		m_id;
	IdGen*		m_idGen;
	bool		m_hasPassed;
	Track*		m_movingLeft;
	Track*		m_movingRight;
	cv::Rect	m_rect;
	int			m_remainingUpdateSteps;
	cv::Size	m_roiSize;
};


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

	/// reversing tracks are deleted,
	/// a new track is created from last track entry
	std::list<Track>* deleteReversingTracks();

	/// return pointer to list of overlapping tracks
	std::list<Occlusion>* getOcclusions();

	/// at least two tracks overlap, based on overlap regions
	bool isOverlappingTracks();

	/// returns ID for new track, if max number of tracks not exceeded
	int nextTrackID();

	// TODO delete
	void printVehicles();

	/// releases ID for deleted track
	bool returnTrackID(int id);

	// TODO make private in final version
	/// set isOccluded flag for each occluded track, calculates occlusion rectangle
	std::list<Occlusion>* setOcclusion();

	/// returns motion objects that need to be classified by DNN
	std::vector<TrackEntry>triggerDNNClassif();

	/// update observer's (SceneTracker) parameters from subject (Config)
	void update();

	/// updates tracks with new motion objects
	/// returns pointer to updated track list
    std::list<Track>* updateTracks(std::list<cv::Rect>& blobs);

	/// update tracks with new intersection method
	std::list<Track>* updateTracksIntersect(std::list<cv::Rect>& blobs, long long frameCnt);
	
	// DEBUG
	void inspect(int frameCnt);
	// END DEBUG
private:
	// changeable parameters (at run-time)
	ClassifyVehicle			m_classify;
	cv::Size				m_roiSize;
	int						m_maxConfidence;
	double					m_maxDeviation;
	double					m_maxDist;
	unsigned int			m_maxNoIDs;

	// variables
	CountRecorder*			m_recorder; 
	std::list<Occlusion>	m_occlusions;
	IdGen					m_occlusionIDs;
	std::list<Track>		m_tracks;
	std::list<int>			m_trackIDs;

};

/// adjust position of substitute track entry based on blob edges (occluded tracks)
void adjustSubstPos(const cv::Rect& blob, cv::Rect& rcRight, cv::Rect& rcLeft);

/// create substitute track entry by using track velocity
cv::Rect calcSubstitute(const Track& track);

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
void printRect(Occlusion& occ);
// END_DEBUG

// DEBUG
void printVelocity(Track& track);
// END_DEBUG

/// steps in occlusion, considering average velocities
int remainingOccludedUpdateSteps(const Track& mvLeft, const Track& mvRight);