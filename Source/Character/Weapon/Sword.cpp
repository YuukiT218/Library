#include "Sword.h"
#include "Math/Mathf.h"
#include "Graphics/Graphics.h"
#include <imgui.h>

Sword::Sword(ID3D11Device* device, const char* filename)
{
	//struct Object
	//{
	//	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	//	DirectX::XMFLOAT3 angle = { 0, 0, 0 };
	//	DirectX::XMFLOAT3 scale = { 1, 1, 1 };
	//	DirectX::XMFLOAT4X4 transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	//	std::unique_ptr<Model> model;
	//	DirectX::XMFLOAT3 weaponHitOffset[5];
	//	DirectX::XMFLOAT3 weaponHitPosition[5];
	//	Model::EmissiveColors colors;

	//	static const int MAX_POLYGON = 16 * 3;
	//	DirectX::XMFLOAT3					trailPositions[2][MAX_POLYGON];
	//	DirectX::XMFLOAT3					trailoffset[2] =
	//	{
	//		{0.0f,0,2.0f},
	//
	//		{6.0f,0,0},
	//	};

	//	DirectX::XMFLOAT4 Begin = {};
	//	DirectX::XMFLOAT4 End = {};

	//	float Colorscale{};

	//	float dissolve{};

	//	bool IsAttack{};

	//};
	////    //剣の原点から根本と先端までのオフセット値
	// //    DirectX::XMVECTOR RootOffset = DirectX::XMVectorSet(0, 0, 0.3f, 0);
	// //    DirectX::XMVECTOR TipOffset = DirectX::XMVectorSet(0, 0, 2.3f, 0);

	//int hitSphereIndex = 5;
	//float hitSphereRadius = 0.1f;

	//Object weapon;

	//float nodeRadius[6] =
	//{
	//	0.23f,
	//	0.23f,
	//	0.43f,
	//	0.34f,
	//	0.31f,
	//	0.31f,
	//};

	//std::vector<NodeHitSphere> nodeHitSpheres =
	//{
	//	{"J_Bip_L_Foot", nodeRadius[0]},
	//	{"J_Bip_R_Foot", nodeRadius[1]},
	//	{"J_Bip_C_Hips", nodeRadius[2]},
	//	{"J_Bip_C_Head", nodeRadius[3]},
	//	{"J_Bip_L_LowerArm", nodeRadius[4]},
	//	{"J_Bip_R_LowerArm", nodeRadius[5]},
	//};

#if 0	// 猫キャラクター右手剣
	//初期設定
	position = { -9.2f, 6.4f, 24.5f };
	angle = { -0.8f, 4.0f, 0.3f };
	scale = { 8.0f, 8.0f, 8.0f };
	model = std::make_unique<Model>(device, filename);
	model->SetAdMetalness(1.0f);
	model->SetAdRoughness(0.0f);
	colors.emissiveFactor = 4.0f;

	//当たり判定用初期設定
	hitSphereIndex = 5;
	weaponHitOffset[0] = { -1.1f, 0.0f, 0.0f };
	weaponHitOffset[1] = { 0.6f, 0.0f, 0.0f };
	weaponHitOffset[2] = { 2.3f, 0.0f, 0.0f };
	weaponHitOffset[3] = { 3.9f, 0.0f, 0.0f };
	weaponHitOffset[4] = { 5.4f, 0.0f, 0.0f };
	hitSphereRadius = 0.1f;
#else	// 騎士キャラクター右手剣
	//初期設定
	position = { -0.075f, 0.0f, 0.025f };
	angle = { -3.0f, -0.2f, 0.0f };
	scale = { 1.0f, 1.0f, 1.0f };
	model = std::make_unique<Model>(device, filename, 1.0f);
	/*model->SetAdMetalness(1.0f);
	model->SetAdRoughness(0.0f);
	colors.emissiveFactor = 4.0f;*/

	//当たり判定用初期設定
	hitSphereIndex = 5;
	weaponHitOffset[0] = { 0.0f, 0.0f, 0.0f };
	weaponHitOffset[1] = { 0.0f, 0.0f, 0.2f };
	weaponHitOffset[2] = { 0.0f, 0.0f, 0.4f };
	weaponHitOffset[3] = { 0.0f, 0.0f, 0.6f };
	weaponHitOffset[4] = { 0.0f, 0.0f, 1.0f };
	hitSphereRadius = 0.15f;
#endif

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
	/*attackHitEffect = std::make_shared<Effect>("Data/Effect/akaslash/akaslash_middle.efk");
	parryEffect = std::make_shared<Effect>("Data/Effect/Parry/ParrySpark.efkefc");*/
}

Sword::~Sword()
{
}

void Sword::Update(float elapsedTime)
{
	//トレイルを付けたいためトレイル用のアップデートを呼ぶ
	TrailUpdate(elapsedTime);

	//武器モデル行列更新
	model->UpdateTransform(transform);

	//武器エミッシブ更新
	//model->SetEmissiveColors(colors);
}

void Sword::Render(const RenderContext& rc, Shader* shader)
{
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	modelRenderer->Draw(ShaderId::Lambert, model);
	modelRenderer->Render(rc);
	//if (IsAttack)TrailRender(rc);
}

void Sword::DrawDebugImGUi()
{
	if (ImGui::CollapsingHeader("Weapon", ImGuiTreeNodeFlags_DefaultOpen))
	{
		model->DebugGui(u8"武器");

		ImGui::DragFloat3("WeaponPosition", &position.x, 0.1f);
		ImGui::DragFloat3("WeaponAngle", &angle.x, 0.1f);
		ImGui::DragFloat3("WeaponScale", &scale.x, 0.1f);
		//ImGui::Checkbox(u8"WeaponIsEmissive", &colors.isEmissive);
		/*ImGui::DragFloat("WeaponEmissiveFactor", &colors.emissiveFactor, 0.1f, 0.0f, 10.0f);
		ImGui::ColorEdit4("WeaponEmissiveColor", &colors.adjustColor.x);*/

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
