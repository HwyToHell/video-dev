#include "stdafx.h"
#include "../include/config.h"
#include "../include/recorder.h"

using namespace std;

double euclideanDist(const cv::Point& pt1, const cv::Point& pt2)
{
	cv::Point diff = pt1 - pt2;
	return sqrt((double)(diff.x * diff.x + diff.y * diff.y));
}

double round(double number) // not necessary in C++11
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

//////////////////////////////////////////////////////////////////////////////
// TrackEntry ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

TrackEntry::TrackEntry(int x, int y, int width, int height) {
	m_bbox.x = abs(x);
	m_bbox.y = abs(y);
	m_bbox.width = abs(width);
	m_bbox.height = abs(height);
	m_centroid.x = m_bbox.x + (m_bbox.width / 2);
	m_centroid.y = m_bbox.y + (m_bbox.height / 2);
}

TrackEntry::TrackEntry(cv::Rect rect) : m_bbox(rect) {
	m_centroid.x = m_bbox.x + (m_bbox.width / 2);
	m_centroid.y = m_bbox.y + (m_bbox.height / 2);
}

cv::Point2i TrackEntry::centroid() const {
	return m_centroid;
}

/// maxDist between centroids allowed
bool TrackEntry::isClose(const TrackEntry& teCompare, const int maxDist) {
	bool x_close, y_close;

	x_close = ( abs(this->centroid().x - teCompare.centroid().x) < maxDist );
	y_close = ( abs(this->centroid().y - teCompare.centroid().y) < maxDist );

	return (x_close & y_close);
}

/// maxDeviation% in width and height allowed
bool TrackEntry::isSizeSimilar(const TrackEntry& teCompare, const double maxDeviation) {
	if ( abs(this->width() - teCompare.width()) > (this->width() * maxDeviation / 100 ) )
		return false;
	if ( abs(this->height() - teCompare.height()) > (this->height() * maxDeviation / 100 ) )
		return false;
	return true;
}

int TrackEntry::height() const {
	return m_bbox.height;
}

cv::Rect TrackEntry::rect() const {
	return m_bbox;
}

int TrackEntry::width() const {
	return m_bbox.width;
}


//////////////////////////////////////////////////////////////////////////////
// Track /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

Track::Track(const TrackEntry& blob, int id) : m_avgVelocity(0,0), m_confidence(0),
	m_counted(false), m_id(id), m_isMarkedForDelete(false),
	m_leavingRoiTo(Direction::none), m_prevAvgVelocity(0,0)  {
	
	m_history.push_back(blob);
}

Track& Track::operator= (const Track& source) {
	m_history = source.m_history;
	return *this;
}

/// add TrackEntry to history and update velocity
void Track::addTrackEntry(const TrackEntry& blob, cv::Size roi) {
	m_history.push_back(blob);
	m_prevAvgVelocity = m_avgVelocity;
	updateAverageVelocity(roi);
	return;
}

/// create substitute TrackEntry, if no close blob available
void Track::addSubstitute(cv::Size roi) {
	// take average velocity and compose bbox from centroid of previous element
	cv::Point2i velocity((int)round(m_avgVelocity.x), (int)round(m_avgVelocity.y));

	cv::Point2i centroid = m_history.back().centroid();
	centroid += velocity;
	int height = m_history.back().height();
	int width = m_history.back().width();
	int x = centroid.x - width / 2;
	int y = centroid.y - height / 2;
	// TODO: use Track::avgHeight, avgWidth
	// TODO: check lower and upper bounds of video window

	addTrackEntry(TrackEntry(x, y, width, height), roi);
	return;
}

