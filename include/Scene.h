#ifndef SCENE_H
#define SCENE_H

#include "OGRE/Ogre.h"
#include "OIS/OIS.h"
#include "Globals.h"

class Scene : public Ogre::Camera::Listener
{
	public:

		enum CameraModel
		{
			Pinhole,
			Fisheye
		};

		Scene( Ogre::Root* root, OIS::Mouse* mouse, OIS::Keyboard* keyboard );
		~Scene();

		Ogre::SceneManager* getSceneMgr() { return mSceneMgr; }
		Ogre::Camera* getLeftCamera() { return mCamLeft; }
		Ogre::Camera* getRightCamera() { return mCamRight; }
		Ogre::Camera* getGodCamera() { return mCamGod; }

		// One-call functions for SeeThrough rig setup
		void setupVideo(const CameraModel modelToUse, const Ogre::Vector3 eyeToCameraOffset, const float HFov, const float VFov);
		void setupVideo(const CameraModel modelToUse, const Ogre::Vector3 eyeToCameraOffset, const float WSensor, const float HSensor, const float FL);

		// Functions for setup
		void setIPD(float IPD);
		void enableVideo();
		void disableVideo();
		float adjustVideoDistance(const float distIncrement);
		float adjustVideoFov(const float increment);
		void setCameraOffset(const Ogre::Vector3 newCameraOffset);

		// Update functions
		void update( float dt );
		void setRiftPose( Ogre::Quaternion orientation, Ogre::Vector3 pos );
		void setVideoImagePoseLeft(const Ogre::PixelBox &image, Ogre::Quaternion pose);
		void setVideoImagePoseRight(const Ogre::PixelBox &image, Ogre::Quaternion pose);
		//void setCameraTextureRight();
	
		// Keyboard and mouse events (forwarded by App)
		bool keyPressed(const OIS::KeyEvent&);
		bool keyReleased(const OIS::KeyEvent&);
		bool mouseMoved(const OIS::MouseEvent&);
		bool mousePressed(const OIS::MouseEvent&, OIS::MouseButtonID);
		bool mouseReleased(const OIS::MouseEvent&, OIS::MouseButtonID);

		// Virtual Camera events (deriving from Ogre::Camera::Listener)
		void cameraPostRenderScene(Ogre::Camera* cam);
		void cameraPreRenderScene(Ogre::Camera* cam);

	private:

		CameraModel currentCameraModel = Fisheye;
		float cameraHFov;
		float cameraVFov;

		bool videoIsEnabled = false;
		float videoHFov;
		float videoVFov;

		// Default value is 1. This value means:
		// - plane scaled with factor x, at distance x meters from the virtual camera
		// - sphere scaled with factor x, radius x meters with center in virtual camera
		// Warning: this only affects video mesh scale, keeping percieved FOV constant
		float videoClippingScaleFactor = 1.0f;
		
		// Default value is 1. This value means:
		// - plane is scaled along its XY axis by an additional x factor
		// - sphere is scaled along its Z axis by an additional x factor
		// Increase in this value means increase in percieved fov of the video image.
		// This value may be used to compensate the "too close" effect of real objects
		// caused by the existing displacement between real camera and the eye.
		// Warning: this won't solve the problem, since this should vary depending on how far is the real object.
		float videoFovScaleFactor = 1.0f;

		// Default value is (0,0,0), which means:
		// - effects on camera relative distance on the eye are ignored
		// - app will behave like real cameras are in the same position of the eye
		Ogre::Vector3 cameraOffset = Ogre::Vector3::ZERO;

		// Initialising/Loading the scene
		void createRoom();
		void createCameras();
		void createPinholeVideos(const float WPlane, const float HPlane, const Ogre::Vector3 offset);
		void createFisheyeVideos(const Ogre::Vector3 offset);
		void updateVideos();	// called only when a parameter is adjusted

		Ogre::Root* mRoot = nullptr;
		OIS::Mouse* mMouse = nullptr;
		OIS::Keyboard* mKeyboard = nullptr;
		Ogre::SceneManager* mSceneMgr = nullptr;

		Ogre::Camera* mCamLeft = nullptr;
		Ogre::Camera* mCamRight = nullptr;

		Ogre::Image mLeftCameraRenderImage;
		Ogre::Image mRightCameraRenderImage;

		Ogre::TexturePtr mLeftCameraRenderTexture;
		Ogre::TexturePtr mRightCameraRenderTexture;

		Ogre::MaterialPtr mLeftCameraRenderMaterial;
		Ogre::MaterialPtr mRightCameraRenderMaterial;
		
		Ogre::Camera* mCamGod = nullptr;

		Ogre::SceneNode* mVideoLeft = nullptr;
		Ogre::SceneNode* mCamLeftReference = nullptr;
		Ogre::SceneNode* mVideoRight = nullptr;
		Ogre::SceneNode* mCamRightReference = nullptr;
		Ogre::SceneNode* mHeadStabilizationNodeLeft = nullptr;			// on this we apply Image stabilization full Orientation delta
		Ogre::SceneNode* mHeadStabilizationNodeRight = nullptr;			// on this we apply Image stabilization full Orientation delta
		Ogre::SceneNode* mHeadNode = nullptr;						// on this we apply Head full Orientation (Yaw/Pitch/Roll)
		Ogre::SceneNode* mBodyTiltNode = nullptr;					// on this we apply Body Pitch transformation (up/down turn)
		Ogre::SceneNode* mBodyYawNode = nullptr;					// on this we apply Body Yaw transformation (left/right turn)
		Ogre::SceneNode* mBodyNode = nullptr;						// on this we apply Body position transformation (=mBodyYawNode)

		Ogre::SceneNode* mRoomNode = nullptr;
};

#endif
