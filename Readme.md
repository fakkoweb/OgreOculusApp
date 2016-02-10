# Immersive stereo computer vision and augmented reality with Oculus Rift and Ogre3D

## Overview

This project is the result of a Master Thesis work. A very simplified presentation of this work is publicly available in Prezi, which embeds also videos of implemented features on a working demo. The thesis transcript itself has yet to be uploaded.
- **[Work presentation][prezipresentation]**
- **[Some live impressions at CVAP lab in KTH][cvapvideo]**

The code provides an implementation of a 3D scene in Ogre3D (version 1.9) containing:
- a very simple environment, with a sample Skybox and a floor
- a virtual stereo camera, to explore the environment with Oculus Rift (at this point tested with Orientation and Timewarping)
- a plane/sphere projection mesh, on which images from a camera can be displayed per each eye with the correct mapping into each eye, to support video see-through with any camera model and for zero-parallax plane adjustments by moving the meshes
- a virtual object (a 3-axis mesh is used here) which can be positioned in the scene between camera and projection mesh to perform correct stereo augmented reality (which is limited by how cameras are positioned in respect to the eye)

Moreover, a pipeline capable of synchronizing images from both cameras, performing augmented reality on one of them and applying some image filter with CUDA support is proposed (in the example, image is transformed in a comicbook-like version. In this version, undistort is not performed on each image since ArUco was capable to work well with raw images. We would suggest to implement all computer vision algorithms in pure CUDA, while as of now only OpenCV effects are.

The challenge of the project is to be able to show any image with any elaboration on it WITHOUT impacting on the 3D scene framerate: you can experiment with computer vision (or stereo computer vision, if you implement it) and its performance on video without reducing the fluency of head movements in the 3D scene. The code is set for wide-angle images (square-like) but can be easily altered for fish-eye images (circle-like).

Hardware used was both Oculus Rift DK1 and DK2 and a couple of Logitech C920-C cameras (autofocus and linux support, most stable 30fps framerate achievable by a 1080p consumer webcam), mounted in front of each eye and slightly toed-in (I know this should not be done, but the reason was to get stereo for very close objects in view).

The project itself can be used as a SUBPROJECT (in cmake slang) meaning that in this case my cmake file will no longer compile main.cpp but create a static library instead. At this moment, methods and classes and interfaces should be improved to make it easier to understand and usable as an external library.

### Bit of history and acknowledgements

The idea comes originally from (and was proposed as a Master Thesis by) Alessandro Pieropan, Johan Ekenkrantz and Carl Henrik Ek, from CVAP department at Stockholm KTH. The goal was to develop a video see-through augmented reality headset and a demo capable of showing its capabilities of performing computer vision in real time. I am a student from Politecnico di Torino who was eager to learn much on this field, merely starting from basics of computer graphics. Most of the work focused on achieving the high performance needed and displaying the stereo image pairs correctly, facing the challenges of video see-through stereo and geometry issues on how images and stereo are percieved by the human eye. The result is partially satisfactory on the stereo side (due to geometrical limitations and needed custom adjustments that depend on the setup) but we reached all goals on all other aspects (as much as the hardware used allowed.

The code starts from a fork of OgreOculusSample, available here: https://github.com/Germanunkol/OgreOculusSample. A more detailed explanation of which features where included and why can be found in its README. They were then extended in this project, by adding tons of features (most of them listed in the TODO list above). Also the code has been throughly commented, so hopefully you can easily understand what each non-trivial piece of code does.

Thanks also to Jherico, Nesh88, Germanunkol and Oliver Kreylos for helping me solving and understanding most of the problems I had to develop a solution to.



### License
See LICENSE file



## Goals
- [Done, but not automatic yet] Rendering any mono/stereo camera image (any FOV) to any video head mounted display
- [Done, but not automatic yet] Support both pinhole and fisheye lenses and GLSL shader real-time undistortion
- [Done] Minimize latency between real image capture and virtual world render
- [Done under certain conditions] Minimize stereo discrepancy between real image capture and virtual world render
- [Done] Run real-time OpenCV/CUDA computer vision algorithms on captured images and display it into the Rift
- [Done] Perform stereo augmented reality with arUco using only one camera
- [Future development] Run real-time stereo computer vision algorithms to compute depth

## ToDo list
- [ToDo] Implement automatic extraction of full FOV data from camera calibration data
- [ToDo] Implement automatic generation of UV undistortion map from camera calibration data
- [ToDo] Implement support for stereo calibration files and stereo vision algorithms (and their synchronization)
- [ToDo] Autoadjust stereo discrepancy adjustment based on one seen marker or more markers
- [ToDo] Perfect both Windows and Linux support (works with CMake, but not perfect due to high variety of libs and Ogre files)
- [Done] Orientation, Positional tracking and Timewarp support for Oculus (any DK)
- [Done] Custom Image stabilization algorithm (will do its best with OpenCV3 and my customized version of OculusSDK included)
- [ToDo] 3D print custom stereo mount for Logitech C920-C boards and Oculus Rift (I used DK2)

## How does it work

### Explanation (to be improved)

At the point of writing, the Oculus DK1/2 works on both Linux and Windows, as this code does. The projects uses the libraries listed in "Prerequisites". The project folder structure is designed so that any library may be added as a source (in /libs) or installed both in system or locally (in /libs) so that CMake can detect it automatically, add it to the project and compile it if necessary.

At the current state, the cmake file is not perfect yet, so not all combinations are supported. As of now, arUco can only be detected as source, while the rest only if installed (precompiled), except OculusSDK which can be added in both forms. This is just for development necessities encountered during development. To facilitate compilation, OculusSDK and arUco source are already in /libs as submodules, so you really should not care much, until you want to use your own versions.

Note that my cmake file uses the version it finds first in this order:
1) source in /libs
2) installed in /libs
3) installed in system