void Track::setLeavingRoiFlag(cv::Size roi) {

	// reliable velocity necessary, so only longer tracks are considered 
	// once leavingRoiTo has been assigned, no re-calculation necessary
	if ( (this->getHistory() > 4) && (m_leavingRoiTo == Direction::none) ) {

		int borderTolerance = roi.width * 5 / 100;

		// compare edge position depending on direction of movement
		bool isTrackMovingToRight = !(signBit(this->getVelocity().x));

		// track moves to right
		if (isTrackMovingToRight) {
			// right edge of lastTrackEntry close to right border of roi
			int rightEdgeTrack = this->getActualEntry().rect().x + this->getActualEntry().rect().width;
			int rightLimitRoi = roi.width - borderTolerance;
			if (rightEdgeTrack >= rightLimitRoi) {
				m_leavingRoiTo = Direction::right;
			}

		// track moves to left
		} else {
			// left edge of lastTrackEntry close to left border of roi
			int leftEdgeTrack = this->getActualEntry().rect().x;
			int leftLimitRoi = borderTolerance;
			if (leftEdgeTrack <= leftLimitRoi) {
				m_leavingRoiTo = Direction::left;
			}

		} // end_if track movement
	} // end_if assign leavingRoiTo

	return;
}

TrackEntry& Track::getActualEntry() { return m_history.back(); }

int Track::getConfidence() { return m_confidence; }

int Track::getHistory() { return m_history.size(); }

int Track::getId() { return m_id; }

double Track::getLength() {
	double length = euclideanDist(m_history.front().centroid(), m_history.back().centroid());
	return length;
}

TrackEntry& Track::getPreviousEntry() { 
	int idxPrevious = (int)m_history.size() - 2;
	if (idxPrevious < 0) {
		std::cerr << "Track::getPreviousEntry(): track has no previous entry, taking actual one" << std::endl;
		idxPrevious = 0;
	}
	return m_history.at(idxPrevious);
}

cv::Point2d Track::getVelocity() { return m_avgVelocity; }

bool Track::isCounted() { return m_counted; }

bool Track::isMarkedForDelete() {return m_isMarkedForDelete;}

bool Track::isReversingX() {

	// track history must have at least two elements to compare velocity
	//  and determine direction
	if (this->getHistory() >= 3) {
		bool currentDirection = signBit(m_avgVelocity.x);
		bool previousDirection = signBit(m_prevAvgVelocity.x);
		return (currentDirection != previousDirection);
	} else {
		return false;
	}
}

Track::Direction Track::leavingRoiTo() {
	return m_leavingRoiTo;
}

void Track::markForDeletion() {
	m_isMarkedForDelete = true;
	return;
}

void Track::setCounted(bool state) { m_counted = state; }

/// average velocity with recursive formula
cv::Point2d& Track::updateAverageVelocity(cv::Size roi) {
	const int window = 3;
	int lengthHistory = m_history.size();

	// TODO check bounds
	int borderTolerance = roi.width * 5 / 100;
	int leftEdge = getActualEntry().rect().x;
	int rightEdge = leftEdge + getActualEntry().rect().width;
	
	// front and back edge of blob are outside roi -> keep avgVelocity
	if ( (leftEdge <= borderTolerance) && (rightEdge >= (roi.width - borderTolerance)) ) {
		(void)0;
	// one or both blob edges are inside roi -> update moving average 
	} else {
		if (lengthHistory > 1) {
			int idxMax = lengthHistory - 1;
			cv::Point2d actVelocity = m_history[idxMax].centroid() - m_history[idxMax-1].centroid();

			if (idxMax <= window) { // window not fully populated yet
				m_avgVelocity = (m_avgVelocity * (double)(idxMax - 1) + actVelocity) * (1.0 / (double)(idxMax));
			}
			else { // window fully populated, remove window-1 velocity value
				cv::Point2d oldVelocity = m_history[idxMax-window].centroid() - m_history[idxMax-window-1].centroid();
				m_avgVelocity += (actVelocity - oldVelocity) * (1.0 / (double)window);
			}
		}
	}

	return m_avgVelocity;
}


