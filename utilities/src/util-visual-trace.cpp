#include "../../video-dev/car-count/src/stdafx.h"

#include "../../../cpp/inc/id_pool.h"
#include "../../../cpp/inc/pick_list.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/frame_handler.h" // for color const
#include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"
#include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"


// glob vars for temporary tracing
TrackStateVec	g_trackState;
size_t			g_idx = 0;


//////////////////////////////////////////////////////////////////////////////
// Local functions ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Implementation of interface ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool breakEscContinueEnter() {
	using namespace std;
	static bool initialized = false;
			
	// print help once
	if (!initialized) {
		cout << endl << "visual trace" << endl;
		cout << "ENTER: continue" << endl;
		cout << "ESC:   exit" << endl << endl;
		initialized = true;
	}

	// wait for key input (needs at least one highgui window)
	int key = 0;
	while ( key != Key::escape && key != Key::enter ) {
		key = cv::waitKeyEx(0);
	}
	if (key == Key::escape)
		return true;
	else
		return false;
}


Occlusion createOcclusionAt(Track& trackRight, Track& trackLeft, cv::Size roi, int collisionX, cv::Size blobSize, cv::Point velocityRight, cv::Point velocityLeft) {
	
	// adjust collision point, if necessary
    int colXAct(collisionX);
	if (collisionX > roi.width) {
		std::cerr << "collision point: " << collisionX << "outside roi: " << roi.width << std::endl;
		colXAct = roi.width / 2;
		std::cerr << "taking roi/2: " << colXAct << std::endl; 
	}

	// adjust blob height, if necessary
	cv::Size blobSizeAct(blobSize);
	if ((roi.height - blobSize.height - 10) < 0) {
		blobSizeAct.height = 10;
		std::cerr << "blob height adjusted to: " << blobSizeAct.height << std::endl;
	}
	
	cv::Point colAct(colXAct, roi.height - blobSize.height - 10);
	cv::Point orgRight(colAct.x - blobSizeAct.width, colAct.y);
	cv::Point orgLeft(colAct);
	trackRight = createTrackAt(roi, orgRight, blobSizeAct, velocityRight, 1);
	trackLeft = createTrackAt(roi, orgLeft, blobSizeAct, velocityLeft, 2);

	return Occlusion(roi, &trackLeft, &trackRight, 5);
}


Track createTrack(cv::Size roi, cv::Rect end, cv::Point velocity, int steps, size_t id) {
    (void)id;
	Track track;
	cv::Rect rc(end.tl() - velocity * (steps - 1), end.size());
	for (int i = 0; i < steps; ++i) {
		// addTrackEntry modifies rc, if out of roi, thus copy needed
		cv::Rect rcAdd = rc; 
		track.addTrackEntry(rcAdd, roi);
		rc += velocity;
	}
	return track;
}


Track createTrackAt(cv::Size roi, cv::Point blobPos, cv::Size blobSize, cv::Point velocity, size_t id) {
	int iUpdates = 2;
	cv::Rect blobLast(blobPos, blobSize);
	cv::Rect blobAct = blobLast - iUpdates * velocity;
	Track track(id);
	track.addTrackEntry(blobAct, roi);
	std::list<cv::Rect> blobs;
	for (int i = iUpdates; i > 0; --i) {
		blobAct += velocity;
		blobs.push_back(blobAct);
		track.updateTrackIntersect(blobs, roi);
		assert(blobs.size() == 0);
	}
	assert(track.getHistory() == 3);
	//track.addTrackEntry(blobAct, roi);
	return track;
}


bool examineBlobTimeSeries(const BlobTimeSeries& blobTmSer, cv::Size roi) {
	using namespace std;

	// validity check
	size_t maxIdx = blobTmSer.size();
	if (maxIdx == 0) {
		cerr << "time series does not contain any data" << endl;
		return false;
	} else {
		--maxIdx;
	}

	// print help once
	static bool isInitialized = false;
	if (!isInitialized) {
		printUsageHelp();
		isInitialized = true;
	}

	// loop through index, starting at 0
	size_t idx = 0;
	int key = 0;

	while (key != Key::escape) {

		// TODO showBlobsAt = printBlobsAt + imshow
		// TODO remove global index
		showBlobsAt(blobTmSer, roi, idx);
		key = cv::waitKeyEx(0);
		switch (key) {
			case Key::arrowUp:
                if (idx > 0)
                    --idx;
                else
                    idx = maxIdx;
				break;
			case Key::arrowDown:
                if (idx < maxIdx)
                    ++idx;
                else
                    idx = 0;
				break;
		}
		cout << "idx: " << idx << "    \r"; // \r returns to beginning of line
	}
	cout << "end time series visualization" << endl;
	return true;
}


