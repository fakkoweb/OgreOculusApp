#include "Camera.h"
#include <opencv2/gpu/gpu.hpp>
//using namespace cv;

string type2str(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

void getGray(const cv::Mat& input, cv::Mat& gray)
{
  const int numChannes = input.channels();
  
  if (numChannes == 3)
  {
      cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
  }
  else if (numChannes == 1)
  {
    gray = input;
  }
  else throw std::runtime_error("Number of channels for input image not supported");
}

////////////////////////////////////////////////
// Static members for handling OpenCV CUDA API:
////////////////////////////////////////////////
bool FrameCaptureHandler::isInitialized = false;
unsigned short int FrameCaptureHandler::cuda_Users = 0;
void FrameCaptureHandler::initCuda()
{
	if (!isInitialized)
	{
		cv::gpu::setDevice(0);			//set gpu device for cuda (n.b.: launch the app with gpu!)
		isInitialized = true;
	}
	cuda_Users++;
}
void FrameCaptureHandler::shutdownCuda()
{
	cuda_Users--;
	if (cuda_Users == 0 && isInitialized)
	{
		//nop
		isInitialized = false;
	}
}

FrameCaptureHandler::FrameCaptureHandler(const unsigned int input_device, Rift* const input_headset, const bool enable_AR,  const std::chrono::steady_clock::time_point syncStart_time, const unsigned short int desiredFps) : headset(input_headset), deviceId(input_device), arEnabled(enable_AR), captureStart_time(syncStart_time), fps(desiredFps)
{
	fromFile = false;

	// save handle for headset (from which poses are read)
	hmd = headset->getHandle();

	// find and read camera calibration file
	try {
		char calibration_file_name_buffer[30];
		sprintf(calibration_file_name_buffer, "camera%d_intrinsics.yml", deviceId);
		videoCaptureParams.readFromXMLFile(std::string(calibration_file_name_buffer));
	}
	catch (std::exception &ex) {
		cerr << ex.what() << endl;
		throw std::runtime_error("File not found or error in loading camera parameters for .yml file");
	}

	// make the undistorted version of camera parameters (null distortion matrix)
	videoCaptureParamsUndistorted = videoCaptureParams;
	videoCaptureParamsUndistorted.Distorsion = cv::Mat::zeros(4, 1, CV_32F);
}

FrameCaptureHandler::FrameCaptureHandler(const string& input_file, Rift* const input_headset, const bool enable_AR,  const std::chrono::steady_clock::time_point syncStart_time, const unsigned short int desiredFps) : headset(input_headset), filePath(input_file), arEnabled(enable_AR), captureStart_time(syncStart_time), fps(desiredFps)
{
	fromFile = true;

	// save handle for headset (from which poses are read)
	hmd = headset->getHandle();

	if(arEnabled)
	{
		// find and read camera calibration file
		try {
			char calibration_file_name_buffer[30];
			sprintf(calibration_file_name_buffer, "camera%d_intrinsics.yml", deviceId);
			videoCaptureParams.readFromXMLFile(std::string(calibration_file_name_buffer));
		}
		catch (std::exception &ex) {
			cerr << ex.what() << endl;
			throw std::runtime_error("File not found or error in loading camera parameters for .yml file");
		}

		// make the undistorted version of camera parameters (null distortion matrix)
		videoCaptureParamsUndistorted = videoCaptureParams;
		videoCaptureParamsUndistorted.Distorsion = cv::Mat::zeros(4, 1, CV_32F);		
	}

}

// Spawn capture thread and return webcam aspect ratio (width over height)
float FrameCaptureHandler::startCapture()
{
	// Init Cuda for elaboration
	initCuda();

	// Init device for capture
	if(fromFile)
	{
		videoCapture.open(filePath);
		videoCapture.set(CV_CAP_PROP_FOCUS, 0);
		videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 1800);
		videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 900);
		videoCapture.set(CV_CAP_PROP_FPS, 10);
	}
	else
	{
		videoCapture.open(deviceId);
		std::cout << "Camera " << deviceId << " parameters: " << std::endl
			<< "  K = " << videoCaptureParams.CameraMatrix << std::endl
			<< "  D = " << videoCaptureParams.Distorsion.t() << std::endl;
			//<< "  rms = " << rms << "\n\n";
		videoCapture.set(CV_CAP_PROP_FOURCC, CV_FOURCC('H', '2', '6', '4'));
		//videoCapture.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
		videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, FORCE_WIDTH_RESOLUTION);
		videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, FORCE_HEIGHT_RESOLUTION);
		videoCapture.set(CV_CAP_PROP_FPS, fps);		// in future: leave OpenCV to request to device frames as fast as he can (it minimizes delay and not support all range of FPSs)
													// the variable "fps" will still be used to sync the grab() calls, even though in future releases grab() should be called repeatedly and then take from those a subset dependent on FPS
		//videoCapture.set(CV_CAP_PROP_FOCUS, 0);
		//videoCapture.set(CV_CAP_PROP_EXPOSURE, ??);
	}
	
	if (!videoCapture.isOpened() || !videoCapture.read(frame.image.rgb))
	{
		std::cout << "Could not open video source! Could not retrieve first frame!";
		opening_failed = true;
		stopped = true;
	}
	else
	{
		aspectRatio = (float)frame.image.rgb.cols / (float)frame.image.rgb.rows;

		stopped = false;
		opening_failed = false;

		if(fromFile)
		{
			captureThread = std::thread(&FrameCaptureHandler::fromFileLoop, this);
			std::cout << "Capture loop for file "<< filePath <<" started." << std::endl;
		}
		else
		{
			captureThread = std::thread(&FrameCaptureHandler::captureLoop, this);
			std::cout << "Capture loop for camera "<< deviceId <<" started." << std::endl;			
		}

	}
	return aspectRatio;
}

