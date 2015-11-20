#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>
#include <aruco.h>
#include <thread>
#include <mutex>
#include "Rift.h"
#include "OVR.h"
#include "OGRE/Ogre.h"
#include "Globals.h"

struct ImageCaptureData
{
	cv::Mat rgb;
	double orientation[4];
};

struct ARCaptureData
{
	double position[3];
	double orientation[4];
};

struct FrameCaptureData {
	ImageCaptureData image;
	std::vector<ARCaptureData> markers;
};

class FrameCaptureHandler
{
	public:
		enum CompensationMode
		{
			None,
			Approximate,
			Precise_manual,
			Precise_auto
		};

	private:
		const unsigned int deviceId = 0;
		cv::VideoCapture videoCapture;
		std::thread captureThread;
		std::mutex mutex;
		FrameCaptureData frame;
		float aspectRatio = 0;
		bool arEnabled = false;

		static bool isInitialized;
		static unsigned short int cuda_Users;
		static void initCuda();
		static void shutdownCuda();

		bool hasFrame = false;
		bool stopped = true;
		bool opening_failed = false;

		// Explanation:
		// Usually time between "frame is captured by camera" and "frame is returned by OpenCV grab()" is more than 0
		// It keeps approximately constant over time, but changes user by user, run after run.
		// This values (in milliseconds) are meant to match this time and counter this unwanted effect.
		// Usage:
		// If OpenCV and your camera support timestamping, cameraCaptureRealDelayMs is automatically set.
		// Otherwise cameraCaptureManualDelayMs is used and can be adjusted manually with adjustManualCaptureDelay()
		double cameraCaptureRealDelayMs = 0;		// Automatically computed.
		double cameraCaptureManualDelayMs = 0;		// Clamped between 0 and 50.
		short unsigned int fps;
		std::chrono::steady_clock::time_point captureStart_time;

		Rift* headset = nullptr;
		ovrHmd hmd = nullptr;

		CompensationMode currentCompensationMode = Precise_manual;

		// Internal capture functions
		void set(const FrameCaptureData & newFrame);
		void captureLoop();

	public:

		FrameCaptureHandler(const unsigned int input_device, Rift* const input_headset, const bool enable_AR, const std::chrono::steady_clock::time_point syncStart_time, const unsigned short int desiredFps);

		// Spawn capture thread and return webcam aspect ratio (width over height)
		float startCapture();
		void stopCapture();

		// Get data
		bool hasNewFrame();
		bool get(FrameCaptureData & out);
		float getAspectRatio(){ return aspectRatio; }
		//void getCameraParameters(aruco::CameraParameters& outParameters);
		//void getCameraParametersUndistorted(aruco::CameraParameters& outParameters);
		aruco::CameraParameters videoCaptureParams, videoCaptureParamsUndistorted;	// only dependency from aruco. Remove them?

		// Call this to change cameraCaptureManualDelay.
		// Value is not used if currentCompensationMode != Precise_manual
		// adjustValue can be negative or positive.
		// It returns the modified value. 0 returns the current value.
		double adjustManualCaptureDelay(const short int adjustValue);

		void setCompensationMode(const CompensationMode newMode){ currentCompensationMode = newMode; }

};

