#include <string>
#include <ctime>
#include <iostream>

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

std::ofstream logFile;
char token[1024];
char txt[4096];
time_t startTime, now;


// ======================================================================
// ExecHandler hook
// ======================================================================

typedef unsigned (*tExecHandler)(UEngine* Context, wchar_t* cmd, void* unk);
tExecHandler ExecHandler = nullptr;
tExecHandler ExecHandler_orig = nullptr;

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
    
#ifdef DEBUG_LOG
    now = time(NULL);
    long long diff = now - startTime;
    if (diff > 10) {
        startTime = now;
        std::ifstream tokenFile;
        tokenFile.open("Token.txt", ios_base::out);
		if (tokenFile.is_open()) {
			tokenFile.seekg(0, ios_base::beg);
			tokenFile.read(token, sizeof(token));
			tokenFile.close();
        } else {
            sprintf(token, "");
        }
    }
    if (strlen(token) > 0) {
        if (strstr(Function->GetFullName(), token)) {
		//if (strstr(Function->GetFullName(), "BioPlayerController.PlayerWalking.PlayerMove")) {
            char* name = Function->GetName();
            char* nameCPP = Function->GetNameCPP();
            snprintf(txt, sizeof(txt), "Fullname %s, name %s, nameCPP %s", fullName, name, nameCPP);
            logFile << txt << endl;
        }
    }
#endif

    ProcessEvent_orig(Context, Function, Parms, Result);
}

void PlayerMoveExploreHandler_hook(float DeltaTime, float MoveMag, float MoveAngle, FRotator MoveRot) {

    PlayerMoveExploreHandler_orig(DeltaTime, MoveMag, MoveAngle, MoveRot);
}


SPI_IMPLEMENT_ATTACH
{
    Common::OpenConsole();

    if (!logFile.is_open()) {
		logFile.open("Main.log", ios_base::out);
        
		fs::permissions("Main.log",
			fs::perms::all,
			fs::perm_options::replace);
    }

    startTime = time(NULL);

    memset(token, 0, sizeof(token));

    INIT_CHECK_SDK()

/*
    // ProcessEvent
    //INIT_FIND_PATTERN_POSTHOOK(ProcessEvent, // 40 55 41 56 41 // "57 48 81 EC 90 00 00 00 48 8D 6C 24 20");
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
*/

    //BioPlayerController.PlayerWalking.PlayerMoveExplore
    INIT_FIND_PATTERN(PlayerMoveExploreHandler, "0F 00 5B 6E 00 00 2E DB 18 00 00 01 4E FB FF FF 07 2D 00 72 00 5B 6E 00 00 2A 16 04 0B 0F 35 96 FB FF FF EA FA FF FF 00 01 00 5D 6E 00 00 35 96");
	if (auto rc = InterfacePtr->InstallHook(MYHOOK "PlayerMoveExploreHandler", PlayerMoveExploreHandler, PlayerMoveExploreHandler_hook, (void**)&PlayerMoveExploreHandler_orig);
		rc != SPIReturn::Success) {
		writeln(L"Attach - failed to hook PlayerMoveExploreHandler: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}
    if (const auto rc = InterfacePtr->FindPattern(reinterpret_cast<void**>(&PlayerMoveExploreHandler),
        //"00 00 00 00 5A 6E 00 00 64 6E 00 00 6C 04 00 00 E8 02 00 00 0F 00 5B 6E 00 00 2E DB 18 00 00 01 4E FB FF FF 07 2D 00 72 00 5B 6E 00");
        "C9 42 00 00 00 00 00 00 00 00 00 00 5A 6E 00 00 64 6E 00 00 6C 04 00 00 E8 02 00 00 0F 00 5B 6E 00 00 2E DB 18 00 00 01 4E FB FF FF");
        //"0F 00 5B 6E 00 00 2E DB 18 00 00 01 4E FB FF FF 07 2D 00 72 00 5B 6E 00 00 2A 16 04 0B 0F 35 96 FB FF FF EA FA FF FF 00 01 00 5D 6E 00 00 35 96");
        rc != SPIReturn::Success) {
        writeln(L"Attach - failed to find PlayerMoveExploreHandler pattern: %d / %s", rc, SPIReturnToString(rc));
        return false;
	}
    if (const auto rc = InterfacePtr->InstallHook(MYHOOK "T_PlayerMoveExploreHandler", PlayerMoveExploreHandler, PlayerMoveExploreHandler_hook, reinterpret_cast<void**>(&PlayerMoveExploreHandler_orig));
		rc != SPIReturn::Success) {
		writeln(L"Attach - failed to hook PlayerMoveExploreHandler: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}

    return true;
}

SPI_IMPLEMENT_DETACH
{
    if (logFile.is_open()) {
        logFile.close();
    }

    Common::CloseConsole();
    return true;
}