/*
if (!frame) 
{
    printf("!!! cvQueryFrame failed: no frame\n");
    cvSetCaptureProperty(capture, CV_CAP_PROP_POS_AVI_RATIO , 0);
    continue;
} 

cam = cv2.VideoCapture(sourceVideo)            
while True:
    ret, img = cam.read()                      
    cv2.imshow('detection', img)
    print ret
    if (0xFF & cv2.waitKey(5) == 27) or img.size == 0:
        break
*/


void FrameCaptureHandler::stopCapture() {
	if (!stopped)
	{
		stopped = true;
		hasFrame = false;
		aspectRatio = 0;
		cameraCaptureRealDelayMs = 0;
		cameraCaptureManualDelayMs = 0;
		if (!opening_failed)
		{
			captureThread.join();
			videoCapture.release();
		}
		shutdownCuda();
	}
}

// CAN BE OPTIMIZED: frame struct copy between threads is a 1:1 copy (slow)
// it is not clear whether types manage copy internally in a good way (like cv:Mat does)
void FrameCaptureHandler::set(const FrameCaptureData & newFrame) {
	std::lock_guard<std::mutex> guard(mutex);
	frame = newFrame;
	hasFrame = true;
}

bool FrameCaptureHandler::hasNewFrame() {
	if (!hasFrame) {
		return false;
	}
	else return true;
}

bool FrameCaptureHandler::get(FrameCaptureData & out) {
	if (!hasNewFrame()) return false;
	std::lock_guard<std::mutex> guard(mutex);
	out = frame;
	hasFrame = false;
	return true;
}

bool FrameCaptureHandler::setCaptureSource(const unsigned int newDeviceNumber)
{
	if (stopped)
	{	
		deviceId = newDeviceNumber;
		fromFile = false;
		return true;
	}
	else return false;
}
bool FrameCaptureHandler::setCaptureSource(const string& newFilePath)
{
	if (stopped)
	{	
		filePath = newFilePath;
		fromFile = true;
		return true;
	}
	else return false;
}

