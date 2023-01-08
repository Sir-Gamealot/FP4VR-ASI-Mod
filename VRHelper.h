#pragma once

#define MAX_DEVICE_COUNT_TO_CHECK 100
#define MAX_MESSAGE_LENGTH 1024
#define MAX_ERROR_MESSAGE_LENGTH 4096

#define SAFE_FREE_CHAR_P(p) if(NULL != p) { free(p); p = NULL; }
#define SAFE_DELETE(p) if(NULL != p) { delete p; p = NULL; }

#include "tick.h"
#include <stdio.h>
#include <malloc.h>
#include <string>
#include "VorpX/vorpAPI.h"

#define VR_CONTROLLER_LEFT 0
#define VR_CONTROLLER_RIGHT 1

typedef struct VROrientation_struct {
	float pitch, yaw, roll;
	VROrientation_struct() {
		pitch = yaw = roll = 0.0f;
	}
	VROrientation_struct(float pitch, float yaw, float roll)
	{
		this->pitch = pitch;
		this->yaw = yaw;
		this->roll = roll;
	}
} VROrientation;

typedef struct VR_Controller_State_struct {
	unsigned int IsActive;			// VPX_TRUE if active, otherwise VPX_FALSE
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
} VR_Controller_State;

class VRHelper {
private:
	bool valid;
	FILE* logFile;
	int lineNr;
	Tick tick;
	void setValid(bool v) { valid = v; }
	int leftControllerInd;
public:
	char* message, * lastErrorMessage;
	VROrientation headsetRotation, leftControllerRotation, rightControllerRotation;
	VRHelper();
	~VRHelper();
	bool isValid() { return valid; }
	void log(const char* txt);
	void clrmsg();
	void clrerrmsg();
	void msg(const char* formatString, ...);
	void errmsg(const char* formatString, ...);
	bool Init();
	void Update();
	void Shutdown();
	// Direct VorpX calls
	void Get_HMD_Orientation(VROrientation& vrOrientation);
	void Get_Controller_Orientation(VROrientation& orientation, int controllerInd);
	void Get_Controller_State(VR_Controller_State& state, int controllerInd);
	void SetRenderControllers(bool state);
};

