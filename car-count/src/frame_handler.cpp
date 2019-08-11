#include "stdafx.h"
#include "../include/frame_handler.h"

//#include <exception>
#include <sstream>

void arrowedLine_cv_2_9(cv::Mat& image, cv::Point start, cv::Point end, cv::Scalar color,
        int thickness = 1, int lineType = 8, int shift = 0, double tipLength = 0.1) {
    double tipAngle = CV_PI / 6; // 30 degrees
    double tipSize = cv::norm(end - start) * tipLength;
    double angle = atan2(double(start.y - end.y), double(start.x - end.x));
    cv::Point p;
    p.x = cvRound(end.x + tipSize * cos(angle + tipAngle));
    p.y = cvRound(end.y + tipSize * sin(angle + tipAngle));
    cv::line(image, end, p, color, thickness, lineType);
    p.x = cvRound(end.x + tipSize * cos(angle - tipAngle));
    p.y = cvRound(end.y + tipSize * sin(angle - tipAngle));
    cv::line(image, end, p, color, thickness, lineType, shift);
    cv::line(image, start, end, color, thickness, lineType, shift);
    return;
}

//////////////////////////////////////////////////////////////////////////////
// Arrow /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Arrow::Arrow(cv::Point start, cv::Point end, cv::Scalar color, int thickness):
	m_start(start),
	m_end(end),
	m_color(color),
	m_thickness(thickness) {
}


void Arrow::put(cv::Mat& image) {
#if defined CV_VERSION_EPOCH && CV_VERSION_MINOR <=9
    arrowedLine_cv_2_9(image, m_start, m_end, m_color, m_thickness, 8, 0, 0.05);
#else
    cv::arrowedLine(image, m_start, m_end, m_color, m_thickness, 8, 0, 0.05);
#endif
	return;
}


//////////////////////////////////////////////////////////////////////////////
// Font /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Font::Font():
	face(cv::FONT_HERSHEY_SIMPLEX), 
	scale(0.5),
	color(white),
	thickness(1) {
}


//////////////////////////////////////////////////////////////////////////////
// TextRow ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
TextRow::TextRow(const int orgY, const Font font):
	m_font(font), m_origin(cv::Point(0,orgY)) {
}


/// Get text width in pixels
/// \return Width of text in pixels
int TextRow::getWidth() {
	int baseLine;
	cv::Size textSize = cv::getTextSize(m_text, m_font.face, m_font.scale,
		m_font.thickness, &baseLine);
	return textSize.width;
}


/// Draw actual text of row on image
/// \param[in] image Image to draw on
/// \param[in] colOrigin Column origin in pixels
void TextRow::put(cv::Mat& image, cv::Point colOrigin) {
	cv::Point rowOrigin = colOrigin + m_origin;
	cv::putText(image, m_text, rowOrigin, m_font.face, m_font.scale, m_font.color, m_font.thickness);
	return;
}

/// Set horizontal indentation according to row alignment (left, right)
/// \param[in] rowAlign (left, right)
/// \param[in] colWidth Column width in pixels
void TextRow::setIndent(Align rowAlign, int maxTextWidth) {

	// calc text width for row
	int baseLine;
	cv::Size textSize = cv::getTextSize(m_text, m_font.face, m_font.scale,
			m_font.thickness, &baseLine);

	// calc origin depending on alignment
	if (rowAlign == Align::left) {
		m_origin.x = 0;
	} else { // right align
		m_origin.x = maxTextWidth - textSize.width;
	}
}


/// Set text, if text width fits into available column width
/// otherwise fill available space with '#'
/// \param[in] text Text to set
/// \param[in] colWidth Column width in pixels
/// \return Width of text in pixels
int TextRow::setText(const std::string& text, const int colWidth) {
	int baseLine;
	cv::Size textSize = cv::getTextSize(text, m_font.face, m_font.scale,
		m_font.thickness, &baseLine);
	
	// fill with '#', if text too wide
	if (textSize.width < colWidth) {
		m_text = text;
	} else { // text too wide
		std::string textTooLong;
		cv::Size oneChar = cv::getTextSize("#", m_font.face, m_font.scale,
			m_font.thickness, &baseLine);
		int numChar = colWidth / oneChar.width;
		for (int n = 0; n < numChar; ++n) 
			textTooLong.push_back('#');
		textSize = cv::getTextSize(textTooLong, m_font.face, m_font.scale,
			m_font.thickness, &baseLine);
		m_text = textTooLong;
	}
	return textSize.width;
}



