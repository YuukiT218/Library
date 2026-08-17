#include "EnemySword.h"
#include "Math/Mathf.h"
#include "Graphics/Graphics.h"
#include <imgui.h>


#include <stdlib.h>


#include "System/Audio/Audio.h"


EnemySword::EnemySword(ID3D11Device* device, const char* filename)
{
	//初期設定
	position = { 0.025f, 0.01f, 0.030f };
	angle = { -1.5f, -1.0f, -1.6f };
	scale = { 1.0f, 1.0f, 1.0f };
	model = std::make_unique<Model>(device, filename, 1.0f);
	model->SetAdMetalness(1.0f);
	model->SetAdRoughness(0.0f);
	colors.emissiveFactor = 4.0f;

	//当たり判定用初期設定
	SetupBladeHitSpheres(0.225f);

	//炎トレイル用ポイントライト
	attenuation = 1.1f;
	pointColor = { Mathf::Color255ToNormalized({240.0f,135.0f,15.0f,0.0f}) };

	//武器用トレイル初期設定
	tipBegin = { Mathf::Color255ToNormalized({255.f,9.f,16.f,200.0f}) };
	tipEnd = { Mathf::Color255ToNormalized({255.f,135.f,9.f,200.0f}) };
	rootBegin = { Mathf::Color255ToNormalized({255.f,16.f,16.f,200.0f}) };
	rootEnd = { Mathf::Color255ToNormalized({255.f,135.f,9.f,10.0f}) };
	dissolve = 0.5f;
	colorScale = 1.2f;
	LoadCommonResources();
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
	if (isAttack)TrailRender(rc);
}

void EnemySword::DrawDebugGUI()
{
	if (ImGui::CollapsingHeader("Weapon", ImGuiTreeNodeFlags_DefaultOpen))
	{
		model->DebugGUI(u8"武器");

		DrawCommonDebugGUI();

		ImGui::DragFloat("WeaponEmissiveFactor", &colors.emissiveFactor, 0.1f, 0.0f, 10.0f);
		ImGui::ColorEdit4("WeaponEmissiveColor", &colors.adjustColor.x);

		DrawDebugTrailGUI();
	}
}
