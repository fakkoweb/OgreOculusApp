#include "Rift.h"

bool Rift::isInitialized = false;
unsigned short int Rift::ovr_Users = 0;

//////////////////////////////////////////
// Static members for handling the API:
//////////////////////////////////////////
void Rift::init()
{
	if( ! isInitialized )
	{
		ovr_Initialize();
		isInitialized = true;
	}
	ovr_Users++;
}
void Rift::shutdown()
{
	ovr_Users--;
	if( ovr_Users == 0 && isInitialized )
	{
		ovr_Shutdown();
		isInitialized = false;
	}
}

/////////////////////////////////////////
// Per-Device methods (non static):
/////////////////////////////////////////


Rift::Rift(const unsigned int ID, Ogre::Root* const root, Ogre::RenderWindow* &renderWindow, const bool rotateView)
{
	if (root == nullptr) throw std::invalid_argument("'root' is null. Rift instance not created.");
	mRenderTexture[0] = nullptr;
	mRenderTexture[1] = nullptr;

	// Init OVR lib (if I am the first instance created)
	Rift::init();

	// ---------------------------------
	// Try to locate physical Rift device
	hmd = NULL;
	std::cout << "Creating Rift (ID: " << ID << ")" << std::endl;
	hmd = ovrHmd_Create(ID);
	if (!hmd)
	{
		// No Rift detected: no sensor data, but Oculus window will be displayed anyway
		hmd = NULL;
		std::cout << "Oculus Rift NOT found.\nSimulating Oculus DK2..." << std::endl;
		
		hmd = ovrHmd_CreateDebug(ovrHmd_DK2);
		if (!hmd)
		{
			Rift::shutdown();
			throw std::ios_base::failure("Unable to initialize Rift class.");
		}
		mRiftID = ovrHmd_DK2;
		simulationMode = true;
		std::cout<<"Simulation enabled: a window will show a dummy Oculus DK2 output." << std::endl;

	}
	else
	{
		// Rift detected
		simulationMode = false;
		std::cout << "Oculus Rift found." << std::endl;
		std::cout << "\tProduct Name: " << hmd->ProductName << std::endl;
		std::cout << "\tProduct ID: " << hmd->ProductId << std::endl;
		std::cout << "\tFirmware: " << hmd->FirmwareMajor << "." << hmd->FirmwareMinor << std::endl;
		std::cout << "\tResolution: " << hmd->Resolution.w << "x" << hmd->Resolution.h << std::endl;
		std::cout << "Oculus Rift found." << std::endl;

		// If Rift not supported, throw exception
		if (!ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0))
		{
			ovrHmd_Destroy(hmd);
			Rift::shutdown();
			throw std::ios_base::failure("\tThis Rift does not support the features needed by the application.");
		}

		// Save id
		mRiftID = ID;

		// Determine Oculus Type
		if (hmd->Type == ovrHmd_DK1)
			this->rotateView = false;
		else if (hmd->Type == ovrHmd_DK2)
			this->rotateView = true;

	}


	// -----------------------------------
	//createRiftDisplayScene(root);

	/*
	// -----------------------------------
	// Create Oculus render window, if needed
	if (renderWindow == NULL)
	{
		// no window defined by the user: create it and save to the user
		createRiftDisplayWindow(root);
		renderWindow = mRenderWindow;
	}
	else
	{
		// window already available: save it
		mRenderWindow = renderWindow;
	}
	// Link mCamera (orthographic inner scene Oculus camera) to Oculus rendering window
	mViewport = mRenderWindow->addViewport(mCamera);
	mViewport->setBackgroundColour(Ogre::ColourValue::Black);
	mViewport->setOverlaysEnabled(true);
	*/
}
Rift::~Rift()
{
	if (hmd) ovrHmd_Destroy(hmd);

	// Shutdown OVR lib (if I am the last Rift object to be destroyed)
	Rift::shutdown();
}