//////////////////////////////////////////////////////////////////////////////
// TextColumn ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
TextColumn::TextColumn(const cv::Point origin, const int colWidth, const Align colAlign):
    m_colAlign(colAlign), m_colWidth(colWidth), m_origin(origin) {}


/// Add text row to column block
/// \param[in] originY Vertical position of row text in pixels
/// \param[in] fon Font structure for row text
/// \return row index of added row
int TextColumn::addRow(const int originY, const Font font) {
	m_rowArray.push_back(TextRow(originY, font));
	int index = m_rowArray.size() - 1;
	return index;
}


// TextColumn::alignRows() functor for getting maximum width of all rows
class MaxRowWidth {
public:
	MaxRowWidth(): m_maxWidth(0) {};
	void operator() (TextRow& textRow) {
		if (textRow.getWidth() > m_maxWidth)
			m_maxWidth =  textRow.getWidth();
	}
	int	getWidth() {
		return m_maxWidth;
	}
private:
	int		m_maxWidth;
};


// TextColumn::alignRows() functor for setting row indentation
class SetRowIndent {
public:
	SetRowIndent(Align align, int textWidth): 
			m_align(align),
			m_textWidth(textWidth)
			{};
	void operator() (TextRow& textRow) {
		textRow.setIndent(m_align, m_textWidth);
	}
private:
	Align	m_align;
	int		m_textWidth;
};


/// Align text column block
/// \param[in] rowAlign Alignment of single text row (left, right)
/// \param[in] colAlign Alignment of embracing rectangle of text colum block (left, right)
/// \return True, if all texfields can be displayed in full length
void TextColumn::alignRows(Align rowAlign) {

	// get max text width in pixels for all rows
	MaxRowWidth maxRowWidth = std::for_each(m_rowArray.begin(), m_rowArray.end(), MaxRowWidth() );
	m_maxTextWidth = maxRowWidth.getWidth();

	// set row alignment for each row
	std::for_each(m_rowArray.begin(), m_rowArray.end(), SetRowIndent(rowAlign, m_maxTextWidth) );
	return;
}


/// Remove row
/// \param[in] rowIdx Row index
/// \return True, if row was successfully removed
bool TextColumn::removeRow(const int rowIdx) {
	size_t idx = static_cast<size_t>(rowIdx);
	if (idx >= m_rowArray.size() ) {
		std::cerr << "removeRow: row index: " << idx << " out of array size: " 
			<< m_rowArray.size() << std::endl;
		return false;
	} else { // index within bounds
		m_rowArray.erase(m_rowArray.begin() + rowIdx);
		return true;
	}
}


/// Draw text of all rows in column on image
/// column block left or right aligned
/// \param[in] image Image to draw on
void TextColumn::put(cv::Mat& image) {
	// no extra indent for left aligned
	cv::Point offset(0,0);
	
	// extra indent for right aligned column
	if (m_colAlign == Align::right)
		offset.x = m_colWidth - m_maxTextWidth;

	// show each row with offset applied
	RowArray::iterator iRow = m_rowArray.begin();
	while (iRow != m_rowArray.end()) {
		iRow->put(image, m_origin + offset);
		++iRow;
	}
}


/// Resize text column
/// \param[in] origin Origin of text column
/// \param[in] width Width of text column
/// \return True, if text for given row was successfully set
void TextColumn::resize(const cv::Point origin, const int colWidth) {
	m_origin = origin;
	m_colWidth = colWidth;
	return;
}


