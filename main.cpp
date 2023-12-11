#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include <cstdio>
#define _USE_MATH_DEFINES
#include <cmath>
#include <chrono>
#include <cstdint>

#define GAMELE1

#include "../../Shared-ASI/Interface.h"
#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"
#include "../../Shared-ASI/ConsoleCommandParsing.h"
#include "../UnrealscriptDebugger/ScriptDebugLogger.h"
#include "../../Shared-ASI/StaticVariablePointers.h"

#pragma warning(push, 2)

#include "main.h"
#include "MainHUD.h"

#define MYHOOK "FP4VR-ASI-Mod_"

#define PATTERN_PROCESS_EVENT                /*40 55 41 56 41*/ "57 48 81 EC 90 00 00 00 48 8D 6C 24 20"
#define PATTERN_EXEC_EVENT                                      "48 8b c4 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8d a8 d8 fe ff ff"
#define PATTERN_PROCESS_INTERNAL_PH          /*40 53 55 56 57*/ "48 81 EC 88 00 00 00 48 8B ?? ?? ?? ?? ?? 48 33 C4 48 89 44 24 70 48 8B 01 48 8B DA 48 8B 52 14"
#define PATTERN_FIND_FUNCTION_CHECKED_PH     /*48 8B C4 48 89*/ "50 10 56 57 41 56 48 83 EC 60 48 C7 40 B8 FE FF FF FF 48 89 58 08 48 89 68 18 48 8B E9 E8 ?? ?? ?? ?? 48 8B F0"

// Pattern for hooking the constructor function for an FName/SFXName (LE1/LE2/LE3)
#define LE_PATTERN_POSTHOOK_SFXNAMECONSTRUCTOR   /*40 55 56 57 41*/ "54 41 55 41 56 41 57 48 81 ec 00 07 00 00"

typedef void (*tProcessEvent)(UObject* Context, UFunction* Function, void* Parms, void* Result);
tProcessEvent ProcessEvent = nullptr;
tProcessEvent ProcessEvent_orig = nullptr;

typedef void (*tPlayerMoveExploreHandler)(float DeltaTime, float MoveMag, float MoveAngle, FRotator MoveRot);
tPlayerMoveExploreHandler PlayerMoveExploreHandler = nullptr;
tPlayerMoveExploreHandler PlayerMoveExploreHandler_orig = nullptr;

#define CALL_VIRTUAL_UFUNC(obj, name, parms) ProcessEvent_orig(obj, FindFunctionChecked_orig(obj, FName{ name }, 0), & parms, nullptr)

namespace fs = std::filesystem;

SPI_PLUGINSIDE_SUPPORT(L"FP4VR-ASI-Mod", L"Sir Gamealot", L"1.0.2", SPI_GAME_LE1, 2);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

MainHUD* hud = new MainHUD;

ME3TweaksASILogger logger("Function Logger v2", "LE1FunctionLog.log");
char token[256];
char txt[4096];

VRHelper* vrHelper;

// VR variables
VROrientation hmdRot, leftRot, rightRot;
VRPosition hmdPos, leftPos, rightPos;
VR_Controller_State leftState, rightState;
//FVector hmd, left, right;

// Game variables
FRotator print_MoveRot, print_Facing;
float print_MoveAngle, print_MoveMag;
float print_Yaw;
// Sir variables
//FRotator *t_MoveRot, *t_FacingRot;
//FVector *t_AccellerationVec;
//FRotator R;

// ======================================================================
// ExecHandler and ProcessInternal hook
// ======================================================================

typedef unsigned (*tExecHandler)(UEngine* Context, wchar_t* cmd, void* unk);
tExecHandler ExecHandler = nullptr;
tExecHandler ExecHandler_orig = nullptr;

typedef void (*tProcessInternal)(UObject* Context, FFrame* Stack, void* Result);
tProcessInternal ProcessInternal = nullptr;
tProcessInternal ProcessInternal_orig = nullptr;

typedef UFunction* (*tFindFunctionChecked)(UObject* Context, FName Name, DWORD Global);
tFindFunctionChecked FindFunctionChecked = nullptr;
tFindFunctionChecked FindFunctionChecked_orig = nullptr;