Ogre::RenderWindow* Rift::createRiftDisplayWindow(Ogre::Root* const root)
{

	//Setup Oculus rendering window options
	Ogre::NameValuePairList miscParams;
	if (simulationMode)
	{
		// No rift: creating standard window
		miscParams["monitorIndex"] = Ogre::StringConverter::toString(0);
	}
	else
	{
		// Yes rift: creating full screen window in secondary screen (extended mode)
		// Options Oculus rendering window
		miscParams["monitorIndex"] = Ogre::StringConverter::toString(1);
		miscParams["border "] = "none";
	}

	// Creating Oculus rendering window
	if (hmd->Type == ovrHmd_DK1)
		mRenderWindow = root->createRenderWindow("Oculus Rift Live Visualization", 1280, 800, true, &miscParams);
		//mWindow = mRoot->createRenderWindow("Oculus Rift Liver Visualization", 1920*0.5, 1080*0.5, false, &miscParams);
	else if (hmd->Type == ovrHmd_DK2)
	{
		if (simulationMode) mRenderWindow = root->createRenderWindow("Oculus Rift Live Visualization", 1920, 1080, false, &miscParams);
		else /* rotated */	mRenderWindow = root->createRenderWindow("Oculus Rift Live Visualization", 1080, 1920, true, &miscParams);
	}

	// Register render listener (to perform operations right before and after rendering the window)
	mRenderWindow->addListener(this);

	return mRenderWindow;
}

Ogre::RenderWindow* Rift::createDebugRiftDisplayWindow(Ogre::Root* const root)
{
	//Setup Oculus rendering window options
	Ogre::NameValuePairList miscParams;
	Ogre::RenderWindow* debugWindow = nullptr;
	if (!simulationMode)
	{
		// setting on main screen
		miscParams["monitorIndex"] = Ogre::StringConverter::toString(0);
		// creating debug rendering window
		if (hmd->Type == ovrHmd_DK1)
			debugWindow = root->createRenderWindow("Oculus DEBUG Rift Live Visualization", 1280, 800, false, &miscParams);
		else if (hmd->Type == ovrHmd_DK2)
		{
			debugWindow = root->createRenderWindow("Oculus Rift DEBUG Live Visualization", 1080/2, 1920/2, false, &miscParams);
		}
	}

	return debugWindow;

}

