#include "App.h"
#include <chrono>
#include <thread>

void frames_per_second(int delay)
{
	static int num_frame = 0;
	static int time = 0;

	num_frame++;
	time += delay;
	if (time >= 1000)
	{
		std::cout << "\tFPS:" << num_frame << std::endl;
		num_frame = 0;
		time = 0;
	}

}

//Globals used only in App.cpp
std::chrono::steady_clock::time_point ogre_last_frame_displayed_time = std::chrono::system_clock::now();
std::chrono::duration< int, std::milli > ogre_last_frame_delay;

////////////////////////////////////////////////////////////
// Init application
////////////////////////////////////////////////////////////

App::App(const std::string& configurationFilesPath = "cfg/", const std::string& settingsFileName = "parameters.cfg") : configurationFilesPath(configurationFilesPath)
{
	// Init variables
	mRoot = nullptr;
	mKeyboard = nullptr;
	mMouse = nullptr;
	mScene = nullptr;
	mShutdown = false;
	mWindow = nullptr;
	mSmallWindow = nullptr;
	mRift = nullptr;


	//Load custom configuration for the App (parameters.cfg)
	std::cout << "APP: Loading configuration file..." << std::endl;
	loadConfig(configurationFilesPath + settingsFileName);
	std::cout << "APP: Loaded." << std::endl;

	//Ogre engine setup (loads plugins.cfg/resources.cfg and creates Ogre main rendering window NOT THE SCENE)
	std::cout << "APP: Initializing framework..." << std::endl;
	initFramework();
	std::cout << "APP: Initialized. Enumeration: (...)" << std::endl;

	//The correct order should be, from inner to outer: resources - scene - viewport - window - IO
	//Unfortunately:
	//	- resources cannot be loaded correctly before defining at least a window
	//	- viewport must be configured after defining a window and a scene
	initWindows();

	initIO();

	initResources();

	initScenes();

	initViewports();

	start();

	//Rift Setup (creates Oculus rendering window and Oculus inner scene - user shouldn't care about it)
	initRift();

	//Stereo camera rig setup ()
	//initCameras();

	//Input/Output setup (associate I/O to Oculus window)
	initOIS();


	// when setup has finished successfully, enable Video into scene
	// mScene->enableVideo();	USER CAN ACTIVATE IT, SEE INPUT KEYS LISTENERS!

	//Start info display interface
	//initTray();

	//Viewport setup (link scene cameras to Ogre/Oculus windows)
	createViewports();



	//Ogre::WindowEventUtilities::messagePump();


}

App::~App()
{
	std::cout << "Deleting Ogre application." << std::endl;

	quitCameras();
	quitRift();

	std::cout << "Deleting Scene:" << std::endl;
	if( mScene ) delete mScene;
	//quitTray();
	std::cout << "Closing OIS:" << std::endl;
	quitOIS();
	std::cout << "Closing Ogre:" << std::endl;
	quitOgre();
}

/////////////////////////////////////////////////////////////////
// Load User parameters and App configuration
/////////////////////////////////////////////////////////////////
void App::loadConfig(const std::string& configurationFilesPath)
{

	mConfig = new ConfigDB(configurationFilesPath);

	// Overwrite default parameters values
	CAMERA_BUFFERING_DELAY = mConfig->getValueAsInt("Camera/BufferingDelay");
	ROTATE_VIEW = mConfig->getValueAsBool("Oculus/RotateView");
	CAMERA_ROTATION = mConfig->getValueAsInt("Camera/Rotation");
	if (CAMERA_ROTATION <= -180 || CAMERA_ROTATION > 180)
		CAMERA_ROTATION = 180;

}

void App::initFramework()
{
	// Do HERE all needed Frameworks/SDKs initialization and configuration loading

	initOgre();

	initRift();

	initCameras();

}