bool examineTrackState(const TrackStateVec trackState, cv::Size roi) {
	using namespace std;

	// validity check
	size_t maxIdx = trackState.size();
	if (maxIdx == 0) {
		cerr << "track state does not contain any data" << endl;
		return false;
	} else {
		--maxIdx;
	}

	// print help once
	static bool isInitialized = false;
	if (!isInitialized) {
		printUsageHelp();
		isInitialized = true;
	}

	// loop through index
	int key = 0;
	while (key != Key::escape) {
		showTrackStateAt(trackState, roi, g_idx);
		key = cv::waitKeyEx(0);
		switch (key) {
			case Key::arrowUp:
                if (g_idx > 0)
                    --g_idx;
                else
                    g_idx = maxIdx;
				break;
			case Key::arrowDown:
                if (g_idx < maxIdx)
                    ++g_idx;
                else
                    g_idx = 0;
				break;
		}
		// cout << "idx: " << g_idx << "    \r"; // \r returns to beginning of line
	}
	cout << "end TrackState visualization" << endl;
	return true;
}


BlobTimeSeries moveBlobsThroughRoi(const cv::Size& roi, const MovingBlob& right, const MovingBlob& left) {
	using namespace std;
	vector<list<cv::Rect>> timeSeries;

	// check velocity direction
	if (right.velocity.x <= 0) {
		cout << "velocity to right must be greater than 0" << endl;
		return timeSeries;
	}
	if (left.velocity.x >= 0) {
		cout << "velocity to left must be less than 0" << endl;
		return timeSeries;
	}

	// check blob inside roi
	cv::Rect roiRect(cv::Point(0,0), roi);
	cv::Rect rightRect(right.origin, right.size);
	if ( (roiRect & rightRect).empty() ) {
		cout << "blob right: " << rightRect << " outside roi" << endl;
		return timeSeries;
	}
	cv::Rect leftRect(left.origin, left.size);
	if ( (roiRect & leftRect).empty() ) {
		cout << "blob left: " << leftRect << " outside roi" << endl;
		return timeSeries;
	}

	// update steps to go through roi in x coord
	int nRight = (roi.width - right.origin.x) / right.velocity.x;
	int nLeft = (left.origin.x + left.size.width) / std::abs(left.velocity.x);
	int n = nRight > nLeft ? nRight : nLeft;

	// push blobs to list for all update steps
	for (; n >=0; --n) {
		list<cv::Rect> blobs;

		// combine rects, if intersection
		// clip rects, if outside roi
		cv::Rect intersection = rightRect & leftRect;
		if ( intersection.empty() ) {
			blobs.push_back(clipAtRoi(rightRect, roi));
			blobs.push_back(clipAtRoi(leftRect, roi));
		} else {
			blobs.push_back(clipAtRoi(rightRect | leftRect, roi));
		}
		
		timeSeries.push_back(blobs);
		rightRect += right.velocity;
		leftRect += left.velocity;
	}
	return timeSeries;
}


BlobTimeSeries moveOcclusionThroughRoi(cv::Size roi, MovingBlob right, MovingBlob left, int collisionX, int gapStartX) {
	// collision point
    int colX = (collisionX == 0) ? (roi.width / 2) : collisionX;
	colX = (colX > roi.width) ? roi.width : colX;
	cv::Point colOrgLeft(colX,70);
	cv::Point colOrgRight(colX-right.size.width, 70);

	// update steps
	int nUpdates = static_cast<int>( ceil (static_cast<double>(gapStartX) / (abs(left.velocity.x) + abs(right.velocity.x)) ) );

	// origin for blob moves to left
	cv::Point startOrgLeft = colOrgLeft - nUpdates * left.velocity;
	
	// origin for blob moves to right		
	cv::Point startOrgRight = colOrgRight - nUpdates * right.velocity; // - cv::Point(blob.width,0);
	
	MovingBlob rightActual(startOrgRight, right.size, right.velocity);
	MovingBlob leftActual(startOrgLeft, left.size, left.velocity);
	BlobTimeSeries blobTimeSeries = moveBlobsThroughRoi(roi, rightActual, leftActual);

	return blobTimeSeries;
}


void printBlob(cv::Mat& canvas, cv::Rect blob, cv::Scalar color) {
	// make size one line smaller for better visibiliy
	cv::Rect rcPrint(blob);
	rcPrint.height -= 2;
	rcPrint.width -= 2;
	rcPrint.x += 1;
	rcPrint.y +=1;
	cv::rectangle(canvas, rcPrint, color, Line::thin);
	return;
}