/// iterate through detected blobs, check if size is similar and distance close
/// save closest blob to history and delete from blob input list 
void Track::updateTrack(std::list<TrackEntry>& blobs, cv::Size roi, int maxConf, 
	double maxDeviation, double maxDist) {
	
	typedef std::list<TrackEntry>::iterator TiterBlobs;
	// minDist determines closest track
	//	initialized with maxDist in order to consider only close tracks
	double minDist = maxDist; 
	TiterBlobs iBlobs = blobs.begin();
	TiterBlobs iBlobsMinDist = blobs.end(); // points to blob with closest distance to track
	
	while (iBlobs != blobs.end()) {
		if ( getActualEntry().isSizeSimilar(*iBlobs, maxDeviation) ) {
			double distance = euclideanDist(iBlobs->centroid(), getActualEntry().centroid());
			if (distance < minDist) {
				minDist = distance;
				iBlobsMinDist = iBlobs;
			}
		}
		++iBlobs;
	}// end iterate through all input blobs

	// assign closest shape to track and delete from list of blobs
	if (iBlobsMinDist != blobs.end()) // shape to assign available
	{
		addTrackEntry(*iBlobsMinDist, roi);
		iBlobs = blobs.erase(iBlobsMinDist);
		m_confidence <  maxConf ? ++m_confidence : m_confidence =  maxConf;
	}
	else // no shape to assign availabe, calculate substitute
	{
		--m_confidence;
		if (m_confidence <= 0) 
		{
			m_isMarkedForDelete = true;
		}
		else
			addSubstitute(roi);
	}

	return;
}


/// iterate through detected blobs, check if intersection
/// assign all intersecting blobs to track
/// store embracing rect of all assigned blobs to track
void Track::updateTrackIntersect(std::list<TrackEntry>& blobs, cv::Size roi, double minInter, int maxConf) {
	bool isTrackUpdateAvailable = false;
	bool isTrackMovingToRight = !(signBit(this->getVelocity().x));

	typedef std::list<TrackEntry>::iterator TiterBlobs;
	TiterBlobs iBlobToAssign;
	TiterBlobs iBlobs = blobs.begin();
	while (iBlobs != blobs.end()) {

		// assign based on area intersection between lastTrackEntry and newBlob
		// larger than defined minimum intersection (in percentage of newBlob area)
		cv::Rect rcIntersect = iBlobs->rect() & this->getActualEntry().rect();
		if ( rcIntersect.area() > (static_cast<int>(minInter * iBlobs->rect().area())) ) {

			// track moves to right OR leaves roi to right --> assign rightmost blob
			if ( (isTrackMovingToRight) || (m_leavingRoiTo == Direction::right) ) {

				// no blobToAssign yet -> take first matching one
				if (!isTrackUpdateAvailable) {
					iBlobToAssign = iBlobs;
					isTrackUpdateAvailable = true;

				// there is one blobToAssign already, check if there is a better one
				} else {
					if (iBlobs->centroid().x > iBlobToAssign->centroid().x)
						iBlobToAssign = iBlobs;
				}
			}

			// track moves to left OR leaves roi to left --> assign leftmost blob
			if ( (!isTrackMovingToRight) || (m_leavingRoiTo == Direction::left) ) { 

				// no blobToAssign yet -> take first matching one
				if (!isTrackUpdateAvailable) {
					iBlobToAssign = iBlobs;
					isTrackUpdateAvailable = true;

				// there is one blobToAssign already, check if there is a better one
				} else {
					if (iBlobs->centroid().x < iBlobToAssign->centroid().x)
						iBlobToAssign = iBlobs;
				}
			}

		} // end_if area intersection between lastTrackEntry and newBlob

		++iBlobs;
	}// end iterate through all input blobs


	// assign best fitting blob to track, delete blob, increase track confidence
	if (isTrackUpdateAvailable) {
		this->addTrackEntry(*iBlobToAssign, roi);
		iBlobs = blobs.erase(iBlobToAssign);
		m_confidence <  maxConf ? ++m_confidence : m_confidence =  maxConf;

	// no fitting blob available --> add substitute blob and decrease confidence
	} else {
		--m_confidence;
		if (m_confidence <= 0) 
		{
			m_isMarkedForDelete = true;
		}
		else
		// TODO move assignment of substitute after assignment of other tracks (SceneTracker::updateTracksInterSect)
		// reason: if other tracks are assigned, no substitute value needs to be added
			addSubstitute(roi);
	}
}


