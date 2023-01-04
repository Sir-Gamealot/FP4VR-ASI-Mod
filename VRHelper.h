#pragma once

#define MAX_DEVICE_COUNT_TO_CHECK 100
#define MAX_MESSAGE_LENGTH 1024
#define MAX_ERROR_MESSAGE_LENGTH 4096

#define SAFE_FREE_CHAR_P(p) if(NULL != p) { free(p); p = NULL; }

#include "tick.h"
#include <stdio.h>
#include <malloc.h>
#include <string>
#include "VorpX/vorpAPI.h"

typedef struct VROrientation_struct {
	float Pitch, Yaw, Roll;
} VROrientation;

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
	float headsetPitch, headsetYaw, headsetRoll;
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
};