void App::initWindows()
{
	// Do HERE all the windows configuration (according to what initFramework() has loaded)

	loadOgreWindows();

	mWindow = mRift->createRiftDisplayWindow(mRoot);
}

void App::initIO()
{
	initOIS();
}

void App::initResources()
{
	// Do HERE all the resources file loading stuff

	loadOgreResources();

}

void App::initScenes()
{
	// Create Rift inner scene (for stereo vision and lens distortion)
	mRift->createRiftDisplayScene(mRoot);

	// Create Ogre main scene
	mScene = new Scene(mRoot, mMouse, mKeyboard);
	//if (mOverlaySystem)	mScene->getSceneMgr()->addRenderQueueListener(mOverlaySystem);	//Only Ogre main scene will render overlays!
	try
	{
		// try first to load HFOV/VFOV values (higher priority)
		mScene->setupVideo(Scene::CameraModel::Fisheye, Ogre::Vector3::ZERO, mConfig->getValueAsReal("Camera/HFOV"), mConfig->getValueAsReal("Camera/VFOV"));
	}
	catch (Ogre::Exception &e)
	{
		// in case one of these parameters is not found in configuration file or is invalid...
		if (e.getNumber() == Ogre::Exception::ERR_ITEM_NOT_FOUND || e.getNumber() == Ogre::Exception::ERR_INVALIDPARAMS)
		{
			try
			{
				// ..try to load other parameters (these are preferred, but lower priority since not everyone know these)
				mScene->setupVideo(Scene::CameraModel::Fisheye, Ogre::Vector3::ZERO, mConfig->getValueAsReal("Camera/SensorWidth"), mConfig->getValueAsReal("Camera/SensorHeight"), mConfig->getValueAsReal("Camera/FocalLenght"));
			}
			catch (Ogre::Exception &e)
			{
				// if another exception is thrown (any), forward
				throw e;
			}
		}
		// if another exception is thrown, forward
		else
		{
			throw e;
		}
	}
	
	// Setup Ogre main scene in respect to Oculus parameters
	mScene->setIPD(mRift->getIPD());												// adjust IPD
	mRift->setCameraMatrices(mScene->getLeftCamera(), mScene->getRightCamera());	// adjust matrices

}

void App::initViewports()
{
	// Link Ogre stereo cameras to Oculus distortion meshes
	// (to be rendered into Oculus window through Oculus ortho camera)
	mRift->attachCameras(mScene->getLeftCamera(), mScene->getRightCamera());

	// Link orthographic inner scene Oculus camera to Oculus rendering window
	Ogre::Viewport* oculusView = mWindow->addViewport(mRift->getCamera());
	oculusView->setBackgroundColour(Ogre::ColourValue::Black);
	oculusView->setOverlaysEnabled(true);

	// Link other Ogre cameras to debug windows
	if (mSmallWindow)
	{
		Ogre::Viewport* debugL = mSmallWindow->addViewport(mScene->getLeftCamera(), 0, 0.0, 0.0, 0.5, 1.0);
		debugL->setBackgroundColour(Ogre::ColourValue(0.15, 0.15, 0.15));

		Ogre::Viewport* debugR = mSmallWindow->addViewport(mScene->getRightCamera(), 1, 0.5, 0.0, 0.5, 1.0);
		debugR->setBackgroundColour(Ogre::ColourValue(0.15, 0.15, 0.15));
	}

	// Create a God view window from a third view camera
	if (mGodWindow)
	{
		Ogre::Viewport* god = mGodWindow->addViewport(mScene->getGodCamera(), 0, 0.0, 0.0, 1.0, 1.0);
	}
}

void App::start()
{
	// START RENDERING!
	// WHO CREATES AN INSTANCE OF THIS CLASS WILL WAIT INDEFINITELY UNTIL THIS CALL RETURNS
	mRoot->startRendering();
}

/////////////////////////////////////////////////////////////////
// Init Ogre (construction, setup and destruction of ogre root):
/////////////////////////////////////////////////////////////////