// First parameter is actually an array of 2 integers
typedef void* (*tGenerateName2)(unsigned int* param1, wchar_t* nameValue, int indexValue, BOOL parm4, BOOL parm5);
tGenerateName2 sfxNameConstructor = nullptr;
tGenerateName2 sfxNameConstructor_orig = nullptr;

BYTE* bin;
static bool yes = false;

#define F_DEGREES_TO_GAME_ROTATION_UNITS 182.0444444f // Factor is from 32768.f / 180.f

Tick startupDelay(20, true), tenSeconds(10, false), oneSecond(1, false);

ScriptDebugLogger* scriptDebugLogger;

// VR variables
float TheHeadYaw = 0.0f;
float TheControllerYaw = 0.0f;

#pragma pack(push, 4)
typedef struct BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack_struct {
    float DeltaTime;
    float MoveMag;
    float MoveAngle;
    FRotator MoveRot;
    // Sir variables
    //FRotator t_MoveRot;
    //FRotator t_FacingRot;
    //FVector t_AccelerationVec;
    //FRotator R;
} BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack;
#pragma pack(pop)


uint64_t timeSinceEpochMillisec() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void logState(VR_Controller_State state) {
	logger.writeToConsole(string_format("State: Stick X = %f, Stick Y = %f, Trigger - %f", state.StickX, state.StickY, state.Trigger));
}

void logParams(BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack* params) {
    logger.writeToConsole(
        string_format("State: MoveAngle = %f, MoveMag = %f, MoveRot[Pitch: %d, Yaw: %d, Roll: %d]",
                        params->MoveAngle, params->MoveMag,
                        params->MoveRot.Pitch, params->MoveRot.Yaw, params->MoveRot.Roll
        )
    );
}

float getToken() {
    float rez = 0.0f;
	FILE* f = fopen("token.txt", "r");
	fseek(f, 0, SEEK_END);
	int size = ftell(f) + 1;
	fseek(f, 0, SEEK_SET);
	char c[64];
	memset(c, 0, 64);
	fread(c, 1, size, f);
	fclose(f);
	int OldTheYaw = (int)rez;
	try {
		rez = stof(c);
	} catch (...) {
		rez = NAN;
	}
	logger.writeToConsole(string_format("TheYaw = %f", rez));
    return rez;
}

void getVRVariables() {
	vrHelper->Get_HMD_Orientation(hmdRot);
    vrHelper->Get_HMD_Position(hmdPos);
	//vrHelper->Get_Controller_State(leftState, VR_CONTROLLER_LEFT);
	vrHelper->Get_Controller_Orientation(leftRot, VR_CONTROLLER_LEFT);
    vrHelper->Get_Controller_Position(leftPos, VR_CONTROLLER_LEFT);
	//vrHelper->Get_Controller_State(rightState, VR_CONTROLLER_RIGHT);
	vrHelper->Get_Controller_Orientation(rightRot, VR_CONTROLLER_RIGHT);
    vrHelper->Get_Controller_Position(rightPos, VR_CONTROLLER_RIGHT);
}

/*void getParams(UObject* Context, FFrame* Stack, void* Result) {
	BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack* params =
		reinterpret_cast<BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack*>(Stack->Locals);
	MoveMag = &params->MoveMag;
	MoveAngle = &params->MoveAngle;
	MoveRot = &params->MoveRot;
    // Sir
    //t_MoveRot = &params->t_MoveRot;
    //t_FacingRot = &params->t_FacingRot;
    //t_AccellerationVec = &params->t_AccelerationVec;
    //*(&params->R) = R;
}*/

void MoveFRotatorToFVector(FRotator& src, FVector& dest) {
    dest.X = (float)src.Pitch;
    dest.Y = (float)src.Yaw;
    dest.Z = (float)src.Roll;
}

void MoveFVectorToFRotator(FVector& src, FRotator& dest) {
	dest.Pitch = (int)src.X;
	dest.Yaw = (int)src.Y;
	dest.Roll = (int)src.Z;
}

