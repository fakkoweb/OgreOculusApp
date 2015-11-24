#ifndef GLOBALS_H
#define GLOBALS_H

// LOWER RESOLUTION FOR THIS DEMO!!
#define FORCE_WIDTH_RESOLUTION	640 //suggested max: 1024
#define FORCE_HEIGHT_RESOLUTION	360	//suggested max: 576
#define FORCE_3D_RENDERING_FPS	60  //?? 640x360 for gooch

#include <chrono>

extern bool ROTATE_VIEW;
//extern bool NO_RIFT;
extern bool DEBUG_WINDOW;
extern unsigned short int CAMERA_BUFFERING_DELAY;
extern int CAMERA_TOEIN_ANGLE;
extern int CAMERA_KEYSTONING_ANGLE;
extern bool undistort, toon;	//temp variables used for demo
extern bool noStab;				//temp ""
//Globals used from Camera.cpp and App.cpp
extern std::chrono::steady_clock::time_point camera_last_frame_request_time;
extern std::chrono::duration< int, std::milli > camera_last_frame_display_delay;


#endif