void App::initOgre()
{

	// Config file class is an utility that parses and stores values from a .cfg file
	Ogre::ConfigFile cf;
#ifdef _DEBUG
	std::string pluginsFileName = "plugins_d.cfg";		// plugins config file name (Debug mode)
#else
	std::string pluginsFileName = "plugins.cfg";		// plugins config file name (Release mode)
#endif


	// LOAD OGRE PLUGINS
	// Try to load load up a valid config file (and don't start the program if none is found)
	try
	{
		//This will work ONLY when application is installed (only Release application)!
		std::cout << "OGRE: Loading plugins file..." << std::endl;
		cf.load(configurationFilesPath + pluginsFileName);
	}
	catch (Ogre::FileNotFoundException &e)
	{
		std::cout << "OGRE: Failed." << std::endl;
		throw e;
	}


	// INSTANCIATE OGRE ROOT (IT INSTANCIATES ALSO ALL OTHER OGRE COMPONENTS)
	// In Ogre, the singletons are instanciated explicitly (with new) the first time,
	// then it can be accessed with Ogre::Root::getSingleton()
	// Plugins are passed as argument to the "Root" constructor
	std::cout << "OGRE: Constructing Root object..." << std::endl;
	mRoot = new Ogre::Root(configurationFilesPath + pluginsFileName);
	// No Ogre::FileNotFoundException is thrown by this, that's why we tried to open it first with ConfigFile::load()
	


	// SELECT AND CUSTOMIZE OGRE RENDERING (OpenGL)
	// Get a reference of the RenderSystem in Ogre that I want to customize
	Ogre::RenderSystem* pRS = mRoot->getRenderSystemByName("OpenGL Rendering Subsystem");
	// Get current config RenderSystem options in a ConfigOptionMap
	Ogre::ConfigOptionMap cfgMap = pRS->getConfigOptions();
	// Modify them
	cfgMap["Full Screen"].currentValue = "No";
	cfgMap["VSync"].currentValue = "Yes";
	#ifdef _DEBUG
		cfgMap["FSAA"].currentValue = "0";
	#else
		cfgMap["FSAA"].currentValue = "8";
	#endif
	cfgMap["Video Mode"].currentValue = "1200 x 800";
	// Set them back into the RenderSystem
	for(Ogre::ConfigOptionMap::iterator iter = cfgMap.begin(); iter != cfgMap.end(); iter++) pRS->setConfigOption(iter->first, iter->second.currentValue);
	// Set this RenderSystem as the one I want to use
	mRoot->setRenderSystem(pRS);
	// Initialize it: "false" is DO NOT CREATE A WINDOW FOR ME
	mRoot->initialise(false, "Oculus Rift Visualization");
	


	//std::this_thread::sleep_for(std::chrono::duration< int, std::deca >(1));
	//Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	//mOverlaySystem = new Ogre::OverlaySystem();


	// Then setup THIS CLASS INSTANCE as a frame listener
	// This means that Ogre will call frameStarted(), frameRenderingQueued() and frameEnded()
	// automatically and periodically if defined in this class
	mRoot->addFrameListener(this);

}

void App::loadOgreResources()
{
	// Config file class is an utility that parses and stores values from a .cfg file
	Ogre::ConfigFile cf;
	std::string resourcesFileName = "resources.cfg";	// resources config file name (Debug/Release mode)


	// LOAD OGRE RESOURCES
	// Load up resources according to resources.cfg ("cf" variable is reused)
	try
	{
		//This will work ONLY when application is installed!
		std::cout << "OGRE: Loading resources file..." << std::endl;
		cf.load(configurationFilesPath + resourcesFileName);
	}
	catch (Ogre::FileNotFoundException &e)	// It works, no need to change anything
	{
		std::cout << "OGRE: Failed." << std::endl;
		throw e;
	}


	// Go through all sections & settings in the file and ADD them to Resource Manager
	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;
			//For each section/key-value, add a resource to ResourceGroupManager
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
				archName, typeName, secName);
		}
	}


	//Initialize all resources groups (read from file)
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

}

