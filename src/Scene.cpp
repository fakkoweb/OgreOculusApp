
#include "Scene.h"
#include <cmath>
#define _USE_MATH_DEFINES
#include <math.h>


bool camera_frame_updated = false;

//////////////////////////////////////////////////////////////
// Initialization and initial setup:
//////////////////////////////////////////////////////////////
Scene::Scene( Ogre::Root* root, OIS::Mouse* mouse, OIS::Keyboard* keyboard )
{
	mRoot = root;
	mMouse = mouse;
	mKeyboard = keyboard;
	mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC); 

	// Set up main Lighting and Shadows in Scene:
	mSceneMgr->setAmbientLight( Ogre::ColourValue(0.1f,0.1f,0.1f) );
	mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);

	mSceneMgr->setShadowFarDistance( 30 );

	createRoom();
	createCameras();
	//Video is created on setupVideo() call
	//createPinholeVideos();
}
Scene::~Scene()
{
	if (mSceneMgr) delete mSceneMgr;
}

void Scene::createRoom()
{
	mRoomNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("RoomNode");
	Ogre::SceneNode* cubeNode = mRoomNode->createChildSceneNode();
	Ogre::Entity* cubeEnt = mSceneMgr->createEntity( "Cube.mesh" );
	cubeEnt->getSubEntity(0)->setMaterialName( "CubeMaterialRed" );
	cubeNode->attachObject( cubeEnt );
	cubeNode->setPosition( 1.0, 0.0, 0.0 );
	Ogre::SceneNode* cubeNode2 = mRoomNode->createChildSceneNode();
	Ogre::Entity* cubeEnt2 = mSceneMgr->createEntity( "Cube.mesh" );
	cubeEnt2->getSubEntity(0)->setMaterialName( "CubeMaterialGreen" );
	cubeNode2->attachObject( cubeEnt2 );
	cubeNode2->setPosition( 3.0, 0.0, 0.0 );
	cubeNode->setScale( 0.5, 0.5, 0.5 );
	cubeNode2->setScale( 0.5, 0.5, 0.5 );
	
	Ogre::SceneNode* cubeNode3 = mRoomNode->createChildSceneNode();
	Ogre::Entity* cubeEnt3 = mSceneMgr->createEntity( "Cube.mesh" );
	cubeEnt3->getSubEntity(0)->setMaterialName( "CubeMaterialWhite" );
	cubeNode3->attachObject( cubeEnt3 );
	cubeNode3->setPosition( -1.0, 0.0, 0.0 );
	cubeNode3->setScale( 0.5, 0.5, 0.5 );

	Ogre::Entity* roomEnt = mSceneMgr->createEntity( "Room.mesh" );
	roomEnt->setCastShadows( false );
	mRoomNode->attachObject( roomEnt );

	Ogre::Light* roomLight = mSceneMgr->createLight();
	roomLight->setType(Ogre::Light::LT_POINT);
	roomLight->setCastShadows( false );
	roomLight->setShadowFarDistance( 30 );
	roomLight->setAttenuation( 65, 1.0, 0.07, 0.017 );
	roomLight->setSpecularColour( .25, .25, .25 );
	roomLight->setDiffuseColour( 0.85, 0.76, 0.7 );

	roomLight->setPosition( 5, 5, 5 );

	mRoomNode->attachObject( roomLight );
}
void Scene::createCameras()
{
	mCamLeft = mSceneMgr->createCamera("LeftCamera");
	mCamRight = mSceneMgr->createCamera("RightCamera");
	mCamGod = mSceneMgr->createCamera("GodCamera");

	// Create base scene node for body positioning
	mBodyNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("BodyNode");
	// Create scene nodes for body orientation in space (it is easier to separate Yaw and Tilt)
	mBodyYawNode = mBodyNode;								// base is used also as Yaw node
	mBodyYawNode->setFixedYawAxis(true);					// don't roll! (Useful when using lookat())
	mBodyTiltNode = mBodyYawNode->createChildSceneNode();	// Tilt node is child of Yaw node
	// Create head node where virtual stereo rig will be attached:
	mHeadNode = mBodyTiltNode->createChildSceneNode("HeadNode");

	// Prepare two Ogre SceneNodes (mHeadStabilizationNode) child of mHeadNode that will be used for image stabilization
	// NOTE:	since any real camera has a delay, we want to move the image shown in the head pose as it was taken, not in the current headpose.
	//			If the feature is disabled, mHeadStabilizationNode will simply follow mHeadNode position/orientation
	mHeadStabilizationNodeLeft = mHeadNode->createChildSceneNode("HeadStabilizationNodeLeft");
	mHeadStabilizationNodeRight = mHeadNode->createChildSceneNode("HeadStabilizationNodeRight");
	
	// Attach cameras
	// Cameras are created with a STANDARD INITIAL projection matrix (Ogre's default)
	// Cameraa projection matrices should be set later to SDK suggested values by calling setCameraMatrices()
	mHeadNode->attachObject(mCamLeft);
	mHeadNode->attachObject(mCamRight);
	mCamLeft->setFarClipDistance(50);
	mCamRight->setFarClipDistance(50);
	mCamLeft->setNearClipDistance(0.001);
	mCamRight->setNearClipDistance(0.001);

	// Camera positioning to a STANDARD INITIAL interpupillary and eyetoneck distance
	// It should be set later to SDK user custom value by calling setIPD() method
	float default_IPdist = 0.064f;			// distance between the eyes
	float default_HorETNdist = 0.0805f;		// horizontal distance from the eye to the neck
	float default_VerETNdist = 0.075f;		// vertical distance from the eye to the neck
	mCamLeft->setPosition ( -default_IPdist / 2.0f , default_VerETNdist, -default_HorETNdist ); //x is side, y is up, z is towards the screen
	mCamRight->setPosition( default_IPdist / 2.0f  , default_VerETNdist, -default_HorETNdist );

	/*
	// We want to reuse the image shown by the stereo cameras (since they present the undistorted image)
	// We must then make sure that
	// Create render textures to output undistorted image for AR/3rd party usage
	//Create two special textures (TU_RENDERTARGET) that will be applied to the two videoPlaneEntities
	mLeftCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraLeft", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1920, 1080, 0, Ogre::PF_R8G8B8,
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	mRightCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraRight", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1920, 1080, 0, Ogre::PF_R8G8B8,
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	*/

	// OTHER SETUP

	// If possible, get the camera projection matricies given by the rift:	
	/*if (mRift->isAttached())
	{
		mCamLeft->setCustomProjectionMatrix( true, mRift->getProjectionMatrix_Left() );
		mCamRight->setCustomProjectionMatrix( true, mRift->getProjectionMatrix_Right() );
	}*/


	/*mHeadLight = mSceneMgr->createLight();
	mHeadLight->setType(Ogre::Light::LT_POINT);
	mHeadLight->setCastShadows( false );
	mHeadLight->setShadowFarDistance( 30 );
	mHeadLight->setAttenuation( 65, 1.0, 0.07, 0.017 );
	mHeadLight->setSpecularColour( 1.0, 1.0, 1.0 );
	mHeadLight->setDiffuseColour( 1.0, 1.0, 1.0 );
	mHeadNode->attachObject( mHeadLight );*/

	mBodyNode->setPosition( 4.0, 1.5, 4.0 );
	//mBodyYawNode->lookAt( Ogre::Vector3::ZERO, Ogre::SceneNode::TS_WORLD );


	// Position God camera according to the room
	mRoomNode->attachObject(mCamGod);
	mCamGod->setFarClipDistance(50);
	mCamGod->setNearClipDistance(0.001);
	mCamGod->setPosition(5, 5, 5);
	mCamGod->lookAt(mBodyNode->getPosition());


	// Position a light on the body
	Ogre::Light* light = mSceneMgr->createLight();
	light->setType(Ogre::Light::LT_POINT);
	light->setCastShadows( false );
	light->setAttenuation( 65, 1.0, 0.07, 0.017 );
	light->setSpecularColour( .25, .25, .25 );
	light->setDiffuseColour( 0.35, 0.27, 0.23 );
	mBodyNode->attachObject(light);
}
void Scene::setupVideo(const CameraModel modelToUse, const Ogre::Vector3 eyeToCameraOffset, const float HFov, const float VFov)
{
	if (!mVideoLeft)
	{
		if (HFov > 0 && VFov > 0)
		{
			// save initial values as backup for later adjustments
			cameraHFov = HFov;
			cameraVFov = VFov;

			float hFovRads = (HFov * M_PI) / 180;
			float vFovRads = (VFov * M_PI) / 180;
			float planeWidth;
			float planeHeight;

			switch (modelToUse)
			{
			case Pinhole:
				planeWidth = std::tan(hFovRads / 2) * videoClippingScaleFactor * 2;
				planeHeight = std::tan(vFovRads / 2) * videoClippingScaleFactor * 2;
				createPinholeVideos(planeWidth, planeHeight, eyeToCameraOffset);
				break;
			case Fisheye:
				createFisheyeVideos(eyeToCameraOffset);
				break;
			default:
				throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid Camera Model selection!", "Scene::setupVideo");
				break;
			}

			// save current values for later adjustments
			currentCameraModel = modelToUse;
			videoHFov = cameraHFov;
			videoVFov = cameraVFov;

			// update position/scale video with parameters
			updateVideos();

		}
		else
		{
			throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid HFov or VFov input values", "Scene::setupVideo");
		}
	}
	else
	{
		throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "Video has already been setup and created into Scene. setupVideo() must be called only once for Scene instance", "Scene::setupVideo");
	}
}
void Scene::setupVideo(const CameraModel modelToUse, const Ogre::Vector3 eyeToCameraOffset, const float WSensor, const float HSensor, const float FL)
{
	if (!mVideoLeft)
	{
		if (WSensor > 0 && HSensor > 0 && FL > 0)
		{

			float planeWidth;
			float planeHeight;

			switch (modelToUse)
			{
			case Pinhole:
				planeWidth = (WSensor * videoClippingScaleFactor) / FL;
				planeHeight = (HSensor * videoClippingScaleFactor) / FL;
				createPinholeVideos(planeWidth, planeHeight, eyeToCameraOffset);
				// compute current fov values for later adjustments
				cameraHFov = ((2 * std::atan(planeWidth / (2 * videoClippingScaleFactor))) * 180) / M_PI;
				cameraVFov = ((2 * std::atan(planeHeight / (2 * videoClippingScaleFactor))) * 180) / M_PI;

			case Fisheye:
				createFisheyeVideos(eyeToCameraOffset);
				break;
			default:
				throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid Camera Model selection!", "Scene::setupVideo");
				break;
			}

			// save current values for later adjustments
			currentCameraModel = modelToUse;
			videoHFov = cameraHFov;
			videoVFov = cameraVFov;
		}
		else
		{
			throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid WSensor, HSensor or FL input values", "Scene::setupVideo");
		}
	}
	else
	{
		throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "Video has already been setup and created into Scene. setupVideo() must be called only once for Scene instance", "Scene::setupVideo");
	}
}
void Scene::createPinholeVideos(const float WPlane, const float HPlane, const Ogre::Vector3 offset = Ogre::Vector3::ZERO)
{
	// CREATE SHAPES
	// -------------

	//Create an Plane class instance that describes our plane (no position or orientation, just mathematical description)
	Ogre::Plane videoPlane(Ogre::Vector3::UNIT_Z, 0);

	//Create a static mesh out of the plane (as a REUSABLE "resource")
	Ogre::MeshManager::getSingleton().createPlane(
		"videoMesh",										// this is the name that our resource will have for the whole application!
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		videoPlane,											// this is the instance from which we build the mesh
		WPlane, HPlane, 1, 1,
		true,
		1, 1, 1,
		Ogre::Vector3::UNIT_Y);								// this is the vector that will be used as mesh UP direction

	//Create an ogre Entity out of the resource we created (more Entities can be created out of a resource!)
	Ogre::Entity* videoPlaneEntityLeft = mSceneMgr->createEntity("videoMesh");
	Ogre::Entity* videoPlaneEntityRight = mSceneMgr->createEntity("videoMesh");

	//Attach to each mHeadStabilizationNode a SceneNode that follows virtual camera position in respect to the neck
	//so that we can deal with mVideoLeft and mVideoRight positions relatively to each camera despite stabilization node
	mCamLeftReference = mHeadStabilizationNodeLeft->createChildSceneNode("HeadStabilizationCameraReferenceLeft");
	mCamLeftReference->setPosition(mCamLeft->getPosition());
	mCamRightReference = mHeadStabilizationNodeRight->createChildSceneNode("HeadStabilizationCameraReferenceRight");
	mCamRightReference->setPosition(mCamRight->getPosition());

	//Finally attach mVideo nodes to camera references
	mVideoLeft = mCamLeftReference->createChildSceneNode("LeftVideo");
	mVideoRight = mCamRightReference->createChildSceneNode("RightVideo");

	//Attach videoPlaneEntityLeft to mVideoLeft SceneNode (now it will have a Position/Scale/Orientation)
	mVideoLeft->attachObject(videoPlaneEntityLeft);
	mVideoRight->attachObject(videoPlaneEntityRight);

	//Setup videoPlaneEntityLeft rendering proprieties (INDEPENDENT FROM mVideoLeft SceneNode!!)
	videoPlaneEntityLeft->setCastShadows(false);
	videoPlaneEntityRight->setCastShadows(false);
	//videoPlaneEntityRight->setMaterialName("CubeMaterialWhite");

	// Save updated camera offset (for later runtime adjustments)
	videoOffset = offset;

	//Set camera listeners to this class (so that I can do stuff before and after each renders)
	//mCamLeft->addListener(this);			// THIS IS DONE WHEN SEETHROUGH FEATURE IS ENABLED

	//mVideoLeft is disabled by default: enableVideo() and disableVideo() methods are provided
	mVideoLeft->setVisible(false, false);
	mVideoRight->setVisible(false, false);


	// CREATE TEXTURES
	// ---------------

	//Create two special textures (TU_RENDERTARGET) that will be applied to the two videoPlaneEntities
	mLeftCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraLeft", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1920, 1080, 0, Ogre::PF_R8G8B8,
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	mRightCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraRight", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1920, 1080, 0, Ogre::PF_R8G8B8,
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

	// Creare new materials and assign the two textures that can be used on the shapes created
	// WARNING: apparently modifying these lines in another equivalent form will cause errors!!
	mLeftCameraRenderMaterial = Ogre::MaterialManager::getSingleton().create("Scene/LeftCamera", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::Technique *technique1 = mLeftCameraRenderMaterial->createTechnique();
	technique1->createPass();
	mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("RenderTextureCameraLeft");
	//mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mLeftCameraRenderTexture);
	mRightCameraRenderMaterial = Ogre::MaterialManager::getSingleton().create("Scene/RightCamera", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::Technique *technique2 = mRightCameraRenderMaterial->createTechnique();
	technique2->createPass();
	mRightCameraRenderMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("RenderTextureCameraRight");

	// Assign materials to videoPlaneEntities
	videoPlaneEntityLeft->setMaterial(mLeftCameraRenderMaterial);
	videoPlaneEntityRight->setMaterial(mRightCameraRenderMaterial);

	// Retrieve the "render target pointer" from the two textures (so we can use it as a standard render target as a window)
	//Ogre::RenderTexture* mLeftCameraRenderTextureA = mLeftCameraRenderTexture->getBuffer()->getRenderTarget();

	// Link the cameras to the render targets
	//mLeftCameraRenderTextureA->addViewport(mCamLeft);	// no value parameters = camera will take the whole viewport space
	//mLeftCameraRenderTextureA->getViewport(0)->setClearEveryFrame(true);	// false = infinite trails effect
	//mLeftCameraRenderTextureA->getViewport(0)->setBackgroundColour(Ogre::ColourValue::Black);
	//mLeftCameraRenderTextureA->getViewport(0)->setOverlaysEnabled(false);	// false = no overlays rendered, if any
}
void Scene::createFisheyeVideos(const Ogre::Vector3 offset = Ogre::Vector3::ZERO)
{
	// CREATE SHAPES
	// -------------

	//Create an entity out of a loaded static mesh (a half sphere modeled/uvmapped in blender)
	Ogre::Entity* videoSphereEntityLeft = mSceneMgr->createEntity("FisheyeUndistort.polysphere.mesh");
	Ogre::Entity* videoSphereEntityRight = mSceneMgr->createEntity("FisheyeUndistort.polysphere.mesh");

	//Attach to each mHeadStabilizationNode a SceneNode that follows virtual camera position in respect to the neck
	//so that we can deal with mVideoLeft and mVideoRight positions relatively to each camera despite stabilization node
	mCamLeftReference = mHeadStabilizationNodeLeft->createChildSceneNode("HeadStabilizationCameraReferenceLeft");
	mCamLeftReference->setPosition(mCamLeft->getPosition());
	mCamRightReference = mHeadStabilizationNodeRight->createChildSceneNode("HeadStabilizationCameraReferenceRight");
	mCamRightReference->setPosition(mCamRight->getPosition());

	//Finally attach mVideo nodes to camera references
	mVideoLeft = mCamLeftReference->createChildSceneNode("LeftVideo");
	mVideoRight = mCamRightReference->createChildSceneNode("RightVideo");

	//Attach videoPlaneEntityLeft to mVideoLeft SceneNode (now shape will have a Position/Scale/Orientation)
	mVideoLeft->attachObject(videoSphereEntityLeft);
	mVideoRight->attachObject(videoSphereEntityRight);

	//Setup videoPlaneEntityLeft rendering proprieties (INDEPENDENT FROM mVideoLeft SceneNode!!)
	videoSphereEntityLeft->setCastShadows(false);
	videoSphereEntityRight->setCastShadows(false);

	// Save updated camera offset (for later runtime adjustments)
	videoOffset = offset;

	//Set camera listeners to this class (so that I can do stuff before and after each renders)
	//mCamLeft->addListener(this);			// THIS IS DONE WHEN SEETHROUGH FEATURE IS ENABLED

	//mVideoLeft is disabled by default: enableVideo() and disableVideo() methods are provided
	mVideoLeft->setVisible(false, false);
	mVideoRight->setVisible(false, false);


	// CREATE TEXTURES
	// ---------------
	//Create two special textures (TU_RENDERTARGET) that will be applied to the two videoPlaneEntities
	mLeftCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraLeft", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1920, 1080, 0, Ogre::PF_R8G8B8,
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	mRightCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraRight", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1920, 1080, 0, Ogre::PF_R8G8B8,
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	
	// Load scripted materials and assign the two dynamic textures just created
	// (Remember: UV mapping of the mesh loaded is independent from the texture loaded)
	mLeftCameraRenderMaterial = Ogre::MaterialManager::getSingleton().getByName("FisheyeImageMappingMaterial/LeftEye");
	//mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mLeftCameraRenderTexture);
	mRightCameraRenderMaterial = Ogre::MaterialManager::getSingleton().getByName("FisheyeImageMappingMaterial/RightEye");
	//mRightCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mRightCameraRenderTexture);

	// Assign materials to videoPlaneEntities
	videoSphereEntityLeft->setMaterial(mLeftCameraRenderMaterial);
	videoSphereEntityRight->setMaterial(mRightCameraRenderMaterial);
	
	// Retrieve the "render target pointer" from the two textures (so we can use it as a standard render target as a window)
	//Ogre::RenderTexture* mLeftCameraRenderTextureA = mLeftCameraRenderTexture->getBuffer()->getRenderTarget();

	// Link the cameras to the render targets
	//mLeftCameraRenderTextureA->addViewport(mCamLeft);	// no value parameters = camera will take the whole viewport space
	//mLeftCameraRenderTextureA->getViewport(0)->setClearEveryFrame(true);	// false = infinite trails effect
	//mLeftCameraRenderTextureA->getViewport(0)->setBackgroundColour(Ogre::ColourValue::Black);
	//mLeftCameraRenderTextureA->getViewport(0)->setOverlaysEnabled(false);	// false = no overlays rendered, if any
}

//////////////////////////////////////////////////////////////
// Handle Rift update:
//////////////////////////////////////////////////////////////
void Scene::setRiftPose( Ogre::Quaternion orientation, Ogre::Vector3 pos )
{
	mHeadNode->setOrientation( orientation );
	mHeadNode->setPosition( pos );
}

//////////////////////////////////////////////////////////////
// Handle Camera update:
//////////////////////////////////////////////////////////////
void Scene::setVideoImagePoseLeft(const Ogre::PixelBox &image, Ogre::Quaternion pose)
{
	if (videoIsEnabled)
	{
		// update image pixels
		mLeftCameraRenderTexture->getBuffer()->blitFromMemory(image);
		camera_frame_updated = true;

		// update image position/orientation (THIS IS TOO COOL SO I KEEP THIS)
		//Ogre::Quaternion delta = mCamLeft->getOrientation().Inverse() * pose;
		//mVideoLeft->setOrientation(delta);

		// update image position/orientation
		// since:	Hpast-to-body = Hpres-to-body * Hpast-to-pres
		// then:	Hpast-to-pres = INV(Hpres-to-body) * Hpast-to-body
		Ogre::Quaternion deltaHeadPose = mHeadNode->getOrientation().Inverse() * pose;
		mHeadStabilizationNodeLeft->setOrientation(deltaHeadPose);
	}

}
void Scene::setVideoImagePoseRight(const Ogre::PixelBox &image, Ogre::Quaternion pose)
{
	if (videoIsEnabled)
	{
		// update image pixels
		mRightCameraRenderTexture->getBuffer()->blitFromMemory(image);
		//camera_frame_updated = true;

		// update image position/orientation (THIS IS TOO COOL SO I KEEP THIS)
		//Ogre::Quaternion delta = mCamLeft->getOrientation().Inverse() * pose;
		//mVideoLeft->setOrientation(delta);

		// update image position/orientation
		// since:	Hpast-to-body = Hpres-to-body * Hpast-to-pres
		// then:	Hpast-to-pres = INV(Hpres-to-body) * Hpast-to-body
		Ogre::Quaternion deltaHeadPose = mHeadNode->getOrientation().Inverse() * pose;
		mHeadStabilizationNodeRight->setOrientation(deltaHeadPose);

	}

}

//////////////////////////////////////////////////////////////
// Handle Scene update (for input and settings):
//////////////////////////////////////////////////////////////
void Scene::update(float dt)
{
	/*
	float forward = (mKeyboard->isKeyDown(OIS::KC_W) ? 0 : 1) + (mKeyboard->isKeyDown(OIS::KC_S) ? 0 : -1);
	float leftRight = (mKeyboard->isKeyDown(OIS::KC_A) ? 0 : 1) + (mKeyboard->isKeyDown(OIS::KC_D) ? 0 : -1);

	if (mKeyboard->isKeyDown(OIS::KC_LSHIFT))
	{
		forward *= 3;
		leftRight *= 3;
	}
	*/

	// get full body absolute orientation (in world reference)
	Ogre::Vector3 dirX = mBodyTiltNode->_getDerivedOrientation()*Ogre::Vector3::UNIT_X;
	Ogre::Vector3 dirZ = mBodyTiltNode->_getDerivedOrientation()*Ogre::Vector3::UNIT_Z;

	//mBodyNode->setPosition(mBodyNode->getPosition() + dirZ*forward*dt + dirX*leftRight*dt);
}
void Scene::updateVideos()
{
	// Combine scaling factors (to use when needed)
	float direct_scaling = videoClippingScaleFactor * videoFovScaleFactor;
	float inverse_scaling = videoClippingScaleFactor * (1/videoFovScaleFactor);

	// Pointer to pass arguments to shader
	Ogre::GpuProgramParametersSharedPtr parametersFisheyeShader;

	// Setup mVideoLeft SceneNode position/scale/orientation
	// Position:For Pinhole model:
	//			X-axis:	we assume real cameras distance (ICD) is the same as IPD, so X should be solidal to virtual cameras X value -> see setIPD()
	//			Y-axis:	0 = always solidal to mHeadNode
	//			Z-axis:	this MAY VARY depending on how far the objects are "clipped" in front/behind the plane
	//			To obtain this without varying the FOV, z will always be proportional to plane's scale factor
	//			For Fisheye model:
	//			X-axis:	we assume by default that real cameras distance (ICD) is the same as IPD, so X = 0 (solidal to virtual cameras X value) -> see setIPD()
	//			Y-axis:	we assume by default that real cameras height is the same as the eye, so Y = 0 (solidal to virtual cameras X value)
	//			Z-axis:	this should NOT vary to keep stereographic projection consistent (KEEP the camera near the center of the sphere =0!)
	// Scale:	For the Pinhole model:
	//				- by default has ALWAYS SAME ABSOLUTE VALUE of Z-axis position value
	//				- for FOV compensation/adjustments, scale along X and Y axis is used
	//			For Fisheye model:
	//			by default, scale factor IS KEPT WITH SAME ABSOLUTE VALUE as Sphere radius
	//			scaling sphere along Z enhances the percieved image FOV without a noticable undesired distortion.
	//			This is useful to compensate (at least partially) the distance of the real camera in front of the eye.
	//			The undesired effect is that object captured from real camera are closer than how they would appear as seen by the eye.
	// Offset:	EXPERIMENTAL: this value is left for future use!
	//			this is a constant reference passed as argument, added to video position.
	//			By default this value is (0,0,0) where all considerations above are true.
	//			Since real cameras are NOT in the same position of the eye, some compensation may be required.
	//			By default, only Scale value is modified to partially compensate this problem.
	//			Offset should be dependent on Scale and should be modified accordingly.
	//			N.B.:
	//				- offset is expressed in meters
	//				- if adjusted independently from Scale, offset will produce less/more visible effects (higher Scale, lower effect)
	//				- offset will produce distortion on Fisheye model (since camera has to stay near the center of the Sphere!)
	switch (currentCameraModel)
	{
	case Pinhole:
		// plane z position must keep the same as its scale
		mVideoLeft->setPosition
			(
			-videoOffset.x,
			videoOffset.y,
			-videoClippingScaleFactor - videoOffset.z
			);
		mVideoLeft->setScale
			(
			inverse_scaling,	// more fov means smaller image
			inverse_scaling,
			videoClippingScaleFactor
			);
		mVideoRight->setPosition
			(
			videoOffset.x,
			videoOffset.y,
			-videoClippingScaleFactor - videoOffset.z
			);
		mVideoRight->setScale
			(
			inverse_scaling,
			inverse_scaling,
			videoClippingScaleFactor
			);
		break;

	case Fisheye:
		// Remember: camera must stay in the center of the sphere to keep projection!
		mVideoLeft->setPosition
			(
			-videoOffset.x,
			videoOffset.y,
			-videoOffset.z
			);
		mVideoLeft->setScale
			(
			videoClippingScaleFactor,
			videoClippingScaleFactor,
			direct_scaling		// more fov means higher scale along z
			);
		mVideoRight->setPosition
			(
			-videoOffset.x,
			videoOffset.y,
			-videoOffset.z
			);
		mVideoRight->setScale
			(
			direct_scaling,
			videoClippingScaleFactor,
			videoClippingScaleFactor
			);		
		parametersFisheyeShader = mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
		parametersFisheyeShader->setNamedConstant("adjustTextureAspectRatio", videoLeftTextureCalibrationAspectRatio);
		parametersFisheyeShader->setNamedConstant("adjustTextureScale", videoLeftTextureCalibrationScale);
		parametersFisheyeShader->setNamedConstant("adjustTextureOffset", videoLeftTextureCalibrationOffset);
		parametersFisheyeShader = mRightCameraRenderMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
		parametersFisheyeShader->setNamedConstant("adjustTextureAspectRatio", videoRightTextureCalibrationAspectRatio);
		parametersFisheyeShader->setNamedConstant("adjustTextureScale", videoRightTextureCalibrationScale);
		parametersFisheyeShader->setNamedConstant("adjustTextureOffset", videoRightTextureCalibrationOffset);
		parametersFisheyeShader.setNull();
		break;

	default:
		break;
	}

	std::cout << videoLeftTextureCalibrationAspectRatio << std::endl;

	// Setup orientation (if needed) -- for now is constant
	//mVideoLeft->roll(Ogre::Degree(-CAMERA_ROTATION));
	//mVideoRight->roll(Ogre::Degree(CAMERA_ROTATION));

}

//////////////////////////////////////////////////////////////
// Handle User Settings:
//////////////////////////////////////////////////////////////
void Scene::setIPD(float IPD)
{
	// only change X position of each camera
	mCamLeft->setPosition(-IPD / 2.0f, mCamLeft->getPosition().y, mCamLeft->getPosition().z);
	mCamRight->setPosition(IPD / 2.0f, mCamLeft->getPosition().y, mCamLeft->getPosition().z);

	// propagate modification to camera references for stabilization
	mCamLeftReference->setPosition(mCamLeft->getPosition());
	mCamRightReference->setPosition(mCamRight->getPosition());
}
float Scene::adjustVideoDistance(const float distIncrement = 0)
{
	// Adjust video mesh distance from the viewer
	// It DOES NOT affect FOV or image
	// It ONLY affects clipping of object behind/in front of the mesh
	videoClippingScaleFactor += distIncrement;

	// Set minimum to 0.6 (never clip objects nearer than 60cm)
	if (videoClippingScaleFactor < 0.6f) videoClippingScaleFactor = 0.6f;
	// For Fisheye case, it is better if this value is higher as possible
	// when compensation is on: this is because if this value is too small, 
	// when head rotates (neck Y axis) virtual cameras will move too much
	// from sphere center, or worse could penetrate the sphere.
	// minimum good tolerance: 60 cm ~ 100 * IPD
	// This is meant to keep distortion effects under control.
	// This is very important when using Fisheye and video meshes rotate
	// 
	// virtual cameras will move too much from sphere center, enhancing distortion,
	// or worse penetrate the sphere when image stabilization is active.
	
	updateVideos();

	return videoClippingScaleFactor;

}
float Scene::adjustVideoFov(const float increment = 0)
{
	// Adjust video mesh shape scale
	// It WILL affect percieved FOV of the image
	// It CAN affect clipping of object behind/in front of the mesh
	videoFovScaleFactor += increment;

	// Set minimum to 0.8 (never clip objects nearer than 80cm)
	if (videoFovScaleFactor < 1.0f) videoFovScaleFactor = 1.0f;
	else if (videoFovScaleFactor > 2.0f) videoFovScaleFactor = 2.0f;
	// This is meant to keep distortion effects under control.
	// A Fov scale factor = 1 means (ideally) that camera is where the eye is.
	// A Fov scale factor < 1 would create a tele effect (as if camera is behind the eye).
	// A Fov scale factor > 1 creates a wide angle effect. This is made to partially
	// compensate the image of a camera in front of the eye.
	// Around 2 distortion is too high.

	updateVideos();

	return videoFovScaleFactor;
}
Ogre::Vector2 Scene::adjustVideoLeftTextureCalibrationOffset(const Ogre::Vector2 offsetIncrement = Ogre::Vector2::ZERO)
{
	videoLeftTextureCalibrationOffset += offsetIncrement;
	updateVideos();
	return videoLeftTextureCalibrationOffset;
}
Ogre::Vector2 Scene::adjustVideoRightTextureCalibrationOffset(const Ogre::Vector2 offsetIncrement = Ogre::Vector2::ZERO)
{
	videoRightTextureCalibrationOffset += offsetIncrement;
	updateVideos();
	return videoRightTextureCalibrationOffset;
}
void Scene::setVideoLeftTextureCalibrationAspectRatio(const float ar)
{
	if (ar >= 1) videoLeftTextureCalibrationAspectRatio = ar;
	updateVideos();
}
float Scene::adjustVideoLeftTextureCalibrationAspectRatio(const float arIncrement)
{
	videoLeftTextureCalibrationAspectRatio += arIncrement;
	updateVideos();
	return videoLeftTextureCalibrationAspectRatio;
}
void Scene::setVideoRightTextureCalibrationAspectRatio(const float ar)
{
	if (ar >= 1) videoRightTextureCalibrationAspectRatio = ar;
	updateVideos();
}
float Scene::adjustVideoRightTextureCalibrationAspectRatio(const float arIncrement)
{
	videoRightTextureCalibrationAspectRatio += arIncrement;
	updateVideos();
	return videoRightTextureCalibrationAspectRatio;
}
float Scene::adjustVideoLeftTextureCalibrationScale(const float incrementFactor = 0)
{
	videoLeftTextureCalibrationScale += videoLeftTextureCalibrationScale * incrementFactor;
	updateVideos();
	return videoLeftTextureCalibrationScale;
}
float Scene::adjustVideoRightTextureCalibrationScale(const float incrementFactor = 0)
{
	videoRightTextureCalibrationScale += videoRightTextureCalibrationScale * incrementFactor;
	updateVideos();
	return videoRightTextureCalibrationScale;
}
void Scene::setVideoOffset(const Ogre::Vector3 newVideoOffset = Ogre::Vector3::ZERO)
{
	// Alter video mesh so that it models real camera distance from the eye
	// It DOES affect FOV of the image percieved by the virtual camera
	// It models real camera position relative to the eye
	if (newVideoOffset > Ogre::Vector3::ZERO)
	{
		videoOffset = newVideoOffset;
		updateVideos();
	}

}
void Scene::enableVideo()
{
	if (!videoIsEnabled)
	{
		if (mVideoLeft)
		{
			mCamLeft->addListener(this);
			mCamRight->addListener(this);
			videoIsEnabled = true;
		}
		else
		{
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "Video is not setup. setupVideo() must be called first.", "Scene::enableVideo");
		}
	}
}
void Scene::disableVideo()
{
	if (videoIsEnabled)
	{
		if (mVideoLeft)
		{
			mCamLeft->removeListener(this);
			mCamRight->removeListener(this);
			mVideoLeft->setVisible(false, false);
			mVideoRight->setVisible(false, false);
			videoIsEnabled = false;
		}
		else
		{
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "Video is not setup. setupVideo() must be called first.", "Scene::enableVideo");
		}
	}
}

