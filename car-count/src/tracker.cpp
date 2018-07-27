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
	mBbox.x = abs(x);
	mBbox.y = abs(y);
	mBbox.width = abs(width);
	mBbox.height = abs(height);
	mCentroid.x = mBbox.x + (mBbox.width / 2);
	mCentroid.y = mBbox.y + (mBbox.height / 2);
}

TrackEntry::TrackEntry(cv::Rect _bbox) : mBbox(_bbox) {
	mCentroid.x = mBbox.x + (mBbox.width / 2);
	mCentroid.y = mBbox.y + (mBbox.height / 2);
}

cv::Point2i TrackEntry::centroid() const {
	return mCentroid;
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
	return mBbox.height;
}

cv::Rect TrackEntry::rect() const {
	return mBbox;
}

int TrackEntry::width() const {
	return mBbox.width;
}


//////////////////////////////////////////////////////////////////////////////
// Track /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

Track::Track(const TrackEntry& blob, int id) : mAvgVelocity(0,0), mConfidence(0),
    mCounted(false), mId(id), mMarkedForDelete(false)  {
	mHistory.push_back(blob);
}

Track& Track::operator= (const Track& source) {
	mHistory = source.mHistory;
	return *this;
}

/// add TrackEntry to history and update velocity
void Track::addTrackEntry(const TrackEntry& blob) {
	mHistory.push_back(blob);
	updateAverageVelocity();
	return;
}

/// create substitute TrackEntry, if no close blob available
void Track::addSubstitute() {
	// take average velocity and compose bbox from centroid of previous element
	cv::Point2i velocity((int)round(mAvgVelocity.x), (int)round(mAvgVelocity.y));

	cv::Point2i centroid = mHistory.back().centroid();
	centroid += velocity;
	int height = mHistory.back().height();
	int width = mHistory.back().width();
	int x = centroid.x - width / 2;
	int y = centroid.y - height / 2;
	// TODO: use Track::avgHeight, avgWidth
	// TODO: check lower and upper bounds of video window

	addTrackEntry(TrackEntry(x, y, width, height));
	return;
}

TrackEntry& Track::getActualEntry() { return mHistory.back(); }

int Track::getConfidence() { return mConfidence; }

int Track::getId() { return mId; }

double Track::getLength() {
	double length = euclideanDist(mHistory.front().centroid(), mHistory.back().centroid());
	return length;
}

cv::Point2d Track::getVelocity() { return mAvgVelocity; }

bool Track::isCounted() { return mCounted; }

bool Track::isMarkedForDelete() {return mMarkedForDelete;}

void Track::setCounted(bool state) { mCounted = state; }

/// average velocity with recursive formula
cv::Point2d& Track::updateAverageVelocity() {
	const int window = 5;
	int lengthHistory = mHistory.size();

	if (lengthHistory > 1) {
		int idxMax = lengthHistory - 1;
		cv::Point2d actVelocity = mHistory[idxMax].centroid() - mHistory[idxMax-1].centroid();

		if (idxMax <= window) { // window not fully populated yet
			mAvgVelocity = (mAvgVelocity * (double)(idxMax - 1) + actVelocity) * (1.0 / (double)(idxMax));
		}
		else { // window fully populated, remove window-1 velocity value
			cv::Point2d oldVelocity = mHistory[idxMax-window].centroid() - mHistory[idxMax-window-1].centroid();
			mAvgVelocity += (actVelocity - oldVelocity) * (1.0 / (double)window);
		}
	}

	return mAvgVelocity;
}

/// iterate through detected blobs, check if size is similar and distance close
/// save closest blob to history and delete from blob input list 
void Track::updateTrack(std::list<TrackEntry>& blobs,  int maxConf, 
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
		addTrackEntry(*iBlobsMinDist);
		iBlobs = blobs.erase(iBlobsMinDist);
		mConfidence <  maxConf ? ++mConfidence : mConfidence =  maxConf;
	}
	else // no shape to assign availabe, calculate substitute
	{
		--mConfidence;
		if (mConfidence <= 0) 
		{
			mMarkedForDelete = true;
		}
		else
			addSubstitute();
	}

	return;
}


//////////////////////////////////////////////////////////////////////////////
// SceneTracker //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

SceneTracker::SceneTracker(Config* pConfig) : Observer(pConfig) {
	// max number of tracks assignable (9)
	mMaxNoIDs = stoi(mSubject->getParam("max_n_of_tracks"));
	for (int i = mMaxNoIDs; i > 0; --i) // fill trackIDs with 9 ints
		mTrackIDs.push_back(i);
	// update all other relevant parameters from Config
	update();
}