void Rift::createRiftDisplayScene(Ogre::Root* const root)
{
	mSceneMgr = root->createSceneManager(Ogre::ST_GENERIC);
	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));

	// OCULUS CLIENT-SIDE DISTORTION RENDERING
	// ----------------------------------------
	// See Oculus Developer Guide 0.5.0!
	// See also .glsl shaders in /media folder!
	// ---------------------------------------

	// CONFIGURE Render Textures sizes (with recommended FOV by SDK)
	Sizei recommendedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left,
		hmd->DefaultEyeFov[0], 1.0f);
	Sizei recommendedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right,
		hmd->DefaultEyeFov[1], 1.0f);

	/*Sizei renderTargetSize;
	renderTargetSize.w = recommendedTex0Size.w + recommendedTex1Size.w;
	renderTargetSize.h = std::max( recommendedTex0Size.h, recommendedTex1Size.h );*/
	//std::cout << "Error?" << std::endl;

	// Generate Ogre RenderTarget Textures (to apply to distortion meshes)
	mLeftEyeRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RiftRenderTextureLeft", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, recommendedTex0Size.w, recommendedTex0Size.h, 0, Ogre::PF_R8G8B8,
		Ogre::TU_RENDERTARGET);
	mRightEyeRenderTexture = Ogre::TextureManager::getSingleton().createManual(
		"RiftRenderTextureRight", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, recommendedTex1Size.w, recommendedTex1Size.h, 0, Ogre::PF_R8G8B8,
		Ogre::TU_RENDERTARGET);

	// Assign the textures to materials (to be used later)
	mMatLeft = Ogre::MaterialManager::getSingleton().getByName("Oculus/LeftEye");
	mMatLeft->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mLeftEyeRenderTexture);
	mMatRight = Ogre::MaterialManager::getSingleton().getByName("Oculus/RightEye");
	mMatRight->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(mRightEyeRenderTexture);

	// CONFIGURE viewports to render on Ogre RenderTarget Textures
	ovrRecti viewports[2];
	viewports[0].Pos.x = 0;
	viewports[0].Pos.y = 0;
	viewports[0].Size.w = recommendedTex0Size.w;
	viewports[0].Size.h = recommendedTex0Size.h;
	viewports[1].Pos.x = recommendedTex0Size.w;
	viewports[1].Pos.y = 0;
	viewports[1].Size.w = recommendedTex1Size.w;
	viewports[1].Size.h = recommendedTex1Size.h;

	// CONFIGURE per-eye proprieties (for creating distortion meshes)
	// ovrEyeRenderDesc eyeRenderDesc[2]; // commented: moved in class definition, they are also needed before rendering!
	eyeRenderDesc[0] = ovrHmd_GetRenderDesc(hmd, ovrEye_Left, hmd->DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovrHmd_GetRenderDesc(hmd, ovrEye_Right, hmd->DefaultEyeFov[1]);

	std::cout << eyeRenderDesc[0].Fov.DownTan << std::endl;
	std::cout << eyeRenderDesc[0].Eye << std::endl;

	Ogre::SceneNode* meshNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();

	// Define features to enable from Oculus SDK
	unsigned int distortionCaps = 0
		| ovrDistortionCap_Vignette
		| ovrDistortionCap_Overdrive
		| ovrDistortionCap_TimeWarp;

	// Generate the Distortion Meshes for each eye (and do Oculus vertex shader first setup)
	ovrVector2f UVScaleOffset[2];	//0 = Scale, 1 = Offset		// these two parameters will be set NOW into the shaders, since there will be
	Ogre::GpuProgramParametersSharedPtr params;					// no need to change them at runtime (as Documentation suggests..)
	for (int eyeNum = 0; eyeNum < 2; eyeNum++)
	{
		ovrDistortionMesh meshData;
		ovrHmd_CreateDistortionMesh(hmd,
			eyeRenderDesc[eyeNum].Eye,
			eyeRenderDesc[eyeNum].Fov,
			distortionCaps,
			&meshData);
		
		if (eyeNum == 0)
		{
			ovrHmd_GetRenderScaleAndOffset(eyeRenderDesc[eyeNum].Fov,
				recommendedTex0Size, viewports[eyeNum],
				UVScaleOffset);
			params = mMatLeft->getTechnique(0)->getPass(0)->getVertexProgramParameters();
		}
		else
		{
			ovrHmd_GetRenderScaleAndOffset(eyeRenderDesc[eyeNum].Fov,
				recommendedTex1Size, viewports[eyeNum],
				UVScaleOffset);
			params = mMatRight->getTechnique(0)->getPass(0)->getVertexProgramParameters();
		}
		params->setNamedConstant("eyeToSourceUVScale", Ogre::Vector2(UVScaleOffset[0].x, UVScaleOffset[0].y));
		params->setNamedConstant("eyeToSourceUVOffset", Ogre::Vector2(UVScaleOffset[1].x, UVScaleOffset[1].y));

		//std::cout << "UVScaleOffset[0]: " << UVScaleOffset[0].x << ", " << UVScaleOffset[0].y << std::endl;
		//std::cout << "UVScaleOffset[1]: " << UVScaleOffset[1].x << ", " << UVScaleOffset[1].y << std::endl;

		// create ManualObject
		// TODO: Destroy the manual objects!!
		Ogre::ManualObject* manual;
		if (eyeNum == 0)
		{
			manual = mSceneMgr->createManualObject("RiftRenderObjectLeft");
			manual->begin("Oculus/LeftEye", Ogre::RenderOperation::OT_TRIANGLE_LIST);
			//manual->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
		}
		else
		{
			manual = mSceneMgr->createManualObject("RiftRenderObjectRight");
			manual->begin("Oculus/RightEye", Ogre::RenderOperation::OT_TRIANGLE_LIST);
			//manual->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
		}

		for (unsigned int i = 0; i < meshData.VertexCount; i++)
		{
			ovrDistortionVertex v = meshData.pVertexData[i];
			manual->position(v.ScreenPosNDC.x,
				v.ScreenPosNDC.y, 0);
			manual->textureCoord(v.TanEyeAnglesR.x,//*UVScaleOffset[0].x + UVScaleOffset[1].x,
				v.TanEyeAnglesR.y);//*UVScaleOffset[0].y + UVScaleOffset[1].y);
			manual->textureCoord(v.TanEyeAnglesG.x,//*UVScaleOffset[0].x + UVScaleOffset[1].x,
				v.TanEyeAnglesG.y);//*UVScaleOffset[0].y + UVScaleOffset[1].y);
			manual->textureCoord(v.TanEyeAnglesB.x,//*UVScaleOffset[0].x + UVScaleOffset[1].x,
				v.TanEyeAnglesB.y);//*UVScaleOffset[0].y + UVScaleOffset[1].y);
			float vig = std::max(v.VignetteFactor, 0.0f);
			float tw = std::max(v.TimeWarpFactor, 0.0f);
			manual->colour(vig, vig, vig, tw);
		}
		for (unsigned int i = 0; i < meshData.IndexCount; i++)
		{
			manual->index(meshData.pIndexData[i]);
		}

		// tell Ogre, your definition has finished
		manual->end();

		ovrHmd_DestroyDistortionMesh(&meshData);

		meshNode->attachObject(manual);
	}

	// Create a camera in the Oculus inner scene so the two meshes can be rendered onto it:
	mCamera = mSceneMgr->createCamera("OculusRiftExternalCamera");
	mCamera->setFarClipDistance(50);
	mCamera->setNearClipDistance(0.001);
	mCamera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
	mCamera->setOrthoWindow(2, 2);
	if (this->rotateView)
	{
		mCamera->roll(Ogre::Degree(-90));
	}
	mSceneMgr->getRootSceneNode()->attachObject(mCamera);
	meshNode->setPosition(0, 0, -1);
	meshNode->setScale(1, 1, -1);

	// Get IPD from Rift Driver and set it up (in meters)
	mIPD = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, 0.064f);
	std::cout << mIPD << std::endl;
	mPosition = Ogre::Vector3::ZERO;
}

