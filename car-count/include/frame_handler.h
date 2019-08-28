#pragma once
#include <memory> // unique_ptr
#if defined (_WIN32)
#if (_MSC_VER == 1600)
#pragma warning(disable: 4996) // MSVC: crt secure warnings
#pragma warning(disable: 4482) // MSVC10: enum nonstd extension
#endif
#include "cam_cap_dshow.h"
#endif
#include "config.h"
#include "recorder.h"


//////////////////////////////////////////////////////////////////////////////
// Types /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
enum class Align {left, right};		// C++11: enum class Align {left, right};
enum Line { thin=1, thick=2};

typedef cv::Rect_ <double> Rect2d; // TODO delete, if opencv > v3.0 

const cv::Scalar black			= cv::Scalar(0,0,0);
const cv::Scalar blue			= cv::Scalar(255,0,0);
const cv::Scalar cyan			= cv::Scalar(255,255,0);
const cv::Scalar red			= cv::Scalar(0,0,255);
const cv::Scalar gray			= cv::Scalar(128,128,128);
const cv::Scalar gray_light		= cv::Scalar(192,192,192);
const cv::Scalar green			= cv::Scalar(0,255,0);
const cv::Scalar magenta		= cv::Scalar(255,0,255);
const cv::Scalar magenta_dark	= cv::Scalar(192,0,192);
const cv::Scalar orange			= cv::Scalar(0,128,255);
const cv::Scalar purple			= cv::Scalar(128,0,128);
const cv::Scalar yellow			= cv::Scalar(0,255,255);
const cv::Scalar white			= cv::Scalar(255,255,255);


//////////////////////////////////////////////////////////////////////////////
// Arrow /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Arrow {
public:
	Arrow(cv::Point start = cv::Point(0,0), cv::Point end = cv::Point(0,0),
		cv::Scalar color = green, int thickness = 1);
	void put(cv::Mat& image);
private:
	cv::Point	m_start;
	cv::Point	m_end;
	cv::Scalar	m_color;
	int			m_thickness;
};


//////////////////////////////////////////////////////////////////////////////
// Font //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
struct Font {
	Font();
	int			face;
	double		scale;
	cv::Scalar	color;
	int			thickness;
};


//////////////////////////////////////////////////////////////////////////////
// TextRow ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class TextRow {
public:
			TextRow(const int orgY = 0, const Font font = Font());
	int		getWidth();
	void	setIndent(const Align rowAlign, const int colWidth);
	int		setText(const std::string& text, const int colWidth);
	void	put(cv::Mat& frame, cv::Point colOrigin);
private:
	Font		m_font;
	cv::Point	m_origin;
	std::string m_text;
};


//////////////////////////////////////////////////////////////////////////////
// TextColumn ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class TextColumn {
public:
	TextColumn(const cv::Point origin = cv::Point(0,0), const int colWidth = 0,
		const Align colAlign = Align::left);
	int		addRow(const int originY, const Font font = Font());
	void	alignRows(const Align rowAlign);
	bool	removeRow(const int rowIdx);
	void	resize(const cv::Point origin, const int colWidth);
	bool	setRowText(const int rowIdx, const std::string& text);
	void	put(cv::Mat& image);
private:
	typedef		std::vector<TextRow> RowArray;
	Align		m_colAlign;
	int			m_colWidth;
	int			m_maxTextWidth;
	cv::Point	m_origin;
	RowArray	m_rowArray;
};


//////////////////////////////////////////////////////////////////////////////
// Inset /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
struct Inset {
				Inset(cv::Mat image);
	Arrow		createArrow(const Align pointsTo);
	TextColumn	createTextColumn(const  Align colAlign);
	void		putCount(CountResults cr);
	cv::Mat		backGrndImage;
	cv::Mat		composedImage;
	Arrow		arrowLeft;
	Arrow		arrowRight;
	TextColumn	textLeft;
	TextColumn	textRight;
};


//////////////////////////////////////////////////////////////////////////////
// FrameHandler //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class FrameHandler : public Observer {
public:
	FrameHandler(Config* pConfig);
	void					adjustFrameSizeDependentParams(int new_size_x, int new_size_y); 
	std::list<cv::Rect>&	calcBBoxes();
	Inset					createInset(std::string insetFilePath);
	int						getFrameInfo(); // #channels, depth, type
	cv::Size2d				getFrameSize();
	bool					initCam(int camID = 0, int camResoutionID = 0);
	bool					initFileReader(std::string videoFilePath);
	std::string				locateFilePath(std::string fileName);
	bool					openCapSource(bool fromFile = true); // wraps initCam() and initFileReader()
	bool					openVideoOut(std::string fileName);
	bool					segmentFrame();
    void					showFrame(std::list<Track>* tracks, Inset inset);
    void					update(); // updates observer with subject's parameters (Config)
	void					writeFrame();
	// DEBUG
	int						getFrameCount();

private:
	// as of opencv 3.0 call createBackgroundSubtractorMOG2() in order to create instance
	// function returns smart pointer to instance
	typedef cv::Ptr<cv::BackgroundSubtractorMOG2> PtrBgrndSubtr;
	std::list<cv::Rect>		m_bBoxes; // bounding boxes of newly detected objects
	struct AreaLimits {
		int min;
		int max; }				m_blobArea;
	struct CamProps {
		int deviceID;
		int resolutionID; }		m_camProps;
	cv::VideoCapture			m_capture;
#if defined (_WIN32)
	std::unique_ptr<CamInput>	m_captureWinCam; // substitute for cv::VideoCapture
#endif

    std::string					m_inVideoFilePath;
	cv::Mat						m_frame;	// frame, read from camera or file
	cv::Mat						m_fgrMask;	// foreground mask of moving objects
	int							m_frameCounter; 
	cv::Size2d					m_frameSize;
	std::string					m_frameWndName;
	//Inset						m_inset;
	bool						m_isCaptureInitialized;
	bool						m_isFileCapture; 
	PtrBgrndSubtr				m_mog2;		// pointer to background subtractor MOG2 (mixuture of gaussians)
	Rect2d						m_roi;		// region of interest, within framesize
	cv::VideoWriter				m_videoOut;
};
