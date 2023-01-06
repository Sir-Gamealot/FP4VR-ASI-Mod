#pragma once

#include "CustomHUD.h"
#include "tick.h"

class MainHUD : public CustomHUD {
public:
	char c[2048];
	WCHAR txt[4096];
	Tick secondDelay, startupDelay, tick;

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

public:
	void SetMiddle(int rangeX, int rangeY) {
		if (NULL != _canvas) {
			cout << "canvas is " << _canvas << endl;
			topLeft.X = (_canvas->SizeX - rangeX) / 2;
			topLeft.Y = (_canvas->SizeY - rangeY) / 2;
			cout << "TopLeft X = " << topLeft.X << ", Y = " << topLeft.Y << endl;
		}
	}
};