void SceneTracker::attachCountRecorder(CountRecorder* pRecorder) {
	mRecorder = pRecorder;
}

/// helper functor for SceneTracker::countVehicles
class CntAndClassify {
	int mFrameCnt;
	int mDbgCnt;
	ClassifyVehicle mClassify;
	CountResults mCr;
public:
	CntAndClassify(int frameCnt, ClassifyVehicle classify) : mFrameCnt(frameCnt), mClassify(classify) {mDbgCnt = 0;}

	void operator()(Track& track) {
		double trackLength = track.getLength();
		
		if (track.getConfidence() > mClassify.countConfidence) {
			cv::Point2d velocity(track.getVelocity());
			int width = track.getActualEntry().width();
			int height = track.getActualEntry().height();

			if (signBit(velocity.x)) { // moving to left
				if ((track.getActualEntry().centroid().x < mClassify.countPos) && !track.isCounted()) {
					if (trackLength > mClassify.trackLength) { // "count_track_length"
						track.setCounted(true);
						if ( (width >= mClassify.truckSize.width) && (height >= mClassify.truckSize.height) )
							++mCr.truckLeft;
						else // car
							++mCr.carLeft;
					}
				}
			}
			else {// moving to right
				if ((track.getActualEntry().centroid().x > mClassify.countPos) && !track.isCounted()) {
					if (trackLength > mClassify.trackLength) {
						track.setCounted(true);
						if ( (width >= mClassify.truckSize.width) && (height >= mClassify.truckSize.height) )
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
	
	CntAndClassify cac = for_each(mTracks.begin(), mTracks.end(), CntAndClassify(frameCnt, mClassify));
	CountResults cr = cac.results();
	return cr;
}

/// provide track-ID, if available
int SceneTracker::nextTrackID()
{
	if (mTrackIDs.empty()) return 0;
	else {
		int id = mTrackIDs.back();
		mTrackIDs.pop_back();
		return id;
	}
}

/// return unused track-ID
bool SceneTracker::returnTrackID(int id)
{
	if (id > 0 ) {
		if (mTrackIDs.size() < mMaxNoIDs) {
			mTrackIDs.push_back(id);
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
	// max confidence for tracks allowed
	mMaxConfidence = stoi(mSubject->getParam("track_max_confidence"));

	// assign to track: max deviation of track entries in percent
	mMaxDeviation = stod(mSubject->getParam("track_max_deviation"));

	// assign to track: max distance between track entries in pixels
	mMaxDist = stod(mSubject->getParam("track_max_distance"));

	// min confidence for counting vehicle
	mClassify.countConfidence = stoi(mSubject->getParam("count_confidence"));

	// horizontal counting position in pixels
	mClassify.countPos = stoi(mSubject->getParam("count_pos_x")); 
	
	// min track length for counting vehicles
	mClassify.trackLength = stoi(mSubject->getParam("count_track_length"));

	// min truck size for classification
	mClassify.truckSize = cv::Size2i(stoi(mSubject->getParam("truck_width_min")),
		stoi(mSubject->getParam("truck_height_min")));
}

/// assign blobs to existing tracks
///  create new tracks from unassigned blobs
///  erase, if marked for deletion
std::list<Track>* SceneTracker::updateTracks(list<TrackEntry>& blobs) {

	// assign blobs to existing tracks
	// delete orphaned tracks and free associated Track-ID
	//  for_each track
	std::list<Track>::iterator iTrack = mTracks.begin();
	while (iTrack != mTracks.end())
	{

		iTrack->updateTrack(blobs, mMaxConfidence, mMaxDeviation, mMaxDist); // assign new blobs to existing track

		if (iTrack->isMarkedForDelete())
		{
			returnTrackID(iTrack->getId());
			iTrack = mTracks.erase(iTrack);
		}
		else
			++iTrack;
	}
	
	// create new tracks for unassigned blobs
	mTrackIDs.sort(std::greater<int>());
	std::list<TrackEntry>::iterator iBlob = blobs.begin();
	while (iBlob != blobs.end())
	{
		int trackID = nextTrackID();
		if (trackID > 0)
			// TODO check on trackID
			mTracks.push_back(Track(*iBlob, trackID));
		++iBlob;
	}
	blobs.clear();
	
    return &mTracks;
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
    (void(frameCnt));
	list<Track>::iterator iTrack = mTracks.begin();
	while (iTrack != mTracks.end()) {
		tracks.push_back(Trk(iTrack->getId(), iTrack->getConfidence(), iTrack->getVelocity().x, iTrack->isCounted()));
		++iTrack;
	}
}
// END DEBUG
