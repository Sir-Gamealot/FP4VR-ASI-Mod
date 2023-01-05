#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include <cstdio>

#define GAMELE1

#include "main.h"
#include "../../Shared-ASI/Interface.h"
#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"
#include "../../Shared-ASI/ConsoleCommandParsing.h"

#define MYHOOK "FP4VR-ASI-Mod_"

namespace fs = std::filesystem;

SPI_PLUGINSIDE_SUPPORT(L"FP4VR-ASI-Mod", L"Sir Gamealot", L"1.0.0", SPI_GAME_LE1, 2);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

AppearanceHUD* customHud = new AppearanceHUD;

ME3TweaksASILogger logger("Function Logger v2", "LE1FunctionLog.log");
char token[1024];
char txt[4096];

VRHelper* vrHelper;

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

#define F_DEGREES_TO_GAME_ROTATION_UNITS 204.8f // Factor is from 32768.f / 180.f

Tick tenSeconds;

void ProcessInternal_hook(UObject* Context, FFrame* Stack, void* Result) {

    if (tenSeconds.is(10, false)) {
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
                        if (
                            strstr(szName, "BioPlayerController.PlayerWalking.PlayerMoveExplore")                            
                            )
                        {
                            //logger.writeToLog(string_format("Found: %s", szName), true, true);
                            //yes = true;
                            //logger.writeToLog(string_format("Data %d, Count %d, Max %d", node->Script.Data, node->Script.Count, node->Script.Max), true, true);
                            //node->Script.Data = bin;
                            //node->Script.
                            //ProcessInternal(Context, Stack, Result);
							//float* p = reinterpret_cast<float*>(Stack->Locals + 12 + 0);
                            FRotator* rot = reinterpret_cast<FRotator*>(Stack->Locals + 12);
                            if (NULL != vrHelper && vrHelper->isValid()) {
                                float before = rot->Yaw;
                                VROrientation orr;
                                vrHelper->Get_Controller_Orientation(orr, VPX_LEFT);
                                float after = 32768.0f + orr.yaw * F_DEGREES_TO_GAME_ROTATION_UNITS;
                                rot->Yaw = int(after);
                                //logger.writeToConsole(string_format("Yaw before: %f, after %f", before, after));
                            }
							//logger.writeToConsole(string_format("Value = %3.3f", p[0].Yaw));
                            //logger.writeToConsole(string_format("Player Yaw Value = %d", rot->Yaw));
                            //logger.flush();
                            //logger.writeToConsole(string_format("VorpX is %s", (vrHelper->isValid() ? "valid" : "not valid") ));                            
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
    static AppearanceHUD* hud = nullptr;
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

    vrHelper = new VRHelper();

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
    return true;
}

/**
  * Loads binary form of Unreal Script from file, as exported by ME3Tweaks' Package Editor.
  *     This needs to be unpacked by Unreal script loader.
  * 
  *     Param path: Absolute or Relative to the game's exe.
  * 
  *     Returns true if the addr points to a valid byte array of size length.
  */
bool loadBin(const char* path, BYTE* addr, long& size) {
	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		writeln("Load of modded bins failed.");
		return false;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f) + 1;
	bin = (BYTE*)malloc(size);
	fread(bin, sizeof(char), size, f);
	fclose(f);
    return true;
}