Also some additions where necessary to work with Ogre. Paths in the .cfg files WILL BE WRITTEN by cmake depending on where you did put your Ogre installation, for both windows and linux. Awesome.



Almost the entire Rift-Interaction is done using Rift.h and Rift.cpp. Copy those into your project if you want to use them.
Getting it up and running is designed to be as simple as possible. See App::initRift() in App.cpp for an example.

- Create a Renderwindow, but DON'T ATTATCH ANY VIEWPORTS! The rift class will do this later. You can create the window directly on your second monitor (set "monitorIndex" to "1" instead of "0"), or let the user choose a screen at startup.
- Create two Cameras in your Scene, next to each other, facing in the same direction. Best practice: Create a scene node "mBodyNode" which represents the user's body. Then add a child scene node (mHeadNode) which is the head node. Let the user control the body (walking, turning) and the Rift control the head node. Attach your cameras to the head node.
- Call Rift::init() once.
- Create a new Rift and pass the Window, your ogre root and a bool to it - the bool controls whether the window should be rotated sideways (in case your Graphics Card doesn't allow you to rotate the attached monitor which the Rift creates). If using rotated view, make sure to create your window in a rotated manner as well, i.e. the window dimensions should be 1080x1920 instead of 1920x1080, if you rotate.

    mRift = new Rift( 0, mRoot, mWindow, false );

- Fix the IPD in your Scene by caling mRift::getIPD() and using that return value to set the distance between your cameras (all Rift values returned in meters, so the Default value is 0.064m (6.4 cm)).
- Call mRift->setCameras( camLeft, camRight ); to pass your cameras to the Rift class. The rift will now create viewports using these cameras and set the projection matrices. Now rendering is already set up!
- For headtracking, call mRift->getOrientation() and/or mRift->getPosition() every frame and apply it to the cameras' scene node ("mHeadNode").

- Make sure your FPS stays high! This is often not the case in Debug mode, but check if it works in Release mode and you should be fine.

The fluency of 3D environment regardless of image framrate is possible since the projection mesh is uploaded only when image is ready AND moved to an orientation coherent with when the frame was captured (by gathering Oculus Rift orientation information). A spherical mesh is also provided for very high FOV images, in the attempt to improve immersivity even when image is reaching lower frame rates, since head movement is always fluent and image (even if not refreshed often) is always covering view in the correct position.

### How Ogre runs under Windows

When launching the application, Ogre first looks for runtime .dll (such as OIS and others defined in CMakeLists.txt in target_link_libraries.). In windows, any application looks for these .dlls in directories set in PATH environment variable. This does not happen wih OculusSDK since in this project static libraries are used (.lib). Alternatively, these .dlls should be copied in the same directory where the executable resides. This will be done by cmake only with INSTALL command, in /dist folder, to make application portable.

Only then, plugins can be hot linked. This is when plugins.cfg is parsed and other .dlls are loaded from plugin path directory defined in it.
Similarly, all media files are searched from directories defined in resources.cfg

When debugging/compiling application, application looks for cfg files in /cfg, for media files in OgreSDK default media folder and /media folder (where you should put personal media files) and for .dlls in OgreSDK (as defined in PATH variable).

All paths are however set by my cmake file and changed accordingly when Configuring or Installing the project. When installing application, all these files are COPIED in paths relative to /dist, thus creating a portable version that can be distributed more easily. Don't modify these files! Update the original ones instead and then install again!

## Instructions

### Project files structure
```txt
root 	  	->	cmakelists.txt (for cmake) + info docs
\cfg	  	->	ogre local configuration files (both debug and release versions)
\cmake   	->	.cmake files (for cmake)
\dist 	  	->	distribution directory (readme inside)
\include	->	include directory
\media	  	->	ogre local media files
\src 	  	->	source directory
\src\demo	->	the main.cpp file used when this project is NOT used as a subproject by cmake
\libs		->	directory where 3rd party libraries can be placed (as source submodules or installed)
```

### Notes on submodules

The repo includes some submodules by default, which will NOT be downloaded if you do not specifically ask to:
- OculusSDK custom version source: a modified version of OculusSDK I have extended for this project
- ArUco 1.3.0 source: the official version of arUco (by compiling it I could trigger optimization flags)

To download the submodules, after cloning, you must run from the repo folder:
```sh
git submodule update --init --recursive
```
Alternatively, you may simply clone this way:
```sh
git clone --recursive git://...
```
### Prerequisites

- gcc 4.8 with C++11 support (SUPERIMPORTANT)
- Boost (OPTIONAL)
- TBB (OPTIONAL but extremely recommended!)
- CUDA (NECESSARY if you want to activate optimized computer vision or any CUDA filter/algorithm)
- Ogre3D 1.9 installed on system or in /libs folder (scripts provided for it in linux)
- OculusSDK 0.5.0.1 (scripts are provided for all options in linux)
	- official version source in /libs folder (included with the project as submodule) - CURRENTLY NOT SUPPORTED
	- official version installed in system or /libs folder - CURRENTLY NOT SUPPORTED
	- custom version source in /libs folder (included with the project as submodule) - DEFAULT OPTION
	- custom version installed in system or /libs folder
- Oculus Runtime 0.5.0.1
	- [Win] Requires to be installed separately, download it from website
	- [Linux] It is included in SDK, the rules.d file must be installed and then run the daemon from anywhere
- OpenCV with CUDA and TBB (tested with >= 2.4.8 and 3.0 which adds more functionality with CUDA and capture device control)
	- official version installed in system or /libs folder (script provided for linux)
	- official version source in /libs folder - NOT SUPPORTED AND WOULD ALSO BE USELESS
- ArUco (tested with 1.3.0)
	- official version installed in system or /libs folder - CURRENTLY NOT SUPPORTED
	- official version source in /libs folder (included with the project as submodule) - DEFAULT OPTION, NOT THE BEST

### Building

1. Install cmake >2.8.12
2. Setup OgreSDK
	- WIN:
		- create env variable OGRE_HOME and set to the place where you've installed or built OGRE
		- add to env PATH the folders containing Release and Debug dlls (ex. OIS.dll and OIS_d.dll)
		  NOTE: This avoids to copy any .dlls to run Debug or Release version after compiling.
          If you want to deploy the final version with .dll and all, run Install after compiling Release.
	- LINUX:
		- run script "install_system_dependenciesONLY_OGRE1.9_linux.sh" OR the "install_here.." counterpart to compile and install deps in system OR local folder
		- run script "install_system_OGRE1.9_linux.sh" OR the "install_here.." counterpart to compile and install Ogre3D1.9 in system OR local folder.
		- If process fails do not panic: read the script and do it manually, there can be an error on names or network.
3. If your version of OGRE was built with boost (i.e. threadding was enabled) then you'll also need boost (get binaries online). Make sure to install a version for the Compiler you're using (for example: Visual C++ 2010 Express).
	- WIN: if using boost,
		- set env var BOOST_ROOT to C:\local\boost_1_56_0 (or wherever you installed boost)
		- set env var BOOST_INCLUDEDIR to C:\local\boost_1_56_0 (or wherever you installed boost)
		- set env var BOOST_LIBRARYDIR to C:\local\boost_1_56_0\lib32-msvc-10.0 (or wherever you installed boost)
	- LINUX: it should work like a charm if you installed it in system correctly, so I did not go deeper
4. Setup OculusSDK
	- if you want to compile it with the project (DEFAULT)
		- WIN:
			1) Make sure you have pulled OculusSDK submodule, its source is in /libs folder
            	- you can switch between the custom and the untouched version (NOT SUPPORTED NOW) by manually checking out branch "0.5_standard" and "0.5_extended_prediction" of the submodule
			2) install necessary dependencies to compile OculusSDK (I did not need to install anything though)
            3) Run cmake and compile project with it
		- LINUX:
			1) Make sure you have pulled OculusSDK submodule, its source is in /libs folder
            	- you can switch between custom and official version by running the appropriate "use_here.." scripts
			2) run "ConfigureDebian.sh" from OculusSDK folder (needed to install dependencies and rules.d file for device) 
			3) Run cmake and compile project with it 
	- if you want only to install it (will be only LINKED by project) make sure there is no OculusSDK source in /libs, then
		- WIN:
			- create env variable OCULUS_HOME and set to the place where you've installed or built OculusSDK (official or custom)
			- add to env PATH the folders containing Release and Debug dlls
			- custom version is available from https://github.com/fakkoweb/OculusSDK.git on branch "0.5_extended_prediction"
		- LINUX:
			- there are 4 scripts you can choose from (according to version and where to install it):
				- "install_system_OculusSDK0.5.0.1+dependencies_linux.sh"
                - "install_system_OculusSDK0.5.0.1custom+dependencies_linux.sh"
                - "install_here_OculusSDK0.5.0.1+dependencies_linux.sh"
                - "install_here_OculusSDK0.5.0.1custom+dependencies_linux.sh"
             - If process fails do not panic: read the script and do it manually, there can be an error on names or network.
