/******************************************************************************
 *
 *    vorpX C API for modders and developers
 *    Copyright 2019 Ralf Ostertag, Animation Labs. All rights reserved.
 *
 *****************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vpxint2 { int x; int y; } vpxint2;
typedef struct vpxint3 { int x; int y; int z; } vpxint3;
typedef struct vpxint4 { int x; int y; int z; int w; } vpxint4;
typedef struct vpxfloat2 { float x; float y; } vpxfloat2;
typedef struct vpxfloat3 { float x; float y; float z; } vpxfloat3;
typedef struct vpxfloat4 { float x; float y; float z; float w; } vpxfloat4;
typedef struct vpxmtx4x4 { float m[16]; } vpxmtx4x4;

typedef unsigned int VPX_EYE;
#define VPX_EYE_LEFT	0
#define VPX_EYE_RIGHT	1
#define VPX_LEFT		0
#define VPX_RIGHT		1

typedef unsigned int VPX_BOOL;
#define VPX_FALSE		0
#define VPX_TRUE		1

typedef unsigned int VPX_RESULT;
#define	VPX_RES_OK								0
#define	VPX_RES_FAIL							1
#define	VPX_RES_INVALID_ARGUMENT				101
#define	VPX_RES_NULL_POINTER					102
#define	VPX_RES_NOT_INITIALIZED					103
#define	VPX_RES_FUNCTION_UNAVAILABLE			104

typedef unsigned int VPX_GAME_STEREO_MODE;
#define	VPX_GAME_STEREO_MODE_NONE				0
#define	VPX_GAME_STEREO_MODE_SINGLE_FRAME		1
#define	VPX_GAME_STEREO_MODE_ALTERNATE_FRAME	2	

enum VPX_CONTROLLER_BUTTON
{
	VPX_CONTROLLER_BUTTON_0						= 0x001,	// A/X on Touch/Index, pad click on Vive wands
	VPX_CONTROLLER_BUTTON_1						= 0x002,	// B/Y on Touch/Index, unavailable on Vive wands!
	VPX_CONTROLLER_BUTTON_MENU					= 0x004,	// Menu button on Touch/Vive, pad click on Index. Note that on Touch the right menu button is reserved for Oculus and thus can't be accessed.
	VPX_CONTROLLER_BUTTON_STICK_PAD_0			= 0x008,	// Stick click on Touch/Index, pad click on Vive wands
	VPX_CONTROLLER_BUTTON_TRIGGER				= 0x010,	// Trigger click on Vive wands, on Touch/Index emulated with a trigger axis threshold
	VPX_CONTROLLER_BUTTON_GRIP					= 0x020		// Grip click on Vive wands, on Touch/Index emulated with a grip axis threshold
};

typedef struct VPX_CONTROLLER_STATE
{
	VPX_BOOL IsActive;				// VPX_TRUE if active, otherwise VPX_FALSE
	float StickX;					// Thumbstick/pad x-axis [-1|1]
	float StickY;					// Thumbstick/pad y-axis [-1|1]
	float Trigger;					// Trigger axis [0|1]
	float Grip;						// Grip axis [0|1], on controllers with a grip button (e.g. Vive wands) either 0.0 or 1.0
	float Extra0;					// Extra axis (for future use)
	float Extra1;					// Extra axis (for future use)
	float Extra2;					// Extra axis (for future use)
	float Extra3;					// Extra axis (for future use)
	float Finger0;					// Finger axis: thumb (for future use)
	float Finger1;					// Finger axis: index (for future use)
	float Finger2;					// Finger axis: middle (for future use)
	float Finger3;					// Finger axis: ring (for future use)
	float Finger4;					// Finger axis: pinky (for future use)
	unsigned int ButtonsPressed;	// Check with a flag, e.g.: if (ButtonsPressed & VPX_CONTROLLER_BUTTON_0)
	unsigned int ButtonsTouched;	// Check with a flag, e.g.: if (ButtonsTouched & VPX_CONTROLLER_BUTTON_0)
} VPX_CONTROLLER_STATE;

// Always call vpxInit() once in your project before accessing any other vorpX API function.
// Call as late as possible to ensure vorpX is fully loaded before.
// Ideally call after the game/app has created its main rendering device/context.
VPX_RESULT vpxInit();

// When you are done with using the API, use this to hand full control back to vorpX.
VPX_RESULT vpxFree();

// Check whether the API is initialized and active.
VPX_BOOL vpxIsActive();

// Vertical field of view in degrees.
// Check every frame, may change depending on vorpX settings.
// Calling this function disables vorpX's internal FOV handling.
float vpxGetHeadsetFOV();

// Converts a vertical FOV in degrees to a horizontal FOV in degrees.
// fov_deg: vertical FOV in degrees
// aspect: aspect ratio used for the conversion (e.g. 4/3 or 16/9)
float vpxVertToHorFOV(const float fov_deg, const float aspect);

// Rotation as Euler angles (pitch/yaw/roll) in degrees.
// Calling this function disables vorpX's internal rotation tracking
vpxfloat3 vpxGetHeadsetRotationEuler();

// Rotation as quaternion in radians.
// Calling this function disables vorpX's internal rotation tracking
vpxfloat4 vpxGetHeadsetRotationQuaternion();

// Position from the calibrated headset center in meters (lr/ud/fb).
// Has to be converted if your game does not use meters as base unit.
// You may have to use vpxYawCorrection() depending on your camera code. 
// Calling this function disables vorpX's internal position tracking
vpxfloat3 vpxGetHeadsetPosition();

// Returns a VR controller state.
// Valid controllerNum values: VPX_LEFT, VPX_RIGHT
// Calling this function disables vorpX's internal VR controller state handling
VPX_CONTROLLER_STATE vpxGetControllerState(const unsigned int controllerNum);

// Rotation as Euler angles (pitch/yaw/roll) in degrees.
// Valid controllerNum values: VPX_LEFT, VPX_RIGHT
// Calling this function disables vorpX's internal VR controller rendering
vpxfloat3 vpxGetControllerRotationEuler(const unsigned int controllerNum);

// Rotation as quaternion in radians.
// Valid controllerNum values: VPX_LEFT, VPX_RIGHT
// Calling this function disables vorpX's internal VR controller rendering
vpxfloat4 vpxGetControllerRotationQuaternion(const unsigned int controllerNum);

// Position from the calibrated headset center in meters (lr/ud/fb).
// Has to be converted if your game does not use meters as base unit.
// You may have to use vpxYawCorrection() depending on your camera code. 
// Valid controllerNum values: VPX_LEFT, VPX_RIGHT
// Calling this function disables vorpX's internal VR controller rendering
vpxfloat3 vpxGetControllerPosition(const unsigned int controllerNum);

// Override vorpX's decision to render VR controllers or not.
// VPX_TRUE: always render, VPX_FALSE: let vorpX decide
void vpxForceControllerRendering(const VPX_BOOL val);

// Use with headset and controller positions if necessary to translate them into world coordinates.
// yawInDegrees is your camera yaw (y-axis rotation) BEFORE adding the headset rotation.
vpxfloat3 vpxYawCorrection(const vpxfloat3 position, const float yawInDegrees);

// Use this with VPX_TRUE/VPX_FALSE whenever you enter/leave a menu, cutscene,
// or any other part of the game that works better in EdgePeek mode.
void vpxRequestEdgePeek(const VPX_BOOL val);

// Returns the value last set with vpxRequestEdgePeek();
VPX_BOOL vpxGetEdgePeekRequested();

// Use this to check the actual state of EdgePeek mode.
// May differ from the one set with vpxRequestEdgePeek if the user changes it manually.
VPX_BOOL vpxGetEdgePeekActual();

// Gets the vorpX eye position in meters from calibrated headset origin.
// You only need this function if you want to do the stereo rendering yourself.
// If the user has enabled vorpX stereo rendering this returns the same position as vpxGetHeadsetPosition().
// eye: VPX_LEFT (0), VPX_RIGHT (1)
// ipd: interpupillary distance in meters
// Calling this function disables vorpX's internal position tracking
vpxfloat3 vpxGetEyePosition(const VPX_EYE eye, const float ipd);

// Returns the G3D Strength multiplier dialed in the vorpX menu
// You only need this function if you want to do the stereo rendering yourself.
float vpxGetG3DStrength();

int vpxGetCurrentVorpFrame();

// Returns TRUE if the user has chosen to let vorpX do stereo rendering. If necessary handle this case in your code.
VPX_BOOL vpxIsVorpDoing3D();

// Has to be the raw game camera rotation delta (without headset rotation) between two frames in degrees (pitch, yaw, roll).
// vorpX can utilize the camera rotation to enhance frame interpolation.
// Doesn't have to be provided. If you do, you MUST do so each frame.
void vpxSetGameCamRotationDeltaEuler(const vpxfloat3 rotation);

// Reduce the stereo separation (strength) set by the user.
// Caveat: Reducing the separation changes the perceived world scale. 
// Only use this temporarily if for example cutscenes look off in regard to stereo 3D.
// Default 0.0f, valid range : 0.0f - 1.0f, other values are clamped to the valid range.
void vpxSetStereoReduction(const float reduction);

// Call this directly after initializing the API to let vorpX know you handle stereo yourself
// Note that you still have to call vpxSetGameStereoRenderMode() later to enable stereo.
void vpxInitStereo();

// If you do stereo rendering yourself, you MUST call this method after init and every time the stero rendering mode changes. 
// For alternate frame rendering also make sure to call vpxGetAlternateFrame3DNextFrameEye() directly before each Present() call.
void vpxSetGameStereoRenderMode(VPX_GAME_STEREO_MODE stereoMode);

// Let vorpX know the eye that belongs to the last rendered frame.
// MUST be called on the Present thread, directly BEFORE present.
void vpxSetAlternateFrame3DEye(const VPX_EYE eye);

// Converts an angle from degrees to radians.
inline float vpxDegToRad(const float a) { return a * 0.0174532925f; }

// Converts an angle from radians to degrees.
inline float vpxRadToDeg(const float a) { return a * 57.295795131f; }

typedef unsigned int VPX_PROP_ID;
#define VPX_PROP_HEADSET_FOV					100
#define VPX_PROP_HEADSET_ROT_EULER				101
#define VPX_PROP_HEADSET_ROT_QUATERNION			102
#define VPX_PROP_HEADSET_POS					103
#define VPX_PROP_CONTROLLER_L_ROT_EULER			200
#define VPX_PROP_CONTROLLER_L_ROT_QUATERNION	201
#define VPX_PROP_CONTROLLER_L_POS				202
#define VPX_PROP_CONTROLLER_R_ROT_EULER			203
#define VPX_PROP_CONTROLLER_R_ROT_QUATERNION	204
#define VPX_PROP_CONTROLLER_R_POS				205
#define VPX_PROP_CONTROLLER_FORCE_RENDER		206
#define	VPX_PROP_STEREO_REDUCTION				300
#define	VPX_PROP_EDGE_PEEK_REQUESTED			400
#define	VPX_PROP_EDGE_PEEK_ACTUAL				401
#define	VPX_PROP_AFR3D_CURRENT_FRAME_EYE		500
#define	VPX_PROP_G3D_SEPARATION_SETTING			501
#define	VPX_PROP_IS_VORP_DOING_3D				502
#define	VPX_PROP_CURRENT_VORP_FRAME				503
#define	VPX_PROP_GAME_CAM_ROTATION_DELTA_EULER	600
#define	VPX_PROP_GAME_CAN_STEREO_3D				600
#define	VPX_PROP_GAME_STEREO_MODE				601

#ifdef __cplusplus
}
#endif
