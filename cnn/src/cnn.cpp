#include <iostream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>


using namespace std;
const cv::Scalar green	= cv::Scalar(0,255,0);
struct Detection {
	string label;
	float confidence;
	cv::Rect bbox;
	Detection(string _label, float _confidence, cv::Rect _bbox) :
		label(_label), confidence(_confidence), bbox(_bbox) {}

};

int main(int argc, const char* argv[]) {
	/// directories
	// caffe
	string caffeDir("D:\\Holger\\app-dev\\video-dev\\cnn\\caffe\\");
	string prototxt(caffeDir + "MobileNetSSD_deploy.prototxt.txt");
	string model(caffeDir + "MobileNetSSD_deploy.caffemodel");
	
	
	/// video input
	string videoDir("D:\\Users\\Holger\\counter\\");
	string videoFileName("traffic320x240.avi");
	string videoPath = videoDir + videoFileName;
	cv::VideoCapture cap;
	
	if (!cap.open(videoPath)) {
		cerr << "cap.open: cannot open video file" << endl;
		return -1;
	}
	
	/// set ROI
	cv::Rect roi(120, 70, 100, 100);

	cv::Mat frame;

	// still image
	string fileName;
	if (argc > 1) {
		fileName = argv[1];
	} else {
		fileName = "vlc-bus.jpg";
		//"car-toyota.jpg";//"example_05.jpg"
	}
	string imgPath = caffeDir + "images\\" + fileName;

	frame = cv::imread(imgPath);
	//cv::imshow("pic", frame);
	
	int inputSize = 200;
	cv::Mat frameResized;
	cv::resize(frame, frameResized, cv::Size(inputSize, inputSize));
	cv::imshow("resized", frameResized);

	cout << "loading neural network ..." << endl;
	cv::waitKey(300);

	/// neural network
	// define classes
	string classesTrained[] = {
		"background", 
		"aeroplane",
		"bicycle",
		"bird",
		"boat",
		"bottle", 
		"bus",
		"car",
		"cat",
		"chair",
		"cow",
		"diningtable",
		"dog",
		"horse",
		"motorbike",
		"person",
		"pottedplant",
		"sheep",
		"sofa",
		"train"};
	
	cv::dnn::Net net = cv::dnn::readNetFromCaffe(prototxt, model);
	vector<cv::String> layers = net.getLayerNames();



	cv::Mat blob = cv::dnn::blobFromImage(frameResized, 0.007843, frameResized.size(), 127.5);
	net.setInput(blob);
	cv::Mat netOutput;
	net.forward(netOutput, "detection_out");	

	vector<Detection> detections;

	// data structre of detection_output: 1x1xNx7 blob
	// N: number of detections
	// 7: [batchID, classID, confidence, left, top, right, bottom]
	int nDetections = netOutput.total() / 7;
	float* data = (float*)netOutput.data;
	for (size_t i = 0; i < netOutput.total(); i += 7) {
		float confidence = data[i + 2];
		if (confidence > 0.5) {
			int classID = (int)data[i + 1];

			int left	= (int)(data[i + 3] * frame.cols);
			int top		= (int)(data[i + 4] * frame.rows);
			int right	= (int)(data[i + 5] * frame.cols);
			int bottom	= (int)(data[i + 6] * frame.rows);
			int width	= right - left + 1;
			int height	= bottom - top + 1;

			detections.push_back(Detection(classesTrained[classID], confidence, cvRect(left, top, width, height)));
		
			cout << "class:       " << classesTrained[classID] << endl;
			cout << "confidence:  " << confidence << endl;
			cout << "coordinates: " << "[" << left << "," << top << "; " << right << "," << bottom << "]" << endl;
			cout << endl;
		}
	}
	
	for (size_t i = 0; i < detections.size(); ++i) {
		Detection detection(detections.at(i));
		cv::rectangle(frame, detection.bbox, green);
		
		stringstream label;
		int confidencePerCent = (int)(detection.confidence * 100);
		label.precision(3);
		label << detection.label << ": " << confidencePerCent << "%";
		cv::Point org(detection.bbox.x, detection.bbox.y);
		cv::putText(frame, label.str(), org, cv::FONT_HERSHEY_SIMPLEX, 1, green, 2);
	}
	
	cv::imshow("detections", frame);
	
	
	cout << "press esc on picture window to exit";
	cv::waitKey(0);
	return 0;
	

	while(true)
	{
		if (!cap.read(frame)) {
			cerr << "cap.read: cannot read frame" << endl;	
			break;
		}

		cv::rectangle(frame, roi, green);
		cv::imshow(videoFileName, frame);

		// 100px x 100px --> convert to blob
		cv::Mat frameRoi = frame(roi);
		cv::Mat blob = cv::dnn::blobFromImage(frameRoi, 0.007843, frameRoi.size(), 127.5);

		// apply network
		net.setInput(blob, "data");
		cv::Mat detection = net.forward("detection_out");

		cv::Size detSize = detection.size();


		//cv::imshow("roi", frameRoi);

		if (cv::waitKey(10) == 27) 	{
			cout << "ESC pressed -> end video processing" << endl;
			//cv::imwrite("frame.jpg", frame);
			break;
		}
	}


	cout << "press any key to exit";
	cv::waitKey(0);
	return 0;
}