/*
void FrameCaptureHandler::getCameraParameters(aruco::CameraParameters& outParameters)
{
	outParameters = videoCaptureParams;
}
void FrameCaptureHandler::getCameraParameters(aruco::CameraParameters& outParameters)
{
	outParameters = videoCaptureParamsUndistorted;
}
*/
double FrameCaptureHandler::adjustManualCaptureDelay(const short int adjustValue = 0)
{
	if (currentCompensationMode == Precise_manual)
	{
		// set manual delay compensation
		cameraCaptureManualDelayMs += adjustValue;
		if (cameraCaptureManualDelayMs > 200) cameraCaptureManualDelayMs = 50;
		else if (cameraCaptureManualDelayMs < 0) cameraCaptureManualDelayMs = 0;
	}
	return cameraCaptureManualDelayMs;
}

void FrameCaptureHandler::fromFileLoop() {

	Ogre::Quaternion noRotation = Ogre::Quaternion::IDENTITY;
	FrameCaptureData captured; // cpudst is the cv::Mat in FrameCaptureData struct

	while (!stopped)
	{
		// grab a new frame
		if (videoCapture.grab())	// grabs a frame without decoding it
		{

			cv::Mat distorted;
			// if frame is valid, decode and save it
			videoCapture.retrieve(distorted);
			// No orientation info is saved for the image
			captured.image.orientation[0] = noRotation.x;
			captured.image.orientation[1] = noRotation.y;
			captured.image.orientation[2] = noRotation.z;
			captured.image.orientation[3] = noRotation.w;
			captured.image.rgb = distorted;
			cout<<"CAPTURED "<<videoCapture.get(CV_CAP_PROP_FPS)<<endl;
			set(captured);
		}


	}
}