void App::loadOgreWindows()
{
	// CREATE WINDOWS
	/* REMOVED: Rift class creates the window if no null is passed to its constructor
	// Options for Window 1 (rendering window)
	Ogre::NameValuePairList miscParams;
	if( NO_RIFT )
	miscParams["monitorIndex"] = Ogre::StringConverter::toString(0);
	else
	miscParams["monitorIndex"] = Ogre::StringConverter::toString(1);
	miscParams["border "] = "none";
	*/

	/*
	// Create Window 1
	if( !ROTATE_VIEW )
	mWindow = mRoot->createRenderWindow("Oculus Rift Liver Visualization", 1280, 800, true, &miscParams);
	//mWindow = mRoot->createRenderWindow("Oculus Rift Liver Visualization", 1920*0.5, 1080*0.5, false, &miscParams);
	else
	mWindow = mRoot->createRenderWindow("Oculus Rift Liver Visualization", 1080, 1920, true, &miscParams);
	*/

#ifdef _DEBUG
	// Options for Window 2 (debug window)
	// This window will simply show what the two cameras see in two different viewports
	Ogre::NameValuePairList miscParamsSmall;
	miscParamsSmall["monitorIndex"] = Ogre::StringConverter::toString(0);

	// Create Window 2
	if (DEBUG_WINDOW)
		mSmallWindow = mRoot->createRenderWindow("DEBUG Oculus Rift Liver Visualization", 1920 * debugWindowSize, 1080 * debugWindowSize, false, &miscParamsSmall);

	// Options for Window 3 (god window)
	// This debug window will show the whole scene from a top view perspective
	Ogre::NameValuePairList miscParamsGod;
	miscParamsSmall["monitorIndex"] = Ogre::StringConverter::toString(0);

	// Create Window 3
	if (DEBUG_WINDOW)
		mGodWindow = mRoot->createRenderWindow("DEBUG God Visualization", 1920 * debugWindowSize, 1080 * debugWindowSize, false, &miscParamsSmall);
#endif

}

void App::initTray()
{
	// Register as a Window listener
	//Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

	/*
	if (mWindow)
	{
		mInputContext.mKeyboard = mKeyboard;
		mInputContext.mMouse = mMouse;

		mTrayMgr = new OgreBites::SdkTrayManager("InterfaceName", mWindow, mInputContext, this);
		mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
		//mTrayMgr->showLogo(OgreBites::TL_BOTTOMRIGHT);
		mTrayMgr->hideCursor();

		// Create a params panel for displaying sample details
		Ogre::StringVector items;
		items.push_back("cam.pX");
		items.push_back("cam.pY");
		items.push_back("cam.pZ");
		items.push_back("");
		items.push_back("cam.oW");
		items.push_back("cam.oX");
		items.push_back("cam.oY");
		items.push_back("cam.oZ");
		items.push_back("");
		items.push_back("Filtering");
		items.push_back("Poly Mode");

		mDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_NONE, "DetailsPanel", 200, items);
		mDetailsPanel->setParamValue(9, "Bilinear");
		mDetailsPanel->setParamValue(10, "Solid");
		mDetailsPanel->hide();
	}
	*/
	
}

