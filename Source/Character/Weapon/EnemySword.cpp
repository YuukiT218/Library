#include "EnemySword.h"
#include "Math/Mathf.h"
#include "Graphics/Graphics.h"
#include <imgui.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

EnemySword::EnemySword(ID3D11Device* device, const char* filename)
{
	//初期設定
	position = { 0.025f, 0.01f, 0.015f };
	angle = { -3.45f, 3.15f, 0.0f };
	scale = { 1.0f, 1.0f, 1.0f };
	model = std::make_unique<Model>(device, filename, 1.0f);
	model->SetAdMetalness(1.0f);
	model->SetAdRoughness(0.0f);
	colors.emissiveFactor = 4.0f;

	//当たり判定用初期設定
	hitSphereIndex = 5;
	weaponHitOffset[0] = { 0.0f, 0.0f, 0.0f };
	weaponHitOffset[1] = { 0.0f, 0.0f, 0.6f };
	weaponHitOffset[2] = { 0.0f, 0.0f, 0.85f };
	weaponHitOffset[3] = { 0.0f, 0.0f, 1.1f };
	weaponHitOffset[4] = { 0.0f, 0.0f, 1.35f };
	hitSphereRadius = 0.225f;

	//炎トレイル用ポイントライト
	attenuation = 1.1f;
	pointColor = { Mathf::Color255ToNormalized({240.0f,135.0f,15.0f,0.0f}) };

	//武器用トレイル初期設定
	TipBegin = { Mathf::Color255ToNormalized({243.f,201.f,104.f,200.0f}) };
	TipEnd = { Mathf::Color255ToNormalized({255.f,9.f,9.f,200.0f}) };
	RootBegin = { Mathf::Color255ToNormalized({255.f,9.f,9.f,200.0f}) };
	RootEnd = { Mathf::Color255ToNormalized({255.f,9.f,9.f,10.0f}) };
	dissolve = 0.5f;
	Colorscale = 1.2f;
	attackHitEffect = std::make_shared<Effect>("Data/Effect/HitEffect.efkefc");
	/*parryEffect = std::make_shared<Effect>("Data/Effect/Parry/ParrySpark.efkefc");*/
}

EnemySword::~EnemySword()
{
}

void EnemySword::Update(float elapsedTime)
{
	// トレイルを付けたいためトレイル用のアップデートを呼ぶ
	TrailUpdate(elapsedTime);

	// 武器モデル行列更新
	model->UpdateTransform(transform);

	// 武器エミッシブ更新
	model->SetEmissiveColors(colors);
}

void EnemySword::Render(const RenderContext& rc, ShaderId shaderId)
{
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	if (hasTeleportEffect)
	{
		modelRenderer->DrawWithTeleport(
			ShaderId::PBR,
			model,
			teleportProgress,
			teleportTime
		);
	}
	else
	{
		modelRenderer->Draw(ShaderId::PBR, model);
	}
	modelRenderer->Render(rc);
	if (IsAttack)TrailRender(rc);
}

void EnemySword::DrawDebugImGUi()
{
	if (ImGui::CollapsingHeader("Weapon", ImGuiTreeNodeFlags_DefaultOpen))
	{
		model->DebugGui(u8"武器");

		ImGui::DragFloat3("WeaponPosition", &position.x, 0.01f);
		ImGui::DragFloat3("WeaponAngle", &angle.x, 0.01f);
		ImGui::DragFloat3("WeaponScale", &scale.x, 0.01f);
		//ImGui::Checkbox(u8"WeaponIsEmissive", &colors.isEmissive);
		ImGui::DragFloat("WeaponEmissiveFactor", &colors.emissiveFactor, 0.1f, 0.0f, 10.0f);
		ImGui::ColorEdit4("WeaponEmissiveColor", &colors.adjustColor.x);

		for (int j = 0; j < hitSphereIndex; j++)
		{
			ImGui::PushID(j);
			ImGui::DragFloat3("WeaponHitSpherePos", &weaponHitOffset[j].x, 0.1f);
			ImGui::PopID();
		}
		ImGui::DragFloat("WeaponHitSphereRadius", &hitSphereRadius, 0.01f, 0.0f, 10.0f);

		DrawDebugTrailGui();
	}
}
