
// Main Application for Ogre integration.
// Responsible for comminication with Ogre (rendering) and OIS (mouse and keyboard input)

#ifndef APP_H
#define APP_H

//class Rift;
#include "ConfigDB.h"
#include "Rift.h"
#include <sstream>
#include <string.h>
#include "OGRE/Ogre.h"
#include "OIS/OIS.h"
#include "Scene.h"
#include "Camera.h"
#include "Globals.h"


// The Debug window's size is the Oculus Rift Resolution times this factor.
#define debugWindowSize 0.5

// Creates the Ogre application
// Constructor: initializes Ogre (initOgre) and OIS (initOIS), allocates the Scene and viewports (createViewports), inizializes Oculus (initRift) and STARTS RENDERING
// Destructor: deallocates all above
class App : public Ogre::FrameListener, public OIS::KeyListener, public OIS::MouseListener
{
	public:

		App(const std::string& configurationFilesPath, const std::string& settingsFileName);
		~App();

		void loadConfig(const std::string& configurationFilesPath);
		void initFramework();
		void initWindows();
		void initIO();
		void initResources();
		void initScenes();
		void initViewports();
		void start();

		
		void initOgre();
		void loadOgreResources();
		void loadOgreWindows();
		void quitOgre();
		void initOIS();
		void quitOIS();
		void initTray();
		void quitTray();
		//TODO: separate Ogre initialization from windows creation (Oculus NEEDS a window or to create a window to start!)
		//void createWindows();
		void createViewports();

		void quit();

		void initRift();
		void quitRift();

		void initCameras();
		void quitCameras();

		bool keyPressed(const OIS::KeyEvent& e );
		bool keyReleased(const OIS::KeyEvent& e );
		bool mouseMoved(const OIS::MouseEvent& e );
		bool mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID id );
		bool mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID id );

		bool frameRenderingQueued(const Ogre::FrameEvent& evt);

		bool update();

	private:
		const std::string configurationFilesPath;
		ConfigDB* mConfig = nullptr;

		OIS::Keyboard* mKeyboard = nullptr;
		enum KeyboardLayout
		{
			Idle,
			AspectRatioAdjust,
			CalibrationAdjust,
			DistanceAdjust,
			FovAdjust,
			LagAdjust
		} keyLayout = Idle;
		enum EyeCode
		{
			Left,
			Right
		} eyeSelected = Left;
		bool seethroughEnabled = false;
		Scene::StabilizationModel stabilizationModel = Scene::StabilizationModel::Head;

		OIS::Mouse* mMouse = nullptr;

		Ogre::Root* mRoot = nullptr;

		// interface display
		// Ogre::OverlaySystem* mOverlaySystem;
		// OgreBites::SdkTrayManager*	mTrayMgr;
		// OgreBites::InputContext     mInputContext;
		// OgreBites::ParamsPanel*     mDetailsPanel;   		// Sample details panel
		bool mCursorWasVisible=false;							// Was cursor visible before dialog appeared?

		Ogre::RenderWindow* mLeftEyeViewWindow = nullptr;
		Ogre::RenderWindow* mRightEyeViewWindow = nullptr;
		Ogre::RenderWindow* mRiftViewWindow = nullptr;
		Ogre::RenderWindow* mDebugRiftViewWindow = nullptr;
		Ogre::RenderWindow* mEnvironmentViewWindow = nullptr;
		//Ogre::Viewport* mViewportL;
		//Ogre::Viewport* mViewportR;

		// If this is set to true, the app will close during the next frame:
		bool mShutdown = false;
		bool mPause = false;
		std::chrono::steady_clock::time_point loopStart_time;
		Scene* mScene = nullptr;

		Rift* mRift = nullptr;

		FrameCaptureHandler* mCameraLeft = nullptr;
		FrameCaptureHandler* mCameraRight = nullptr;
		Ogre::PixelBox mOgrePixelBoxLeft;	//Ogre containers for opencv Mat image raw data
		Ogre::PixelBox mOgrePixelBoxRight;	//Ogre containers for opencv Mat image raw data
		FrameCaptureData nextFrameLeft;
		FrameCaptureData nextFrameRight;
		bool imageLeftReady = false;
		bool imageRightReady = false;
};

#endif