void App::createViewports()
{
	// Create two viewports, one for each eye (i.e. one for each camera):
	// Each viewport is assigned to a RenderTarget (a RenderWindow in this case) and spans half of the screen
	// A pointer to a Viewport is returned, so we can access it directly.
	// CAMERA -> render into -> VIEWPORT (rectangle area) -> displayed into -> WINDOW
	/*
	if (mWindow)		//check if Ogre rendering window has been created
	{
		if (NO_RIFT)
		{
			mViewportL = mWindow->addViewport(mScene->getLeftCamera(), 0, 0.0, 0.0, 0.5, 1.0);
			mViewportL->setBackgroundColour(Ogre::ColourValue(0.15, 0.15, 0.15));
			mViewportR = mWindow->addViewport(mScene->getRightCamera(), 1, 0.5, 0.0, 0.5, 1.0);
			mViewportR->setBackgroundColour(Ogre::ColourValue(0.15, 0.15, 0.15));
		}

		if( !ROTATE_VIEW )
		{
		mScene->getLeftCamera()->setAspectRatio( 0.5*mWindow->getWidth()/mWindow->getHeight() );
		mScene->getRightCamera()->setAspectRatio( 0.5*mWindow->getWidth()/mWindow->getHeight() );
		} else {
		mScene->getLeftCamera()->setAspectRatio( 0.5*mWindow->getHeight()/mWindow->getWidth() );
		mScene->getRightCamera()->setAspectRatio( 0.5*mWindow->getHeight()/mWindow->getWidth() );
		/
	}
	*/




}

void App::quitTray()
{
	/*
	delete mTrayMgr;
	mTrayMgr = nullptr;
	delete mOverlaySystem;
	mOverlaySystem = nullptr;
	*/
}

void App::quitOgre()
{
	// This should really not be commented out, but it crashes on my system... ?!
	// if( mRoot ) delete mRoot;
}

////////////////////////////////////////////////////////////
// Init OIS (construction, setup and destruction of input):
////////////////////////////////////////////////////////////

void App::initOIS()
{
	OIS::ParamList pl;
    size_t windowHnd = 0;
    std::ostringstream windowHndStr;
 
    // Tell OIS about the Ogre Rendering window (give its id)
    mWindow->getCustomAttribute("WINDOW", &windowHnd);
    windowHndStr << windowHnd;
    pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

	// Setup the manager, keyboard and mouse to handle input
    OIS::InputManager* inputManager = OIS::InputManager::createInputSystem(pl);
    mKeyboard = static_cast<OIS::Keyboard*>(inputManager->createInputObject(OIS::OISKeyboard, true));
    mMouse = static_cast<OIS::Mouse*>(inputManager->createInputObject(OIS::OISMouse, true));
 
    // Tell OIS about the window's dimensions
    unsigned int width, height, depth;
    int top, left;
    mWindow->getMetrics(width, height, depth, left, top);
    const OIS::MouseState &ms = mMouse->getMouseState();
    ms.width = width;
    ms.height = height;

	// Setup THIS CLASS INSTANCE as a OIS mouse listener AND key listener
	// This means that OIS will call keyPressed(), mouseMoved(), etc.
	// automatically and whenever needed
	mKeyboard->setEventCallback(this);
	mMouse->setEventCallback(this);
}

void App::quitOIS()
{
	if( mKeyboard ) delete mKeyboard;
	if( mMouse ) delete mMouse;
}

////////////////////////////////////////////////////////////////
// Init Rift (Device and API initialization, setup and close):
////////////////////////////////////////////////////////////////

void App::initRift()
{
	// Try to initialize the Oculus Rift (ID 0):
	try {
		// This class implements a custom C++ Class version of RIFT C API
		//Rift::init();		//OPTIONAL: automatically called by Rift constructor, if necessary
		mRift = new Rift( 0, mRoot, mWindow /*if null, Rift creates the window*/, ROTATE_VIEW );
	}
	catch (const std::ios_base::failure& e) {
		std::cout << ">> " << e.what() << std::endl;
		//NO_RIFT = true;
		mRift = NULL;
		mShutdown = true;
	}
}

void App::quitRift()
{
	std::cout << "Shutting down Oculus Rifts:" << std::endl;
	if( mRift ) delete mRift;
	//Rift::shutdown();
}

////////////////////////////////////////////////////////////////
// Init Cameras (Device and API initialization, setup and close):
////////////////////////////////////////////////////////////////

