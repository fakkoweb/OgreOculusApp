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

		enum StabilizationModel
		{
			Head,
			Eye
		};

		Scene( Ogre::Root* root, OIS::Mouse* mouse, OIS::Keyboard* keyboard );
		~Scene();

		Ogre::SceneManager* getSceneMgr() { return mSceneMgr; }
		Ogre::Camera* getLeftCamera() { return mCamLeft; }
		Ogre::Camera* getRightCamera() { return mCamRight; }
		Ogre::Camera* getGodCamera() { return mCamGod; }

		// One-call functions for SeeThrough rig setup
		void setupVideo(const CameraModel camModelToUse, const StabilizationModel stabModelToUse, const Ogre::Vector3 eyeToCameraOffset, const float HFov, const float VFov);
		void setupVideo(const CameraModel camModelToUse, const StabilizationModel stabModelToUse, const Ogre::Vector3 eyeToCameraOffset, const float WSensor, const float HSensor, const float FL);

		// Functions for setup
		void setIPD(float IPD);
		void enableVideo();
		void disableVideo();
		void setStabilizationMode(StabilizationModel modelToUse);

		// Functions for realtime runtime calibration
		float adjustVideoDistance(const float distIncrement);
		float adjustVideoFov(const float increment);
		Ogre::Vector2 adjustVideoLeftTextureCalibrationOffset(const Ogre::Vector2 offsetIncrement);
		Ogre::Vector2 adjustVideoRightTextureCalibrationOffset(const Ogre::Vector2 offsetIncrement);
		void setVideoLeftTextureCalibrationAspectRatio(const float ar);
		float adjustVideoLeftTextureCalibrationAspectRatio(const float arIncrement);
		void setVideoRightTextureCalibrationAspectRatio(const float ar);
		float adjustVideoRightTextureCalibrationAspectRatio(const float arIncrement);
		float adjustVideoLeftTextureCalibrationScale(const float incrementFactor);
		float adjustVideoRightTextureCalibrationScale(const float incrementFactor);
		void setVideoOffset(const Ogre::Vector3 newVideoOffset);

		// Update functions
		void update( float dt );
		void setRiftPose( Ogre::Quaternion orientation, Ogre::Vector3 pos );
		void setVideoImagePoseLeft(const Ogre::PixelBox &image, Ogre::Quaternion pose);
		void setVideoImagePoseRight(const Ogre::PixelBox &image, Ogre::Quaternion pose);
		// Apply relative AR pose and save it as absolute in world coordinates
		void setCubePosition(Ogre::Vector3 pos){ mCubeRedReference->setPosition(pos); mCubeRed->setPosition(mCubeRedReference->_getDerivedPosition()); }
		void setCubeOrientation(Ogre::Quaternion ori){ mCubeRedReference->setOrientation(ori); mCubeRed->setOrientation(mCubeRedReference->_getDerivedOrientation()); };
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

		Ogre::SceneNode* mHeadNode = nullptr;
	private:

		CameraModel currentCameraModel = Fisheye;
		StabilizationModel currentStabilizationModel = Head;
		float cameraHFov;
		float cameraVFov;

		bool videoIsEnabled = false;
		float videoHFov;
		float videoVFov;

		// Default value is 6.4 (about 100*IPD). This value means:
		// - plane scaled with factor x, at distance x meters from the virtual camera
		// - sphere scaled with factor x, radius x meters with center in virtual camera
		// Warning: this only affects video mesh scale, keeping percieved FOV constant
		float videoClippingScaleFactor = 6.4f;
		
		// Default value is 1. This value means:
		// - plane is scaled along its XY axis by an additional 1/x factor
		// - sphere is scaled along its Z axis by an additional x factor
		// Increase in this value means increase in percieved fov of the video image.
		// This value may be used to compensate the "too close" effect of real objects
		// caused by the existing displacement between real camera and the eye.
		// Warning: this won't solve the problem, since this should vary depending on how far is the real object.
		float videoFovScaleFactor = 1.0f;

		// This value is used to alter video mesh UV coordinates and calibrate projection (Fisheye model).
		// Adjusting Offset, fisheye TEXTURE will move right/left or up/down.
		// N.B. This is achieved by actually subtracting this values from UV coordinates.
		// This is very useful since sensor proportions and the center of fisheye circle on the sensor
		// vary from user to user.
		// User can find the best value (for his camera/lens setup) when the center of fisheye circle
		// (the part of the image with less distortion) is in front of the eye.
		// It is suggested to adjust this while having small texture scale: when image is small enough, it will show
		// as a small circle and therefore easier to position.
		// VALUES CAN BE DIFFERENT BETWEEN THE TWO CAMERAS, SO TWO DIFFERENT VARIABLES ARE USED.
		// Default value is (-0.5,-0.5). For this project, UV coordinates of the sphere central vertex is 0,0
		// (which corresponds to bottom-left edge of the texture). We want the center of the image in front of the user,
		// therefore we translate all UV points by 0.5, which means to translate the image texture by -0.5.
		Ogre::Vector2 videoLeftTextureCalibrationOffset = Ogre::Vector2(-0.5f,-0.5f);
		Ogre::Vector2 videoRightTextureCalibrationOffset = Ogre::Vector2(-0.5f,-0.5f);

		// This value is used to alter video mesh UV coordinates and calibrate projection (Fisheye model).
		// Adjusting AspectRatio, fisheye texture will get wider along x.
		// User will find the correct value (for his camera/lens setup) when image will appear as a perfect
		// circle.
		// This value is the exact aspect ratio of the image user is using (ex. 4:3 -> 1.333)
		// VALUES CAN BE DIFFERENT BETWEEN THE TWO CAMERAS, SO THEY ARE USED SEPARATELY.
		// Default value is 1.0, which means square image. For now this value must be setup manually..
		float videoLeftTextureCalibrationAspectRatio = 1.0f;
		float videoRightTextureCalibrationAspectRatio = 1.0f;

		// This value is used to alter video mesh UV coordinates and calibrate projection (Fisheye model).
		// Adjusting Scale, fisheye texture will get smaller or wider.
		// User will find the correct value (for his camera/lens setup) when lines supposed to be straight
		// will show as straight in the headset.
		// VALUES CAN BE DIFFERENT BETWEEN THE TWO CAMERAS, SO THEY ARE USED SEPARATELY.
		float videoLeftTextureCalibrationScale = 1.0f;
		float videoRightTextureCalibrationScale = 1.0f;

		// This vector is an offset added to to plane/sphere mesh relative position from the virtual
		// camera. It was supposed to model the displacement between the real camera and the eye.
		// Default value is (0,0,0) which means:
		// - videoClippingScaleFactor is the only parameter influencing position
		// - app will behave like real cameras are in the same position of the eye
		// This parameter is currently unused by the application, but is left for future use.
		// Z value means forward/back from the nose.
		// Y value means up/down from the nose.
		// X value means farther/closer to the nose: applied symmetrically (right/left axis)
		Ogre::Vector3 videoOffset = Ogre::Vector3::ZERO;

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
		Ogre::SceneNode* mLeftStabilizationNode = nullptr;			// to this we assign one of the two nodes used for stabilization (neck or eye)
		Ogre::SceneNode* mRightStabilizationNode = nullptr;			// to this we assign one of the two nodes used for stabilization (neck or eye)

		Ogre::Image mLeftCameraRenderImage;
		Ogre::Image mRightCameraRenderImage;

		Ogre::TexturePtr mLeftCameraRenderTexture;
		Ogre::TexturePtr mRightCameraRenderTexture;

		Ogre::MaterialPtr mLeftCameraRenderMaterial;
		Ogre::MaterialPtr mRightCameraRenderMaterial;
		
		Ogre::Camera* mCamGod = nullptr;
	
		// This is the implementation of Enhanced Head Mode, with some additions:
		// - there is an additional nodes: mHeadStabilizationNode, which can apply stabilization in respect to head (just to experiment)
		// - pointers mLeftStabilizationNode and mRightStabilizationNode select if stabilization should be around eye (default) or head (to experiment)
		// - since before render for each eye is rendered a new head pose is set, also delta of stabilization should be recalculated; to
		//	 avoid this, setInheritOrientation(false) is set on the current used stabilization nodes (so the delta needs to be applied just once, just with a new frame)

		// This is the implementation of Enhanced Head Mode, with some additions:
		// - there is an additional nodes: mHeadStabilizationNode, which can apply stabilization in respect to head (just to experiment)
		// - pointers mLeftStabilizationNode and mRightStabilizationNode select if stabilization should be around eye (default) or head (to experiment)
		// - since before render for each eye is rendered a new head pose is set, also delta of stabilization should be recalculated; to
		//	 avoid this, setInheritOrientation(false) is set on the current used stabilization nodes (so the delta needs to be applied just once, just with a new frame)
		Ogre::SceneNode* mVideoLeft = nullptr;
		Ogre::SceneNode* mVideoRight = nullptr;
		Ogre::SceneNode* mCamLeftStabilizationNode = nullptr;		// on this we apply Image EYE stabilization full Orientation delta (if active)
		Ogre::SceneNode* mCamRightStabilizationNode = nullptr;		// on this we apply Image EYE stabilization full Orientation delta (if active)
		Ogre::SceneNode* mCamLeftReference = nullptr;				// this node just keeps track of mCamLeft position
		Ogre::SceneNode* mCamRightReference = nullptr;				// this node just keeps track of mCamRight position
		Ogre::SceneNode* mHeadStabilizationNodeLeft = nullptr;		// on this we apply Image NECK stabilization full Orientation delta (if active)
		Ogre::SceneNode* mHeadStabilizationNodeRight = nullptr;		// on this we apply Image NECK stabilization full Orientation delta (if active)
		//Ogre::SceneNode* mHeadNode = nullptr;						// on this we apply Head full Orientation (Yaw/Pitch/Roll)
		Ogre::SceneNode* mBodyTiltNode = nullptr;					// on this we apply Body Pitch transformation (up/down turn)
		Ogre::SceneNode* mBodyYawNode = nullptr;					// on this we apply Body Yaw transformation (left/right turn)
		Ogre::SceneNode* mBodyNode = nullptr;						// on this we apply Body position transformation (=mBodyYawNode)

		Ogre::SceneNode* mRoomNode = nullptr;
		Ogre::SceneNode* mCubeRed = nullptr;
		Ogre::SceneNode* mCubeRedReference = nullptr;
		Ogre::SceneNode* mCubeRedOffset = nullptr;
		Ogre::SceneNode* mCubeGreen = nullptr;
};

#endif
