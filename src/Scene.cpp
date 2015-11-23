
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
	
	// Prepare mesh/entity for AR object
	Ogre::Entity* cubeEnt = mSceneMgr->createEntity("Axis.mesh");
	cubeEnt->getSubEntity(0)->setMaterialName("BaseAxis");
	
	// Create a node in WORLD coordinates to which attach AR object
	mCubeRed = mRoomNode->createChildSceneNode("CubeRed");
	mCubeRed->setScale(0.1, 0.1, 0.1);
	mCubeRed->attachObject( cubeEnt );

	// Create the floor
	// 	Create a plane class instance that describes floor plane (no position or orientation, just mathematical description)
	Ogre::Plane videoPlane(Ogre::Vector3::UNIT_Y, 0);
	// 	Create a static mesh out of the plane (as a REUSABLE "resource")
	Ogre::MeshManager::getSingleton().createPlane(
		"floorMesh",										// this is the name that our resource will have for the whole application!
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		videoPlane,											// this is the instance from which we build the mesh
		30, 30, 1, 1,
		true,
		1, 1, 1,
		Ogre::Vector3::UNIT_Z);								// this is the vector that will be used as mesh UP direction
	//	Create an ogre Entity out of the resource we created (more Entities can be created out of a resource!)
	Ogre::Entity* floorPlaneEntity = mSceneMgr->createEntity("floorMesh");
	// 	Add texture
	floorPlaneEntity->setMaterialName("floor");
	//	Create an ogre SceneNode to which attach the plane entity
	Ogre::SceneNode* mFloorNode = mRoomNode->createChildSceneNode();
	mFloorNode->attachObject( floorPlaneEntity );
	mFloorNode->setPosition( 0.0, 0.0, 0.0 );


	// Other useless objects (temporary)
	//mCubeGreen = mRoomNode->createChildSceneNode();
	//Ogre::Entity* cubeEnt2 = mSceneMgr->createEntity( "Cube.mesh" );
	//cubeEnt2->getSubEntity(0)->setMaterialName( "CubeMaterialGreen" );
	//mCubeGreen->attachObject( cubeEnt2 );
	//mCubeGreen->setPosition( 0.0, 0.0, 0.0 );
	//mCubeGreen->setScale( 0.1, 0.1, 0.1 );
	
	Ogre::SceneNode* cubeNode3 = mRoomNode->createChildSceneNode();
	Ogre::Entity* cubeEnt3 = mSceneMgr->createEntity( "Cube.mesh" );
	cubeEnt3->getSubEntity(0)->setMaterialName( "CubeMaterialWhite" );
	cubeNode3->attachObject( cubeEnt3 );
	cubeNode3->setPosition( -1.0, 0.0, 0.0 );
	cubeNode3->setScale( 0.5, 0.5, 0.5 );

	//Ogre::Entity* roomEnt = mSceneMgr->createEntity( "Room.mesh" );
	//roomEnt->setCastShadows( false );
	//mRoomNode->attachObject( roomEnt );