5. Setup OpenCV
	- WIN: follow tutorials to compile it with CUDA (depends on your graphic card and driver) and TBB, then add the needed env variables and PATH entries to use it right away (usually is done automatically by the installer in the default version)
	- LINUX: do the same for your system or, alternatively if you are lazy like me:
		-  run one of the scripts "install_here_opencv2.4_linux.sh" OR "install_here_opencv3_linux.sh".. quite clear on what they do, right?
		- If process fails do not panic: read the script and do it manually, there can be an error on names or network.
6. Run Cmake (possibly through CmakeGUI). **PLEASE FIRST learn how to use a cmake file, how to Configure and Generate a project**. Every lib should be found (in whatever form it is, to be compiled or not). Check the "USE_" flags to make sure what is used and what not. Possibly I will add screenshots in the future, since I want to make this Cmake file a template for future projects.

### Installing

See README in /dist. Install means that, after compiling the Release, every .dll or resource file needed to run the project as a STANDALONE and REDISTRIBUTABLE to anyone will be copied to /dist directory.
Have no fear, since my cmake file is responsible to do so. In the specific it will:
- copy executable in /dist
- copy static and dynamic libs files where the executable is
- copy the plugins used by your Ogre project (by reading the plugins.cfg file)
- copy the resource files used by your Ogre project (by reading the resources.cfg file)
- copy the .cfg files and changes the PATHs inside the text file to be relative (pretty cool, huh?), independently from the fact they linked to Ogre installation folder or local project /media folder.

