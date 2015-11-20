#ifndef GLOBALS_H
#define GLOBALS_H

#include <chrono>

extern bool ROTATE_VIEW;
//extern bool NO_RIFT;
extern bool DEBUG_WINDOW;
extern unsigned short int CAMERA_BUFFERING_DELAY;
extern int CAMERA_ROLL_LEFT;
extern int CAMERA_ROLL_RIGHT;
extern bool undistort, toon;
//Globals used from Camera.cpp and App.cpp
extern std::chrono::steady_clock::time_point camera_last_frame_request_time;
extern std::chrono::duration< int, std::milli > camera_last_frame_display_delay;


#endif