/*
	Ogre::Light* roomLight = mSceneMgr->createLight();
	roomLight->setType(Ogre::Light::LT_POINT);
	roomLight->setCastShadows( false );
	roomLight->setShadowFarDistance( 30 );
	roomLight->setAttenuation( 65, 1.0, 0.07, 0.017 );
	roomLight->setSpecularColour( .25, .25, .25 );
	roomLight->setDiffuseColour( 0.85, 0.76, 0.7 );

	roomLight->setPosition( 5, 5, 5 );
*/
	//mRoomNode->attachObject( roomLight );

	Ogre::Light* light = mSceneMgr->createLight("tstLight");
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    Ogre::Vector3 lightdir(-0.55, -0.4, -0.75);
    //Ogre::Vector3 lightdir(0, -1, 0);
    lightdir.normalise();
    light->setDirection(lightdir);
    light->setDiffuseColour(Ogre::ColourValue::White);
    light->setSpecularColour(Ogre::ColourValue(0.4, 0.4, 0.4));
 
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.2, 0.2, 0.2));

    // Add skybox (look at the skyrender.material script file for more details)
	mSceneMgr->setSkyBox(true, "skyrender", 25, true);	//value is half of cube edge size
											 			// default should be 5000, AND NOT RENDER ALWAYS!!
											 			// see here: http://www.ogre3d.org/forums/viewtopic.php?f=2&t=40792&p=521011#p521011
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

	// STABILIZATION NODES (1): NECK IMAGE STABILIZATION - image will stabilize by rotating around the neck
	// Prepare two Ogre SceneNodes (mHeadStabilizationNode) child of mHeadNode that will be used for image stabilization
	// NOTE:	since any real camera has a delay, we want to move the image shown in the head pose as it was taken, not in the current headpose.
	//			If this feature is disabled, mHeadStabilizationNode will simply follow mHeadNode position/orientation
	mHeadStabilizationNodeLeft = mHeadNode->createChildSceneNode("HeadStabilizationNodeLeft");
	mHeadStabilizationNodeRight = mHeadNode->createChildSceneNode("HeadStabilizationNodeRight");
	
	// Attach cameras
	// Cameras are created with a STANDARD INITIAL projection matrix (Ogre's default)
	// Camera projection matrices will be set later to SDK suggested values by calling setCameraMatrices()
	mHeadNode->attachObject(mCamLeft);
	mHeadNode->attachObject(mCamRight);
	mCamLeft->setFarClipDistance(10000);						// see here: http://www.ogre3d.org/forums/viewtopic.php?f=2&t=40792&p=521011#p521011
	mCamRight->setFarClipDistance(10000);
	mCamLeft->setNearClipDistance(0.001);
	mCamRight->setNearClipDistance(0.001);

	// Camera positioning to a STANDARD INITIAL interpupillary and eyetoneck distance
	// It should be set later to SDK user custom value by calling setIPD() method
	float default_IPdist = 0.064f;			// distance between the eyes
	float default_HorETNdist = 0.0805f;		// horizontal distance from the eye to the neck
	float default_VerETNdist = 0.075f;		// vertical distance from the eye to the neck
	mCamLeft->setPosition ( -default_IPdist / 2.0f , default_VerETNdist, -default_HorETNdist ); //x is right, y is up, z is towards the screen
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

	mBodyNode->setPosition( 0.0, 1.7, 0.0 );
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
void Scene::setupVideo(const CameraModel camModelToUse, const StabilizationModel stabModelToUse, const Ogre::Vector3 eyeToCameraOffset, const float HFov, const float VFov)
{
	if (!mVideoLeft && !mVideoRight)
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

			switch (camModelToUse)
			{
			case Pinhole:
				planeWidth = std::tan(hFovRads / 2) * 1.0f /**/ * 2;				// ** Plane mesh is created using videoClippingScaleFactor = 1 as a distance reference.
				planeHeight = std::tan(vFovRads / 2) * 1.0f /**/ * 2;				//	  Then we only need to act on its scale and position multiplying custom user's value
				createPinholeVideos(planeWidth, planeHeight, eyeToCameraOffset);	//    to resize plane accordingly: no need to recreate the mesh.
				break;
			case Fisheye:
				createFisheyeVideos(eyeToCameraOffset);
				break;
			default:
				throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid Camera Model selection!", "Scene::setupVideo");
				break;
			}

			//set stabilization model
			setStabilizationMode(stabModelToUse);

			// save current values for later adjustments
			currentCameraModel = camModelToUse;
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
void Scene::setupVideo(const CameraModel camModelToUse, const StabilizationModel stabModelToUse, const Ogre::Vector3 eyeToCameraOffset, const float WSensor, const float HSensor, const float FL)
{
	if (!mVideoLeft && !mVideoRight)
	{
		if (WSensor > 0 && HSensor > 0 && FL > 0)
		{

			float planeWidth;
			float planeHeight;

			switch (camModelToUse)
			{
			case Pinhole:
				
				planeWidth = (WSensor * 1.0f /**/) / FL;							// ** Plane mesh is created using videoClippingScaleFactor = 1 as a distance reference.
				planeHeight = (HSensor * 1.0f /**/) / FL;							//	  Then we only need to act on its scale and position multiplying custom user's value
				createPinholeVideos(planeWidth, planeHeight, eyeToCameraOffset);	//    to resize plane accordingly: no need to recreate the mesh.
				// compute current fov values for later adjustments
				cameraHFov = ((2 * std::atan(planeWidth / (2 * 1.0f /**/))) * 180) / M_PI;
				cameraVFov = ((2 * std::atan(planeHeight / (2 * 1.0f /**/))) * 180) / M_PI;
				break;
			case Fisheye:
				createFisheyeVideos(eyeToCameraOffset);
				break;
			default:
				throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid Camera Model selection!", "Scene::setupVideo");
				break;
			}

			//set stabilization model
			setStabilizationMode(stabModelToUse);

			// save current values for later adjustments
			currentCameraModel = camModelToUse;
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
void Scene::setStabilizationMode(StabilizationModel modelToUse)
{
	
	if (mLeftStabilizationNode!=nullptr && mRightStabilizationNode!=nullptr)
	{
		// reset last stabilization poses to IDENTITY and their default behaviour
		mLeftStabilizationNode->resetOrientation();
		mRightStabilizationNode->resetOrientation();
		mLeftStabilizationNode->setInheritOrientation(true);
		mRightStabilizationNode->setInheritOrientation(true);

	}
	
	// change stabilization node to use
	switch (modelToUse)
	{
	case Head:
		mLeftStabilizationNode = mHeadStabilizationNodeLeft;
		mRightStabilizationNode = mHeadStabilizationNodeRight;
		break;
	case Eye:
		mLeftStabilizationNode = mCamLeftStabilizationNode;
		mRightStabilizationNode = mCamRightStabilizationNode;
		break;
	default:
		throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid Stabilization Model selection!", "Scene::setupVideo");
		break;
	}
	// Set current stabilization node independent from parent (see why in Scene.h on Ehnanced Head Model comments)
	// STILL TO TEST OUT!
	//mLeftStabilizationNode->setInheritOrientation(false);
	//mRightStabilizationNode->setInheritOrientation(false);

	currentStabilizationModel = modelToUse;
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
	mCamLeftReference = mHeadStabilizationNodeLeft->createChildSceneNode("CameraReferenceLeft");
	mCamLeftReference->setPosition(mCamLeft->getPosition());
	mCamRightReference = mHeadStabilizationNodeRight->createChildSceneNode("CameraReferenceRight");
	mCamRightReference->setPosition(mCamRight->getPosition());

	// STABILIZATION NODES (2): EYE IMAGE STABILIZATION - image will stabilize by rotating around the virtual camera
	// Prepare two Ogre SceneNodes (mCamStabilizationNode) child of mCamReference that will be used for image stabilization
	// NOTE:	since any real camera has a delay, we want to move the image shown in the head pose as it was taken, not in the current headpose.
	//			If this feature is disabled, mCamStabilizationNode will simply follow mCamReference position/orientation
	mCamLeftStabilizationNode = mCamLeftReference->createChildSceneNode("CameraReferenceStabilizationNodeLeft");
	mCamRightStabilizationNode = mCamLeftReference->createChildSceneNode("CameraReferenceStabilizationNodeRight");


	// AR REFERENCE: Create empty node to use as a relative reference of AR object respect to camera
	// This is a program optimization (it is easier than calculating inverse transform manually)
	Ogre::SceneNode* arreference = mCamLeftStabilizationNode->createChildSceneNode("ARreferenceAdjust");// node to apply needed transformation from arUco to Ogre reference
	mCubeRedReference = arreference->createChildSceneNode("CubeRedReference");							// node to which marker position/orientation is applied
	mCubeRedOffset = mCubeRedReference->createChildSceneNode("CubeRedOffset");							// node to which custom offset of entity in respect to marker is applied
	mCubeRedReference->setPosition(0.0, 0.0, 0.0);	//just initial position , will be overwritten as soon as marker is detected
	mCubeRedOffset->setPosition(0.0f, 0.0f, 0.0f);	//offset to place cube as you wish around marker reference	
	// Adjustments of coordinates between arUco and Ogre
	// Applying automatically coordinates of arUco does not work, probably because arUco returns pose of the camera in respect to marker, not viceversa
	// By using an intermediate node "ARreferenceAdjust" between stabilization node and mCubeRedReference we can do it easily and only once!
	mCamLeftStabilizationNode->getChild("ARreferenceAdjust")->yaw(Ogre::Degree(180));
	//mCubeRedReference->roll(Ogre::Degree(90));


	//GREEN CUBE REFERENCE HERE (used for testing only)!
	//mCubeGreen->getParentSceneNode()->removeChild(mCubeGreen);
	//mCamLeftStabilizationNode->addChild(mCubeGreen);

	//Finally create and attach mVideo nodes to camera references (also apply roll from configuration)
	mVideoLeft = mCamLeftStabilizationNode->createChildSceneNode("LeftVideo");
	mVideoLeft->yaw(Ogre::Degree(CAMERA_TOEIN_ANGLE));
	mVideoRight = mCamRightStabilizationNode->createChildSceneNode("RightVideo");
	mVideoRight->yaw(Ogre::Degree(-CAMERA_TOEIN_ANGLE));

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

	//Create two special textures (TU_DYNAMIC_WRITE_ONLY_DISCARDABLE) that will be applied to the two videoPlaneEntities
	mLeftCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraLeft", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1024, 576, 0, Ogre::PF_R8G8B8,
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	mRightCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraRight", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1024, 576, 0, Ogre::PF_R8G8B8,
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

	/* OLD CODE - manual material
	// Creare new materials and assign the two textures that can be used on the shapes created
	// WARNING: apparently modifying these lines in another equivalent form will cause runtime crashes!! BE CAREFUL!!
	mLeftCameraRenderMaterial = Ogre::MaterialManager::getSingleton().create("Scene/LeftCamera", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::Technique *technique1 = mLeftCameraRenderMaterial->createTechnique();
	technique1->createPass();
	mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("RenderTextureCameraLeft");
	//mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mLeftCameraRenderTexture);
	mRightCameraRenderMaterial = Ogre::MaterialManager::getSingleton().create("Scene/RightCamera", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::Technique *technique2 = mRightCameraRenderMaterial->createTechnique();
	technique2->createPass();
	mRightCameraRenderMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("RenderTextureCameraRight");
	*/

	// Load scripted materials and assign the two dynamic textures just created.
	// ONLY the texture image is changed, other parameters are already set in .material file!
	// WARNING: apparently modifying these lines in another equivalent form will cause runtime crashes!! BE CAREFUL!!
	// (Remember: UV mapping of the mesh loaded is independent from the texture loaded)
	mLeftCameraRenderMaterial = Ogre::MaterialManager::getSingleton().getByName("PinholeImageMappingMaterial/LeftEye");
	mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mLeftCameraRenderTexture);
	//mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAddressingMode(Ogre::TextureUnitState::TAM_MIRROR);
	//mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mLeftCameraRenderTexture);
	mRightCameraRenderMaterial = Ogre::MaterialManager::getSingleton().getByName("PinholeImageMappingMaterial/RightEye");
	mRightCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mRightCameraRenderTexture);
	//mRightCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mRightCameraRenderTexture);

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
	//so that we can deal with mVideoLeft and mVideoRight positions relatively to each camera despite Stabilization
	mCamLeftReference = mHeadStabilizationNodeLeft->createChildSceneNode("HeadStabilizationCameraReferenceLeft");
	mCamLeftReference->setPosition(mCamLeft->getPosition());
	mCamRightReference = mHeadStabilizationNodeRight->createChildSceneNode("HeadStabilizationCameraReferenceRight");
	mCamRightReference->setPosition(mCamRight->getPosition());

	// STABILIZATION NODES (2): EYE IMAGE STABILIZATION - image will stabilize by rotating around the virtual camera
	// Prepare two Ogre SceneNodes (mCamStabilizationNode) child of mCamReference that will be used for image stabilization
	// NOTE:	since any real camera has a delay, we want to move the image shown in the head pose as it was taken, not in the current headpose.
	//			If this feature is disabled, mCamStabilizationNode will simply follow mCamReference position/orientation
	mCamLeftStabilizationNode = mCamLeftReference->createChildSceneNode("CameraReferenceStabilizationNodeLeft");
	mCamRightStabilizationNode = mCamLeftReference->createChildSceneNode("CameraReferenceStabilizationNodeRight");

	//Finally create and attach mVideo nodes to camera references (also apply roll from configuration)
	mVideoLeft = mCamLeftStabilizationNode->createChildSceneNode("LeftVideo");
	mVideoLeft->yaw(Ogre::Degree(CAMERA_TOEIN_ANGLE));
	mVideoRight = mCamRightStabilizationNode->createChildSceneNode("RightVideo");
	mVideoRight->yaw(Ogre::Degree(-CAMERA_TOEIN_ANGLE));

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
	//Create two special textures (TU_DYNAMIC_WRITE_ONLY_DISCARDABLE) that will be applied to the two videoPlaneEntities
	mLeftCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraLeft", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1024, 576, 0, Ogre::PF_R8G8B8A8,	// A8 is added so Texture can be transparent outside the image!
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);				// see also here -> http://www.ogre3d.org/tikiwiki/-material
	mRightCameraRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RenderTextureCameraRight", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, 1024, 576, 0, Ogre::PF_R8G8B8A8,	// A8 is added so Texture can be transparent outside the image!
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);				// see also here -> http://www.ogre3d.org/tikiwiki/-material
	
	// Load scripted materials and assign the two dynamic textures just created.
	// ONLY the texture image is changed, other parameters are already set in .material file!
	// WARNING: apparently modifying these lines in another equivalent form will cause runtime crashes!! BE CAREFUL!!
	// (Remember: UV mapping of the mesh loaded is independent from the texture loaded)
	mLeftCameraRenderMaterial = Ogre::MaterialManager::getSingleton().getByName("FisheyeImageMappingMaterial/LeftEye");
	mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mLeftCameraRenderTexture);
	//mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAddressingMode(Ogre::TextureUnitState::TAM_MIRROR);
	//mLeftCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mLeftCameraRenderTexture);
	mRightCameraRenderMaterial = Ogre::MaterialManager::getSingleton().getByName("FisheyeImageMappingMaterial/RightEye");
	mRightCameraRenderMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mRightCameraRenderTexture);
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
		mLeftStabilizationNode->setOrientation(deltaHeadPose);
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

		// IMAGE STABILIZATION: take HEAD-IMAGE orientation DELTA and apply it to stabilization node
		// update image position/orientation
		// since:	Hpast-to-body = Hpres-to-body * Hpast-to-pres
		// then:	Hpast-to-pres = INV(Hpres-to-body) * Hpast-to-body
		Ogre::Quaternion deltaHeadPose = mHeadNode->getOrientation().Inverse() * pose;
		mRightStabilizationNode->setOrientation(deltaHeadPose);

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
		mVideoLeft->resetOrientation();
		mVideoLeft->yaw(Ogre::Degree(videoToeInAngle));
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
		mVideoRight->resetOrientation();
		mVideoRight->yaw(Ogre::Degree(-videoToeInAngle));
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

	// Setup orientation (if needed) -- for now this is read from .cfg file and applied on mesh creation!
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
float Scene::adjustVideoToeInAngle(const float incrementAngle = 0)
{
	videoToeInAngle += incrementAngle;
	updateVideos();
	return videoToeInAngle;
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
