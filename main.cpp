#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <math.h>

#define GAMELE1

#include "../../Shared-ASI/Interface.h"
#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"
#include "../../Shared-ASI/ConsoleCommandParsing.h"
#include "../UnrealscriptDebugger/ScriptDebugLogger.h"

#pragma warning(push, 2)

#include "main.h"
#include "MainHUD.h"


#define _USE_MATH_DEFINES

#define MYHOOK "FP4VR-ASI-Mod_"

#define PATTERN_PROCESSINTERNAL_PH          /*"40 53 55 56 57 */ "48 81 EC 88 00 00 00 48 8B ?? ?? ?? ?? ?? 48 33 C4 48 89 44 24 70 48 8B 01 48 8B DA 48 8B 52 14"

namespace fs = std::filesystem;

SPI_PLUGINSIDE_SUPPORT(L"FP4VR-ASI-Mod", L"Sir Gamealot", L"1.0.0", SPI_GAME_LE1, 2);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

MainHUD* hud = new MainHUD;

ME3TweaksASILogger logger("Function Logger v2", "LE1FunctionLog.log");
char token[256];
char txt[4096];

VRHelper* vrHelper;

// VR variables
VROrientation hmdRot, leftRot, rightRot;
VR_Controller_State leftState, rightState;
FVector hmd, left, right;

// Game variables
//FRotator *MoveRot, *Facing;
//float *MoveAngle, *MoveMag;
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

BYTE* bin;
static bool yes = false;

#define F_DEGREES_TO_GAME_ROTATION_UNITS 182.0444444f // Factor is from 32768.f / 180.f

Tick startupDelay(30, true), tenSeconds(10, false), oneSecond(1, false);

ScriptDebugLogger* scriptDebugLogger;

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
	//vrHelper->Get_Controller_State(leftState, VR_CONTROLLER_LEFT);
	vrHelper->Get_Controller_Orientation(leftRot, VR_CONTROLLER_LEFT);
	//vrHelper->Get_Controller_State(rightState, VR_CONTROLLER_RIGHT);
	vrHelper->Get_Controller_Orientation(rightRot, VR_CONTROLLER_RIGHT);
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

float yaw = 0.0f;
float TheHeadYaw = 0.0f;
float TheControllerYaw = 0.0f;

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
}

