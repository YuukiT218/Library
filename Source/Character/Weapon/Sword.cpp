#include "Sword.h"
#include "Math/Mathf.h"
#include "Graphics/Graphics.h"
#include <imgui.h>


#include <stdlib.h>


#include "System/Audio/Audio.h"


Sword::Sword(ID3D11Device* device, const char* filename)
{
	//初期設定
	position = { -0.075f, 0.0f, 0.025f };
	angle = { -3.0f, -0.2f, 0.0f };
	scale = { 1.0f, 1.0f, 1.0f };
	model = std::make_unique<Model>(device, filename, 1.0f);
	model->SetAdMetalness(1.0f);
	model->SetAdRoughness(0.0f);
	
	//当たり判定用初期設定
	SetupBladeHitSpheres(0.25f);

	//炎トレイル用ポイントライト
	attenuation = 1.1f;
	pointColor = { Mathf::Color255ToNormalized({240.0f,135.0f,15.0f,0.0f}) };

	//武器用トレイル初期設定
	tipBegin = { Mathf::Color255ToNormalized({255.f,250.f,80.f,200.0f}) };
	tipEnd = { Mathf::Color255ToNormalized({255.f,255.f,80.f,10.0f}) };
	rootBegin = { Mathf::Color255ToNormalized({255.f,250.f,80.f,200.0f}) };
	rootEnd = { Mathf::Color255ToNormalized({255.f,255.f,80.f,10.0f}) };
	dissolve = 0.5f;
	colorScale = 1.2f;
	LoadCommonResources();
}

void Sword::Update(float elapsedTime)
{
	//トレイルを付けたいためトレイル用のアップデートを呼ぶ
	TrailUpdate(elapsedTime);

	//武器モデル行列更新
	model->UpdateTransform(transform);
}

void Sword::Render(const RenderContext& rc, ShaderId shaderId)
{
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	modelRenderer->Draw(shaderId, model);
	modelRenderer->Render(rc);
	if (isAttack) 
		TrailRender(rc);
}

void Sword::DrawDebugGUI()
{
	if (ImGui::CollapsingHeader("Weapon", ImGuiTreeNodeFlags_DefaultOpen))
	{
		model->DebugGUI(u8"武器");

		DrawCommonDebugGUI();

		DrawDebugTrailGUI();
	}
}