/// Set text for row
/// \param[in] rowIdx Row index
/// \param[in] text Text to set
/// \return True, if text for given row was successfully set
bool TextColumn::setRowText(const int rowIdx, const std::string& text) {
	size_t idx = static_cast<size_t>(rowIdx);
	if (idx >= m_rowArray.size() ) {
		std::cerr << "setRowText: row index: " << idx << " out of array size: " 
			<< m_rowArray.size() << std::endl;
		return false;
	} else { // index within bounds
		m_rowArray.at(rowIdx).setText(text, m_colWidth);
		return true;
	}
}





//////////////////////////////////////////////////////////////////////////////
// Inset /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Inset::Inset(cv::Mat image) : backGrndImage(image), composedImage(image) {}


/// create arrow pointing to left or right
/// based on inset image origin
Arrow Inset::createArrow(const Align pointsTo) {
	// layout
	// left border | arrow to left | divider | arrow to right | right border
	int length = backGrndImage.size().width * 10 / 24;
	int xBorder = backGrndImage.size().width * 1 / 24;
	int yBorder = backGrndImage.size().height * 3 / 24;

	// appearance
	int thickness = (backGrndImage.size().width > 200) ? (backGrndImage.size().width / 200) : 1;

	// arrow coords depending on points to
	cv::Point start(0,0), end(0,0);
	if (pointsTo == Align::left) {	// arrow to left
		start = cv::Point(xBorder + length, yBorder);
		end = cv::Point(xBorder, yBorder);
	} else {						// arrow to right
		start = cv::Point(backGrndImage.size().width - xBorder - length, yBorder);
		end = cv::Point(backGrndImage.size().width - xBorder, yBorder);
	}

	return Arrow(start, end, green, thickness);
}


/// create left or right column based on inset size
/// text column origin relative to inset origin
TextColumn Inset::createTextColumn(const  Align colAlign) {
	// layout:
	// left border | image column | text column | divider | text column | image column | right border
	int widthImageCol = backGrndImage.size().width * 3 / 24;
	int widthTextCol = backGrndImage.size().width * 8 / 24;

	// yOrigin = 0 -> top of inset
	int yOrigin = 0;

	// xOrigin based on alignment
	int xOrigin = 0;
	if (colAlign == Align::right)
		xOrigin = backGrndImage.size().width - widthImageCol - widthTextCol;
	else
		xOrigin = widthImageCol;

	// create temp column as ret val
	TextColumn textCol(cv::Point(xOrigin, yOrigin), widthTextCol, colAlign);

	// font for all rows
	Font font;
	font.face      = cv::FONT_HERSHEY_SIMPLEX;
	font.scale	   = static_cast<double>(backGrndImage.size().width) / 600;
	font.color	   = green;
	font.thickness = (backGrndImage.size().width > 200) ? (backGrndImage.size().width / 200) : 1;

	// calculate row origins from inset size
	int yFirstRow = backGrndImage.size().height * 11 / 24;
	int ySecondRow = backGrndImage.size().height * 20 / 24; 

	// insert rows
	textCol.addRow(yFirstRow, font);
	textCol.addRow(ySecondRow, font);

	return textCol;
}



void Inset::putCount(CountResults cr) {
	// compose image from background
	composedImage = backGrndImage.clone();

	// set text at left text column
	std::string carLeft = std::to_string(static_cast<long long>(cr.carLeft));
	textLeft.setRowText(0, carLeft);
	std::string truckLeft = std::to_string(static_cast<long long>(cr.truckLeft));
	textLeft.setRowText(1, truckLeft);

	// right-align rows of left column depending on set text
	textLeft.alignRows(Align::right);

	// set text at right text column
	std::string carRight = std::to_string(static_cast<long long>(cr.carRight));
	textRight.setRowText(0, carRight);
	std::string truckRight = std::to_string(static_cast<long long>(cr.truckRight));
	textRight.setRowText(1, truckRight);

	// right-align rows of right column depending on set text
	textRight.alignRows(Align::right);

	// show arrows in output image
	arrowLeft.put(composedImage);
	arrowRight.put(composedImage);

	// show text in output image
	textLeft.put(composedImage);
	textRight.put(composedImage);
		
	return;
}