// Takes the two cameras created in the scene and creates Viewports in the correct render textures:
void Rift::setCameraMatrices( Ogre::Camera* camLeft, Ogre::Camera* camRight )
{
	
	// Fix virtual camera aspect ratio by asking OculusSDK for the correct FOV/Distortion matrix
	ovrFovPort fovLeft = hmd->DefaultEyeFov[ovrEye_Left];
	ovrFovPort fovRight = hmd->DefaultEyeFov[ovrEye_Right];

	float combinedTanHalfFovHorizontal = std::max( fovLeft.LeftTan, fovLeft.RightTan );
	float combinedTanHalfFovVertical = std::max( fovLeft.UpTan, fovLeft.DownTan );

	//float aspectRatioLeft = std::max(fovLeft.LeftTan, fovLeft.RightTan) / std::max(fovLeft.UpTan, fovLeft.DownTan);
	//float aspectRatioRight = std::max(fovRight.LeftTan, fovRight.RightTan) / std::max(fovRight.UpTan, fovRight.DownTan);
	
	float aspectRatioLeft = std::max(fovLeft.LeftTan, fovLeft.RightTan) / std::max(fovLeft.UpTan, fovLeft.DownTan);
	float aspectRatioRight = std::max(fovRight.LeftTan, fovRight.RightTan) / std::max(fovRight.UpTan, fovRight.DownTan);


	//camLeft->setAspectRatio(aspectRatioLeft);
	//camRight->setAspectRatio(aspectRatioRight);
	
	ovrMatrix4f projL = ovrMatrix4f_Projection ( fovLeft, 0.001, 50.0, true );
	ovrMatrix4f projR = ovrMatrix4f_Projection ( fovRight, 0.001, 50.0, true );

	camLeft->setCustomProjectionMatrix( true,
		Ogre::Matrix4( projL.M[0][0], projL.M[0][1], projL.M[0][2], projL.M[0][3],
				projL.M[1][0], projL.M[1][1], projL.M[1][2], projL.M[1][3],
				projL.M[2][0], projL.M[2][1], projL.M[2][2], projL.M[2][3],
				projL.M[3][0], projL.M[3][1], projL.M[3][2], projL.M[3][3] ) );
	camRight->setCustomProjectionMatrix( true,
		Ogre::Matrix4( projR.M[0][0], projR.M[0][1], projR.M[0][2], projR.M[0][3],
				projR.M[1][0], projR.M[1][1], projR.M[1][2], projR.M[1][3],
				projR.M[2][0], projR.M[2][1], projR.M[2][2], projR.M[2][3],
				projR.M[3][0], projR.M[3][1], projR.M[3][2], projR.M[3][3] ) );
}

