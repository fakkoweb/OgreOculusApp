#ifndef RIFT_H
#define RIFT_H

#include <iostream>
#include "OVR.h"
#include "Extras/OVR_Math.h"
#include "OGRE/Ogre.h"
using namespace OVR;

class Rift : public Ogre::RenderTargetListener
{
	public:
		// Contructor initiates Rift Rendering environment, as follows:
		//	- looks for Rift physical device (it can be FOUND or NOT FOUND)
		//	- create a new scene under root with:
		//		- two distorsion meshes so that render results can be displayed on their textures
		//		- an ortho camera looking at the meshes, so final render can be displayed into Rift
		//	- creates a window to render the ortho camera:
		//		- IF renderWindow == nullptr
		//			- if Rift is FOUND, creates a full screen window in the secondary screen (default: id=1)
		//			- if Rift is NOT FOUND, creates a window in the default screen
		//		- IF renderWindow != nullptr, result of ortho camera is displayed regardless of Oculus state
		//	- rotates the camera/meshes by 90 degrees if rotateView == true (DK2 Setup)
		Rift( const unsigned int ID, Ogre::Root* const root, Ogre::RenderWindow* &renderWindow, const bool rotateView = false);
		~Rift();

		// Call this if you want to manually associate a render window to the Rift
		// Current used window will be replaced
		// void setWindow(Ogre::Root* root, Ogre::RenderWindow* renderWindow, bool rotateView);

		// Create stereo rig in your scene, then call this function to setup them the best way possible for Oculus
		void setCameraMatrices( Ogre::Camera* camLeft, Ogre::Camera* camRight );
		// Call this function to render the two cameras on Oculus distortion meshes
		void attachCameras(Ogre::Camera* const camLeft, Ogre::Camera* const camRight);

		// Update Rift data every frame. This should return true as long as data is read from rift.
		bool update( float dt );
		void pauseRender(bool pauseRender) { pause = pauseRender; }
		bool pause = false;

		// Pre-render listeners (for reduced latency and time-warping)
		virtual void preRenderTargetUpdate(const Ogre::RenderTargetEvent& rte);
		virtual void postRenderTargetUpdate(const Ogre::RenderTargetEvent& rte);

		Ogre::Quaternion getOrientation() { return mOrientation; }
		Ogre::Vector3 getPosition() { return mPosition; }

		// returns interpupillary distance in meters: (Default: 0.064m)
		float getIPD() { return mIPD; }

		// TEMP
		ovrHmd getHandle() { return hmd; }

		Ogre::SceneManager* getSceneMgr() { return mSceneMgr; }

		// Used to reset head position and orientation to "foreward".
		// Call this when user presses 'R', for example.
		void recenterPose();

		Ogre::RenderWindow* createRiftDisplayWindow(Ogre::Root* const root);
		Ogre::RenderWindow* createDebugRiftDisplayWindow(Ogre::Root* const root);
		void createRiftDisplayScene(Ogre::Root* const root);

		Ogre::Camera* getCamera(){ return mCamera; }


		Ogre::SceneNode* mHeadNode = nullptr;
		/*
		well why be an arist if the only thing you could give is the way you express it?

		*/

	private:

		// Rift data
		int mRiftID = 0;
		ovrHmd hmd = nullptr;
		bool rotateView = false;
		bool simulationMode = false;
		float mIPD = 0.064f;
		ovrFrameTiming frameTiming;
		static bool isInitialized;
		static unsigned short int ovr_Users;
		ovrEyeType nextEyeToRender;
		ovrEyeRenderDesc eyeRenderDesc[2];
		ovrPosef headPose[2];
		static void init();
		static void shutdown();

		// Oculus Rift Outer Scene (head pose)
		//Ogre::SceneNode* mHeadNode = nullptr;
		Ogre::Vector3 mPosition;
		Ogre::Quaternion mOrientation;
		// Oculus Rift Inner Scene representation (barrell effect and timewarping for optimal Oculus display)
		Ogre::SceneManager* mSceneMgr = nullptr;
		Ogre::SceneNode* mCamNode = nullptr;
		Ogre::Camera* mCamera = nullptr;
		Ogre::TexturePtr mLeftEyeRenderTexture;
		Ogre::TexturePtr mRightEyeRenderTexture;
		Ogre::MaterialPtr mMatLeft;
		Ogre::MaterialPtr mMatRight;
		Ogre::RenderTexture* mRenderTexture[2];

		
		
		// Oculus Rift Display rendering window (displayed on Oculus)
		Ogre::RenderWindow* mRenderWindow = nullptr;
		Ogre::Viewport* mViewport = nullptr;

	
};

#endif