### Running

1. Run Oculus Runtime
	- WIN: make sure the right version of Oculus Runtime is installed and running
	- LINUX: for more help, see [HERE](https://www.reddit.com/r/oculus/comments/2rjklz/ubuntu_i_got_an_oculus_but_where_do_i_get_started/) 
		1) Run "ConfigureDebian.sh" from OculusSDK folder (needed to install dependencies and rules.d file for device)
		2) Start service "ovrd", which is in some subfolder, or "oculusd -d" if it is installed in system
		3) Run "RiftConfigUtil" (to set up profiles). It is embedded in OculusSDK, but I am not sure if in every version.
2. Plug Oculus Rift and make sure it is correctly detected. This application uses Extended Mode, so a secondary desktop should appear. The orientation should be vertical if using Oculus DK2. Application should rotate accordingly the view without user intervention.
3. Plug the camera(s) and make sure .yml calibration files are in /cfg directory (with names camera_0.yml and camera_1.yml)
	- camera0 will be used as LEFT camera. Usually 0 is assigned to the first camera plugged in. I had no way of controlling this behaviour nor associate the desired calibration file to the desired camera. Can be improved.
	- we suggest to place the cameras in front of each eye, in:
		- PARALLEL configuration, if you are using very wide angle lenses (or fisheye lenses)
		- TOE-IN configuration in other cases (DEFAULT, there is still no option to switch to fisheye configuration yet!)
4. Run the application/demo
	- A guide on how to use application as an external library for your projects has to be made.

