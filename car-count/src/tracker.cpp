#include "stdafx.h"
#include <cassert>

#include "../include/config.h"
#include "../include/recorder.h"
#include "../include/frame_handler.h"
#include "../include/tracker.h"
#include "../../utilities/inc/util-visual-trace.h"

using namespace std;

// status variables for visual tracking
extern TrackStateVec g_trackState;
extern size_t g_idx;

double euclideanDist(const cv::Point& pt1, const cv::Point& pt2) {
	cv::Point diff = pt1 - pt2;
    return sqrt(static_cast<double>( (diff.x * diff.x + diff.y * diff.y) ));
}

double round(double number) { // not necessary in C++11
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

//////////////////////////////////////////////////////////////////////////////
// Occlusion
//////////////////////////////////////////////////////////////////////////////

// Local functions 
// ---------------------------------------------------------------------------
cv::Rect extendRectToLeftBorder(cv::Size roi, const cv::Rect& rect) {
	cv::Rect rcExtended(rect);
    (void)roi;
	rcExtended.x = 0;
	rcExtended.width = rect.width + rect.x;
	return rcExtended;
}


cv::Rect extendRectToRightBorder(cv::Size roi, const cv::Rect& rect) {
	cv::Rect rcExtended(rect);
	rcExtended.width = roi.width - rect.x;
	return rcExtended;
}


bool isRectAtRightRoiBorder(cv::Size roi, const cv::Rect& rect) {
	const int borderTolerance = roi.width * 3 / 100;
	if (rect.x + rect.width > roi.width - borderTolerance)
		return true;
	else
		return false;
}


bool isRectAtLeftRoiBorder(cv::Size roi, const cv::Rect& rect) {
	const int borderTolerance = roi.width * 3 / 100;
	if (rect.x < borderTolerance)
		return true;
	else
		return false;
}


bool movingRightAtLeftBorder(cv::Size roi, const Track* trackRight) {
	const int borderTolerance = roi.width * 3 / 100;
	if (trackRight->getActualEntry().rect().x < borderTolerance)
		return true;
	else 
		return false;
}

bool movingLeftAtRightBorder(cv::Size roi, const Track* trackLeft) {
	const int borderTolerance = roi.width * 3 / 100;
	if (trackLeft->getActualEntry().rect().x + trackLeft->getActualEntry().rect().width
		> roi.width - borderTolerance)
		return true;
	else 
		return false;
}
// ---------------------------------------------------------------------------

Occlusion::Occlusion(cv::Size roi, Track* movingLeft, Track* movingRight, int steps, size_t id) :
	m_hasPassed(false),
	m_id(id),
	m_isMarkedForDelete(false),
	m_movingLeft(movingLeft),	
	m_movingRight(movingRight),
	m_remainingUpdateSteps(steps),
	m_roiSize(roi)	
	{
		// DEBUG std::cout << "c'tor id: " << m_id << endl;
		m_rect = updateRect();
}


void Occlusion::assignBlobs(std::list<cv::Rect>& blobs) {

	assert(m_isMarkedForDelete == false);
	assert(m_movingRight != nullptr);
	assert(m_movingLeft != nullptr);
	
	// find all blobs in occlusion area and remove from blobs list
	std::list<cv::Rect> blobsInOcclusion;
	std::list<cv::Rect>::iterator iBlob = blobs.begin();
	while (iBlob != blobs.end()) {
		cv::Rect intersect = m_rect & *iBlob;
		// blob predominantly within occlusion area -> remove
		if (intersect.area() > static_cast<int>(0.7 * iBlob->area()) ) {
			blobsInOcclusion.push_back(*iBlob);
			iBlob = blobs.erase(iBlob);
		} else {
			++iBlob;
		}
	}


	switch (blobsInOcclusion.size()) {
		// zero or one blob -> use substitute blobs for track update
		case 0: 
			// update tracks with no blob
			// -> for both tracks: calc substitute and decrease confidence
			m_movingRight->updateTrackIntersect(blobsInOcclusion, m_roiSize, 0.4, 4); 
			m_movingLeft->updateTrackIntersect(blobsInOcclusion, m_roiSize, 0.4, 4); 
			if (m_hasPassed)
				m_isMarkedForDelete = true;
			break;

		case 1: { // bracket necessary for declaring variables
			// last assignment step 
			if (m_hasPassed) {
				
				// allow for small tolerance at roi border in order to deal with noisy blob
				int borderTolerance = m_roiSize.width * 3 / 100;
				
				// occlusion touches left border -> assign blob to right moving track
				if (m_rect.x <= borderTolerance) {
					m_movingRight->updateTrackPassedOcclusion(blobsInOcclusion, m_roiSize, 4);			
				}

				// occlusion touches right border -> assign blob to left moving track
				if (m_rect.x + m_rect.width >= m_roiSize.width - borderTolerance) {
					m_movingLeft->updateTrackPassedOcclusion(blobsInOcclusion, m_roiSize, 4);			
				}

				m_isMarkedForDelete = true;

			// all other assignments
			} else {
				// calc substitutes for both occluded tracks
				cv::Rect rcRight = calcSubstitute(*m_movingRight);
				cv::Rect rcLeft = calcSubstitute(*m_movingLeft);

				// extend substitute to roi border, if track still enters scene
				//   and opposite track has not reached roi border yet
				if ( isRectAtRightRoiBorder(m_roiSize, blobsInOcclusion.front()) 
					&& !isRectAtRightRoiBorder(m_roiSize, rcRight) )
					rcLeft = extendRectToRightBorder(m_roiSize, rcLeft);
				if ( isRectAtLeftRoiBorder(m_roiSize, blobsInOcclusion.front())
					&& !isRectAtLeftRoiBorder(m_roiSize, rcLeft) )
					rcRight = extendRectToLeftBorder(m_roiSize, rcRight);

				// add rcRight / Left (original or adjusted)
				m_movingRight->addTrackEntry(rcRight, m_roiSize);
				m_movingLeft->addTrackEntry(rcLeft, m_roiSize);
			}
			break;	

		}
		
		default:
		// two or more blobs -> regular track update
		m_movingRight->updateTrackIntersect(blobsInOcclusion, m_roiSize, 0.4, 4); 
		m_movingLeft->updateTrackIntersect(blobsInOcclusion, m_roiSize, 0.4, 4);
		if (m_hasPassed)
			m_isMarkedForDelete = true;
	}

	// adjust occlusion rect after updates
	this->updateRect();

	// occlusion has been passed -> mark occlusion as hasPassed (for deletion in OcclusionIdList::assignBlobs)
	int mvRight_edgeLeft = m_movingRight->getActualEntry().rect().x;
	int mvLeft_edgeRight = m_movingLeft->getActualEntry().rect().x
		+ m_movingLeft->getActualEntry().rect().width;
	if (mvRight_edgeLeft >= mvLeft_edgeRight)
		m_hasPassed = true;

	return;
}


bool Occlusion::hasPassed() const { return m_hasPassed; }


size_t Occlusion::id() const { return m_id; }


bool Occlusion::isMarkedForDelete() const { return m_isMarkedForDelete; }


Track* Occlusion::movingLeft() const { return m_movingLeft; }


Track* Occlusion::movingRight() const { return m_movingRight; }


cv::Rect Occlusion::rect() const { return m_rect; }


int Occlusion::remainingUpdateSteps() { return m_remainingUpdateSteps; }


void Occlusion::setId(size_t id) { m_id = id; }


cv::Rect Occlusion::updateRect() {
// adjustment based on presumed next update step
	cv::Rect nextRight = clipAtRoi(calcSubstitute(*m_movingRight), m_roiSize);
	cv::Rect nextLeft = clipAtRoi(calcSubstitute(*m_movingLeft), m_roiSize);

// if track enters roi from border -> extend occlusion rect to border
	// track movingRight at left roi border
	if (movingRightAtLeftBorder(m_roiSize, m_movingRight))
		nextRight = extendRectToLeftBorder(m_roiSize, nextRight);
	if (movingLeftAtRightBorder(m_roiSize, m_movingLeft))
		nextLeft = extendRectToRightBorder(m_roiSize, nextLeft);

	m_rect = nextRight | nextLeft;
	return m_rect;
}

//////////////////////////////////////////////////////////////////////////////
// OcclusionIdList
//////////////////////////////////////////////////////////////////////////////
OcclusionIdList::OcclusionIdList(size_t maxIdCount) : m_occlusionIds(maxIdCount) {}

bool OcclusionIdList::add(Occlusion& occlusion) {
	size_t id = m_occlusionIds.allocID();
	if (id) {
		occlusion.setId(id);
		m_occlusions.push_back(occlusion);
		return true;
	} else { 
		std::cerr << "cannot add occlusion, no ID available" << std::endl;
		return false;
	}
}

void OcclusionIdList::assignBlobs(std::list<cv::Rect>& blobs) {
	// pass blobs by reference in order to make sure they are deleted (passing by value deletes copy only)
	//for_each m_occlusions
	std::list<Occlusion>::iterator iOcc = m_occlusions.begin();
	while (iOcc != m_occlusions.end()) {

		// occlusion has been passed or at least one track has been deleted
		// -> delete occlusion, return ID, unset track occlusion status
		if (iOcc->isMarkedForDelete()) {
			iOcc->movingLeft()->setOccluded(false);
			iOcc->movingRight()->setOccluded(false);
			iOcc = remove(iOcc);
			//m_occlusionIds.freeID(iOcc->id());
			//iOcc = m_occlusions.erase(iOcc);
			
		// inside occlusion -> update occluded tracks
			//
		} else {
			// update occluded tracks
			iOcc->assignBlobs(blobs);
			++iOcc;
		}
	} // end_for_each m_occlusions
	return;
}
		
	// TODO_END move to OcclusionIdList::assignBlobs()

std::list<Occlusion>* OcclusionIdList::getList() {
	return &m_occlusions;
}

bool OcclusionIdList::isOcclusion() {
	if (m_occlusions.size() > 0)
		return true;
	else
		return false;
}

OcclusionIdList::IterOcclusion OcclusionIdList::remove(IterOcclusion iOcclusion) {
	m_occlusionIds.freeID(iOcclusion->id());
	return m_occlusions.erase(iOcclusion);
}




//////////////////////////////////////////////////////////////////////////////
// TrackEntry
//////////////////////////////////////////////////////////////////////////////

TrackEntry::TrackEntry(cv::Rect rect, cv::Point2d velocity) : m_bbox(rect), m_velocity(velocity) {
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
bool TrackEntry::isSizeSimilar(const TrackEntry& teCompare, const double maxDeviation) const {
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

cv::Point2i TrackEntry::velocity() const {
	return m_velocity;
}

int TrackEntry::width() const {
	return m_bbox.width;
}


//////////////////////////////////////////////////////////////////////////////
// Track /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Local functions 
// ---------------------------------------------------------------------------
/// velocity difference between new blob and last track entry 
cv::Point diffVelocity(cv::Rect& blob, Track* const track, const cv::Size roi) {
	// inside border tolerance limits blob edges are considered at or outside roi
	// in order to deal with blob noise at the roi border
	int borderTolerance = roi.width * 5 / 100;

	cv::Point velocity(0,0);
	// at least two history elements needed to calculate velocity
	if (track->getHistory() < 1) {
		velocity = cv::Point(0,0);
	// two elements available
	} else { 
		// default velocity: difference of centroids
		cv::Point centroid(blob.x + (blob.width / 2), blob.y + (blob.height / 2));
		velocity = centroid - track->getActualEntry().centroid();
	
		// blob covers entire roi width -> return average velocity
		int leftEdge = blob.x;
		int rightEdge = leftEdge + blob.width;
		if ( (leftEdge < borderTolerance) && (rightEdge > (roi.width - borderTolerance)) ) {
			return track->getVelocity();
		}

		// left edge outside roi -> velocity based on right edge
		int leftEdgePrevious = track->getActualEntry().rect().x;
		int rightEdgePrevious = leftEdgePrevious + track->getActualEntry().width();
		if (leftEdge < borderTolerance) {
			velocity.x = rightEdge - rightEdgePrevious; 
			return velocity;
		// left edge inside roi
		} else {
			// previous left edge outside roi -> velocity based on right edge
			if (leftEdgePrevious < borderTolerance) {
				velocity.x = rightEdge - rightEdgePrevious; 
			// current and previous inside roi -> velocity based on centroid difference 
			} else { 
				velocity = centroid - track->getActualEntry().centroid();
			}
		} // end_if left edge outside roi

		// right edge outside roi -> velocity based on left edge
		if (rightEdge > (roi.width - borderTolerance)) {
			velocity.x = leftEdge - leftEdgePrevious; 
			return velocity;
		// right edge inside roi
		} else {
			// previous right edge outside roi -> velocity based on left edge
			if (rightEdgePrevious > (roi.width - borderTolerance)) {
				velocity.x = leftEdge - leftEdgePrevious; 
			// current and previous inside roi -> velocity based on centroid difference 
			} else { 
				velocity = centroid - track->getActualEntry().centroid();
			}
		} // end_if right edge outside roi
	} // end_if two elements available
	return velocity;
}

/// true if blob update does not reverse track direction 
bool isBlobNotReversingTrack(const Track& track, const cv::Rect& blob, cv::Point2d velocity) {
    (void)velocity;
	int blobCentroidX = blob.x + (blob.width / 2);
	int trackCentroidX = track.getActualEntry().centroid().x;
	int velocityX = static_cast<int>(ceil(track.getVelocity().x));
	
	// track new (velocity zero)
	if (velocityX == 0)
		return true;

	// track moves to right
	if (velocityX > 0) {
		if (trackCentroidX - blobCentroidX <= velocityX)
			return true;
		else
			return false;
	// track moves to left
	} else {
		if (trackCentroidX - blobCentroidX >= velocityX)
			return true;
		else
			return false;
	}
}

/// true if entire rect is located outside of roi
bool outsideRoi(const cv::Rect& rect, const cv::Size roi) {
	if ( rect.x + rect.width <= 0 || rect.x > roi.width )
		return true;
	if ( rect.y + rect.height <= 0 || rect.y > roi.height )
		return true;
	return false;
}
// ---------------------------------------------------------------------------

Track::Track(size_t id) : m_avgVelocity(0,0), m_confidence(0),
	m_counted(false), m_id(id), m_isMarkedForDelete(false), m_isOccluded(false),
	m_leavingRoiTo(Direction::none), m_prevAvgVelocity(0,0)  {
}


/// add TrackEntry to history and update velocity
bool Track::addTrackEntry(const cv::Rect& blobIn, const cv::Size roi) {
	
	// don't add track entry, if blob is outside roi entirely
    if (outsideRoi(blobIn, roi)) {
		std::cerr << "addTrackEntry: blob outside roi" << std::endl;
		return false;
	}

	// clip x, if blob exceeds roi
    cv::Rect blob = clipAtRoi(blobIn, roi);

	cv::Point velocity = diffVelocity(blob, this, roi);
	m_history.push_back(TrackEntry(blob, velocity));

	m_prevAvgVelocity = m_avgVelocity;
	updateAverageVelocity(roi);

	setLeavingRoiFlag(roi);

	return true;
}

/// create substitute TrackEntry, if no close blob available
bool Track::addSubstitute(cv::Size roi) {
	// TODO adjust for leaving and entering roi
	// take average velocity and compose bbox from previous element
    cv::Point2i velocity( static_cast<int>(round(m_avgVelocity.x)), static_cast<int>(round(m_avgVelocity.y)) );
	cv::Rect substitute = getActualEntry().rect() + velocity;

	// TODO: use Track::avgHeight, avgWidth

	// roi check and clipping done in addTrackEntry
	if (addTrackEntry(substitute, roi))
		return true;
	else
		return false;
}


const TrackEntry& Track::getActualEntry() const { return m_history.back(); }

int Track::getConfidence() const { return m_confidence; }

size_t Track::getHistory() const { return m_history.size(); }

size_t Track::getId() const { return m_id; }

double Track::getLength() const {
	double length = euclideanDist(m_history.front().centroid(), m_history.back().centroid());
	return length;
}

const TrackEntry& Track::getPreviousEntry() const { 
    int idxPrevious = static_cast<int>(m_history.size()) - 2;
	if (idxPrevious < 0) {
		std::cerr << "Track::getPreviousEntry(): track has no previous entry, taking actual one" << std::endl;
		idxPrevious = 0;
	}
    return m_history.at(static_cast<size_t>(idxPrevious));
}

cv::Point2d Track::getVelocity() const { return m_avgVelocity; }

bool Track::isCounted() { return m_counted; }

bool Track::isMarkedForDelete() const {return m_isMarkedForDelete;}

bool Track::isOccluded() {return m_isOccluded;}

bool Track::isReversingX(const double backlash) {

	// track history must have at least two elements in order to
	// compare velocity and determine direction
	if (this->getHistory() >= 3) {
		
		// previous direction different than current
		if (signBit(m_prevAvgVelocity.x) != signBit(m_avgVelocity.x)) {
			// at least one velocity must be outside backlash
			if ( abs(m_prevAvgVelocity.x) > backlash || abs(m_avgVelocity.x) > backlash ) {
				return true;
			} else {
				return false;
			}
		// same direction
		} else {
			return false;
		}

	// track history too short
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

Track::Direction Track::setLeavingRoiFlag(cv::Size roi) {

	// reliable velocity necessary, so only longer tracks are considered 
	// once leavingRoiTo has been assigned, no re-calculation necessary
	if ( (this->getHistory() >= 4) && (m_leavingRoiTo == Direction::none) ) {

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

	return m_leavingRoiTo;
}


void Track::setOccluded(bool state) { m_isOccluded = state; }

/// average velocity with recursive formula
cv::Point2d& Track::updateAverageVelocity(cv::Size roi) {
	const int window = 3;
    int lengthHistory = static_cast<int>(m_history.size());
    (void)roi;

	// need at least two track entries in order to calculate velocity
	if (lengthHistory > 1) {
		int idxMax = lengthHistory - 1;
        cv::Point2d actVelocity = m_history[static_cast<size_t>(idxMax)].velocity();
	
		// moving average formula
		if (idxMax <= window) { // window not fully populated yet
            m_avgVelocity = (m_avgVelocity * static_cast<double>(idxMax - 1) + actVelocity) * (1.0 / static_cast<double>(idxMax));
		}
		else { // window fully populated, remove window-1 velocity value
            cv::Point2d oldVelocity = m_history[static_cast<size_t>(idxMax-window)].velocity();
            m_avgVelocity += (actVelocity - oldVelocity) * (1.0 / static_cast<double>(window));
		}
	}
	
	return m_avgVelocity;
}


/// iterate through detected blobs, check if intersection
/// assign all intersecting blobs to track
/// store embracing rect of all assigned blobs to track
void Track::updateTrackIntersect(std::list<cv::Rect>& blobs, cv::Size roi, double minInter, int maxConf) {
	bool isTrackUpdateAvailable = false;
	bool isTrackMovingToRight = !(signBit(this->getVelocity().x));

	typedef std::list<cv::Rect>::iterator TiterBlobs;
	TiterBlobs iBlobToAssign;
	TiterBlobs iBlobs = blobs.begin();
	while (iBlobs != blobs.end()) {

		// assign based on area intersection between lastTrackEntry and newBlob
		// larger than defined minimum intersection (in percentage of newBlob area)
		cv::Rect rcIntersect = *iBlobs & this->getActualEntry().rect();
		if ( rcIntersect.area() > (static_cast<int>(minInter * iBlobs->area())) ) {

			// track moves to right OR leaves roi to right --> assign rightmost blob
			if ( (isTrackMovingToRight) || (m_leavingRoiTo == Direction::right) ) {

				// avoid track reversion
				// TODO delete if (isBlobNotReversingTrack(*this, *iBlobs, this->getVelocity())) {

					// no blobToAssign yet -> take first matching one
					if (!isTrackUpdateAvailable) {
						iBlobToAssign = iBlobs;
						isTrackUpdateAvailable = true;

					// there is one blobToAssign already, check if there is a better one (rightmost)
					} else {
						int blobCentroidX = iBlobs->x + (iBlobs->width / 2);
						int blobToAssignCentroidX = iBlobToAssign->x + (iBlobToAssign->width / 2);
						if (blobCentroidX > blobToAssignCentroidX)
							iBlobToAssign = iBlobs;
					}
				// TODO delete}
			}

			// track moves to left OR leaves roi to left --> assign leftmost blob
			if ( (!isTrackMovingToRight) || (m_leavingRoiTo == Direction::left) ) { 

				// avoid track reversion
				// TODO delete if (isBlobNotReversingTrack(*this, *iBlobs, this->getVelocity())) {

					// no blobToAssign yet -> take first matching one
					if (!isTrackUpdateAvailable) {
						iBlobToAssign = iBlobs;
						isTrackUpdateAvailable = true;

					// there is one blobToAssign already, check if there is a better one (leftmost)
					} else {
						int blobCentroidX = iBlobs->x + (iBlobs->width / 2);
						int blobToAssignCentroidX = iBlobToAssign->x + (iBlobToAssign->width / 2);
						if (blobCentroidX < blobToAssignCentroidX)
							iBlobToAssign = iBlobs;
					}
				// TODO delete}
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
		// TODO move assignment of substitute after assignment of other tracks (SceneTracker::updateTracks)
		// reason: if other tracks are assigned, no substitute value needs to be added
		
			// if blob outside roi -> mark track for deletion
			if (!addSubstitute(roi))
				m_isMarkedForDelete = true;
	}
}


/// update track that has just passed occlusion area
/// compare intersection with track instead of blob 
bool Track::updateTrackPassedOcclusion(std::list<cv::Rect>& blobs, cv::Size roi, int maxConf) {
	// assignment based on area intersection between lastTrackEntry and newBlob
	// larger than defined minimum intersection (in percentage of lastTrackEntry area)
	std::list<cv::Rect>::iterator iBlobs = blobs.begin();
	cv::Rect rcIntersect = *iBlobs & this->getActualEntry().rect();
	const double minInter = 0.5;
	int minTrackIntersectArea = static_cast<int>(minInter * this->getActualEntry().rect().area());
		
	// blob fits -> assign blob, increase track confidence
	if ( rcIntersect.area() > minTrackIntersectArea ) {
		this->addTrackEntry(*iBlobs, roi);
		iBlobs = blobs.erase(iBlobs);
		m_confidence <  maxConf ? ++m_confidence : m_confidence =  maxConf;
		return true;

	// no fitting blob available --> add substitute blob and decrease confidence
	} else {
		--m_confidence;
		if (m_confidence <= 0) 
		{
			m_isMarkedForDelete = true;
		} else {
			// if blob outside roi -> mark track for deletion
			if (!addSubstitute(roi))
				m_isMarkedForDelete = true;
		}
		return false;
	}
}


//////////////////////////////////////////////////////////////////////////////
// SceneTracker //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

SceneTracker::SceneTracker(Config* pConfig) : 
	Observer(pConfig),
	m_occlusions(6) {
	// max number of tracks assignable (9)
    m_maxNoIDs = stoul(mSubject->getParam("max_n_of_tracks"));
    for (size_t n = m_maxNoIDs; n > 0; --n) { // fill trackIDs with 9 ints
        m_trackIDs.push_back(n);
    }
	// update all other relevant parameters from Config
	update();
}


std::list<Track>* SceneTracker::assignBlobs(std::list<cv::Rect>& blobs) {
	// assign blobs to existing tracks
	// delete orphaned tracks and free associated Track-ID
	//  for_each track
	typedef std::list<Track>::iterator TiterTracks;
	TiterTracks iTrack = m_tracks.begin();
	while (iTrack != m_tracks.end()) {
		// update only if track is not occluded
		if (!iTrack->isOccluded())
			iTrack->updateTrackIntersect(blobs, m_roiSize, 0.4, 4); // assign new blobs to existing track
		++iTrack;
	}
	
	// create new tracks for unassigned blobs
	m_trackIDs.sort(std::greater<int>());
	typedef std::list<cv::Rect>::iterator TiterBlobs;
	TiterBlobs iBlob = blobs.begin();
	while (iBlob != blobs.end()) {
        size_t trackID = nextTrackID();
		if (trackID > 0) {
			Track newTrack(trackID);
			// blob must not be added, if outside roi
			// otherwise getActualEntry() will fail later
			if (newTrack.addTrackEntry(*iBlob, m_roiSize))
				m_tracks.push_back(newTrack);
			else
				returnTrackID(trackID);
		}
		++iBlob;
	}
	blobs.clear();
	return &m_tracks;
}



void SceneTracker::attachCountRecorder(CountRecorder* pRecorder) {
	m_recorder = pRecorder;
}


/// helper functor for SceneTracker::countVehicles
///  counting conditions for vehicle track:
///  - above confidence level
///  - beyond counting position
///  - completed track length
///  classifying conditions for trucks:
///  - average width and height
class CntAndClassify {
    int m_frameCount;
	int m_debugCount;
	ClassifyVehicle m_classify;
	CountResults m_vehicleCount;
public:
    CntAndClassify(int frameCnt, ClassifyVehicle classify) : m_frameCount(frameCnt), m_classify(classify) {m_debugCount = 0;}

	void operator()(Track& track) {
        (void)m_frameCount;
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
							++m_vehicleCount.truckLeft;
						else // car
							++m_vehicleCount.carLeft;
					}
				}
			}
			else {// moving to right
				if ((track.getActualEntry().centroid().x > m_classify.countPos) && !track.isCounted()) {
					if (trackLength > m_classify.trackLength) {
						track.setCounted(true);
						if ( (width >= m_classify.truckSize.width) && (height >= m_classify.truckSize.height) )
							++m_vehicleCount.truckRight;
						else // car
							++m_vehicleCount.carRight;
					}
				}
			}
		} // end_if track.getConfidence > countConfidence
	} // end operator()

	CountResults results() { 
		return m_vehicleCount; 
	}
};


class Classify {
	int m_frameCount;
	int m_debugCount;
	ClassifyVehicle m_classify;
	CountResults m_vehicleCount;
public:
	Classify(int frameCnt, ClassifyVehicle classify) : m_frameCount(frameCnt), m_classify(classify) {m_debugCount = 0;}

	void operator()(Track& track) {
        (void)m_frameCount;
		double trackLength = track.getLength();

		// track not counted yet and required track length reached
		if (!track.isCounted() && trackLength > m_classify.trackLength) {
			cv::Point2d velocity(track.getVelocity());
			int width = track.getActualEntry().width();
			int height = track.getActualEntry().height();

			track.setCounted(true);
			
			// moving to left
			if (signBit(velocity.x)) { 
				// truck
				if ( (width >= m_classify.truckSize.width) && (height >= m_classify.truckSize.height) )
					++m_vehicleCount.truckLeft;
				// car
				else 
					++m_vehicleCount.carLeft;
			}

			// moving to right
			else {
				// truck
				if ( (width >= m_classify.truckSize.width) && (height >= m_classify.truckSize.height) )
					++m_vehicleCount.truckRight;
				// car
				else 
					++m_vehicleCount.carRight;
			}

		} // end_if track not counted yet
	} // end operator()

	CountResults results() { 
		return m_vehicleCount; 
	}
};



CountResults SceneTracker::countVehicles(int frameCnt) {
	// track confidence > x
	// moving left && position < yy
	// moving right && position > yy
	
	CntAndClassify cac = for_each(m_tracks.begin(), m_tracks.end(), CntAndClassify(frameCnt, m_classify));
	CountResults cr = cac.results();
	return cr;
}


const std::list<Occlusion>* SceneTracker::deleteOcclusionsWithMarkedTracks() {
// if one of the tracks is marked for deletion -> remove occlusion from occlusion list
	OcclusionIdList::IterOcclusion iOcc = m_occlusions.getList()->begin();
	while (iOcc != m_occlusions.getList()->end()) {
		if ( iOcc->movingLeft()->isMarkedForDelete() || iOcc->movingRight()->isMarkedForDelete() ) {
			iOcc->movingLeft()->setOccluded(false);
			iOcc->movingRight()->setOccluded(false);
			iOcc = m_occlusions.remove(iOcc);
		} else {
			++iOcc;
		}
	}

	return m_occlusions.getList();
}


std::list<Track>* SceneTracker::deleteMarkedTracks() {
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
	return &m_tracks;
}


std::list<Track>* SceneTracker::deleteReversingTracks() {
	// velocity difference must be significant in order to 
	// avoid re-assigning tracks of stand still motion, e.g. waving leaves
	const double backlash = 0.5;

	typedef std::list<Track>::iterator TiterTracks;
	TiterTracks iTrack = m_tracks.begin();

	// for_each track
	while (iTrack != m_tracks.end()) {
		// track reverses (only for tracks outside occlusion)
		if( iTrack->isReversingX(backlash) && !iTrack->isOccluded() ) { 
            size_t id = this->nextTrackID();

			// if trackID available: create new track from lastTrackEntry, delete reversing track
			if (id > 0) {
				m_tracks.push_back(Track(id));
                m_tracks.back().addTrackEntry(iTrack->getActualEntry().rect(), m_roiSize);
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

	return &m_tracks;
}


const std::list<Occlusion>* SceneTracker::occlusionList() {
	return m_occlusions.getList();
}


// DEBUG
struct Trk {
    size_t id;
	int confidence;
	double velocity;
	bool counted;
    Trk(size_t id_, int confidence_, double velocity_, bool counted_) : id(id_), confidence(confidence_), velocity(velocity_), counted(counted_) {}
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


bool SceneTracker::isOverlappingTracks() {
	return m_occlusions.isOcclusion();
}


size_t SceneTracker::nextTrackID()
{
	if (m_trackIDs.empty()) return 0;
	else {
        size_t id = m_trackIDs.back();
		m_trackIDs.pop_back();
		return id;
	}
}


bool SceneTracker::returnTrackID(size_t id)
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


const std::list<Occlusion>*  SceneTracker::setOcclusion() {
	typedef std::list<Track>::iterator IterTrackConst;
	IterTrackConst iTrack = m_tracks.begin();

	// for_each track
	while (iTrack != m_tracks.end()) {
		// check only for tracks, that are not occluded
		if (!iTrack->isOccluded()) {
			// assing compare track to next list element
			IterTrackConst iTrackComp = iTrack;
			++iTrackComp;

			// for_each remaining tracks in list
			while (iTrackComp != m_tracks.end()) {
				if (!iTrackComp->isOccluded()) {
					if (isDirectionOpposite(*iTrack, *iTrackComp, 0.5)) {
				
						// determine left and right moving track
						IterTrackConst movesRight = iTrack;
						IterTrackConst movesLeft = iTrackComp;
						if (signBit(iTrack->getVelocity().x)) {
							movesLeft = iTrack;
							movesRight = iTrackComp;
						} else {
							movesLeft = iTrackComp;
							movesRight = iTrack;
						}

						if (isNextUpdateOccluded(*movesLeft, *movesRight)) {
							int remainingUpdateSteps = remainingOccludedUpdateSteps(*movesLeft, *movesRight);
							//Occlusion occ(&m_occlusionIDs, m_roiSize, &movesLeft, &movesRight, remainingUpdateSteps);
							Occlusion occ(m_roiSize, &(*movesLeft), &(*movesRight), remainingUpdateSteps);

				
							// create occlusion list entry (if ID available)
							if (m_occlusions.add(occ)) {
								// mark tracks as occluded
								movesLeft->setOccluded(true);
								movesRight->setOccluded(true);
							}	
						}
					} // end_if isDirectionOpposite
				} // end_if track opposite is not occluded
				++iTrackComp;
			} // end_for_each remaining tracks in list
		} // end_if track is not occluded

		++iTrack;
	} // end_for_each track

	return m_occlusions.getList();
}


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


void printIsOccluded(Track& track) {
	std::cout << "id#" << track.getId() << " is occluded: " << track.isOccluded() << endl;
	return;
}

std::list<Track>* SceneTracker::updateTracks(std::list<cv::Rect>& blobs, long long frameCnt) {
    (void)frameCnt;
	// DEBUG
	using namespace std;
	std::list<TrackState> traceTrackState;
	traceTrackState.push_back(TrackState("before blob assign", blobs, m_occlusions.getList(), m_tracks));
	// END_DEBUG

	//1 assign blobs based on occlusion
	// occluded tracks -> special track update
	if (isOverlappingTracks()) {
		// first assign blobs in occlusion areas, then remaining blobs
		m_occlusions.assignBlobs(blobs);
		assignBlobs(blobs);

	// no occluded tracks -> normal track update
	} else {
		assignBlobs(blobs);
	}

	// DEBUG
	traceTrackState.push_back(TrackState("after blob assign", blobs, m_occlusions.getList(), m_tracks));
	/*cout << "after occlusion assign - blobs: " << blobs.size() << endl;
	for_each(blobs.begin(), blobs.end(), printRect);
	cout << "after occlusion assign - tracks: " << m_tracks.size() << endl;
	for_each(m_tracks.begin(), m_tracks.end(), printTrackRect);
	*/
	// END_DEBUG

	//2 delete occlusions, then tracks marked for deletion
	this->deleteOcclusionsWithMarkedTracks();
	this->deleteMarkedTracks();

	//3 check reversing direction
	// delete relevant tracks (blobs will be assigned to new track in next update step)
	this->deleteReversingTracks();

	//4 combine tracks
	combineTracks(m_tracks, m_roiSize);

	//5 check occlusion
	const std::list<Occlusion>* pOcclusions;
	pOcclusions = setOcclusion();
    (void)pOcclusions;
	// DEBUG
    /*
	cout << "idx#" << frameCnt << endl;
	if (pOcclusions->size() >=1) {
		cout << "right occluded: " << pOcclusions->front().movingRight()->isOccluded() << endl;
		cout << "left occluded: " << pOcclusions->front().movingLeft()->isOccluded() << endl;
	}
	for_each(m_tracks.begin(), m_tracks.end(), printIsOccluded);
	*/
	// DEBUG
	traceTrackState.push_back(TrackState("after deletion", blobs, m_occlusions.getList(), m_tracks));
	// END_DEBUG

	// DEBUG
	g_trackState.push_back(traceTrackState);	
    g_trackStateMap.insert(std::pair<long long, std::list<TrackState>>(frameCnt, traceTrackState));
    return &m_tracks;
}


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void adjustSubstPos(const cv::Rect& blob, cv::Rect& rcRight, cv::Rect& rcLeft) {
	// TODO make sure valid references are passed
	int mvRight_edgeLeft = rcRight.x;
	int mvRight_edgeRight = rcRight.x + rcRight.width;
	int mvLeft_edgeLeft = rcLeft.x;
	int mvLeft_edgeRight = rcLeft.x + rcLeft.width;

	// assign blob's right edge
	// to left moving track
	if (mvLeft_edgeRight > mvRight_edgeRight) {
		rcLeft.x = (blob.x + blob.width) - rcLeft.width;
	// to right moving track
	} else {
		rcRight.x = (blob.x + blob.width) - rcRight.width;
	}

	// assign blob's left edge
	// to left moving track
	if (mvLeft_edgeLeft < mvRight_edgeLeft) {
		rcLeft.x = blob.x;
	// to right moving track
	} else {
		rcRight.x = blob.x;
	}
	return;
}


cv::Rect calcSubstitute(const Track& track) {
    cv::Point velocity( static_cast<int>(round(track.getVelocity().x)), static_cast<int>(round(track.getVelocity().y)) );
	cv::Rect substitute = track.getActualEntry().rect() + velocity;
	return substitute;
}


cv::Rect clipAtRoi(cv::Rect rec, cv::Size roi) {
	cv::Rect rcRoi(cv::Point(0,0), roi);
	return rec & rcRoi;
}


std::list<Track>* combineTracks(std::list<Track>& tracks, cv::Size roi) {
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

		// combine un-occluded tracks only
		if (!iTrack->isOccluded()) {

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
							iTrack->addTrackEntry(iTrackComp->getActualEntry().rect(), roi);
							iTrackComp->markForDeletion();

						// vice versa
						} else {
							iTrackComp->addTrackEntry(iTrack->getActualEntry().rect(), roi);
							iTrack->markForDeletion();
						}
					}
				// at least one track has a too short history (fewer than 2 elements) -> do nothing
				} else {
			
				}

				++iTrackComp;
			} // end_for_each remaining tracks in list
		} // end_if is_not_occluded

		++iTrack;
	} // end_for_each track
	return &tracks;
}


// current x distance between tracked objects
int distanceCurrent(const Track& mvLeft, const Track& mvRight) {
	int mvLeft_leftEdge = mvLeft.getActualEntry().rect().x;
	int mvRight_rightEdge = (mvRight.getActualEntry().rect().x + mvRight.getActualEntry().rect().width);
	int distCurr = mvLeft_leftEdge - mvRight_rightEdge;
	return distCurr;
}


// x distance between tracked objects after next update
int distanceNextUpdate(Track& mvLeft, Track& mvRight) {
	int mvLeft_leftEdge = mvLeft.getActualEntry().rect().x;
	int mvRight_rightEdge = (mvRight.getActualEntry().rect().x + mvRight.getActualEntry().rect().width);
	int distNext = mvLeft_leftEdge + static_cast<int>(round(mvLeft.getVelocity().x))
	- (mvRight_rightEdge + static_cast<int>(round(mvRight.getVelocity().x)));
	return distNext;
}


// at least one track should move relatively fast
bool isDirectionOpposite(Track& track, Track& trackCompare, const double backlash) {
	// opposite direction
	if (signBit(track.getVelocity().x) != signBit(trackCompare.getVelocity().x)) {
		// at least one velocity must be outside backlash
		if ( abs(track.getVelocity().x) > backlash || abs(trackCompare.getVelocity().x) > backlash ) {
			return true;
		} else {
			return false;
		}
	// same direction
	} else {
		return false;
	}
}


bool isNextUpdateOccluded(const Track& mvLeft, const Track& mvRight) {
	int mvLeft_leftEdge = mvLeft.getActualEntry().rect().x;
	int mvRight_rightEdge = (mvRight.getActualEntry().rect().x + mvRight.getActualEntry().rect().width);

	// current distance between objects and distance in next update step
	int distCurr = mvLeft_leftEdge - mvRight_rightEdge;
	int distNext = mvLeft_leftEdge + static_cast<int>(round(mvLeft.getVelocity().x))
		- (mvRight_rightEdge + static_cast<int>(round(mvRight.getVelocity().x)));

	// set threshold for distNext in order to compensate for shaky velocity
	int velocitySum = static_cast<int>(round(abs(mvLeft.getVelocity().x) + mvRight.getVelocity().x) );
	int threshold = velocitySum / 2;
	if (distCurr >= 0 && distNext < threshold)
		return true;
	else
		return false;
}


cv::Rect occludedArea(Track& mvLeft, Track& mvRight, int updateSteps) {
	cv::Rect rcStart = mvLeft.getActualEntry().rect() | mvRight.getActualEntry().rect();
	cv::Rect rcLeftEnd = mvLeft.getActualEntry().rect() + static_cast<cv::Point>(cv::Point2d(updateSteps * mvLeft.getVelocity()));
	cv::Rect rcRightEnd = mvRight.getActualEntry().rect() + static_cast<cv::Point>(cv::Point2d(updateSteps * mvRight.getVelocity()));
	cv::Rect rcEnd = rcLeftEnd | rcRightEnd;
	return rcStart | rcEnd;
}


// DEBUG
void printRect(cv::Rect& rect) {
	std::cout << rect << endl;
	return; 
}

void printTrackRect(Track& track) {
	std::cout << track.getActualEntry().rect() << endl;
	return; 
}

void printVelocity(Track& track) {
    size_t id = track.getId();
	double velocity = track.getVelocity().x;
	int leftEdge = track.getActualEntry().rect().x;
	int rightEdge = track.getActualEntry().rect().x + track.getActualEntry().rect().width;
	std::cout << "id#" << id << " l:" << leftEdge << " r:" << rightEdge << " vel:" 
			<< std::fixed << std::setprecision(1) << velocity << endl;
	return; 
}


int remainingOccludedUpdateSteps(const Track& mvLeft, const Track& mvRight) {
	int dist = distanceCurrent(mvLeft, mvRight);
	int widthRight = mvRight.getActualEntry().rect().width;
	int widthLeft = mvLeft.getActualEntry().rect().width;

	double velocitySum = mvRight.getVelocity().x + abs(mvLeft.getVelocity().x);

	int nSteps = static_cast<int>( ceil((dist + widthRight + widthLeft) / velocitySum) );
	return nSteps;
}
