#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include <cstdio>

#define GAMELE1

#include "../../Shared-ASI/Interface.h"
#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"
#include "../../Shared-ASI/ConsoleCommandParsing.h"
#include "../UnrealscriptDebugger/ScriptDebugLogger.h"

#pragma warning(push, 2)

#include "main.h"
#include "MainHUD.h"

#define MYHOOK "FP4VR-ASI-Mod_"

namespace fs = std::filesystem;

SPI_PLUGINSIDE_SUPPORT(L"FP4VR-ASI-Mod", L"Sir Gamealot", L"1.0.0", SPI_GAME_LE1, 2);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

MainHUD* customHud = new MainHUD;

ME3TweaksASILogger logger("Function Logger v2", "LE1FunctionLog.log");
char token[1024];
char txt[4096];

VRHelper* vrHelper;

MainHUD* hud;

#define PATTERN_PROCESSINTERNAL_PH          /*"40 53 55 56 57 */ "48 81 EC 88 00 00 00 48 8B ?? ?? ?? ?? ?? 48 33 C4 48 89 44 24 70 48 8B 01 48 8B DA 48 8B 52 14"

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
} BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack;
#pragma pack(pop)

void logState(VR_Controller_State state) {
	logger.writeToConsole(string_format("State: Stick X = %f, Stick Y = %f, Trigger - %f", state.StickX, state.StickY, state.Trigger));
}

void logParams(BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack* params) {
    logger.writeToConsole(
        string_format("State: MoveAngle = %f, MoveMag = %f, MoveRot[Pitch: %d, Yaw: %d, Roll: %d]",
                        params->MoveAngle, params->MoveMag,
                        params->MoveRot.Pitch, params->MoveRot.Yaw, params->MoveRot.Roll)
    );
}

int TheYaw = 0;

