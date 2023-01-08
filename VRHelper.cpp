#include "VRHelper.h"
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <cstdarg>
#include <sstream>
#include <Windows.h>

using namespace std;

VRHelper::VRHelper() :
	tick(1, false)
{
	// Allocations
	message = new char[MAX_MESSAGE_LENGTH];
	lastErrorMessage = new char[MAX_ERROR_MESSAGE_LENGTH];
	clrmsg();
	clrerrmsg();
	logFile = fopen("VRHelper.log", "w");
	lineNr = 0;
	log("Start of log.");
	// Status
	valid = false;
};

VRHelper::~VRHelper() {
	log("Exiting...");
	fclose(logFile);
	SAFE_FREE_CHAR_P(lastErrorMessage);
	SAFE_FREE_CHAR_P(message);
}

void VRHelper::msg(const char *formatString, ...) {
	
	/*va_list args;
	va_start(args, formatString);
	std::stringstream ss;
	while (*formatString != '\0') {
		if (*formatString == 'd') {
			int i = va_arg(args, int);
			ss << i << '\n';
		}
		else if (*formatString == 'c') {
			// note automatic conversion to integral type
			int c = va_arg(args, int);
			ss << static_cast<char>(c) << '\n';
		}
		else if (*formatString == 'f') {
			double d = va_arg(args, double);
			ss << d << '\n';
		}
		++formatString;
	}
	va_end(formatString);

	snprintf(message, MAX_MESSAGE_LENGTH, ss.str().c_str());*/
}

void VRHelper::errmsg(const char* formatString, ...) {
	/*
	va_list args;
	va_start(args, formatString);
	std::stringstream ss;
	while (*formatString != '\0') {
		if (*formatString == 'd') {
			int i = va_arg(args, int);
			ss << i << '\n';
		}
		else if (*formatString == 'c') {
			// note automatic conversion to integral type
			int c = va_arg(args, int);
			ss << static_cast<char>(c) << '\n';
		}
		else if (*formatString == 'f') {
			double d = va_arg(args, double);
			ss << d << '\n';
		}
		++formatString;
	}
	va_end(formatString);

	snprintf(lastErrorMessage, MAX_MESSAGE_LENGTH, ss.str().c_str());*/
}

void VRHelper::clrmsg() {
	memset(message, 0, MAX_MESSAGE_LENGTH);
}

void VRHelper::clrerrmsg() {
	memset(lastErrorMessage, 0, MAX_ERROR_MESSAGE_LENGTH);
}

void VRHelper::log(const char* txt) {
	fprintf(logFile, "%5d: %s\n", lineNr++, txt);
	fflush(logFile);
}

bool VRHelper::Init() {
	tick.start();
	snprintf(message, MAX_MESSAGE_LENGTH, "");
	int rez = vpxInit();
	if (VPX_RES_OK != rez) {
		snprintf(lastErrorMessage, MAX_ERROR_MESSAGE_LENGTH, "VorpX initializing error: %d", rez);
		log(lastErrorMessage);
		return false;
	}

	log("VorpX initialized.");
	setValid(true);

	return true;
}

void VRHelper::Shutdown() {
	if (isValid()) {
		if (VPX_RES_OK != vpxFree()) {
			snprintf(lastErrorMessage, MAX_ERROR_MESSAGE_LENGTH, "VorpX freeing error");
		}
	}
}

void VRHelper::Update() {
    if (tick.is()) {
		//sprintf(c, "VR[%d]: %f, %f, %f", k++, headsetPitch, headsetYaw, headsetRoll);
		snprintf(message, MAX_MESSAGE_LENGTH, "VorpX is %s", (vpxIsActive() ? "active!" : "NOT active!"));
		log(message);
    }
}

void VRHelper::Get_HMD_Orientation(VROrientation& vrOrientation) {
	vpxfloat3 rez = vpxGetHeadsetRotationEuler();
	vrOrientation.pitch = rez.x;
	vrOrientation.yaw = rez.y;
	vrOrientation.roll = rez.z;
}

void VRHelper::Get_Controller_Orientation(VROrientation& vrOrientation, int controllerInd) {
	vpxfloat3 rez = vpxGetControllerRotationEuler(controllerInd);
	vrOrientation.pitch = rez.x;
	vrOrientation.yaw = rez.y;
	vrOrientation.roll = rez.z;
}

void VRHelper::Get_Controller_State(VR_Controller_State& state, int controllerInd) {
	VPX_CONTROLLER_STATE vpxCS = vpxGetControllerState(controllerInd);
	state.IsActive = vpxCS.IsActive;
	state.StickX = vpxCS.StickX;
	state.StickY = vpxCS.StickY;
	state.Trigger = vpxCS.Trigger;
	state.Grip = vpxCS.Grip;
	state.Extra0 = vpxCS.Extra0;
	state.Extra1 = vpxCS.Extra1;
	state.Extra2 = vpxCS.Extra2;
	state.Extra3 = vpxCS.Extra3;
	state.Finger0 = vpxCS.Finger0;
	state.Finger1 = vpxCS.Finger1;
	state.Finger2 = vpxCS.Finger2;
	state.Finger3 = vpxCS.Finger3;
	state.Finger4 = vpxCS.Finger4;
	state.ButtonsPressed = vpxCS.ButtonsPressed;
	state.ButtonsTouched = vpxCS.ButtonsTouched;
}

void VRHelper::SetRenderControllers(bool state) {
	vpxForceControllerRendering(state);
}