void FrameCaptureHandler::captureLoop() {

	cv::gpu::CudaMem src_image_pagelocked_buffer(cv::Size(FORCE_WIDTH_RESOLUTION, FORCE_HEIGHT_RESOLUTION), CV_8UC3);	//page locked buffer in RAM ready for asynchronous transfer to GPU (same color code and resolution as image!)
	cv::gpu::CudaMem dst_image_pagelocked_buffer(cv::Size(FORCE_GPU_WIDTH_RESOLUTION, FORCE_GPU_HEIGHT_RESOLUTION), CV_8UC3);
	cv::Mat cpusrc = src_image_pagelocked_buffer;
	cv::Mat fx = dst_image_pagelocked_buffer;
	cv::gpu::Stream image_processing_pipeline;
	cv::gpu::GpuMat gpusrc, gpudst;
	FrameCaptureData captured; // cpudst is the cv::Mat in FrameCaptureData struct
	aruco::MarkerDetector videoMarkerDetector;
	std::vector<aruco::Marker> markers;

	Ogre::Quaternion noRotation = Ogre::Quaternion::IDENTITY;
	captured.image.orientation[0] = noRotation.x;
	captured.image.orientation[1] = noRotation.y;
	captured.image.orientation[2] = noRotation.z;
	captured.image.orientation[3] = noRotation.w;

	// TIME VARIABLES FOR MANUAL CAPTURING TIME
	//int fps = 30;		// FPS is set in constructor
	std::chrono::duration< double, std::micro > frame_delay;
	if(fps<=0) frame_delay = std::chrono::duration< double, std::micro >::zero();
	else frame_delay = std::chrono::microseconds(1000000/fps);
    // Time structures for jitter/delay removal
    std::chrono::steady_clock::time_point frameStart_time;
    std::chrono::steady_clock::time_point frameEnd_time;
    bool lateOnSchedule = false;			// flag to change behaviour when in need to sync the thread with captureStart_time
    std::chrono::duration< double, std::micro > wakeup_jitter = std::chrono::duration< double, std::micro >::zero();
    std::chrono::duration< double, std::micro > needed_sleep_delay = std::chrono::duration< double, std::micro >::zero();
    std::chrono::duration< double, std::micro > computation_delay = std::chrono::duration< double, std::micro >::zero();

	// START CAPTURE LOOP!
	frameStart_time = captureStart_time;	// force start capture time (first loop won't make sense but the following loops will stay in sync)
	while (!stopped) {

		//if (videoCapture.set(CV_CAP_PROP_EXPOSURE, 0)) cout << f << endl;
		//f = f + 1.0f;
		//cout << videoCapture.get(CV_CAP_PROP_EXPOSURE) << endl;

		//Save time point for request time of the frame (for camera frame rate calculation)
		camera_last_frame_request_time = std::chrono::steady_clock::now();	//GLOBAL VARIABLE	
		//std::cout << "Retrieving frame from " << deviceId << " ..." << std::endl;
		
		// save tracking state before grabbing a new frame
		// grab() will ALWAYS return a frame OLDER than time of its call..
		// so "cameraCaptureDelayMs" is used to predict a PAST pose relative to this moment
		// LOCAL OCULUSSDK HAS BEEN TWEAKED TO "PREDICT IN THE PAST" (extension of: ovrHmd_GetTrackingState)
		double ovrTimestamp = ovr_GetTimeInSeconds();	// very precise timing! - more than ovr_GetTimeInMilliseconds()
		ovrTrackingState tracking;
		switch (currentCompensationMode)
		{
		case None:
			// No orientation info is saved for the image
			captured.image.orientation[0] = noRotation.x;
			captured.image.orientation[1] = noRotation.y;
			captured.image.orientation[2] = noRotation.z;
			captured.image.orientation[3] = noRotation.w;
			break;
		case Approximate:
			// Just save pose for the image before grabbing a new frame
			tracking = ovrHmd_GetTrackingState(hmd, ovrTimestamp);
			break;
		case Precise_manual:
			// Save the pose keeping count of grab() call delay (manually set)
			// Version of OCULUSSDK included in this project has been tweaked to "PREDICT IN THE PAST"
			tracking = ovrHmd_GetTrackingStateExtended(hmd, (ovrTimestamp - (cameraCaptureManualDelayMs/1000) ));	// Function wants double in seconds
			break;
		case Precise_auto:
			// Save the pose keeping count of grab() call delay (automatically computed)
			// Version of OCULUSSDK included in this project has been tweaked to "PREDICT IN THE PAST"
			tracking = ovrHmd_GetTrackingStateExtended(hmd, (ovrTimestamp - (cameraCaptureRealDelayMs/1000) ));		// Function wants double in seconds
			break;
		default:
			// If something goes wrong in mode selection, disable compensation.
			currentCompensationMode = None;
			break;
		}
				
		// grab a new frame
		if (videoCapture.grab())	// grabs a frame without decoding it
		{
			if (currentCompensationMode == Precise_auto)
			{
				// try to real timestamp when frame was captured by device
				double realTimestamp = videoCapture.get(CV_CAP_PROP_POS_MSEC);
				if (realTimestamp != -1)
				{
					// compute grab() call delay compensation
					cameraCaptureRealDelayMs = (ovrTimestamp/1000) - realTimestamp;

					// Computed value will be used for next frame pose prediction.
					// Explanation:
					// We already know that the frame will be older than the grab() call, so
					// we save ovrTimestamp before it, at a time closer to the real frame capture time.
					// BUT
					// Is not convenient to compute tracking now using this ovrTimestamp. This is because
					// prediction may be too far in the past since there is a wait for the grab() call
					// between ovrTimestamp and tracking computation.
					// The prediction amount would be: (now - ovrTimestamp) + (ovrTimestamp - realTimestamp)
					// Instead, we compute "tracking" before it, right when ovrTimestamp is requested.
					// This way prediction amount is: ovrTimestamp - realTimestamp
					//
					// Since cameraCaptureDelayMs is almost constant in time, it is not a big deal when
					// it is used (it could be computed just once and it would also be ok)

				}
				// else degenerate to manual mode
				else
				{
					cameraCaptureRealDelayMs = 0;
					currentCompensationMode = Precise_manual;
					std::cout << "Precise_Auto mode unsupported (OpenCV returned -1 on timestamp request). Switching to Precise_Manual mode." << std::endl;
				}
			}
			
			

			cv::Mat distorted, undistorted;
			// if frame is valid, decode and save it
			videoCapture.retrieve(distorted);
			// USE THIS LINE TO UNDERSTAND WHICH IMAGE TYPE IS RETURNED BY YOUR videoCapture
			//std::cout<< type2str(distorted.type()) <<std::endl;
			// THEN USE THIS TYPE FOR ANY OPERATION ON THE RETRIEVED IMAGE

			// perform undistortion (with parameters of the camera)
			if(undistort)
				cv::undistort(distorted, undistorted, videoCaptureParams.CameraMatrix, videoCaptureParams.Distorsion);
			else
				undistorted = distorted;

			cpusrc = undistorted;
			// GPU ASYNC OPERATIONS
			// -------------------------------
			// Load source image to pipeline
			bool toonActive = toon;
			if(toonActive)
			{
				image_processing_pipeline.enqueueUpload(cpusrc, gpusrc);
				// Other elaboration on image
				// - - - PUT IT HERE! - - -
				// TOON in GPU - from: https://github.com/BloodAxe/OpenCV-Tutorial/blob/master/OpenCV%20Tutorial/CartoonFilter.cpp
				// RESIZED TO REDUCE CALCULATIONS IN DEMO 2!!
				cv::gpu::GpuMat gpusrc_scaled, gpusrc_a, bgr, bgr_a, gray, edges, edgesBgr;
				cv::gpu::resize(gpusrc, gpusrc_scaled, cv::Size(FORCE_GPU_WIDTH_RESOLUTION, FORCE_GPU_HEIGHT_RESOLUTION));
				cv::gpu::cvtColor(gpusrc_scaled,gpusrc_a, CV_BGR2BGRA );	// hack: meanShiftFiltering for now supports only CV_8UC4!
			    cv::gpu::meanShiftFiltering(gpusrc_a, bgr_a, 15, 40);
			    cv::gpu::cvtColor(bgr_a, gray, cv::COLOR_BGRA2GRAY);		// hack: is BGRA2GRAY instead of BGR2GRAY for the same reason
			    cv::gpu::Canny(gray, edges, 150, 150);
			    cv::gpu::cvtColor(edges, edgesBgr, cv::COLOR_GRAY2BGR);
			    cv::gpu::cvtColor(bgr_a, bgr, cv::COLOR_BGRA2BGR);			// hack: I need the BGR version from alpha result
			    cv::gpu::subtract(bgr, edgesBgr, gpudst);					// gpudst = bgr - edgesBgr;
				// Download final result image to ram
				image_processing_pipeline.enqueueDownload(gpudst, fx);
			}

			// CPU SYNC OPERATIONS
			// -------------------------------		
			// AR detection operation
			if (arEnabled)
			{
				// clear previously captured markers
				captured.markers.clear();
				// detect markers in the image
				videoMarkerDetector.detect(undistorted, markers, videoCaptureParamsUndistorted, 0.1f);	//need marker size in meters
				// show nodes for detected markers
				for (unsigned int i = 0; i<markers.size(); i++) {
					ARCaptureData new_marker;
					markers[i].OgreGetPoseParameters(new_marker.position, new_marker.orientation);
					captured.markers.insert(captured.markers.begin(), new_marker);
					std::cout << "marker " << i << " detected." << endl;
				}
			}
			// finally save pose as well (previously computed)
			if (currentCompensationMode != None)
			{
				if (tracking.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
					Posef pose = tracking.HeadPose.ThePose;		// The cpp compatibility layer is used to convert ovrPosef to Posef (see OVR_Math.h)
					//captured.image.orientation = Ogre::Quaternion(pose.Rotation.w, pose.Rotation.x, pose.Rotation.y, pose.Rotation.z);
					captured.image.orientation[0] = pose.Rotation.w;
					captured.image.orientation[1] = pose.Rotation.x;
					captured.image.orientation[2] = pose.Rotation.y;
					captured.image.orientation[3] = pose.Rotation.z;
				}
				else
				{
					// use last predicted/saved pose
					//std::cerr << "tracking info not available" << std::endl;
				}
			}
			// TEST FX on CPU
			/*
			if(toonActive)
			{
				// TOON - https://github.com/BloodAxe/OpenCV-Tutorial/blob/master/OpenCV%20Tutorial/CartoonFilter.cpp
				cv::Mat bgr, gray, edges, edgesBgr;
			    cv::cvtColor(undistorted, bgr, cv::COLOR_BGRA2BGR);
			    cv::pyrMeanShiftFiltering(bgr.clone(), bgr, 15, 40);
			    getGray(bgr, gray);
			    cv::Canny(gray, edges, 150, 150);
			    cv::cvtColor(edges, edgesBgr, cv::COLOR_GRAY2BGR);
			    bgr = bgr - edgesBgr;
			    cv::cvtColor(bgr, fx, cv::COLOR_BGR2BGRA);
		    }
		    else fx = undistorted;
		    */

			// wait for gpu pipeline to end
			image_processing_pipeline.waitForCompletion();
			// -------------------------------					

			// show image with or without fx?
			if(toonActive)
				captured.image.rgb = fx;
			else
				captured.image.rgb = undistorted;
			// set the new capture as available (result of both gpu/cpu operations)
			set(captured);
			//std::cout << "Frame retrieved from " << deviceId << "." << std::endl;

			//std::cout.precision(20);
			//std::cout << "ovr before grab: " << ovrTimestamp <<"\n opencv after grab: "<<opencvTimestamp<<std::endl;
		}
		else
		{
			std::cout << "FAILED to retrieve frame from "<< deviceId <<"." << std::endl;
		}



		//JITTER AND DELAY CALCULATION -- WARNING: REMOVAL IS DEACTIVATED (see below why)!
		//-----------------------------------------
		//END: save now() as last render loop ends
		frameEnd_time = std::chrono::steady_clock::now();
				//cout<< "computation time delay: "<<std::chrono::duration_cast<std::chrono::microseconds>(frameEnd_time - frameStart_time).count()<<endl;			
		//SLEEP for the next frame_delay, reduced by the last wakeup_jitter and computation time this loop took
		computation_delay = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd_time - frameStart_time);
		needed_sleep_delay = frame_delay - computation_delay - wakeup_jitter;
				//cout<< "new nominal wake-up delay: "<<std::chrono::duration_cast<std::chrono::microseconds>(needed_sleep_delay).count()<<endl;
				//cout<<"------"<<endl;
		if(needed_sleep_delay.count()<0)	//the loop is late on time schedule -> change behaviour to keep in sync with captureStart_time: the closest frameStart_time on schedule will be chosen as the new one 
		{
			lateOnSchedule = true;
			// Reset variables for sleep (thread will continue running with no sleep to help recover the time lost)
			needed_sleep_delay = std::chrono::duration< double, std::micro >::zero();
			wakeup_jitter = std::chrono::duration< double, std::micro >::zero();
			std::cout<<"Warning: camera "<<deviceId<<" capture was late on schedule. It took more than "<<(1000000/fps)<<" microseconds to execute."<<std::endl;
			// Compute run-out skew of the thread in respect to schedule frequency (it will be the difference between the last frameEnd_time and the closest oldest frameStart_time on the ideal schedule)
			int skew_micros = std::chrono::duration_cast<std::chrono::milliseconds>(computation_delay).count() % std::chrono::duration_cast<std::chrono::milliseconds>(frame_delay).count();
			std::chrono::duration< double, std::micro > skew = std::chrono::microseconds(skew_micros);
			// Compute the two closest ideal frameStart_time (the previous and the next)
			std::chrono::steady_clock::time_point frameStart_time_ideal_prev = frameEnd_time - std::chrono::duration_cast<std::chrono::microseconds>(skew);						// previous as current end time - skew
			std::chrono::steady_clock::time_point frameStart_time_ideal_next = frameStart_time_ideal_prev + std::chrono::duration_cast<std::chrono::microseconds>(frame_delay);	// next as previous + ideal frame delay
			// Check to which frameEnd_time is closest and use the result as next frameStart_time
			int window_micros = std::chrono::duration_cast<std::chrono::milliseconds>(frame_delay).count();
			if( skew_micros > (window_micros/16) )  			// RIGHT NOW IS BIASED TO CHOOSE THE NEXT! CHANGE THIS CHECK FORMULA TO CHANGE BEHAVIOUR!! (ex. skew_micros > (window_micros/2))
			{
				frameStart_time = frameStart_time_ideal_next;	// selected from 1/16 of of the frame_delay ideal window to its end
			}
			else
			{
				frameStart_time = frameStart_time_ideal_prev;	// selected when still in the first 1/16 of the frame_delay ideal window
			}
			// TO TWEAK CONSIDER THAT:
			// If the previous time is selected, thread will still have less than a frame_delay to finish, it will take more converge to schedule, but will retrieve next frames as soon as possible with no pause.
			// If next time is selected, thread will have more than a frame_delay to finish, so it will start on perfect schedule from the next loop. However it will have one pause of more than one frame.
			// N.B. It does not matter if the two camera threads will choose differently: they will eventually converge in the same schedule.
			// N.B. If a camera thread is ALWAYS exceeding the time, sync cannot converge with this algorithm!!
		}
		else
		{
			// SLEEP COMMENTED OUT: WHY?
			// Whenever we need the frames to not exceed the specified amount, it is ok to put thread to sleep (as we do for Ogre to crop to 60fps and spare CPU cycles).
			// However, for camera thread, two considerations must be taken:
			// -  OpenCV ALREADY controls FPS when opening device and ALREADY puts the current thread to sleep when calling grab() to get the right framerate
			// -  This thread needs all the time possible: if we put it to sleep, it will never wake up at the exact time he should, so some time will always be wasted (wakeup_jitter)
			// If the thread has free time, it will try to call the grab() again, which will return with the right frequency. This is different from Ogre, which will always try to 
			// render more and more frames indefinitely, which is useless and consumpt a lot of cpu cycles!!
			
			/*
			#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	        	Sleep( std::chrono::duration_cast<std::chrono::milliseconds>(needed_sleep_delay).count() );
			#else
	        	usleep( std::chrono::duration_cast<std::chrono::microseconds>(needed_sleep_delay).count() );
				//sleep(fps/1000000); // this would be ok if we ignored computation time and wakeup_jitter
			#endif
			*/


			//BEGIN: save now() as the time loop begins
			frameStart_time = std::chrono::steady_clock::now();
			//compute the jitter as the time it took this thread to wake-up since the time it had to wake up (in ideal world, wake_jitter would be 0...)
			//if no sleep is performed, wakeup_jitter can be very small or zero, but never negative
			wakeup_jitter = std::chrono::duration_cast<std::chrono::microseconds>(frameStart_time - frameEnd_time) - needed_sleep_delay;
				//cout<<"------"<<endl;
				//cout<< "nominal wake-up delay was: "<<std::chrono::duration_cast<std::chrono::microseconds>(needed_sleep_delay).count()<<endl;			// declared sleep time
				//cout<< "real wake-up delay: "<<std::chrono::duration_cast<std::chrono::microseconds>(frameStart_time - frameEnd_time).count()<<endl;		// effective slept time
				//cout<< "there was a wakeup jitter of : "<<std::chrono::duration_cast<std::chrono::microseconds>(wakeup_jitter).count()<<endl;				// computed jitter	
		}

		//cv::flip(captured.image.clone(), captured.image, 0);
		/*
		auto camera_last_frame_request_time_since_epoch = camera_last_frame_request_time.time_since_epoch();
		std::cout << "Time differences in implementations"
		<< "\n1. OpenCV Timestamp ms: " << (long)videoCapture.get(CV_CAP_PROP_POS_MSEC)
		<< "\n2. Chrono TimeSinceEpoch ms " << std::chrono::duration_cast<std::chrono::milliseconds>(camera_last_frame_request_time_since_epoch).count()
		<< "\n3. OculusSDK Timestamp ms " << captureTime << std::endl;
		*/
	}
}
