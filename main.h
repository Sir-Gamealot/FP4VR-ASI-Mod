#pragma once
#include <fstream>
#include <ostream>
#include <streambuf>
#include <sstream>
#include "../LE1-SDK/SdkHeaders.h"
#include "VRHelper.h"
#include "tick.h"
#define _USE_MATH_DEFINES
#include <math.h>

/// <summary>
/// Abstract class for creating custom HUDs similar to StreamingLevelsHUD
/// </summary>
class CustomHUD {
public:
	bool ShouldDraw = true;
	CustomHUD(LPSTR windowName = "MassEffect") {
		_windowName = windowName;
	}
	virtual void Update(UCanvas* canvas) {
		_canvas = canvas;
	}
	virtual void Draw() = 0;

protected:
	UCanvas* _canvas;
	float textScale = 1.25f;
	float lineHeight = 14.0f;
	int yIndex;
	LPSTR _windowName;

	void SetTextScale() {
		HWND activeWindow = FindWindowA(NULL, _windowName);
		if (activeWindow) {
			RECT rcOwner;
			if (GetWindowRect(activeWindow, &rcOwner)) {
				long width = rcOwner.right - rcOwner.left;
				long height = rcOwner.bottom - rcOwner.top;

				if (width > 2560 && height > 1440) {
					textScale = 4.0f;
				}
				else if (width > 1920 && height > 1080) {
					textScale = 3.5f;
				}
				else {
					textScale = 3.0f;
				}
				lineHeight = 14.0f * textScale;
			}
		}
	}

	void RenderText(std::wstring msg, const float x, const float y, const char r, const char g, const char b, const float alpha) {
		if (_canvas) {
			_canvas->SetDrawColor(r, g, b, (unsigned char)(alpha * 255));
			_canvas->SetPos(x, y + 64); //+ is Y start. To prevent overlay on top of the power bar thing
			_canvas->DrawTextW(FString{ const_cast<wchar_t*>(msg.c_str()) }, 1, textScale, textScale, nullptr);
		}
	}

	void RenderTextLine(std::wstring msg, const char r, const char g, const char b, const float alpha) {
		RenderText(msg, 5, lineHeight * yIndex, r, g, b, alpha);
		yIndex++;
	}

	void RenderName(const wchar_t* label, FName name) {
		wchar_t output[512];
		swprintf_s(output, 512, L"%s: %S", label, name.GetName());
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}

	void RenderYaw(const wchar_t* label, float yaw)	{
		wchar_t output[512];
		swprintf_s(output, 512, L"%s: %f", label, yaw);
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}

	void RenderString(const wchar_t* label, FString str) {
		wchar_t output[512];
		swprintf_s(output, 512, L"%s: %s", label, str.Data);
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}

	void RenderInt(const wchar_t* label, int value) {
		wchar_t output[512];
		swprintf_s(output, 512, L"%s: %d", label, value);
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}

	void RenderBool(const wchar_t* label, bool value) {
		wchar_t output[512];
		if (value) {
			swprintf_s(output, 512, L"%s: %s", label, L"True");
		} else{
			swprintf_s(output, 512, L"%s: %s", label, L"False");
		}
		RenderTextLine(output, 0, 255, 0, 1.0f);
	}
};

class AppearanceHUD : public CustomHUD
{
public:
	VRHelper *vrHelper;
	char c[2048];
	WCHAR txt[4096];
	Tick secondDelay, startupDelay, tick;

	AppearanceHUD() : CustomHUD("MassEffect") {		
		secondDelay.start();
		startupDelay.start();
		vrHelper = new VRHelper();
	}

	virtual ~AppearanceHUD() {
		if (vrHelper != NULL) {
			vrHelper->Shutdown();
			vrHelper = NULL;
		}
	}

	void Update(UCanvas* canvas, ABioPawn* pawn, ABioPlayerController* playerController) {
		__super::Update(canvas);
		_pawn = pawn;
		_playerController = playerController;

		if (!startupDelay.is(10, true)) {
			vrHelper->log("Before starting delay of 10 seconds is over.");
			return;
		}

		// Preinit
		if (!secondDelay.is(1, false))
			return;

		if (!vrHelper->isValid()) {
			vrHelper->log("vrHelper is NOT active");
			if (vrHelper->Init()) {
				vrHelper->log("vrHelper is valid");
				tick.start();
			} else
				return;
		}

		VROrientation orientation;
		//vrHelper->Get_HMD_Orientation(orientation);
		vrHelper->Get_Controller_Orientation(orientation, VPX_LEFT);
		sprintf(c, "Left Yaw = %f", orientation.Yaw);
		vrHelper->log(c);
		/*vrHelper->Get_Controller_Orientation(orientation, VPX_RIGHT);
		sprintf(c, "Right Yaw = %f", orientation.Yaw);
		vrHelper->log(c);*/

		//playerController->HandleWalking

		//_playerController->Rotation.Pitch = (int)(204.8 * orientation.Pitch);
		//_playerController->Rotation.Pitch = (int)(204.8 * orientation.Pitch);
		//_playerController->Rotation.Yaw = (int)(204.8f * orientation.Yaw);
		//_playerController->Rotation.Roll = (int)(204.8 * orientation.Roll);

		// Postinit		
		//vrHelper->Update();
		//int yaw = (int)(32768.0f + vrHelper.headsetYaw * 32768.f);
		//if (vrHelper.isValid()) {
		//_playerController->Rotation.Yaw = yaw;
		//}

		FVector fv;
		//fv.X = (int)(204.8f * cos(orientation.Yaw * M_PI / 180.0f));
		//fv.Y = (int)(204.8f * sin(orientation.Yaw * M_PI / 180.0f));
		//fv.Z = 100;

		double angleRad = orientation.Yaw * M_PI / 180.0f;
		fv.X = 10.0 * cos(angleRad);
		fv.Y = 0;
		fv.Z = 10.0 * sin(angleRad);

		if (tick.is(1, false)) {
			sprintf(c, "Yaw = %f", orientation.Yaw);
			vrHelper->log(c);
		}		
	}