void printBlobs(cv::Mat& canvas, const std::list<cv::Rect>& blobs) {
	// appearance
	cv::Scalar color[] = {white, gray_light, gray};
	size_t nColors = sizeof(color) / sizeof(color[0]);
	size_t idx = 0;

	// loop through available blobs
	std::list<cv::Rect>::const_iterator iBlob = blobs.begin();
	while (iBlob != blobs.end()) {
		printBlob(canvas, *iBlob, color[idx % nColors]);
		++iBlob;
		++idx;
	}
	return;
}


void printBlobsAt(cv::Mat& canvas, const BlobTimeSeries& timeSeries, const size_t idx) {
		using namespace std;
		// check upper index bound
		size_t idxTime = (idx >= timeSeries.size()) ? timeSeries.size()-1 : idx;
        (void)idxTime;
		cv::Scalar color[3] = {red, green, yellow};
		size_t idxColor = 0;

		// loop through available blobs
		list<cv::Rect>::const_iterator iBlob = timeSeries[idx].begin();
		while (iBlob != timeSeries[idx].end()) {
			cv::Rect rcPrint(*iBlob);
			rcPrint.height -= 2;
			rcPrint.width -= 2;
			rcPrint.x += 1;
			rcPrint.y +=1;
			cv::rectangle(canvas, rcPrint, color[idxColor]);
			++iBlob;
		}

		return;
}


void printIndex(cv::Mat& canvas, size_t index) {
	int font = cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
	double fontScale = 0.4;
	int baseline = 0;
	std::string idxString("idx #" + std::to_string(static_cast<long long>(index)));
	cv::Size textSize = cv::getTextSize(idxString, font, fontScale, 1, &baseline);
	int xOffset = 5;
	int yOffset = canvas.size().height - textSize.height;
	cv::putText(canvas, idxString, cv::Point(xOffset, yOffset),
			font, fontScale, white);
	return;
}


void printOcclusion(cv::Mat& canvas, const Occlusion& occlusion, cv::Scalar color) {
	// make size one line larger for better visibiliy
	cv::Rect rcPrint(occlusion.rect());
	rcPrint.height += 2;
	rcPrint.width += 2;
	rcPrint.x -= 1;
	rcPrint.y -=1;
	cv::rectangle(canvas, rcPrint, color);
	return;
}

void printOcclusions(cv::Mat& canvas, const std::list<Occlusion>& occlusions) {
	// appearance
	cv::Scalar color[] = {magenta, magenta_dark, purple};
	size_t nColors = sizeof(color) / sizeof(color[0]);
	
	// loop through available occlusions
	std::list<Occlusion>::const_iterator iOcc = occlusions.begin();
	while (iOcc != occlusions.end()) {
		printOcclusion(canvas, *iOcc, color[iOcc->id() % nColors]);
		++iOcc;
	}
	return;
}


void printTrack(cv::Mat& canvas, const Track& track, cv::Scalar color) {
	cv::Rect rcActual = track.getActualEntry().rect();
	cv::Rect rcPrev = track.getPreviousEntry().rect();
	cv::rectangle(canvas, rcActual, color, Line::thick);
	cv::rectangle(canvas, rcPrev, color, Line::thin);
	return;
}


void printTracks(cv::Mat& canvas, const std::list<Track>& tracks, bool withPrevious) {
	// appearance
	cv::Scalar color[] = {blue, green, red, yellow};
	size_t nColors = sizeof(color) / sizeof(color[0]);

	// loop through available tracks
	std::list<Track>::const_iterator iTrack = tracks.begin();
	while (iTrack != tracks.end()) {
        size_t id = iTrack->getId();
		cv::Rect rcActual = iTrack->getActualEntry().rect();
		if (withPrevious) {
			cv::Rect rcPrev = iTrack->getPreviousEntry().rect();
			cv::rectangle(canvas, rcActual, color[id % nColors], Line::thick);
			cv::rectangle(canvas, rcPrev, color[id % nColors], Line::thin);
		} else {
			cv::rectangle(canvas, rcActual, color[id % nColors], Line::thin);
		}
		++iTrack;
	}

	return;
}