void FRotatorToUnitFVector(FRotator& src, FVector& dest) {
    double angle = (double)src.Yaw * M_PI / 32768.0;
    double x = cos(angle);
    double y = sin(angle);
    dest.X = x;
    dest.Y = y;
    dest.Z = 0;
}

FVector FVectorMulScalar(FVector& vec, float& val) {
    FVector rez;
    rez.X = (double)vec.X * (double)val;
    rez.Y = (double)vec.Y * (double)val;
    rez.Z = (double)vec.Z * (double)val;
    return rez;
}

void FVectorInit(FVector& vec) {
    vec.X = 0.0f;
    vec.Y = 0.0f;
    vec.Z = 0.0f;
}

void FRotatorInit(FRotator& rot) {
    rot.Pitch = 0;
    rot.Yaw = 0;
    rot.Roll = 0;
}

float UnrealRotationUnitsToRadiansD(int unrealRotationUnits) {
    return (float)(unrealRotationUnits * 360.0 / 65536.0 * M_PI / 180.0);
}

void GetDirectionalVectorFromRotator(FVector& vec, FRotator& rot) {
	double cp = cos(UnrealRotationUnitsToRadians(rot.Pitch));
	double cy = cos(UnrealRotationUnitsToRadians(rot.Yaw));
	double sp = sin(UnrealRotationUnitsToRadians(rot.Pitch));
	double sy = sin(UnrealRotationUnitsToRadians(rot.Yaw));
    vec.X = (cp * cy);
    vec.Y = (cp * sy);
    vec.Z = (sp);	
    TheControllerYaw = (float)DegreesToUnrealRotationUnits(leftRot.yaw);
}

FRotator makeRotatorFromVRHMDOrientation() {
    FRotator rot;
    rot.Pitch = DegreesToUnrealRotationUnits(hmdRot.pitch);
    rot.Yaw = DegreesToUnrealRotationUnits(hmdRot.yaw);
    rot.Roll = DegreesToUnrealRotationUnits(hmdRot.roll);
    return rot;
}

FVector makeVectorFromVR_HMD_Position() {
    FVector vec;
    vec.X = hmdPos.x;
    vec.Y = hmdPos.y;
    vec.Z = hmdPos.z;
    return vec;
}

vpxfloat3 getCameraRotation(AActor* camera) {
    vpxfloat3 rez;
    FRotator camRot = camera->Rotation;
    rez.x = (float)camRot.Pitch;
    rez.y = (float)camRot.Yaw;
    rez.z = (float)camRot.Roll;
    return rez;
}

vpxfloat3 getCameraPosition(AActor* camera) {
	vpxfloat3 rez;
	FVector camRot = camera->Location;
    rez.x = camRot.X;
	rez.y = camRot.Y;
	rez.z = camRot.Z;
	return rez;
}