void Rift::attachCameras(Ogre::Camera* const camLeft, Ogre::Camera* const camRight)
{
	// Tell Ogre to put virtual camera rendered images on the two textures for distortion meshes (RenderTexture is a viewport on a texture)
	mRenderTexture[ovrEye_Left] = mLeftEyeRenderTexture->getBuffer()->getRenderTarget();
	mRenderTexture[ovrEye_Left]->addViewport(camLeft);	// camLeft rendering will warp so that it covers all mLeftEyeRenderTexture surface
	mRenderTexture[ovrEye_Right] = mRightEyeRenderTexture->getBuffer()->getRenderTarget();
	mRenderTexture[ovrEye_Right]->addViewport(camRight);	// same for camRight and mRightEyeRenderTexture

	// Setup viewports proprieties
	mRenderTexture[ovrEye_Left]->getViewport(0)->setClearEveryFrame(true);
	mRenderTexture[ovrEye_Left]->getViewport(0)->setBackgroundColour(Ogre::ColourValue::Black);
	mRenderTexture[ovrEye_Left]->getViewport(0)->setOverlaysEnabled(true);

	mRenderTexture[ovrEye_Right]->getViewport(0)->setClearEveryFrame(true);
	mRenderTexture[ovrEye_Right]->getViewport(0)->setBackgroundColour(Ogre::ColourValue::Black);
	mRenderTexture[ovrEye_Right]->getViewport(0)->setOverlaysEnabled(true);

	// Oculus documentation suggests to ask to SDK for which eye should be rendered first.
	// Just before rendering mWindow, we will ask that and render the two textures manually.
	// So they must not be in any case rendered automatically by Ogre rendering loop.
	mRenderTexture[ovrEye_Left]->setAutoUpdated(false);
	mRenderTexture[ovrEye_Right]->setAutoUpdated(false);

	// Register render listener (to perform operations right before and after rendering the textures)
	mRenderTexture[ovrEye_Left]->addListener(this);
	mRenderTexture[ovrEye_Right]->addListener(this);
}

bool Rift::update( float dt )
{
	if( !hmd ) return true;

	ovrTrackingState ts = ovrHmd_GetTrackingState(hmd, frameTiming.ScanoutMidpointSeconds);

	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
		// The cpp compatibility layer is used to convert ovrPosef to Posef (see OVR_Math.h)
		Posef pose = ts.HeadPose.ThePose;
		//headPose = pose;
		//float yaw, pitch, roll;
		//pose.Rotation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &pitch, &roll);

		// Optional: move cursor back to starting position and print values
		//std::cout << "yaw: "   << RadToDegree(yaw)   << std::endl;
		//std::cout << "pitch: " << RadToDegree(pitch) << std::endl;
		//std::cout << "roll: "  << RadToDegree(roll)  << std::endl;
		mOrientation = Ogre::Quaternion( pose.Rotation.w,
			pose.Rotation.x, pose.Rotation.y, pose.Rotation.z );

		mPosition = Ogre::Vector3( pose.Translation.x, pose.Translation.y, pose.Translation.z );
	}
	

	/*
	frameTiming = ovrHmd_BeginFrameTiming(hmd, 0);
	ovrPosef headPose;


	for (int eyeNum = 0; eyeNum < 2; eyeNum++)
	{
		ovrMatrix4f tWM[2];
		ovrHmd_GetEyeTimewarpMatrices(hmd, (ovrEyeType)eyeNum,
			headPose, tWM);

		Ogre::GpuProgramParametersSharedPtr params = mMatRight->getTechnique(0)->getPass(0)->getVertexProgramParameters();
		params->setNamedConstant("eyeRotationStart",
			Ogre::Matrix4(tWM[0].M[0][0], tWM[0].M[0][1], tWM[0].M[0][2], tWM[0].M[0][3],
			tWM[0].M[1][0], tWM[0].M[1][1], tWM[0].M[1][2], tWM[0].M[1][3],
			tWM[0].M[2][0], tWM[0].M[2][1], tWM[0].M[2][2], tWM[0].M[2][3],
			tWM[0].M[3][0], tWM[0].M[3][1], tWM[0].M[3][2], tWM[0].M[3][3]
			));
		params->setNamedConstant("eyeRotationEnd",
			Ogre::Matrix4(tWM[1].M[0][0], tWM[1].M[0][1], tWM[1].M[0][2], tWM[1].M[0][3],
			tWM[1].M[1][0], tWM[1].M[1][1], tWM[1].M[1][2], tWM[1].M[1][3],
			tWM[1].M[2][0], tWM[1].M[2][1], tWM[1].M[2][2], tWM[1].M[2][3],
			tWM[1].M[3][0], tWM[1].M[3][1], tWM[1].M[3][2], tWM[1].M[3][3]
			));
	}

	ovr_WaitTillTime( frameTiming.TimewarpPointSeconds );


	ovrHmd_EndFrameTiming(hmd);
	*/
	return true;
}