void printTracksAt(cv::Mat& canvas, const TrackTimeSeries& timeSeries, size_t idxUnchecked) {
	// appearance
	cv::Scalar color[] = {blue, green, red, yellow};
	size_t nColors = sizeof(color) / sizeof(color[0]);

	// check upper index bound
	size_t idx = (idxUnchecked >= timeSeries.size()) ? timeSeries.size()-1 : idxUnchecked;

	// loop through available tracks
	std::list<Track>::const_iterator iTrack = timeSeries[idx].begin();
	while (iTrack != timeSeries[idx].end()) {
		cv::Rect rcActual = iTrack->getActualEntry().rect();
		cv::Rect rcPrev = iTrack->getPreviousEntry().rect();

        size_t id = iTrack->getId();
		cv::rectangle(canvas, rcActual, color[id % nColors], Line::thick);
		cv::rectangle(canvas, rcPrev, color[id % nColors], Line::thin);
		++iTrack;
	}

	return;
}


void printTrackInfo(cv::Mat& canvas, const std::list<Track>& tracks) {
	// appearance
	cv::Scalar color[] = {blue, green, red, yellow};
	size_t nColors = sizeof(color) / sizeof(color[0]);	
	int idxLine = 0;

	std::list<Track>::const_iterator iTrack = tracks.begin();
	while (iTrack != tracks.end()) {

		// collect track info
		int confidence = iTrack->getConfidence();
        size_t id = iTrack->getId();
		int length = static_cast<int>(iTrack->getLength());
		double velocity = iTrack->getVelocity().x;

		// print track info
		std::stringstream ss;
		ss << "#" << id << ", con=" << confidence << ", len=" << length << ", v=" 
			<< std::fixed << std::setprecision(1) << velocity;

		// split status line, if necessary
		int xOffset = 5;
		int font = cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
		double fontScale = 0.4;
		int baseline = 0;
		cv::Size textSize = cv::getTextSize(ss.str(), font, fontScale, 1, &baseline);

		// fits onto one line 
		if (textSize.width + (xOffset * 2) < canvas.size().width) {
			int yOffset = 10 + idxLine * 10;
			cv::putText(canvas, ss.str(), cv::Point(xOffset, yOffset),
				font, fontScale, color[id % nColors]);

		// needs two lines
		} else {
			size_t pos = std::string::npos;
			pos = ss.str().find(", len=");
			std::string line1 = ss.str().substr(0, pos);
			std::string line2 = ss.str().substr(pos + 2);
			int yOffset = 10 + idxLine * 20;
			cv::putText(canvas, line1, cv::Point(xOffset, yOffset),
				font, fontScale, color[id % nColors]);
			cv::putText(canvas, line2, cv::Point(xOffset, yOffset + 10),
				font, fontScale, color[id % nColors]);
		}
		++idxLine;
		++iTrack;
	} // end_while

	return;
}


void printTrackInfoAt(cv::Mat& canvas, const TrackTimeSeries& timeSeries, size_t idxUnchecked) {
	// appearance
	cv::Scalar color[] = {blue, green, red, yellow};
	size_t nColors = sizeof(color) / sizeof(color[0]);	
	int idxLine = 0;


	// check upper index bound
	size_t idx = (idxUnchecked >= timeSeries.size()) ? timeSeries.size()-1 : idxUnchecked;

	std::list<Track>::const_iterator iTrack = timeSeries[idx].begin();
	while (iTrack != timeSeries[idx].end()) {

		// collect track info
		int confidence = iTrack->getConfidence();
        size_t id = static_cast<size_t>(iTrack->getId());
		int length = static_cast<int>(iTrack->getLength());
		double velocity = iTrack->getVelocity().x;

		// print track info
		std::stringstream ss;
		ss << "#" << id << ", con=" << confidence << ", len=" << length << ", v=" 
			<< std::fixed << std::setprecision(1) << velocity;

		// split status line, if necessary
		int xOffset = 5;
		int font = cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
		double fontScale = 0.4;
		int baseline = 0;
		cv::Size textSize = cv::getTextSize(ss.str(), font, fontScale, 1, &baseline);

		// fits onto one line 
		if (textSize.width + (xOffset * 2) < canvas.size().width) {
			int yOffset = 10 + idxLine * 10;
			cv::putText(canvas, ss.str(), cv::Point(xOffset, yOffset),
				font, fontScale, color[id % nColors]);

		// needs two lines
		} else {
			size_t pos = std::string::npos;
			pos = ss.str().find(", len=");
			std::string line1 = ss.str().substr(0, pos);
			std::string line2 = ss.str().substr(pos + 2);
			int yOffset = 10 + idxLine * 20;
			cv::putText(canvas, line1, cv::Point(xOffset, yOffset),
				font, fontScale, color[id % nColors]);
			cv::putText(canvas, line2, cv::Point(xOffset, yOffset + 10),
				font, fontScale, color[id % nColors]);
		}
		++idxLine;
		++iTrack;
	} // end_while

	return;
}