FRotator playerOrientation, leftControllerOrientation;
void processWalkingExplore(UObject* Context, FFrame* Stack, void* Result) {
	// function PlayerMoveExplore(float DeltaTime, float MoveMag, float MoveAngle, Rotator MoveRot, Rotator t_MoveVec, Rotator t_FacingRot, Vector t_AccelVec)

 // Params
	BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack* params =
		reinterpret_cast<BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack*>(Stack->Locals);
	float param_MoveMag = *(&params->MoveMag);
	float param_MoveAngle = *(&params->MoveAngle);
	FRotator param_MoveRot = *(&params->MoveRot);

    print_MoveMag = param_MoveMag;
    print_MoveAngle = param_MoveAngle;
    print_MoveRot = param_MoveRot;

	// Function Variables
	ABioPlayerController* playerController = (ABioPlayerController*)Context;
	FVector MoveDir;
	FRotator MoveFacing;
	float MoveWalkModifier;
	FRotator MoveRot2D;
	float MoveAccMag;
	ABioPawn* MyBP;

    static FRotator oldRot;
    FRotator newRot = param_MoveRot;
    int deg_PitchDelta = (int)UnrealRotationUnitsToDegrees(newRot.Pitch - oldRot.Pitch);
    int deg_YawDelta = (int)UnrealRotationUnitsToDegrees(newRot.Pitch - oldRot.Pitch);
    int deg_RollDelta = 0;
    vrHelper->SetGameCamRotationDeltaEuler(deg_PitchDelta, deg_YawDelta, deg_RollDelta);
    oldRot = newRot;

	MyBP = reinterpret_cast<ABioPawn*>(playerController->Pawn);
	if (MyBP == NULL) {
		return;
	}

    if (param_MoveMag < playerController->MoveStickRunThreshold) {
        playerOrientation = param_MoveRot;
        leftControllerOrientation.Pitch = (int)leftRot.pitch;
        leftControllerOrientation.Yaw = (int)leftRot.yaw;
        leftControllerOrientation.Roll = (int)leftRot.roll;
    }

    float aYaw = (float)(param_MoveRot.Yaw); // leftControllerOrientation.Yaw* F_DEGREES_TO_GAME_ROTATION_UNITS;// (float)(param_MoveRot.Yaw);// -playerOrientation.Yaw);
    print_Yaw = TheControllerYaw;

	ACamera* camera = playerController->PlayerCamera;
    
    FRotator cameraRot = camera->Rotation;
    //camera->AllowPawnRotation();
    //cameraRot.Pitch = 0;
    //static int camYaw = 0;
    //camYaw = (camYaw + 1000) % 65536;
    //cameraRot.Yaw = camYaw;
    //cameraRot.Roll = 0;
	//camera->SetRotation(cameraRot);

    //float fov_vpx_deg = vpxGetHeadsetFOV();
    //float fov_vpx_rad = vpxDegToRad(fov_vpx_deg);
    //if (fov_vpx_deg != camera->GetFOVAngle()) {
    //    camera->SetFOV(fov_vpx_deg);
    //}
    
    MoveRot2D.Yaw = (int)aYaw;
	MoveRot2D.Pitch = 0;
	MoveRot2D.Roll = 0;
	MoveAccMag = MyBP->AccelRate;
	MoveWalkModifier = 1.0f;
	if (param_MoveMag > playerController->MoveStickIdleThreshold) {
        // Walk
		if (MyBP->bIsCrouched) {
			MyBP->ShouldCrouch(FALSE);
		}
		if (!MyBP->bIsCrouched) {
			if (!MyBP->bIsWalking) {
				if (param_MoveMag >= playerController->MoveStickRunThreshold) {
					MoveWalkModifier = 1.0f;
				} else if (param_MoveMag >= playerController->MoveStickWalkThreshold) {
					MoveWalkModifier = MyBP->WalkSpeed / MyBP->GroundSpeed;
				} else {
					double moveMagD = (param_MoveMag);
					MoveWalkModifier = ((moveMagD / (double)playerController->MoveStickWalkThreshold) * (double)MyBP->WalkSpeed) / (double)MyBP->GroundSpeed;
				}
			}
		}
		if (MyBP->bIsCrouched) {
			MoveFacing = MyBP->Rotation;
		} else {
			MoveFacing = MoveRot2D;
		}
        GetDirectionalVectorFromRotator(MoveDir, MoveFacing);
	} else {
        // No Walk and End Walk
		MoveWalkModifier = 0.0f;
		if (playerController->Pawn->DesiredSpeed < 0.0500000007f) {
			MoveAccMag = 0.0f;
		}
		MoveFacing = MyBP->Rotation;
        GetDirectionalVectorFromRotator(MoveDir, MoveFacing);
	}
    // Result
	if (!playerController->IsMoveInputIgnored()) {
        // No input mode
		MyBP->SetDesiredSpeed(MoveWalkModifier, FALSE);
		MyBP->Acceleration = FVectorMulScalar(MoveDir, MoveAccMag);
	} else {
        // Input ok
		MyBP->SetDesiredSpeed(1.0f, FALSE);
		MyBP->Acceleration *= 0.0f;
	}
	MyBP->SetDesiredRotation(MoveFacing, FALSE);
    /*FVector pos = makeVectorFromVR_HMD_Position();
    static vpxfloat3 cam_rot = getCameraRotation(MyBP);
    static vpxfloat3 cam_pos = getCameraPosition(MyBP);

    // Add the headset rotation from vorpX to your camera rotation.
    vpxfloat3 rot_final;
    rot_final.x = hmdRot.pitch; // Headset only camera pitch is good practice
    rot_final.y = cam_rot.y + hmdRot.yaw;
    rot_final.z = cam_rot.z + hmdRot.roll;
    //camera->SetWorldRotation(rot_final.x, rot_final.y, rot_final.z);
    vpxfloat3 pos_vpx = vpxGetHeadsetPosition();
    // Yaw correction converts the position to your world coordinates.
    pos_vpx = vpxYawCorrection(pos_vpx, cam_rot.y);

    // vorpX provides positions in meters. If your unit scale is not 1U = 1m,
    // positions have to be scaled to your unit scale.
    const float scalemod = 1.0f;

    // Add the headset position from vorpX to your camera position.
    vpxfloat3 pos_final;
    pos_final.x = cam_pos.x + (pos_vpx.x * scalemod);
    pos_final.y = cam_pos.y + (pos_vpx.y * scalemod);
    pos_final.z = cam_pos.z + (pos_vpx.z * scalemod);
    FVector camMotion;
    camMotion.X = pos_final.x;
    camMotion.Y = pos_final.y;
    camMotion.Z = pos_final.z;
    MyBP->Move(camMotion);*/
}