//////////////////////////////////////////////////////////////////////////////
// FrameHandler //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
FrameHandler::FrameHandler(Config* pConfig) : 
	Observer(pConfig), 
    m_frameCounter(0),
	m_isCaptureInitialized(false) {

	// instantiate background subtractor MOG2 (mixture of gaussians)
	// pointer does not have to be deleted
	m_mog2 = cv::createBackgroundSubtractorMOG2(100, 25, false);

	// instantiate cam input
	#if defined (_WIN32)
	m_captureWinCam = std::unique_ptr<CamInput>(new CamInput);
	#endif

	// retrieve config data
	update();
}


void FrameHandler::adjustFrameSizeDependentParams(int new_size_x, int new_size_y) {
	using namespace std;
	int old_size_x = stoi(mSubject->getParam("frame_size_x"));
	int old_size_y = stoi(mSubject->getParam("frame_size_y"));

	try {
		// roi
		long long roi_x = stoi(mSubject->getParam("roi_x")) * new_size_x / old_size_x;
		long long roi_width =  stoi(mSubject->getParam("roi_width")) * new_size_x / old_size_x;
		long long roi_y = stoi(mSubject->getParam("roi_y")) * new_size_y / old_size_y;
		long long roi_height = stoi(mSubject->getParam("roi_height")) * new_size_y / old_size_y;	
		mSubject->setParam("roi_x", to_string(roi_x));
		mSubject->setParam("roi_width", to_string(roi_width));
		mSubject->setParam("roi_y", to_string(roi_y));
		mSubject->setParam("roi_height", to_string(roi_height));
		
		// inset
		long long inset_height = stoi(mSubject->getParam("inset_height")) * new_size_y / old_size_y;
		mSubject->setParam("inset_height", to_string(inset_height));

		// blob assignment - long long cast to prevent int 32bit overflow
		long long blob_area_min =  static_cast<long long>(stoi(mSubject->getParam("blob_area_min"))) 
			* new_size_x * new_size_y / old_size_x / old_size_y;
		long long blob_area_max =  static_cast<long long>(stoi(mSubject->getParam("blob_area_max")))
			* new_size_x * new_size_y / old_size_x / old_size_y;
		mSubject->setParam("blob_area_min", to_string(blob_area_min));
		mSubject->setParam("blob_area_max", to_string(blob_area_max));

		// track assignment
		double track_max_dist =  stod(mSubject->getParam("track_max_distance")) * 0.5 
			* (static_cast<double>(new_size_x) / old_size_x	+ static_cast<double>(new_size_y) / old_size_y);
		mSubject->setParam("track_max_distance", to_string(static_cast<long long>(round(track_max_dist))));

		// count pos, count track length
		long long count_pos_x =  stoi(mSubject->getParam("count_pos_x")) * new_size_x / old_size_x;
		mSubject->setParam("count_pos_x", to_string(count_pos_x));
		long long count_track_length =  stoi(mSubject->getParam("count_track_length")) * new_size_x / old_size_x;
		mSubject->setParam("count_track_length", to_string(count_track_length));

		// truck_size
		long long truck_width_min =  stoi(mSubject->getParam("truck_width_min")) * new_size_x / old_size_x;
		mSubject->setParam("truck_width_min", to_string(truck_width_min));

		// TODO next line "invalid argument exception"
		long long truck_height_min =  stoi(mSubject->getParam("truck_height_min")) * new_size_y / old_size_y;
		mSubject->setParam("truck_height_min", to_string(truck_height_min));

		// save new frame size in config
		mSubject->setParam("frame_size_x", to_string((long long)new_size_x));
		mSubject->setParam("frame_size_y", to_string((long long)new_size_y));

		mSubject->notifyObservers();
	}
	catch (exception& e) {
		cerr << "invalid parameter in config: " << e.what() << endl;
	}
}