//////////////////////////////////////////////////////////////////////////////
// SceneTracker //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

SceneTracker::SceneTracker(Config* pConfig) : Observer(pConfig) {
	// max number of tracks assignable (9)
	m_maxNoIDs = stoi(mSubject->getParam("max_n_of_tracks"));
	for (int i = m_maxNoIDs; i > 0; --i) // fill trackIDs with 9 ints
		m_trackIDs.push_back(i);
	// update all other relevant parameters from Config
	update();
}

void SceneTracker::attachCountRecorder(CountRecorder* pRecorder) {
	m_recorder = pRecorder;
}

/// helper functor for SceneTracker::countVehicles
class CntAndClassify {
	int mFrameCnt;
	int mDbgCnt;
	ClassifyVehicle m_classify;
	CountResults mCr;
public:
	CntAndClassify(int frameCnt, ClassifyVehicle classify) : mFrameCnt(frameCnt), m_classify(classify) {mDbgCnt = 0;}

	void operator()(Track& track) {
		double trackLength = track.getLength();
		
		if (track.getConfidence() > m_classify.countConfidence) {
			cv::Point2d velocity(track.getVelocity());
			int width = track.getActualEntry().width();
			int height = track.getActualEntry().height();

			if (signBit(velocity.x)) { // moving to left
				if ((track.getActualEntry().centroid().x < m_classify.countPos) && !track.isCounted()) {
					if (trackLength > m_classify.trackLength) { // "count_track_length"
						track.setCounted(true);
						if ( (width >= m_classify.truckSize.width) && (height >= m_classify.truckSize.height) )
							++mCr.truckLeft;
						else // car
							++mCr.carLeft;
					}
				}
			}
			else {// moving to right
				if ((track.getActualEntry().centroid().x > m_classify.countPos) && !track.isCounted()) {
					if (trackLength > m_classify.trackLength) {
						track.setCounted(true);
						if ( (width >= m_classify.truckSize.width) && (height >= m_classify.truckSize.height) )
							++mCr.truckRight;
						else // car
							++mCr.carRight;
					}
				}
			}
		} // end_if track.getConfidence > countConfidence
	} // end operator()

	CountResults results() { 
		return mCr; 
	}
};

/// count vehicles depending on the following conditions
///  counting conditions for vehicle track:
///  - above confidence level
///  - beyond counting position
///  - completed track length
///  classifying conditions for trucks:
///  - average width and height
CountResults SceneTracker::countVehicles(int frameCnt) {
	// track confidence > x
	// moving left && position < yy
	// moving right && position > yy
	
	CntAndClassify cac = for_each(m_tracks.begin(), m_tracks.end(), CntAndClassify(frameCnt, m_classify));
	CountResults cr = cac.results();
	return cr;
}

/// provide track-ID, if available
int SceneTracker::nextTrackID()
{
	if (m_trackIDs.empty()) return 0;
	else {
		int id = m_trackIDs.back();
		m_trackIDs.pop_back();
		return id;
	}
}