> More work and, possibly, some investments would push the project even further, improving and adding its capabilities for real uses.

### Usage (to be written)


Troubleshooting:
---------------

### FAQ

It compiles and runs but doesn't work and looks all wrong!

- In your Monitor settings, check if the Monitor belonging to the Rift is oriented in Landscape or Portrait orientation. If it's a Portrait, make sure to set the bool in your "new Rift(..., true)" to true so that the image is rotated.
- The Example app creates the Rift Monitor on Screen 2. Change the line "miscParams["monitorIndex"] = Ogre::StringConverter::toString(1);" in App.cpp to a different index (for example, 0 for your first screen), if need be.

My Overlays are all wrong or don't show!

- The Rift class creates a new SceneManager and some of its own viewports. Modify the viewport creation to your needs (to show or not show overlays).
- Generally speaking, I (and the Developers behind the Oculus Rift) advice not to use overlays for your UI - instead put stuff on in-game elements wherever possible.

Input doesn't work!

- If you use OIS, the window needs focus. When using the Rift, the Window is on a seperate screen. Moving your mouse over to the second screen and clicking should give the window focus.

- Linux: If the app starts but shuts down again, make sure that a) the oculusd deamon is running (go into OculusSDK lib and run 'oculusd -d') and b) you're running the program with the dedicated graphics card (if you have multiple). If it still doesn't work, try running with the --no-debug command line switch. This will make sure only ONE window is created (the main render window for the oculus rift)

### Tips
- do not delete sources of Ogre and Dependencies (you would need to go through all the steps again)
- compile both Ogre versions (with and without boost)
- you may encounter similar issues with all other local libraries (this is not really a tip, but use the steps above as a general guide)
- close and reopen terminal when you encounter even stranger issues

### Known errors
- IT DOES NOT COMPILE OGRE 1.9 WITH BOOST IF TOO OLD: wierd errors with boost (used by Ogre or OpenCV)
http://stackoverflow.com/questions/18900730/boostshared-ptrshared-ptrconst-boostshared-ptr-is-implicitly-declared
- ERROR ON gpu.hpp
put "using namespace std;" at the beginning of the file
http://stackoverflow.com/questions/26121604/opencv-2-4-9-compilation-error-with-cuda-6-5
- ERRORS WHILE COMPILING OCULUS SDK or ARUCO
Use gcc 4.8 and flag c++11, specify it in cmake to be sure (easy if using a GUI).
If does not work, delete cache and build directory, redo all configure from scratch

## Special treatment for Ubuntu 12.04 or older (please avoid!!!!)
This project had to run on a Ubuntu 12.04 machine with cmake 2.8.9, gcc 4.6, old boost x.x.x, no admin rights.. a real pain in the a** (Aragorn coff coff..)!!

I will write down what I did to make it work. Just because I lost plenty of time and it might come in handy to learn some tricks.
**Three major truths**:
- Ogre 1.9 to compile requires at least cmake 2.8.9
- Ogre Dependencies require at least cmake 2.8.11
- If you use a local user version of cmake, it won't find all system installed libraries

**If you are on a system running an older version of cmake, g++, boost and also you have no admin rights, follow this steps. Expect errors and TONS of retries.**

TO COMPILE AND INSTALL OGRE 1.9
1. Install locally 2.8.12 using script "install_here_cmake2.8.12_linux.sh"
2. Make it the DEFAULT cmake by adding its variable (to run it)
3. Open a new terminal (to detect the newly created variable) and run dependencies script
4. Move temporarly the local cmake folder from its current position, then open a new terminal so will find your cmake 2.8.9
5. If boost is too old:
	- open Ogre install script and remove boost support (change it to 0 in the script)
    - Open ANOTHER terminal (to detect 2.8.9) and run Ogre script
6. If cmake 2.8.9 gives errors it is ok: check it found the needed libraries, which are now in cache, then:
	- move again the local version of cmake back to its position
	- run the lines of Ogre script manually from a new terminal from cmake line (it is in the script)
	- remove back again from the position the local cmake, so now 2.8.9 works again

You have finally Ogre1.9 locally installed!

ARUCO and OPENCV:
- Compile the project with cmake 2.8.9
- Make sure you are using the local version of OpenCV and not the system's (by using CmakeGUI and changing OPENCV_DIR)

# OR

**JUST UPDATE YOUR SYSTEM!**


[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)


   [prezipresentation]: <https://prezi.com/ugsijfgxn9bd>
   [cvapvideo]: <https://www.youtube.com/watch?v=UDxcTXaMZpE>
