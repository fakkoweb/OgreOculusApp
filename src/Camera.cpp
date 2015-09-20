#include "Camera.h"


FrameCaptureHandler::FrameCaptureHandler(const unsigned int input_device, Rift* const input_headset, const bool enable_AR = false) : headset(input_headset), deviceId(input_device), arEnabled(enable_AR)
{
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

// Spawn capture thread and return webcam aspect ratio (width over height)
float FrameCaptureHandler::startCapture()
{

	videoCapture.open(deviceId);
	if (!videoCapture.isOpened() || !videoCapture.read(frame.image.rgb))
	{
		std::cout << "Could not open video source! Could not capture first frame!";
		opening_failed = true;
		stopped = true;
	}
	else
	{
		std::cout << "Camera " << deviceId << " parameters: " << std::endl
			<< "  K = " << videoCaptureParams.CameraMatrix << std::endl
			<< "  D = " << videoCaptureParams.Distorsion.t() << std::endl;
			//<< "  rms = " << rms << "\n\n";
		//videoCapture.set(CV_CAP_PROP_FOURCC, CV_FOURCC('H', '2', '6', '4'));
		videoCapture.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
		videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 1024);
		videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 576);
		videoCapture.set(CV_CAP_PROP_FPS, 30);
		videoCapture.set(CV_CAP_PROP_FOCUS, 0);
		//videoCapture.set(CV_CAP_PROP_EXPOSURE, ??);
		aspectRatio = (float)frame.image.rgb.cols / (float)frame.image.rgb.rows;
		stopped = false;
		opening_failed = false;
		captureThread = std::thread(&FrameCaptureHandler::captureLoop, this);
		std::cout << "Capture loop for camera " << deviceId << " started." << std::endl;
	}
	return aspectRatio;
}

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
	}
}

// CAN BE OPTIMIZED: frame struct copy between threads is a 1:1 copy (slow)
// it is not clear whether types manage copy internally in a good way (like cv:Mat does)
void FrameCaptureHandler::set(const FrameCaptureData & newFrame) {
	std::lock_guard<std::mutex> guard(mutex);
	frame = newFrame;
	hasFrame = true;
}

bool FrameCaptureHandler::get(FrameCaptureData & out) {
	if (!hasFrame) {
		return false;
	}
	std::lock_guard<std::mutex> guard(mutex);
	out = frame;
	hasFrame = false;
	return true;
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

void FrameCaptureHandler::captureLoop() {
	FrameCaptureData captured;
	Ogre::Quaternion noRotation = Ogre::Quaternion::IDENTITY;
	captured.image.orientation[0] = noRotation.x;
	captured.image.orientation[1] = noRotation.y;
	captured.image.orientation[2] = noRotation.z;
	captured.image.orientation[3] = noRotation.w;
	//double f = 0;
	while (!stopped) {

		//if (videoCapture.set(CV_CAP_PROP_EXPOSURE, 0)) cout << f << endl;
		//f = f + 1.0f;
		//cout << videoCapture.get(CV_CAP_PROP_EXPOSURE) << endl;

		//Save time point for request time of the frame (for camera frame rate calculation)
		camera_last_frame_request_time = std::chrono::system_clock::now();	//GLOBAL VARIABLE
		
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
			
			cv::Mat distorted;
			// if frame is valid, decode and save it
			videoCapture.retrieve(captured.image.rgb);
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
					std::cerr << "tracking info not available" << std::endl;
				}
			}

			/*
			// perform undistortion (with parameters of the camera)
			cv::undistort(distorted, captured.image.rgb, videoCaptureParams.CameraMatrix, videoCaptureParams.Distorsion);

			// [AR] Operations
			if (arEnabled)
			{

				// detect markers in the image
				aruco::MarkerDetector videoMarkerDetector;
				std::vector<aruco::Marker> markers;
				videoMarkerDetector.detect(captured.image.rgb, markers, videoCaptureParamsUndistorted, 0.056f);	//need marker size in meters
				// show nodes for detected markers
				captured.markers.clear();
				for (unsigned int i = 0; i<markers.size(); i++) {
					ARCaptureData new_marker;
					markers[i].OgreGetPoseParameters(new_marker.position, new_marker.orientation);
					captured.markers.insert(captured.markers.begin(),new_marker);
					std::cout << "marker " << i << " detected." << endl;
				}
			}
			*/
			

			// set the new capture as available
			set(captured);
			//std::cout << "Frame retrieved from " << deviceId << "." << std::endl;

			//std::cout.precision(20);
			//std::cout << "ovr before grab: " << ovrTimestamp <<"\n opencv after grab: "<<opencvTimestamp<<std::endl;
		}
		else
		{
			std::cout << "FAILED to retrieve frame from " << deviceId << "." << std::endl;
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
