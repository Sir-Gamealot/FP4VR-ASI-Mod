#pragma once

#include "CustomHUD.h"
#include "tick.h"

class MainHUD : public CustomHUD {
public:
	char c[2048];
	WCHAR txt[4096];
	Tick secondDelay, startupDelay, tick;
	VROrientation hmd, left, right;
	float MoveMag, MoveAngle;
	FRotator MoveRot;
	float yaw;

	MainHUD() : CustomHUD("MassEffect", 0, 0),
		secondDelay(1, false),
		startupDelay(30, true),
		tick(1, false)
	{
		secondDelay.start();
		startupDelay.start();
	}

	virtual ~MainHUD() {
	}

	void Update(UCanvas* canvas, ABioPawn* pawn, ABioPlayerController* playerController) {
		__super::Update(canvas);
		_pawn = pawn;
		_playerController = playerController;
	}

	void Draw() override {
		if (!ShouldDraw) return;

		SetTextScale();
		yIndex = 1;

		textScale = 1.25f;

		wchar_t output[512];

		RenderTextLine(L"FP4VR:", 255, 229, 0, 1.0f);
		RenderTextLine(L"-------------------------", 0, 0, 0, 0);
		
		RenderTextLine(L"Mod", 255, 229, 64, 1.0f);
		Indent();
		swprintf_s(output, 512, L"HMD: [X = %.0f, Y = %.0f, Z = %.0f]", hmd.pitch, hmd.yaw, hmd.roll);
		RenderTextLine(output, 0, 255, 0, 1);
		swprintf_s(output, 512, L"Left Controller: [X = %.0f, Y = %.0f, Z = %.0f]", left.pitch, left.yaw, left.roll);
		RenderTextLine(output, 0, 255, 0, 1);
		swprintf_s(output, 512, L"Right Controller: [X = %.0f, Y = %.0f, Z = %.0f]", right.pitch, right.yaw, right.roll);
		RenderTextLine(output, 0, 255, 0, 1);
		Unindent();

		RenderTextLine(L"Game", 255, 229, 64, 1.0f);
		Indent();
		swprintf_s(output, 512, L"MoveMag = %.3f, MoveAngle = %.3f", MoveMag, MoveAngle);
		RenderTextLine(output, 0, 255, 0, 1);
		swprintf_s(output, 512, L"MoveRot: [Pitch = %d, Yaw = %d, Roll = %d]", MoveRot.Pitch, MoveRot.Yaw, MoveRot.Roll);
		RenderTextLine(output, 0, 255, 0, 1);
		Unindent();

		RenderTextLine(L"Function", 255, 229, 64, 1.0f);
		Indent();
		swprintf_s(output, 512, L"Yaw = %.3f",yaw);
		RenderTextLine(output, 0, 255, 0, 1);
		Unindent();
	}

	void SetVRAndGameData(VROrientation& hmd, VROrientation& left, VROrientation& right, float& MoveMag, float& MoveAngle, FRotator& MoveRot)  {
		this->hmd = hmd;
		this->left = left;
		this->right = right;
		this->MoveMag = MoveMag;
		this->MoveAngle = MoveAngle;
		this->MoveRot = MoveRot;
	}

	void SetYaw(float yaw) {
		this->yaw = yaw;
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