/// return unused track-ID
bool SceneTracker::returnTrackID(int id)
{
	if (id > 0 ) {
		if (m_trackIDs.size() < m_maxNoIDs) {
			m_trackIDs.push_back(id);
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

/// update observer's (SceneTracker) parameters from subject (Config)
void SceneTracker::update() {
	// frame size
	int width = stoi(mSubject->getParam("roi_width"));
	int height = stoi(mSubject->getParam("roi_height"));
	m_roiSize = cv::Size(width, height);

	// max confidence for tracks allowed
	m_maxConfidence = stoi(mSubject->getParam("track_max_confidence"));

	// assign to track: max deviation of track entries in percent
	m_maxDeviation = stod(mSubject->getParam("track_max_deviation"));

	// assign to track: max distance between track entries in pixels
	m_maxDist = stod(mSubject->getParam("track_max_distance"));

	// min confidence for counting vehicle
	m_classify.countConfidence = stoi(mSubject->getParam("count_confidence"));

	// horizontal counting position in pixels
	m_classify.countPos = stoi(mSubject->getParam("count_pos_x")); 
	
	// min track length for counting vehicles
	m_classify.trackLength = stoi(mSubject->getParam("count_track_length"));

	// min truck size for classification
	m_classify.truckSize = cv::Size2i(stoi(mSubject->getParam("truck_width_min")),
		stoi(mSubject->getParam("truck_height_min")));
}

/// assign blobs to existing tracks
///  create new tracks from unassigned blobs
///  erase, if marked for deletion
std::list<Track>* SceneTracker::updateTracks(std::list<TrackEntry>& blobs) {

	// assign blobs to existing tracks
	// delete orphaned tracks and free associated Track-ID
	//  for_each track
	std::list<Track>::iterator iTrack = m_tracks.begin();
	while (iTrack != m_tracks.end())
	{

		iTrack->updateTrack(blobs, m_roiSize, m_maxConfidence, m_maxDeviation, m_maxDist); // assign new blobs to existing track

		if (iTrack->isMarkedForDelete())
		{
			returnTrackID(iTrack->getId());
			iTrack = m_tracks.erase(iTrack);
		}
		else
			++iTrack;
	}
	
	// create new tracks for unassigned blobs
	m_trackIDs.sort(std::greater<int>());
	std::list<TrackEntry>::iterator iBlob = blobs.begin();
	while (iBlob != blobs.end())
	{
		int trackID = nextTrackID();
		if (trackID > 0)
			// TODO check on trackID
			m_tracks.push_back(Track(*iBlob, trackID));
		++iBlob;
	}
	blobs.clear();
	
    return &m_tracks;
}


void combineTracks(std::list<Track>& tracks, cv::Size roi);

void checkOcclusion(std::list<Track>& tracks);



/// assign blobs to existing tracks
///  create new tracks from unassigned blobs
///  erase, if marked for deletion
std::list<Track>* SceneTracker::updateTracksIntersect(std::list<TrackEntry>& blobs, long long frameCnt) {

	//1 assign blobs
	this->assignBlobs(blobs);

	// TODO create substitute value for track without assigned blob

	//2 combine tracks
	combineTracks(m_tracks, m_roiSize);

	//3 delete tracks
	this->deleteMarkedTracks();

	//4 check leaving roi
	this->checkTracksLeavingRoi();

	//5 check reversing direction
	// delete relevant tracks (blobs will be assigned to new track in next update step)
	this->deleteReversingTracks();

	//6 check occlusion
	
    return &m_tracks;
}


void SceneTracker::assignBlobs(std::list<TrackEntry>& blobs) {
	// assign blobs to existing tracks
	// delete orphaned tracks and free associated Track-ID
	//  for_each track
	typedef std::list<Track>::iterator TiterTracks;
	TiterTracks iTrack = m_tracks.begin();
	while (iTrack != m_tracks.end()) {
		iTrack->updateTrackIntersect(blobs, m_roiSize, 0.4, 4); // assign new blobs to existing track
		++iTrack;
	}
	
	// create new tracks for unassigned blobs
	m_trackIDs.sort(std::greater<int>());
	typedef std::list<TrackEntry>::iterator TiterBlobs;
	TiterBlobs iBlob = blobs.begin();
	while (iBlob != blobs.end()) {
		int trackID = nextTrackID();
		if (trackID > 0)
			// TODO check on trackID
			m_tracks.push_back(Track(*iBlob, trackID));
		++iBlob;
	}
	blobs.clear();
	return;
}

void combineTracks(std::list<Track>& tracks, cv::Size roi) {
	// combine tracks, if they have
	//   same direction and
	//   area intersection
	// assign smaller tracks to longer ones (size of track)
	typedef std::list<Track>::iterator TiterTracks;
	TiterTracks iTrack = tracks.begin();

	// for_each track
	while (iTrack != tracks.end()) {
		// assing compare track to next list element
		TiterTracks iTrackComp = iTrack;
		++iTrackComp;

		// for_each remaining tracks in list
		while (iTrackComp != tracks.end()) {

			// track history must have two elements or more in order to compare velocity
			if (iTrack->getHistory() >= 2 && iTrackComp->getHistory() >= 2) {

				bool isSameDirection	= ( signBit(iTrack->getVelocity().x) == signBit(iTrackComp->getVelocity().x) );
				cv::Rect rcIntersec		= ( iTrack->getActualEntry().rect() & iTrackComp->getActualEntry().rect() );
				bool isIntersection		= (rcIntersec.area() > 0);

				if (isSameDirection && isIntersection) {
					// iTrack longer -> assign iTrackComp and delete it afterwards
					if (iTrack->getHistory() > iTrackComp->getHistory()) {
						iTrack->addTrackEntry(iTrackComp->getActualEntry(), roi);
						iTrackComp->markForDeletion();

					// vice versa
					} else {
						iTrackComp->addTrackEntry(iTrack->getActualEntry(), roi);
						iTrack->markForDeletion();
					}
				}
			// at least one track has a too short history (fewer than 2 elements) -> do nothing
			} else {
			
			}

			++iTrackComp;
		} // end_for_each remaining tracks in list

		++iTrack;
	} // end_for_each track
	return;
}

/// for_each track set status variable m_leavingRoiTo to left or right,
///  indicating that the track has touched left or right border of roi
void SceneTracker::checkTracksLeavingRoi() {
	typedef std::list<Track>::iterator TiterTracks;
	TiterTracks iTrack = m_tracks.begin();

	// for_each track
	while (iTrack != m_tracks.end()) {

		iTrack->setLeavingRoiFlag(m_roiSize);
		++iTrack;
	} // end_for_each track

	return;
}

// for reversing tracks:
// delete reversing tracks and assign last entry to new track
void SceneTracker::deleteReversingTracks() {
	typedef std::list<Track>::iterator TiterTracks;
	TiterTracks iTrack = m_tracks.begin();

	// for_each track
	while (iTrack != m_tracks.end()) {
		// track reverses
		if(iTrack->isReversingX()) { //&& (iTrack->leavingRoiTo() != Track::Direction::none)) {
			int id = this->nextTrackID();

			// if trackID available: create new track from lastTrackEntry, delete reversing track
			if (id > 0) {
				m_tracks.push_back(Track(iTrack->getActualEntry(), id));
				returnTrackID(iTrack->getId());
				iTrack = m_tracks.erase(iTrack);
			
			// no trackID available
			} else {
				cerr << "no track id available" << endl;
			}

		} else {
			++iTrack;
		}
	} // end_for_each track

	return;
}


void SceneTracker::deleteMarkedTracks() {
	// delete orphaned tracks and free associated Track-ID
	typedef std::list<Track>::iterator TiterTracks;
	TiterTracks iTrack = m_tracks.begin();

	while (iTrack != m_tracks.end()) {
		if (iTrack->isMarkedForDelete()) {
			returnTrackID(iTrack->getId());
			iTrack = m_tracks.erase(iTrack);
		} else {
			++iTrack;
		}
	}
	return;
}


void checkOcclusion(std::list<Track>& tracks) {

	return;
}


// DEBUG
struct Trk {
	int id;
	int confidence;
	double velocity;
	bool counted;
	Trk(int id_, int confidence_, double velocity_, bool counted_) : id(id_), confidence(confidence_), velocity(velocity_), counted(counted_) {}
};


void SceneTracker::inspect(int frameCnt) {
	vector<Trk> tracks;
    (void(frameCnt)); // suppress "unused parameter" warning
	list<Track>::iterator iTrack = m_tracks.begin();
	while (iTrack != m_tracks.end()) {
		tracks.push_back(Trk(iTrack->getId(), iTrack->getConfidence(), iTrack->getVelocity().x, iTrack->isCounted()));
		++iTrack;
	}
}
// END DEBUG
