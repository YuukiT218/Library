#include <imgui.h>
#include <algorithm>

#include "SceneLoading.h"
#include "SceneManager.h"
#include "Graphics/Graphics.h"
#include "Math/Mathf.h"
#include "Input/Input.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

//// t: 0?1の値
//float EaseInOutQuad(float t)
//{
//	return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
//}
//
//float EaseOutBack(float t, float s = 1.70158f)
//{
//	t = t - 1.0f;
//	return (t * t * ((s + 1.0f) * t + s) + 1.0f);
//}
//
//float EaseInBack(float t, float s = 1.70158f)
//{
//	return t * t * ((s + 1.0f) * t - s);
//}

// 初期化
void SceneLoading::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	//背景用
	AddSprite("Back", CreateSpriteData(device, "Data/Sprite/Title_Back.png", { 0,0,0 }, { 1280, 720 }, { 0,0 }, { 1920,1080 }, 0, { 1,1,1,1 }));

	//操作説明
	AddSprite("PadInst", CreateSpriteData(device, "Data/Sprite/PadInstruction.png", { 145.0f,100.0f,0.01f }, { 1137.8f, 640.0f }, { 0,0 }, { 1166, 591 }, 0, { 1,1,1,0 }));
	AddSprite("KeyMouInst", CreateSpriteData(device, "Data/Sprite/KeyMouInstruction.png", { 145.0f,100,0 }, { 1137.8f, 640.0f }, { 0,0 }, { 1052, 561 }, 0, { 1,1,1,0 }));

	AddSprite("LoadingIcon", CreateSpriteData(device, "Data/Sprite/LoadingIcon.png", { 1000.0f,-245.0f,0.1f }, { 512.0f, 512.0f }, { 0,0 }, { 512,512 }, 0, { 1,1,1,1 }));
	AddSprite("LoadingIconB", CreateSpriteData(device, "Data/Sprite/LoadingIconBack.png", { 1000.0f,-245.0f,0 }, { 512.0f, 512.0f }, { 0,0 }, { 512,512 }, 0, { 1,1,1,1 }));

	bool isController = true;

	// スレッド開始
	thread = new std::thread(LoadingThread, this);
}

// 終了化
void SceneLoading::Finalize()
{
	// スレッド終了化
	if (thread != nullptr && thread->joinable())
	{
		thread->join();
		delete thread;
		thread = nullptr;
	}
}

// 更新処理
void SceneLoading::Update(float elapsedTime)
{
	isController = Input::Instance().GetIsLastGamePad();
	float fadeSpeed = 6.0f * elapsedTime;

	// 画面サイズ取得
	float screenWidth = static_cast<float>(Graphics::Instance().GetScreenWidth());
	float screenHeight = static_cast<float>(Graphics::Instance().GetScreenHeight());

	// 背景を画面サイズに合わせる
	sprite["Back"].size = { screenWidth, screenHeight };

	// アイコンを背景の右上基準で配置
	const float iconOffsetX = -250.0f;
	const float iconOffsetY = -250.0f;

	sprite["LoadingIcon"].position.x = screenWidth + iconOffsetX;
	sprite["LoadingIcon"].position.y = iconOffsetY;
	sprite["LoadingIconB"].position.x = screenWidth + iconOffsetX;
	sprite["LoadingIconB"].position.y = iconOffsetY;

	auto SetAlphaLerp = [&](const std::string& name, float target)
		{
			sprite[name].color.w = Mathf::Lerp(sprite[name].color.w, target, fadeSpeed);
		};

	sprite["LoadingIcon"].angle -= 180.0f * elapsedTime;
	sprite["LoadingIconB"].angle -= 60.0f * elapsedTime;

	SetAlphaLerp("PadInst", isController ? 1.0f : 0.0f);
	SetAlphaLerp("KeyMouInst", isController ? 0.0f : 1.0f);

	if (!nextScene)return;

	if (nextScene->IsReady())
	{
		SceneManager::Instance().ChangeScene(nextScene);
		nextScene = nullptr;
	}
}

// 描画処理
void SceneLoading::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	// 画面クリア＆レンダーターゲット設定
	DirectX::XMFLOAT4 color = { 0.2f, 0.2f, 0.2f, 1.0f };    // RGBA(0.0～1.0);
	FrameBuffer* display = Graphics::Instance().GetFrameBuffer(FrameBufferId::Display);
	display->Clear(dc, color);
	display->SetRenderTargets(dc);
	RenderState* renderState = Graphics::Instance().GetRenderState();

	ID3D11SamplerState* samplers[] =
	{
		renderState->GetSamplerState(SamplerState::PointClamp)
	};

	// SpriteDataへのポインタを持つvectorを作成
	std::vector<std::pair<std::string, SpriteDataLoad*>> spriteVec;

	for (auto& pair : sprite)
	{
		spriteVec.emplace_back(pair.first, &pair.second);
	}

	// Z位置でソート（ポインタの中身のposition.zを比較）
	std::sort(spriteVec.begin(), spriteVec.end(),
		[](const auto& a, const auto& b) {
			return a.second->position.z < b.second->position.z;
		});

	dc->OMSetBlendState(
		renderState->GetBlendState(BlendState::Transparency),
		nullptr,
		0xFFFFFFFF
	);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// ソートした順に描画
	for (auto& [name, dataPtr] : spriteVec)
	{
		if (dataPtr->sprite)
		{
			dataPtr->sprite->Render(
				dc,
				dataPtr->position.x, dataPtr->position.y, dataPtr->position.z,
				dataPtr->size.x, dataPtr->size.y,
				dataPtr->texPos.x, dataPtr->texPos.y,
				dataPtr->texSize.x, dataPtr->texSize.y,
				dataPtr->angle,
				dataPtr->color.x, dataPtr->color.y, dataPtr->color.z, dataPtr->color.w
			);
		}
	}