void ProcessInternal_hook(UObject* Context, FFrame* Stack, void* Result) {

    if (startupDelay.is() && tenSeconds.is()) {
        if (!vrHelper->isValid()) {
            logger.writeToLog("vrHelper is NOT active, attempting to init...", true, true, true);
            if (vrHelper->Init()) {
                logger.writeToLog("vrHelper is valid", true, true, true);
            }
        }
    }

    if(!yes) {
        try {
            if (Stack != NULL) {
                UStruct* node = Stack->Node;
                if (node != NULL) {
                    const auto szName = node->GetFullName(true);
                    if (szName != NULL) {
                        if (strstr(szName, "BioPlayerController.PlayerWalking.PlayerMoveExplore")) {
                            //BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack* params =
							//	reinterpret_cast<BioPlayerController_PlayerWalking_PlayerMoveExplore_Stack*>(Stack->Locals);

                            //if (NULL != vrHelper && vrHelper->isValid()) {
							if (oneSecond.is()) {
								FILE* f = fopen("token.txt", "r");
								fseek(f, 0, SEEK_END);
								int size = ftell(f) + 1;
								fseek(f, 0, SEEK_SET);
								char c[64];
								memset(c, 0, 64);
								fread(c, 1, size, f);
								fclose(f);
								int OldTheYaw = TheYaw;
								try {
									//TheYaw = stoi(c);
								} catch (...) {
									TheYaw = OldTheYaw;
								}
								logger.writeToConsole(string_format("TheYaw = %d", TheYaw));
							}

									//float before = params->MoveRot.Yaw;
                                
									VROrientation hmdRot, leftRot;// , rightRot;
									//VR_Controller_State leftState;// , rightState;

									vrHelper->Get_HMD_Orientation(hmdRot);
									//vrHelper->Get_Controller_State(leftState, VR_CONTROLLER_LEFT);
									vrHelper->Get_Controller_Orientation(leftRot, VR_CONTROLLER_LEFT);
									////vrHelper->Get_Controller_State(rightState, VR_CONTROLLER_RIGHT);
									////vrHelper->Get_Controller_Orientation(rightRot, VR_CONTROLLER_RIGHT);

									////float* p = reinterpret_cast<float*>(Stack->Locals);
									////float* deltaTime = &p[0];
									////float* MoveMag = reinterpret_cast<float*>(Stack->Locals + 4);
									
									////int* RotYaw = reinterpret_cast<int*>(Stack->Locals + 16);
									FRotator* rot = reinterpret_cast<FRotator*>(Stack->Locals + 12);
									float yaw = leftRot.yaw;// *F_DEGREES_TO_GAME_ROTATION_UNITS;
									TheYaw = (int)yaw;
									rot->Yaw = (TheYaw < 0 ? (360 + TheYaw) : TheYaw);

									float* MoveAngle = reinterpret_cast<float*>(Stack->Locals + 8);
									*MoveAngle = yaw;

                                    //hud->SetMiddle(400, 400);
                                    hud->Draw();

									////params->MoveRot.Yaw = yaw;
									////params->MoveMag = -state.StickY;

									////logger.writeToConsole( string_format("leftController Rotation: Yaw=%f", leftRot.yaw) );


									////float mag = abs(leftState.StickY);// sqrt(pow(leftState.StickX, 2) + pow(leftState.StickY, 2)));
									////mag = clamp(mag, 0.f, 1.f);


									////*MoveAngle = -90;// yaw;
									////*MoveMag = mag;

									//
									////*MoveMag = 1.0f;// abs(sqrt(pow(leftState.StickX, 2) + pow(leftState.StickY, 2)));
									///*(*rot)->Pitch = 0;
									//(*rot)->Yaw = int(yaw);
									//(*rot)->Roll = 0;*/

									////logger.writeToConsole(string_format("Game stack changed: MoveMag = %f, Rot.Yaw = %f", MoveMag, (*rot)->Yaw));

									///*params->MoveRot.Pitch = 0;
									//params->MoveRot.Yaw = yaw;
									//params->MoveRot.Roll = 0;
									//params->MoveMag = abs( sqrt(pow(leftState.StickX, 2) + pow(rightState.StickY, 2)) );*/

									// 
									////params->MoveAngle = rightRot.yaw * F_DEGREES_TO_GAME_ROTATION_UNITS;

									////logger.writeToConsole(string_format("Stack: Rot[ Pitch=%d, Yaw=%d, Roll=%d ]", rot->Pitch, rot->Yaw, rot->Roll));

         //                           //logState(state);
         //                           //logParams(params);
         //                           //logger.writeToConsole(string_format("Controller orientation: x = %f, y = %f, z = %f", leftRot.pitch, leftRot.yaw, leftRot.roll));
         //                           //logger.writeToConsole(string_format("Yaw before: %f, after %f", before, yaw));
         //                           //logger.flush();
									////if (IsDebuggerPresent()) {
									//	//PrintPropertyValues(*scriptDebugLogger, Stack->Locals, Stack->Node, Stack->OutParms);
									//	//scriptDebugLogger->flush();
									////}
								//}

                                //logger.writeToConsole(string_format("Yaw before: %f, after %f", before, after));
                                //logger.writeToConsole(string_format("Stick = %f", state.StickY));                                
                            //}
                        }
                    }
                }
            }
        } catch (...) {
            logger.writeToLog(string_format("%s \n", "Exception occurred inside ProcessInternal_hook"), true);
            logger.flush();
        }
    } else {
        //Stack->Code = bin;
    }

	// YOUR CODE HERE
    {//if (!yes) {
        ProcessInternal_orig(Context, Stack, Result);
    }
	// OR HERE TO MODIFY THE RETURN
}

unsigned ExecHandler_hook(UEngine* Context, wchar_t* cmd, void* unk) {
    if (ExecHandler_orig(Context, cmd, unk)) {
        return TRUE;
    }
	if (IsCmd(&cmd, L"appearance")) {
        customHud->ShouldDraw = !customHud->ShouldDraw;
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
    static MainHUD* hud = nullptr;
    char* fullName = Function->GetFullName();
    if (!strcmp(fullName, "Function SFXGame.BioHUD.PostRender")) {
        auto biohud = reinterpret_cast<ABioHUD*>(Context);
        if (biohud != nullptr) {
            customHud->Update(((ABioHUD*)Context)->Canvas, GetPlayer(biohud->WorldInfo), GetPlayerController(biohud->WorldInfo));
            customHud->Draw();
        }
    }
    
    ProcessEvent_orig(Context, Function, Parms, Result);
}

void PlayerMoveExploreHandler_hook(float DeltaTime, float MoveMag, float MoveAngle, FRotator MoveRot) {
    PlayerMoveExploreHandler_orig(DeltaTime, MoveMag, MoveAngle, MoveRot);
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