void printUsageHelp() {
	using namespace std;
	cout << endl << "Step through time series index - usage:" << endl;
	cout << "ARROW UP:   step backward" << endl;
	cout << "ARROW DOWN: step forward" << endl;
	cout << "ESC:        exit" << endl << endl;
	return;
}


bool showBlobsAt(const BlobTimeSeries& blobTmSer, cv::Size roi, size_t idxUnchecked) {
	using namespace std;
	// check upper index bound
	size_t idx = (idxUnchecked >= blobTmSer.size()) ? blobTmSer.size()-1 : idxUnchecked;

	cv::Mat canvas(roi, CV_8UC3, black);
	size_t idxWindow = 0;
	list<cv::Rect>::const_iterator iBlob = blobTmSer[idx].begin();
	while (iBlob != blobTmSer[idx].end()) {
		// window appearance
		double scaleX = 2.0;
		int offsetX = 0;
        int winPosX = offsetX + static_cast<int>(idxWindow) * (roi.width * static_cast<int>(scaleX) + 6);

		// print blobs and tracks on roi-sized canvax
		canvas.setTo(black);
		printBlobs(canvas, blobTmSer[idx]);

		// show window "iBlob->m_name" 
		// for some weird reasons idx cannot be shown in window title
		cv::namedWindow("blobs");
		cv::moveWindow("blobs", winPosX, 0);

		// print on larger scaled canvas
		cv::Mat canvasLarge;
		cv::resize(canvas, canvasLarge,cv::Size(0,0), 2.0, 2.0);
		cv::imshow("blobs", canvasLarge);
		
		++iBlob;

	}

	return true;
}


void showBlobAssignment(std::string winName, const Track& track, cv::Rect blob, cv::Size roi, size_t id) {
	cv::namedWindow(winName);
	cv::moveWindow(winName, 1060, 0);
	cv::Mat canvas(roi, CV_8UC3, black);
	cv::Scalar color[] = {blue, green, red, yellow};
	size_t nColors = sizeof(color) / sizeof(color[0]);

	printTrack(canvas, track, color[id % nColors]);
	cv::rectangle(canvas, blob, white);

	cv::Mat canvasLarge;	
	cv::resize(canvas, canvasLarge,cv::Size(0,0), 2.0, 2.0);
	cv::imshow(winName, canvasLarge);
	return;
}


void showCanvas(const std::string& name, const cv::Mat& canvas, const double scaling = 1.0) { 
	cv::Mat scaledCanvas;
	cv::resize(canvas, scaledCanvas, cv::Size(0,0), scaling, scaling);
	cv::imshow(name, scaledCanvas);
	return;
}


void showTracks(std::string winName, const SceneTracker& scene) {
	cv::namedWindow(winName);
	cv::moveWindow(winName, 1060, 0);

	cv::Mat canvas(scene.m_roiSize, CV_8UC3, black);
	printTracks(canvas, scene.m_tracks);
	
	cv::Mat canvasLarge;
	cv::resize(canvas, canvasLarge,cv::Size(0,0), 2.0, 2.0);
	printTrackInfo(canvasLarge, scene.m_tracks);
	cv::imshow(winName, canvasLarge);
	return;
}


bool showTrackStateAt(const TrackStateVec& state, cv::Size roi, size_t idxUnchecked) {
	using namespace std;
	// check upper index bound
	size_t idx = (idxUnchecked >= state.size()) ? state.size()-1 : idxUnchecked;

	cv::Mat canvas(roi, CV_8UC3, black);
	size_t idxWindow = 0;
	list<TrackState>::const_iterator iState = state[idx].begin();
	while (iState != state[idx].end()) {
		// window appearance
		double scaleX = 2.0;
		int offsetX = 640;
        int winPosX = offsetX + static_cast<int>(idxWindow) * (roi.width * static_cast<int>(scaleX) + 6);

		// print blobs and tracks on roi-sized canvax
		canvas.setTo(black);
		printBlobs(canvas, iState->m_blobs);
		printTracks(canvas, iState->m_tracks);
		printOcclusions(canvas, iState->m_occlusions);

		// print track info on larger scaled canvas
		cv::Mat canvasLarge;
		cv::resize(canvas, canvasLarge,cv::Size(0,0), 2.0, 2.0);
		printTrackInfo(canvasLarge, iState->m_tracks);
		printIndex(canvasLarge, idx);

		// show window "iState->m_name" 
		// for some weird reasons idx cannot be shown in window title
		cv::namedWindow(iState->m_name);
		cv::moveWindow(iState->m_name, winPosX, 0);
		cv::imshow(iState->m_name, canvasLarge);
		
		++iState;
		++idxWindow;
	}

	return true;
}