void App::initCameras()
{
	//mCameraLeft = new FrameCaptureHandler(0, mRift);
	//mCameraRight = new FrameCaptureHandler(1, mRift);

	//mCameraLeft->startCapture();
	//mCameraRight->startCapture();

	cv::namedWindow("CameraDebugLeft", cv::WINDOW_NORMAL);
	cv::resizeWindow("CameraDebugLeft", 1920 / 4, 1080 / 4);
	cv::namedWindow("CameraDebugRight", cv::WINDOW_NORMAL);
	cv::resizeWindow("CameraDebugRight", 1920 / 4, 1080 / 4);
}

void App::quitCameras()
{
	/*
	mCameraLeft->stopCapture();
	mCameraRight->stopCapture();
	if (mCameraLeft) delete mCameraLeft;
	*/
	if (mCameraRight) delete mCameraRight;
}

////////////////////////////////////////////////////////////
// Handle Rendering (Ogre::FrameListener)
////////////////////////////////////////////////////////////

// This gets called while rendering frame data is loading into GPU
// Good time to update measurements and physics before rendering next frame!
bool App::frameRenderingQueued(const Ogre::FrameEvent& evt) 
{
	// [TIME] FRAME RATE DISPLAY
	//calculate delay from last frame and show
	ogre_last_frame_delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ogre_last_frame_displayed_time);
	frames_per_second(ogre_last_frame_delay.count());

	if (mShutdown) return false;

	// [RIFT] UPDATE
	// update Oculus information and sends it to Scene (Position/Orientation of character's head)
	if(mRift)
	{
		if ( mRift->update( evt.timeSinceLastFrame ) )		// saves new orientation/position information
		{
			mScene->setRiftPose( mRift->getOrientation(), mRift->getPosition() );	// sets orientation/position to a SceneNode
		} else {
			delete mRift;
			mRift = NULL;
		}
	}

	/*
	// [CAMERA] UPDATE
	// update cameras information and sends it to Scene (Texture of pictures planes/shapes)
	FrameCaptureData nextframe1;
	FrameCaptureData nextframe2;
	if (mCameraLeft && mCameraLeft->get(nextframe1))		// if camera is initialized AND there is a new frame
	{
		//std::cout << "Drawing the frame in debug window..." << std::endl;

		cv::imshow("CameraDebugLeft", nextframe1.image);
		cv::waitKey(1);
			
		//std::cout << "converting from cv::Mat to Ogre::PixelBox..." << std::endl;
		mOgrePixelBoxLeft = Ogre::PixelBox(1920, 1080, 1, Ogre::PF_R8G8B8, nextframe1.image.ptr<uchar>(0));
		//std::cout << "sending new image to the scene..." << std::endl;
		mScene->setVideoImagePoseLeft(mOgrePixelBoxLeft,nextframe1.pose);
		//std::cout << "image sent!\nImage plane updated!" << std::endl;

	}
	if (mCameraRight && mCameraRight->get(nextframe2))	// if camera is initialized AND there is a new frame
	{

		cv::imshow("CameraDebugRight", nextframe2.image);
		cv::waitKey(1);

		//std::cout << "converting from cv::Mat to Ogre::PixelBox..." << std::endl;
		mOgrePixelBoxRight = Ogre::PixelBox(1920, 1080, 1, Ogre::PF_R8G8B8, nextframe2.image.ptr<uchar>(0));
		//std::cout << "sending new image to the scene..." << std::endl;
		mScene->setVideoImagePoseRight(mOgrePixelBoxRight, nextframe2.pose);
		//std::cout << "image sent!\nImage plane updated!" << std::endl;
			
	}
	*/

	// [OIS] UPDATE
	// update standard input devices state
	mKeyboard->capture();
	mMouse->capture();

	// [OGRE] UPDATE
	// perform any other update that regards the Scene only (ex. Physics calculations)
	mScene->update( evt.timeSinceLastFrame );
	//mTrayMgr->frameRenderingQueued(evt);

	// [TIME] UPDATE
	// save time point for this frame (for frame rate calculation)
	ogre_last_frame_displayed_time = std::chrono::system_clock::now();


	// [VALUE EXPERIMENTS]
	//ovrTrackingState tracking = ovrHmd_GetTrackingStateExtended(mRift->getHandle(), ovr_GetTimeInSeconds());




	//exit if key ESCAPE pressed 
	if(mKeyboard->isKeyDown(OIS::KC_ESCAPE)) 
		return false;

	return true; 
}