// All render targets (mRenderWindow and mRenderTexture[]) are registered to this listener
void Rift::preRenderTargetUpdate(const Ogre::RenderTargetEvent& rte)
{

	if(rte.source == mRenderWindow)	// I am rendering the main window to be displayed on the Rift
	{
		// Phase (1): tell Oculus SDK that the frame (the one to be displayed on the rift) is about to be rendered
		frameTiming = ovrHmd_BeginFrameTiming(hmd, 0);
		
		// Phase (2) and (3): render the two eye RenderTextures (in the order suggested by SDK)
		// This will call again preRenderTargetUpdate() but for the two RenderTextures
		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
		{
			nextEyeToRender = hmd->EyeRenderOrder[eyeIndex];
			mRenderTexture[nextEyeToRender]->update();
		}

		// Phase (4): Wait till time-warp point to reduce latency (to get closest as possible to the screen time).
		// You can put some operations BEFORE THIS POINT to squeeze some extra CPU.
		ovr_WaitTillTime(frameTiming.TimewarpPointSeconds);

		// Final Rendering Phase (5): predict eye pose one last time and apply timewarp to each eye
		for (int eyeNum = 0; eyeNum < 2; eyeNum++)
		{
			// Init shader parameters pointer
			Ogre::GpuProgramParametersSharedPtr params;
			switch (eyeNum) // order of shading is not important, only need to do everything for each eye
			{
			case 0:
				params = mMatLeft->getTechnique(0)->getPass(0)->getVertexProgramParameters();
				break;
			case 1:
				params = mMatRight->getTechnique(0)->getPass(0)->getVertexProgramParameters();
				break;
			default:
				break;
			}

			// Sample the third time eye pose and get the two time-warp matrices
			ovrMatrix4f tWM[2];
			ovrHmd_GetEyeTimewarpMatrices(hmd, (ovrEyeType)eyeNum, headPose[eyeNum], tWM);

			// Set time-warp matrices in the Oculus vertex shader
			params->setNamedConstant("eyeRotationStart",
				Ogre::Matrix4(tWM[0].M[0][0], tWM[0].M[0][1], tWM[0].M[0][2], tWM[0].M[0][3],
				tWM[0].M[1][0], tWM[0].M[1][1], tWM[0].M[1][2], tWM[0].M[1][3],
				tWM[0].M[2][0], tWM[0].M[2][1], tWM[0].M[2][2], tWM[0].M[2][3],
				tWM[0].M[3][0], tWM[0].M[3][1], tWM[0].M[3][2], tWM[0].M[3][3]
				));
			params->setNamedConstant("eyeRotationEnd",
				Ogre::Matrix4(tWM[1].M[0][0], tWM[1].M[0][1], tWM[1].M[0][2], tWM[1].M[0][3],
				tWM[1].M[1][0], tWM[1].M[1][1], tWM[1].M[1][2], tWM[1].M[1][3],
				tWM[1].M[2][0], tWM[1].M[2][1], tWM[1].M[2][2], tWM[1].M[2][3],
				tWM[1].M[3][0], tWM[1].M[3][1], tWM[1].M[3][2], tWM[1].M[3][3]
				));
		}
	}
	else // I am rendering one of the two RenderTextures
	{
		// Phase (2) and (3): predict eye/head pose, apply head pose then render (one eye at a time)
		ovrTrackingState ts; // order of the eye matters (nextEyeToRender is used)
		ovrHmd_GetEyePoses(hmd, 0, &(eyeRenderDesc[nextEyeToRender].HmdToEyeViewOffset), headPose, &ts);
		if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
		{
			Posef pose = ts.HeadPose.ThePose;
			mOrientation = Ogre::Quaternion(pose.Rotation.w, pose.Rotation.x, pose.Rotation.y, pose.Rotation.z);
			mPosition = Ogre::Vector3(pose.Translation.x, pose.Translation.y, pose.Translation.z);
			mHeadNode->setOrientation(mOrientation);
			mHeadNode->setPosition(mPosition);
		}
	}

}
void Rift::postRenderTargetUpdate(const Ogre::RenderTargetEvent& rte)
{
	// Phase (6): tell Oculus SDK that the frame just finished rendering
	if(rte.source==mRenderWindow) ovrHmd_EndFrameTiming(hmd);
}


void Rift::recenterPose()
{
	if ( hmd )
		ovrHmd_RecenterPose( hmd );
}