typedef struct ABioBaseSquad_execProcessViewRotation_Params_struct {
    float       DeltaTime;
    FRotator    *out_DeltaRot;
    FRotator    *out_ViewRotation;
} ABioBaseSquad_execProcessViewRotation_Params;

ABioBaseSquad_execProcessViewRotation_Params processViewRotationParams;
  
const char* ccProcessViewRotation = "ProcessViewRotation";

Tick minute(60, true, true);
Tick halfMinute(30, true, true);

void processUpdateRotation(UObject* Context, FFrame* Stack, void* Result) {
    FRotator DeltaRot;
	FRotator ViewRotation;

    //ABioBasePlayerController* oCon = reinterpret_cast<ABioBasePlayerController*>(Context);
    APlayerController* oCon = reinterpret_cast<APlayerController*>(Context);
    FRotator resultRot;
    //processViewRotationParams.out_ViewRotation = &resultRot;
    //processViewRotationParams.out_DeltaRot->Yaw += 0.1f;
    //processViewRotationParams.DeltaTime = 0.01f;

    if (FindFunctionChecked_orig != NULL && ProcessEvent_orig != NULL && oCon != NULL) {        
        if (halfMinute.is()) {
            writeln("Calling dr. doolittle but crash");
            FName fName;
            //FName::TryFind("SFXPlayerController.UpdateViewRotation", 0, &fName);
            //unsigned int ia[2] = {0, 0};
            //fName = (FName*)sfxNameConstructor(ia, L"ProcessViewRotation", 0, false, false);
            //CALL_VIRTUAL_UFUNC(oCon, "ProcessViewRotation", updateRotationParams);
            //CALL_VIRTUAL_UFUNC(oCon, (DWORD)ccProcessViewRotation, updateRotationParams);
            //CALL_VIRTUAL_UFUNC(oCon, fName, updateRotationParams);

            if (const auto player = reinterpret_cast<ABioPlayerController*>(FindObjectOfType(ABioPlayerController::StaticClass()))) {
            
                WCHAR *wname = L"BioPawn.ProcessViewRotation";
                StaticVariables::CreateName(wname, 0, &fName);
                //player->CauseEvent(foundName);
                //CALL_VIRTUAL_UFUNC(player, fName, updateRotationParams);
                //CALL_VIRTUAL_UFUNC(player, fName, processViewRotationParams);
            }
        }
    }
	// = parms.ReturnValue;

    ABioPlayerController* playerController = (ABioPlayerController*)Context;

    /*FRotator rot = makeRotatorFromVRHMDOrientation();
    playerController->HMD_Orientation = rot;
    FVector pos = makeVectorFromVR_HMD_Position();
    playerController->HMD_Position = pos;
    float angle = leftRot.yaw * M_PI / 180.0;
    FVector vrLeftDir;
    vrLeftDir.X = cos(angle);
    vrLeftDir.Y = sin(angle);
    //playerController->VR_LeftControllerAsDir = vrLeftDir;*/

    /*APawn* aPawn = playerController->Pawn;
    ABioPawn* pawn = reinterpret_cast<ABioPawn*>(aPawn);
	float* pDeltaTime = reinterpret_cast<float*>(Stack->Locals);
    float DeltaTime = *pDeltaTime;

	ViewRotation = playerController->Rotation;
	DeltaRot.Yaw = int(playerController->PlayerInput->aTurn);
	DeltaRot.Pitch = int(playerController->PlayerInput->aLookUp);
    DeltaRot.Roll = 0;
    playerController->ProcessViewRotation(DeltaTime, ViewRotation, &DeltaRot);
    playerController->SetRotation(ViewRotation);
    playerController->ViewShake(DeltaTime);*/
}