//////////////////////////////////////////////////////////////////////////////////
// Handle Keyboard and Mouse input (OIS::KeyListener, public OIS::MouseListener)
//////////////////////////////////////////////////////////////////////////////////

bool App::keyPressed( const OIS::KeyEvent& e )
{
	mScene->keyPressed( e );

	switch (e.key)
	{
	case OIS::KC_C:
		keyLayout = ClippingAdjust;
		break;
	case OIS::KC_F:
		keyLayout = FovAdjust;
		break;
	case OIS::KC_L:
		keyLayout = LagAdjust;
		break;
	default:
		// keep last key pressed until it is released (see keyReleased)
		break;
	}

	//mWindow->writeContentsToFile("Screenshot.png");

	return true;
}
bool App::keyReleased( const OIS::KeyEvent& e )
{
	mScene->keyReleased( e );

	// when a keyLayout key is released, restore standard behaviour
	if (
		e.key == OIS::KC_C ||
		e.key == OIS::KC_F ||
		e.key == OIS::KC_L
		)	keyLayout = Idle;

	switch (e.key)
	{
	case OIS::KC_ADD:
		std::cout << "+ released" << std::endl;
		//Add Button
		switch (keyLayout)
		{
		case ClippingAdjust:
			mScene->adjustVideoDistance(+0.1f);				// +0.1 units
			std::cout << "Distance +1" << std::endl;
			break;
		case FovAdjust:
			mScene->adjustVideoFov(+0.01f);
			break;
		case LagAdjust:
			mCameraLeft->adjustManualCaptureDelay(+0.005f);	// +5 msec
			break;
		}

		break;
	case OIS::KC_SUBTRACT:

		// Minus Button
		switch (keyLayout)
		{
		case ClippingAdjust:
			mScene->adjustVideoDistance(-0.1f);			// -0.1 units
			break;
		case FovAdjust:
			mScene->adjustVideoFov(-0.01f);
			break;
		case LagAdjust:
			mCameraLeft->adjustManualCaptureDelay(-0.005f);	// -5 msec
			break;
		}

		break;
	case OIS::KC_T:
		
		// T Button: toggle VIDEOCAMERAS in the scene
		if (seethroughEnabled)
		{
			mScene->disableVideo();
			seethroughEnabled = false;
		}
		else
		{
			mScene->enableVideo();
			seethroughEnabled = true;
		}

		break;
	default:
		// Do nothing
		break;
	}
		
	return true;
}
bool App::mouseMoved( const OIS::MouseEvent& e )
{
	mScene->mouseMoved( e );
	return true;
}
bool App::mousePressed( const OIS::MouseEvent& e, OIS::MouseButtonID id )
{
	mScene->mouseReleased( e, id );
	return true;
}
bool App::mouseReleased( const OIS::MouseEvent& e, OIS::MouseButtonID id )
{
	mScene->mouseReleased( e, id );
	return true;
}

// Interrupts App loop
// frameRenderingQueued() will return false, thus interrupting Ogre loop
// App could run again by calling "mRoot->startRendering()"
// Destructor still needs to be called by user to deallocate application!
void App::quit()
{
	std::cout << "QUIT." << std::endl;
	mShutdown = true;
}