	void Draw() override {
		if (!ShouldDraw) return;

		SetTextScale();
		yIndex = 3;

		// -----------------------------
		textScale = 1.5f;
		if (NULL != vrHelper && NULL != vrHelper->message) {
			wsprintf(txt, L"Message: %S", vrHelper->message);
			RenderTextLine(txt, 255, 0, 0, 1);
			RenderTextLine(L"\n", 255, 0, 0, 1);
			RenderTextLine(L"\n", 255, 0, 0, 1);
		}
		if (NULL != vrHelper && NULL != vrHelper->lastErrorMessage) {
			wsprintf(txt, L"Last error: %S", vrHelper->lastErrorMessage);
			RenderTextLine(txt, 255, 0, 0, 1);
			RenderTextLine(L"\n", 255, 0, 0, 1);
			RenderTextLine(L"\n", 255, 0, 0, 1);
		}
		//-------------------------------

		textScale = 1.0f;

		RenderTextLine(L"Appearance Profile", 255, 229, 0, 1.0f);
		if (_pawn) {
			/*if (vrHelper.isValid()) {
				float* rez = vrHelper.GetOrientation(vrHelper.GetHMDInd());
				if (rez != NULL) {
					yaw = (int)((1.0f + rez[1]) * 32768.0);
				}
			}*/
			//_playerController->Rotation.Yaw = rand() * 65536;
			//RenderYaw(L"_pawn->Rotation.Yaw = ", (float) _playerController->Rotation.Yaw);

			RenderName(L"Pawn Name", _pawn->Name);
			auto aType = _pawn->m_oBehavior->m_oActorType;
			if (aType) {
				RenderString(L"Actor Game Name", aType->ActorGameName);
			}
			yIndex++;
			RenderMeshSection();
			if (aType && aType->IsA(UBioPawnChallengeScaledType::StaticClass())) {
				yIndex++;
				RenderAppearanceSection(static_cast<UBioPawnChallengeScaledType*>(aType), _pawn->m_oBehavior->m_oAppearanceType);
			}
		} else {
			RenderTextLine(L"In Mako", 0, 255, 0, 1.0f);
		}
	}
private:
	ABioPawn* _pawn;
	APlayerController* _playerController;

	void RenderMeshSection() {
		RenderTextLine(L"Meshes:", 255, 229, 0, 1.0f);
		if (_pawn->m_oHairMesh) {
			RenderName(L"Hair Mesh", _pawn->m_oHairMesh->SkeletalMesh->Name);
		}
		if (_pawn->m_oHeadMesh) {
			RenderName(L"Head Mesh", _pawn->m_oHeadMesh->SkeletalMesh->Name);
		}
		if (_pawn->m_oHeadGearMesh) {
			RenderName(L"Head Gear Mesh", _pawn->m_oHeadGearMesh->SkeletalMesh->Name);
		}
		if (_pawn->m_oFacePlateMesh) {
			RenderName(L"Face Plate Mesh", _pawn->m_oFacePlateMesh->SkeletalMesh->Name);
		}
		if (_pawn->m_oVisorMesh) {
			RenderName(L"Visor Mesh", _pawn->m_oVisorMesh->SkeletalMesh->Name);
		}
	}

	void RenderAppearanceSection(UBioPawnChallengeScaledType* type, UBioInterface_Appearance* interfaceAppr) {
		auto appearance = type->m_oAppearance;
		auto body = appearance->Body;
		auto headGear = body->m_oHeadGearAppearance;
		auto headGearSettings = headGear->m_oSettings;

		auto iApprPawn = reinterpret_cast<UBioInterface_Appearance_Pawn*>(interfaceAppr);

		RenderTextLine(L"Bio_Appr_Character:", 255, 229, 0, 1.0f);
		RenderName(L"Headgear Prefix:", headGear->m_nmPrefix);
		if (iApprPawn) {
			auto armorSpecIdx = iApprPawn->m_oSettings->m_oBodySettings->m_eArmorType;
			RenderInt(L"Armor Spec Index", armorSpecIdx);
			auto armorSpec = headGear->m_aArmorSpec[armorSpecIdx];
			if (iApprPawn->m_oSettings->m_oBodySettings->m_oHeadGearSettings) {
				auto hgrVslOvr = iApprPawn->m_oSettings->m_oBodySettings->m_oHeadGearSettings->m_eVisualOverride;
				RenderInt(L"Headgear Visual Override Armor Spec", hgrVslOvr);
			}
			// Need good way of determining this index
			auto modelSpecIdx = 2;
			if (modelSpecIdx < armorSpec.m_aModelSpec.Count) {
				auto modelSpec = armorSpec.m_aModelSpec.Data[modelSpecIdx];
				RenderInt(L"Hgr ModelSpec Index (currently hardcoded)", modelSpecIdx);
				RenderInt(L"Hgr ModelSpec MaterialConfigCount", modelSpec.m_nMaterialConfigCount);
				RenderBool(L"Hgr ModelSpec bIsHairHidden", modelSpec.m_bIsHairHidden);
				RenderBool(L"Hgr ModelSpec bIsHeadHidden", modelSpec.m_bIsHeadHidden);
				RenderBool(L"Hgr ModelSpec bSuppressFacePlate", modelSpec.m_bSuppressFacePlate);
				RenderBool(L"Hgr ModelSpec bSuppressVisor", modelSpec.m_bSuppressVisor);
			}
		}
	}
};