//////////////////////////////////////////////////////////////
// Handle Input:
//////////////////////////////////////////////////////////////
bool Scene::keyPressed( const OIS::KeyEvent& e )
{
	return true;
}
bool Scene::keyReleased( const OIS::KeyEvent& e )
{
	return true;
}
bool Scene::mouseMoved( const OIS::MouseEvent& e )
{
	if( mMouse->getMouseState().buttonDown( OIS::MB_Left ) )
	{
		mBodyYawNode->yaw( Ogre::Degree( -0.3*e.state.X.rel ) );
		mBodyTiltNode->pitch( Ogre::Degree( -0.3*e.state.Y.rel ) );
	}
	return true;
}
bool Scene::mousePressed( const OIS::MouseEvent& e, OIS::MouseButtonID id )
{
	return true;
}
bool Scene::mouseReleased( const OIS::MouseEvent& e, OIS::MouseButtonID id )
{
	return true;
}

//////////////////////////////////////////////////////////////
// Handle Virtual Camera Events:
//////////////////////////////////////////////////////////////
void Scene::cameraPreRenderScene(Ogre::Camera* cam)
{
	if (cam == mCamLeft)
	{
		mVideoLeft->setVisible(true, false);
	}
	if (cam == mCamRight)
	{
		mVideoRight->setVisible(true, false);
	}
}
void Scene::cameraPostRenderScene(Ogre::Camera* cam)
{
	if (cam == mCamLeft)
	{
		// FRAME RATE DISPLAY
		//calculate delay from last frame and show
		if (camera_frame_updated)
		{
			//camera_last_frame_display_delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - camera_last_frame_request_time);
			//std::cout << "Camera milliseconds delay: " << camera_last_frame_display_delay.count() << " ms"<< std::endl;
			camera_frame_updated = false;
		}
		mVideoLeft->setVisible(false, false);
	}
	if (cam == mCamRight)
	{
		mVideoRight->setVisible(false, false);
	}
}