std::list<cv::Rect>& FrameHandler::calcBBoxes() {
	// find boundig boxes of newly detected objects, store them in m_bBoxes and return them
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;

	m_bBoxes.clear();

	cv::findContours(m_fgrMask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
	// calc bounding boxes of all detections and store in m_bBoxes
	for (unsigned int i = 0; i < contours.size(); i++) { 
		cv::Rect bbox = boundingRect(contours[i]);
		if ((bbox.area() > m_blobArea.min) && (bbox.area() < m_blobArea.max)) {
				m_bBoxes.push_back(bbox);
		}
	}
	
	return m_bBoxes;
}

/// load inset background from file, fit to frame size
/// create decoration and gauges
Inset FrameHandler::createInset(std::string insetFilePath) {
	using namespace std;

    cv::Size frameSize((int)m_frameSize.width, (int)m_frameSize.height);
	if (frameSize.width == 0 || frameSize.height == 0) {
		cerr << "createInset: wrong frame size: " << frameSize << endl;
	}

	// load background image for inset
	if (!isFileExist(insetFilePath)) {
		cerr << "createInset: path to inset image does not exist: " << insetFilePath << endl;
	}
	cv::Mat imageOriginal = cv::imread(insetFilePath);
	
	// fit image size to frame size 	
	cv::Mat imageFitToFrame;
	if (imageOriginal.data) { 
		// use loaded image, fit to current frame size
		cv::Size imageSize = imageOriginal.size();
		double scaling = frameSize.width / (double)imageSize.width;
		cv::resize(imageOriginal, imageFitToFrame, cv::Size(), scaling, scaling);
	} else { // create black matrix with same dimensions
		cerr << "createInset: could not open " << insetFilePath << ", use substitute inset" << endl;
		int width = (int)frameSize.width;
		int height = 65;
		cv::Mat substitute(height, width, CV_8UC3, black);
		imageFitToFrame = substitute;
	}
	
	// create inset object with background image
	Inset inset = Inset(imageFitToFrame);

	// create arrows
	inset.arrowLeft = inset.createArrow(Align::left);
	inset.arrowRight = inset.createArrow(Align::right);

	// create text columns
	inset.textLeft = inset.createTextColumn(Align::left);
	inset.textRight = inset.createTextColumn(Align::right);

	return inset;
}



int FrameHandler::getFrameInfo() {
	int type = m_frame.type();
	return type;
}


cv::Size2d FrameHandler::getFrameSize() {
	using namespace std;
	// check, if capture source is already initialized
	if (!m_isCaptureInitialized) {
		cerr << "capture must be initialized" << endl;
		return cv::Size2d(0,0);
	} else {
		return m_frameSize;
	}
}

// open cam device, save resolution to m_frameSize
bool FrameHandler::initCam(int camID, int camResolutionID) {
	using namespace std;

	// check, if capture source is already initialized
	if (m_isCaptureInitialized) {
		cerr << "initCam: capture already initialized" << endl;
		return false;
	}

	// enumerate available camera devices
	#if defined (_WIN32)
	m_captureWinCam = std::unique_ptr<CamInput>(new CamInput);
	int iDevices = m_captureWinCam->enumerateDevices();
	
	// camID must be in range
	if (camID < 0 || camID > iDevices) {
		cerr << "initCam: camID: "<< camID << " out of device range" << endl;
		return false;
	} 

	// open cam device with implicit cam resolution selection list
	if (!m_captureWinCam->open(camID, camResolutionID)) {
		cerr << "initCam: cannot open cam" << endl;
		return false;
	}

	#elif defined (__linux__)
    (void)camResolutionID;
	// use cv::Capture
	m_capture.open(camID);
	if (!m_capture.isOpened()) {
		cerr << "initCam: cannot open cam: " << camID << endl;
		return false;
	}
	m_frameSize.width = m_capture.get(CV_CAP_PROP_FRAME_WIDTH);
	m_frameSize.height = m_capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	if (m_frameSize.height == 0 || m_frameSize.width == 0) {
		cerr << "initCam: wrong cam resolution" << endl;
		return false;
	}
	#else
		throw "unsupported OS";
	#endif

	m_isCaptureInitialized = true;
	m_isFileCapture = false;
	return true;
}


// open video file, save resolution to m_frameSize
bool FrameHandler::initFileReader(std::string videoFilePath) {
	using namespace std;

	// check, if capture source is already initialized
	if (m_isCaptureInitialized) {
		cerr << "initFileReader: capture already initialized" << endl;
		return false;
	}

	// try open video input file
	m_capture.open(videoFilePath);
	if (!m_capture.isOpened()) {
		cerr << "initFileReader: cannot open file: " << videoFilePath << endl;
		return false;
	}

	// retrieve frame size
    // one frame must be read in order to get a return value from cap.get()
    cv::Mat firstFrame;
    if (!m_capture.read(firstFrame)) {
        cerr << "initFileReader: cannot read first frame in order to get resolution" << endl;
    }

    // GStreamer 0.1 does not return frame size in linux
    //  so VideoCapture::get(CV_CAP_PROP_FRAME_XXXXX) does not work
    //	m_frameSize.width = m_capture.get(CV_CAP_PROP_FRAME_WIDTH);
    //  m_frameSize.height = m_capture.get(CV_CAP_PROP_FRAME_HEIGHT);
    //
    m_frameSize = firstFrame.size();

	if (m_frameSize.height == 0 || m_frameSize.width == 0) {
		cerr << "initFileReader: wrong cam resolution" << endl;
		return false;
	}

	m_isCaptureInitialized = true;
	m_isFileCapture = true;
	return true;
}

// locate video file, save full path to m_inVideoFilePath
// return empty string, if video file was not found in the search directories
// search order: 1. current directory, 2. /home/work_dir
std::string FrameHandler::locateFilePath(std::string fileName) {
	using namespace std;
	string path;

	// 1. check current_dir
	string currentPath;
	#if defined (_WIN32)
		char bufPath[_MAX_PATH];
		_getcwd(bufPath, _MAX_PATH);
        if (strlen(bufPath) > 0) {
			currentPath = string(bufPath);
		} else {
			cerr << "locateVideoFile: error reading current directory" << endl;
			if (GetLastError() == ERANGE)
				cerr << "locateVideoFile: path length > _MAX_PATH" << endl;
			return path;
		}
	#elif defined (__linux__)
		char bufPath[PATH_MAX];
		getcwd(bufPath, PATH_MAX);
        if (strlen(bufPath) > 0) {
			currentPath = string(bufPath);
		} else {
			cerr << "locateVideoFile: error reading current directory" << endl;
			if (errno == ENAMETOOLONG)
				cerr << "locateVideoFile: path length > PATH_MAX" << endl;
            return path;
		}
	#else
		throw "unsupported OS";
	#endif
	path = currentPath;
	appendDirToPath(path, fileName);
	if (isFileExist(path)) {
		m_inVideoFilePath = path;
		return path;
	}

	// 2. check /home/app_dir
    string applicationPath = mSubject->getParam("application_path");
    path = applicationPath;
	appendDirToPath(path, fileName);
	if (isFileExist(path)) {
		m_inVideoFilePath = path;
		return path;
	}

	// file not found
    cerr << "locateVideoFile: " << fileName << " was not found in" << endl;
    cerr << "  cwd: " << currentPath << endl;
    cerr << "  ~/application_dir: "<< applicationPath << endl;
    return string("");
}


bool FrameHandler::openCapSource(bool fromFile) {
	using namespace std;

	if (fromFile) {
		string videoFile = mSubject->getParam("video_file");
		string filePath = locateFilePath(videoFile);
		if (filePath == "") 
			return false;
		bool succ = initFileReader(filePath);
		return succ;
	} else {
		bool succ = initCam(m_camProps.deviceID, m_camProps.resolutionID);
		return succ;
	}

}


bool FrameHandler::openVideoOut(std::string fileName) {
	std::string path = mSubject->getParam("application_path");
	m_videoOut.open(path + fileName, CV_FOURCC('M','P','4','V'), 10, m_frameSize);
	if (!m_videoOut.isOpened())
		return false;
	return true;
}


bool FrameHandler::segmentFrame() {
	bool isSuccess = false;

	// capture source must be initialized in order to do segementation
	if (!m_isCaptureInitialized) {
		std::cerr << "segmentFrame: capture source must be initialized" << std::endl;
		return false;
	}

	// split between file and cam capture (Win32)
	if (m_isFileCapture) {
		isSuccess = m_capture.read(m_frame);
		if (!isSuccess) {
			std::cout << "segmentFrame: cannot read frame from file" << std::endl;
			return false;
		}

	} else { // isCamCapture
		#if defined (_WIN32)
		isSuccess = m_captureWinCam->read(m_frame);
		if (!isSuccess) {
			std::cout << "segmentFrame: cannot read frame from cam" << std::endl;
			return false;
		}

		#elif defined (__linux__)
		isSuccess = m_capture.read(m_frame);
		if (!isSuccess) {
			std::cout << "segmentFrame: cannot read frame from cam" << std::endl;
			return false;
		}

		#else
			throw "unsupported OS";
		#endif
	}

	++m_frameCounter;

	// apply roi
	cv::Mat frame_roi = m_frame(m_roi);
	cv::Mat fPreProcessed, fgmask, fThresh, fMedBlur;

	// pre-processing
	cv::GaussianBlur(frame_roi, fPreProcessed, cv::Size(9,9), 2);

	// background segmentation 
	m_mog2->apply(fPreProcessed, fgmask);

	// post-processing
	cv::threshold(fgmask, fThresh, 0, 255, cv::THRESH_BINARY);	
	cv::medianBlur(fThresh, fMedBlur, 5);
	cv::dilate(fMedBlur, m_fgrMask, cv::Mat(7,7,CV_8UC1,1), cvPoint(-1,-1),1);
	return true;
}


void FrameHandler::showFrame(std::list<Track>* tracks, Inset inset) {
	// show frame counter, int font = cnt % 8;
	cv::putText(m_frame, std::to_string((long long)m_frameCounter), cv::Point(10,20), 0, 0.5, green, 1);
	cv::rectangle(m_frame, m_roi, blue);
	cv::Scalar boxColor = green;
	int line = Line::thin;

	// show tracking boxes around vehicles
	cv::Rect rec;
    std::list<Track>::iterator iTrack = tracks->begin();
    while (iTrack != tracks->end()){
		rec = iTrack->getActualEntry().rect();
		rec.x += (int)m_roi.x;
		rec.y += (int)m_roi.y;
		boxColor = red;
		line = Line::thin;
		if (iTrack->getConfidence() > 3)
			boxColor = orange;
		if (iTrack->isCounted()) {
			boxColor = green;
			line = Line::thick;
		}
		cv::rectangle(m_frame, rec, boxColor, line);
		++iTrack;
	}

	// copy inset with vehicle counter to frame
	if (inset.composedImage.data) {
		// TODO adjust copy position depending on frame size
		int yInset = m_frame.rows - inset.composedImage.rows; 
		inset.composedImage.copyTo(m_frame(cv::Rect(0, yInset, inset.composedImage.cols, inset.composedImage.rows)));
	}

	// display image
	cv::imshow(m_frameWndName, m_frame);
}

void FrameHandler::update() {
	m_camProps.deviceID = stoi(mSubject->getParam("cam_device_ID"));
	m_camProps.resolutionID = stoi(mSubject->getParam("cam_resolution_ID"));

	m_frameSize.width = stoi(mSubject->getParam("frame_size_x"));
	m_frameSize.height = stoi(mSubject->getParam("frame_size_y"));
	// region of interest
	m_roi.x = stod(mSubject->getParam("roi_x"));
	m_roi.y = stod(mSubject->getParam("roi_y"));
	m_roi.width = stod(mSubject->getParam("roi_width"));
	m_roi.height = stod(mSubject->getParam("roi_height"));
	
	// m_blobArea.min(200)		-> smaller blobs will be discarded 320x240=100   640x480=500
	// m_blobArea.max(20000)		-> larger blobs will be discarded  320x240=10000 640x480=60000
	m_blobArea.min = stoi(mSubject->getParam("blob_area_min"));
	m_blobArea.max = stoi(mSubject->getParam("blob_area_max"));
}


void FrameHandler::writeFrame() {
	m_videoOut.write(m_frame);
}

int FrameHandler::getFrameCount() {
	return m_frameCounter;
}