void processWalkingExplore(UObject* Context, FFrame* Stack, void* Result) {
	// function PlayerMoveExplore(float DeltaTime, float MoveMag, float MoveAngle, Rotator MoveRot, Rotator t_MoveVec, Rotator t_FacingRot, Vector t_AccelVec)

 // Params
	BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack* params =
		reinterpret_cast<BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack*>(Stack->Locals);
	float param_MoveMag = *(&params->MoveMag);
	float param_MoveAngle = *(&params->MoveAngle);
	FRotator param_MoveRot = *(&params->MoveRot);

	// Function Variables
	ABioPlayerController* playerController = (ABioPlayerController*)Context;
	FVector MoveDir;
	FRotator MoveFacing;
	float MoveWalkModifier;
	FRotator MoveRot2D;
	float MoveAccMag;
	ABioPawn* MyBP;

	MyBP = reinterpret_cast<ABioPawn*>(playerController->Pawn);
	if (MyBP == NULL) {
		return;
	}
	MoveRot2D.Yaw =  param_MoveRot.Yaw;
	MoveRot2D.Pitch = 0;
	MoveRot2D.Roll = 0;
	MoveAccMag = MyBP->AccelRate;
	MoveWalkModifier = 1.0f;
	if (param_MoveMag > playerController->MoveStickIdleThreshold) {
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
		MoveWalkModifier = 0.0f;
		if (playerController->Pawn->DesiredSpeed < 0.0500000007f) {
			MoveAccMag = 0.0f;
		}
		MoveFacing = MyBP->Rotation;
        GetDirectionalVectorFromRotator(MoveDir, MoveFacing);
	}
	if (!playerController->IsMoveInputIgnored()) {
		MyBP->SetDesiredSpeed(MoveWalkModifier, FALSE);
		MyBP->Acceleration = FVectorMulScalar(MoveDir, MoveAccMag);
	} else {
		MyBP->SetDesiredSpeed(1.0f, FALSE);
		MyBP->Acceleration *= 0.0f;
	}
	MyBP->SetDesiredRotation(MoveFacing, FALSE);
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
                if (szName != NULL) {
                    if (strstr(szName, "BioPlayerController.PlayerWalking.PlayerMoveExplore")) {
                        //if (NULL != vrHelper && vrHelper->isValid()) {

                            // Token file holds number for direct constant input.
                            int TheYaw = 0;
                            if (oneSecond.is()) {
                                //getToken();
                            }

                            getVRVariables();

                            //getParams(Context, Stack, Result);

                            //processWalkingExplore(Context, Stack, Result);
							processWalkingExplore(Context, Stack, Result);

                            // Draw HUD
                            hud->SetTopLeft(350, 150);
                            //hud->SetVRAndGameData(hmdRot, leftRot, rightRot, param_MoveMag, MoveAngle, MoveRot);
                            static uint64_t oldTime = timeSinceEpochMillisec();
                            uint64_t nowTime = timeSinceEpochMillisec();
                            if (nowTime - oldTime > 500) {
                                oldTime = nowTime;
                                hud->SetYaw(TheControllerYaw);
                            }
                            processed = true;
                        //}
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

typedef void (*tProcessEvent)(UObject* Context, UFunction* Function, void* Parms, void* Result);
tProcessEvent ProcessEvent = nullptr;
tProcessEvent ProcessEvent_orig = nullptr;

typedef void (*tPlayerMoveExploreHandler)(float DeltaTime, float MoveMag, float MoveAngle, FRotator MoveRot);
tPlayerMoveExploreHandler PlayerMoveExploreHandler = nullptr;
tPlayerMoveExploreHandler PlayerMoveExploreHandler_orig = nullptr;

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
    INIT_FIND_PATTERN_POSTHOOK(ProcessInternal, PATTERN_PROCESSINTERNAL_PH);

    if (auto rc = InterfacePtr->InstallHook(MYHOOK "CallFunction", ProcessInternal, ProcessInternal_hook, (void**)&ProcessInternal_orig);
        rc != SPIReturn::Success) {
        writeln(L"Attach - failed to hook ProcessEvent: %d / %s", rc, SPIReturnToString(rc));
        return false;
    } else {
        writeln(L"Attach - successfully hooked ProcessEvent: %d / %s", rc, SPIReturnToString(rc));
    }

    // HUD
	// ProcessEvent
	INIT_FIND_PATTERN_POSTHOOK(ProcessEvent, /*40 55 41 56 41*/ "57 48 81 EC 90 00 00 00 48 8D 6C 24 20");
	if (auto rc = InterfacePtr->InstallHook(MYHOOK "ProcessEvent", ProcessEvent, ProcessEvent_hook, (void**)&ProcessEvent_orig);
		rc != SPIReturn::Success) {
		writeln(L"Attach - failed to hook ProcessEvent: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}

	if (const auto rc = InterfacePtr->FindPattern(reinterpret_cast<void**>(&ExecHandler), "48 8b c4 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8d a8 d8 fe ff ff");
		rc != SPIReturn::Success) {
		writeln(L"Attach - failed to find ExecHandler pattern: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}
	if (const auto rc = InterfacePtr->InstallHook(MYHOOK "ExecHandler", ExecHandler, ExecHandler_hook, reinterpret_cast<void**>(&ExecHandler_orig));
		rc != SPIReturn::Success) {
		writeln(L"Attach - failed to hook ExecHandler: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}

    vrHelper = new VRHelper();

    tenSeconds.start();
    oneSecond.start();

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