#if _DEBUG
	DrawDebugGUI();
#endif
}

void SceneLoading::LoadingThread(SceneLoading* scene)
{
	// COM関連の初期化でスレッド毎に呼ぶ必要がある
	CoInitialize(nullptr);

	if (!scene->nextScene)return;

	// 次のシーン初期化
	scene->nextScene->Initialize();

	// スレッドが終わる前にCOM関連の終了化
	CoUninitialize();

	// 次のシーンで準備完了設定
	scene->nextScene->SetReady();
}

void SceneLoading::AddSprite(const std::string& name, SpriteDataLoad data)
{
	sprite[name] = std::move(data);
}

SpriteDataLoad SceneLoading::CreateSpriteData(ID3D11Device* device, const char* filepath, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& size, const DirectX::XMFLOAT2& texPos, const DirectX::XMFLOAT2& texSize, float angle, const DirectX::XMFLOAT4& color)
{
	SpriteDataLoad data;
	data.sprite = std::make_unique<Sprite>(device, filepath);
	data.position = pos;
	data.size = size;
	data.texPos = texPos;
	data.texSize = texSize;
	data.angle = angle;
	data.color = color;
	return data;
}

void SceneLoading::DrawDebugGUI()
{
	if (ImGui::Begin("Pause Sprites Debug"))
	{
		for (auto& [name, data] : sprite)
		{
			if (ImGui::TreeNode(name.c_str()))
			{
				// 子選択 Combo（親GUIに表示）
				static std::string selectedChild;
				if (ImGui::BeginCombo("Child", spriteChild[name].c_str()))
				{
					for (const auto& [childName, childData] : sprite)
					{
						if (childName == name) continue; // 自分自身は除外

						bool isSelected = (spriteChild[name] == childName);
						if (ImGui::Selectable(childName.c_str(), isSelected))
						{
							spriteChild[name] = childName;
							selectedChild = childName;
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::PushID("Position"); // ID重複防止

				ImGui::PushItemWidth(60);
				ImGui::DragFloat("X", &data.position.x, 1.0f);
				ImGui::SameLine();
				ImGui::DragFloat("Y", &data.position.y, 1.0f);
				ImGui::SameLine();
				ImGui::DragFloat("Z", &data.position.z, 0.01f);
				ImGui::SameLine();
				ImGui::Text("Position");

				ImGui::PopItemWidth();
				ImGui::PopID();

				float prevWidth = data.size.x;
				float prevHeight = data.size.y;

				ImGui::SetNextItemWidth(120.0f);
				ImGui::DragFloat("##Width", &data.size.x, 1.0f, 0.0f, 0.0f, "W: %.1f");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(120.0f);
				ImGui::DragFloat("##Height", &data.size.y, 1.0f, 0.0f, 0.0f, "H: %.1f");

				ImGui::SameLine();
				ImGui::Checkbox("##LinkSize", &linkedSize);

				ImGui::SameLine();
				ImGui::Text("Scale");

				if (linkedSize)
				{
					if (data.size.x != prevWidth)
					{
						float ratio = data.size.x / (prevWidth != 0.0f ? prevWidth : 1.0f);
						data.size.y = prevHeight * ratio;
					}
					else if (data.size.y != prevHeight)
					{
						float ratio = data.size.y / (prevHeight != 0.0f ? prevHeight : 1.0f);
						data.size.x = prevWidth * ratio;
					}
				}

				ImGui::DragFloat2("Tex Pos", &data.texPos.x, 1.0f);
				ImGui::DragFloat2("Tex Size", &data.texSize.x, 1.0f);
				ImGui::DragFloat("Angle", &data.angle, 0.1f);
				ImGui::ColorEdit4("Color", &data.color.x);

				ImGui::TreePop();
			}
		}
	}
	ImGui::End();

	// 親 → 子 の位置変更を反映
	for (const auto& [parentName, childName] : spriteChild)
	{
		if (sprite.count(parentName) == 0 || sprite.count(childName) == 0)
			continue;

		const auto& prevParent = previousSpriteState[parentName];
		const auto& currParent = sprite[parentName];

		// 親の前回サイズ
		const auto& prevSize = previousSpriteState[parentName].size;
		// 親の現在サイズ
		const auto& currSize = sprite[parentName].size;

		// サイズ変化率を計算（0除算回避）
		float scaleX = (prevSize.x != 0.0f) ? (currSize.x / prevSize.x) : 1.0f;
		float scaleY = (prevSize.y != 0.0f) ? (currSize.y / prevSize.y) : 1.0f;

		// 親の位置差分
		DirectX::XMFLOAT3 delta = {
			currParent.position.x - prevParent.position.x,
			currParent.position.y - prevParent.position.y,
			currParent.position.z - prevParent.position.z,
		};

		// 子の位置補正：位置差分とスケール差分を考慮
		sprite[childName].position.x = (sprite[childName].position.x - currParent.position.x) * scaleX + currParent.position.x + delta.x;
		sprite[childName].position.y = (sprite[childName].position.y - currParent.position.y) * scaleY + currParent.position.y + delta.y;

		// 子のサイズも親のスケール差分を反映
		sprite[childName].size.x *= scaleX;
		sprite[childName].size.y *= scaleY;
	}

	// 毎フレーム末尾に親の状態を保存
	for (const auto& [name, data] : sprite) {
		previousSpriteState[name].position = data.position;
		previousSpriteState[name].size = data.size;
		previousSpriteState[name].angle = data.angle;
		previousSpriteState[name].color = data.color;
		previousSpriteState[name].texPos = data.texPos;
		previousSpriteState[name].texSize = data.texSize;
	}
}