void ProcessInternal_hook(UObject* Context, FFrame* Stack, void* Result) {

    if (startupDelay.is() && tenSeconds.is()) {
        if (!vrHelper->isValid()) {
            logger.writeToLog("vrHelper is NOT active, attempting to init...", true, true, true);
            if (vrHelper->Init()) {
                logger.writeToLog("vrHelper is valid", true, true, true);
            }
        }
    }

	bool processed = false;

    try {
        if (Stack != NULL) {
            UStruct* node = Stack->Node;
            if (node != NULL) {
                const auto szName = node->GetFullName(true);
                if (strstr(szName, "BioPlayerController.PlayerWalking.PlayerMoveExplore")) {
                    if (NULL != vrHelper && vrHelper->isValid()) {
                        // Main work
                        getVRVariables();
                        processWalkingExplore(Context, Stack, Result);				
                        processed = true;
                    }
                } else if (strstr(szName, "BioPlayerController.PlayerWalking.UpdateRotation")) {
                    if (NULL != vrHelper && vrHelper->isValid()) {
                        processUpdateRotation(Context, Stack, Result);
                        //processed = true;
                    }
                }
            }
        }
    } catch (...) {
        logger.writeToLog(string_format("%s \n", "Exception occurred inside ProcessInternal_hook"), true);
        logger.flush();
    }

	// YOUR CODE HERE
    if (!processed) {
        ProcessInternal_orig(Context, Stack, Result);
    } else {
        // Draw HUD
        hud->SetTopLeft(350, 150);
        hud->SetVRAndGameData(hmdRot, leftRot, rightRot, print_MoveMag, print_MoveAngle, print_MoveRot);
        static uint64_t oldTime = timeSinceEpochMillisec();
        uint64_t nowTime = timeSinceEpochMillisec();
        if (nowTime - oldTime > 1000) {
            oldTime = nowTime;
            hud->SetYaw(TheControllerYaw);
        }
    }
	// OR HERE TO MODIFY THE RETURN
}

unsigned ExecHandler_hook(UEngine* Context, wchar_t* cmd, void* unk) {
    if (ExecHandler_orig(Context, cmd, unk)) {
        return TRUE;
    }
	if (IsCmd(&cmd, L"appearance")) {
        hud->ShouldDraw = !hud->ShouldDraw;
        return TRUE;
	}
    return FALSE;
}

ABioPawn* GetPlayer(AWorldInfo* worldInfo) {
    if (!worldInfo) return nullptr;
    auto bioWorldInfo = (ABioWorldInfo*)worldInfo;
    auto pc = bioWorldInfo->LocalPlayerController;
    if (!pc) return nullptr;
    auto player = pc->Pawn;
    if (!player || !player->IsA(ABioPawn::StaticClass())) return nullptr;
    else return static_cast<ABioPawn*>(player);
}

ABioPlayerController* GetPlayerController(AWorldInfo* worldInfo) {
    if (!worldInfo) return nullptr;

    auto bioWorldInfo = (ABioWorldInfo*)worldInfo;
    auto lpc = (ABioPlayerController*)bioWorldInfo->LocalPlayerController;
    return lpc;
}

// ProcessEvent hook
// ======================================================================


void ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result) {
    //static MainHUD* hud = nullptr;
    char* fullName = Function->GetFullName();
    if (!strcmp(fullName, "Function SFXGame.BioHUD.PostRender")) {
        auto biohud = reinterpret_cast<ABioHUD*>(Context);
        if (biohud != nullptr) {
            hud->Update(((ABioHUD*)Context)->Canvas, GetPlayer(biohud->WorldInfo), GetPlayerController(biohud->WorldInfo));
            hud->Draw();
        }
    }
    
    ProcessEvent_orig(Context, Function, Parms, Result);
}

void PlayerMoveExploreHandler_hook(float DeltaTime, float MoveMag, float MoveAngle, FRotator MoveRot) {
    //PlayerMoveExploreHandler_orig(DeltaTime, MoveMag, MoveAngle, MoveRot);
}

SPI_IMPLEMENT_ATTACH
{
    Common::OpenConsole();

	hud = new MainHUD();

	scriptDebugLogger = new ScriptDebugLogger();
	scriptDebugLogger->Open(L"scriptDebugLogger.log");

	logger.writeToLog("FP2VR-ASI-Mod started...", true, true, true);

    //loadBin("c:/Steam/steamapps/common/Mass Effect Legendary Edition/Game/ME1/Binaries/Win64/ASI/PlayerMoveExploreModded2.bin", "rb");

    INIT_CHECK_SDK()
    
    INIT_FIND_PATTERN_POSTHOOK(ProcessInternal, PATTERN_PROCESS_INTERNAL_PH);

    if (auto rc = InterfacePtr->InstallHook(MYHOOK "CallFunction", ProcessInternal, ProcessInternal_hook, (void**)&ProcessInternal_orig);
        rc != SPIReturn::Success) {
        writeln(L"Attach - failed to hook ProcessEvent: %d / %s", rc, SPIReturnToString(rc));
        return false;
    } else {
        writeln(L"Attach - successfully hooked ProcessEvent: %d / %s", rc, SPIReturnToString(rc));
    }

    // HUD
	// ProcessEvent
	INIT_FIND_PATTERN_POSTHOOK(ProcessEvent, PATTERN_PROCESS_EVENT);
	if (auto rc = InterfacePtr->InstallHook(MYHOOK "ProcessEvent", ProcessEvent, ProcessEvent_hook, (void**)&ProcessEvent_orig);
		rc != SPIReturn::Success) {
		writeln(L"Attach - failed to hook ProcessEvent: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}

	if (const auto rc = InterfacePtr->FindPattern(reinterpret_cast<void**>(&ExecHandler), PATTERN_EXEC_EVENT);
		rc != SPIReturn::Success) {
		writeln(L"Attach - failed to find ExecHandler pattern: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}
	if (const auto rc = InterfacePtr->InstallHook(MYHOOK "ExecHandler", ExecHandler, ExecHandler_hook, reinterpret_cast<void**>(&ExecHandler_orig));
		rc != SPIReturn::Success) {
		writeln(L"Attach - failed to hook ExecHandler: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}

    // Gameplay

	INIT_FIND_PATTERN(FindFunctionChecked_orig, PATTERN_FIND_FUNCTION_CHECKED_PH);

    INIT_FIND_PATTERN(sfxNameConstructor, LE_PATTERN_POSTHOOK_SFXNAMECONSTRUCTOR);

    vrHelper = new VRHelper();

    tenSeconds.start();
    oneSecond.start();

    //processViewRotationParams.out_DeltaRot->Pitch = 0.0f;
    //processViewRotationParams.out_DeltaRot->Yaw = 0.0f;
    //processViewRotationParams.out_DeltaRot->Roll = 0.0f;
    //processViewRotationParams.DeltaTime = 0.0f;

    return true;
}

SPI_IMPLEMENT_DETACH
{
    if (NULL != vrHelper) {
        vrHelper->Shutdown();
        SAFE_DELETE(vrHelper); 
    }
    logger.flush();
    Common::CloseConsole();
	scriptDebugLogger->Close();
	delete scriptDebugLogger;
    return true;
}


#pragma warning(pop)