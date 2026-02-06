#include "Pause.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Scene/SceneTitle.h"
#include "Scene/SceneManager.h"
#include <imgui.h>

void Pause::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	//背景用
	AddSprite("Back", CreateSpriteData(device, "Data/Sprite/Title_Back.png", { 0,0,0 }, { 1920, 1080 }, { 0,0 }, { 1920,1080 }, 0, { 1,1,1,0 }));

	//操作説明
	AddSprite("PauseBack1", CreateSpriteData(device, "Data/Sprite/PadInstruction.png", { 145.0f,100.0f,0 }, { 1137.8f, 640.0f }, { 0,0 }, { 1166, 591 }, 0, { 1,1,1,0 }));
	AddSprite("PauseBack2", CreateSpriteData(device, "Data/Sprite/KeyMouInstruction.png", { 145.0f,100,0 }, { 1137.8f, 640.0f }, { 0,0 }, { 1052, 561 }, 0, { 1,1,1,0 }));

	//ボタン
	AddSprite("PauseCon", CreateSpriteData(device, "Data/Sprite/Pause_Con.png", { 0.0f,965.0f,0 }, { 195.0f,42.5f }, { 0,0 }, { 512,172 }, 0, { 1,1,1,0 }));
	AddSprite("PauseKeyMou", CreateSpriteData(device, "Data/Sprite/Pause_KeyMou.png", { 0.0f,965.0f,0 }, { 195.0f,42.5f }, { 0,0 }, { 512,172 }, 0, { 1,1,1,0 }));

	AddSprite("DecisionCon", CreateSpriteData(device, "Data/Sprite/Pause_Con.png", { 95,965,0 }, { 234.0f,50.3f }, { 0,172 }, { 512,172 }, 0, { 1,1,1,0 }));
	AddSprite("DecisionKeyMou", CreateSpriteData(device, "Data/Sprite/Pause_KeyMou.png", { 95,965,0 }, { 234.0f,50.3f }, { 0,172 }, { 512,172 }, 0, { 1,1,1,0 }));

	AddSprite("BackCon", CreateSpriteData(device, "Data/Sprite/Pause_Con.png", { -28,974,0 }, { 234.0f,50.3f }, { 0,344 }, { 512,172 }, 0, { 1,1,1,0 }));
	AddSprite("BackKeyMou", CreateSpriteData(device, "Data/Sprite/Pause_KeyMou.png", { -28,968,0 }, { 234.0f,50.3f }, { 0,344 }, { 512,172 }, 0, { 1,1,1,0 }));

	AddSprite("SelectBo", CreateSpriteData(device, "Data/Sprite/SelectBo.png", { 1515,910,0.1f }, { 317.0f,89.0f }, { 0,0 }, { 512,256 }, 0, { 1,1,1,0 }));
	AddSprite("SelectBo1", CreateSpriteData(device, "Data/Sprite/SelectBo.png", { 1525,917,0 }, { 317.0f,89.0f }, { 0,0 }, { 512,256 }, 0, { 1,0,0,0 }));

	AddSprite("Select0", CreateSpriteData(device, "Data/Sprite/SelectTG.png", { 1380,775,0.2f }, { 615.0f,153.8f }, { 0,180 }, { 1280,180 }, 0, { 1,1,1,0 }));
	AddSprite("Select1", CreateSpriteData(device, "Data/Sprite/SelectTG.png", { 1380,930,0.2f }, { 615.0f,153.8f }, { 0,540 }, { 1280,180 }, 0, { 0,0,0,0 }));

	AddSprite("Select0_jp", CreateSpriteData(device, "Data/Sprite/SelectTG.png", { 1410,731,0.2f }, { 552.5f,81.7f }, { 0,0 }, { 1280,180 }, 0, { 1,0,0,0 }));
	AddSprite("Select1_jp", CreateSpriteData(device, "Data/Sprite/SelectTG.png", { 1423,874,0.2f }, { 538.1f,76.9f }, { 0,360 }, { 1280,180 }, 0, { 1,0,0,0 }));

	linkedSize = false;
	isController = true;
	isPause = false;

	baseScale = sprite["SelectBo1"].size;
	scene = SelectScene::Game;
}

void Pause::AddSprite(const std::string& name, SpriteData data)
{
	sprite[name] = std::move(data);
}

SpriteData Pause::CreateSpriteData(ID3D11Device* device, const char* filepath, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& size, const DirectX::XMFLOAT2& texPos, const DirectX::XMFLOAT2& texSize, float angle, const DirectX::XMFLOAT4& color)
{
	SpriteData data;
	data.sprite = std::make_unique<Sprite>(device, filepath);
	data.position = pos;
	data.size = size;
	data.texPos = texPos;
	data.texSize = texSize;
	data.angle = angle;
	data.color = color;
	return data;
}

void Pause::Update(float elapsedTime)
{
	// 共通：入力方式
	isController = Input::Instance().GetIsLastGamePad();
	GamePad& gamepad = Input::Instance().GetGamePad();
	float fadeSpeed = 6.0f * elapsedTime;

	auto SetAlphaLerp = [&](const std::string& name, float target)
		{
			sprite[name].color.w = Mathf::Lerp(sprite[name].color.w, target, fadeSpeed);
		};

	// ========================
	// ポーズ解除時（非表示へ）
	// ========================
	if (!isPause)
	{
		// 背景などを全部非表示
		SetAlphaLerp("Back", 0.0f);
		SetAlphaLerp("PauseBack1", 0.0f);
		SetAlphaLerp("PauseBack2", 0.0f);
		SetAlphaLerp("BackCon", 0.0f);
		SetAlphaLerp("BackKeyMou", 0.0f);
		SetAlphaLerp("DecisionCon", 0.0f);
		SetAlphaLerp("DecisionKeyMou", 0.0f);
		SetAlphaLerp("Select0", 0.0f);
		SetAlphaLerp("Select1", 0.0f);
		SetAlphaLerp("Select0_jp", 0.0f);
		SetAlphaLerp("Select1_jp", 0.0f);

		SetAlphaLerp("SelectBo", 0.0f);
		SetAlphaLerp("SelectBo1", 0.0f);

		// 入力表示切替：どちらかは見せる
		SetAlphaLerp("PauseCon", isController ? 1.0f : 0.0f);
		SetAlphaLerp("PauseKeyMou", isController ? 0.0f : 1.0f);
	}
	// ========================
	// ポーズ中（表示へ）
	// ========================
	else
	{
		// 背景は淡く表示
		SetAlphaLerp("Back", 180.0f / 255.0f);
		SetAlphaLerp("Select0", 1.0f);
		SetAlphaLerp("Select1", 1.0f);
		SetAlphaLerp("Select0_jp", 1.0f);
		SetAlphaLerp("Select1_jp", 1.0f);

		// 「コントローラー／キーボード」表記は消す
		SetAlphaLerp("PauseCon", 0.0f);
		SetAlphaLerp("PauseKeyMou", 0.0f);

		// 背景表示切替
		SetAlphaLerp("PauseBack1", isController ? 1.0f : 0.0f);
		SetAlphaLerp("PauseBack2", isController ? 0.0f : 1.0f);

		SetAlphaLerp("DecisionCon", isController ? 1.0f : 0.0f);
		SetAlphaLerp("DecisionKeyMou", isController ? 0.0f : 1.0f);

		SetAlphaLerp("BackCon", isController ? 1.0f : 0.0f);
		SetAlphaLerp("BackKeyMou", isController ? 0.0f : 1.0f);

		SetAlphaLerp("SelectBo", 230.0f / 255.0f);
		SetAlphaLerp("SelectBo1", 220.0f / 255.0f);

		UpdatePulse(elapsedTime);

		if (gamepad.GetButtonDown() & GamePad::BTN_B)
		{
			isPause = !isPause;
		}
	}

	// ========================
	// ポーズ切り替え
	// ========================
	if (gamepad.GetButtonDown() & GamePad::BTN_START)
	{
		isPause = !isPause;
	}

	if (isPause)
	{
		static SelectScene previousScene = SelectScene::Count; // 最初は無効値に

		if (gamepad.GetButtonDown() & GamePad::BTN_UP)
		{
			int current = static_cast<int>(scene);
			current--;
			if (current < 0)
				current = static_cast<int>(SelectScene::Count) - 1;

			scene = static_cast<SelectScene>(current);
		}
		else if (gamepad.GetButtonDown() & GamePad::BTN_DOWN)
		{
			int current = static_cast<int>(scene);
			current++;
			if (current >= static_cast<int>(SelectScene::Count))
				current = 0;

			scene = static_cast<SelectScene>(current);
		}

		// ここで切り替わったかチェック
		if (scene != previousScene)
		{
			switch (scene)
			{
			case SelectScene::Game:
				sprite["Select0"].color = { 1, 1, 1, 1 };
				sprite["Select1"].color = { 0, 0, 0, 1 };

				sprite["SelectBo"].position = { 1515.f, 910.f, 0.1f };
				sprite["SelectBo1"].position = { 1524.f, 917.f, 0 };

				sprite["SelectBo"].size = { 334.8f, 94.0f };
				sprite["SelectBo1"].size = { 334.8f, 94.0f };
				baseScale = sprite["SelectBo1"].size;
				break;
			case SelectScene::Title:
				sprite["Select0"].color = { 0, 0, 0, 1 };
				sprite["Select1"].color = { 1, 1, 1, 1 };

				sprite["SelectBo"].position = { 1515.0f, 790.f, 0.1f };
				sprite["SelectBo1"].position = { 1525.f, 797.f, 0 };

				sprite["SelectBo"].size = { 334.8f, 94.0f };
				sprite["SelectBo1"].size = { 334.8f, 94.0f };
				baseScale = sprite["SelectBo1"].size;
				break;

			default:
				break;
			}

			pulseState = PulseState::Expanding;
			expandTimer = 0.0f;
			shrinkTimer = 0.0f;

			// 切り替わったので記録しておく
			previousScene = scene;
		}

		// 決定ボタンの処理は毎フレームでOK
		if (gamepad.GetButtonDown() & GamePad::BTN_A_EMU)
		{
			if (scene == SelectScene::Game)
			{
				isPause = !isPause;
			}
			else if (scene == SelectScene::Title)
			{
				SceneManager::Instance().ChangeScene(new SceneTitle);
			}
		}
	}

	//if (offsetIncreasing)
	//{
	//	offsetScale.x += offsetSpeed * elapsedTime;
	//	if (offsetScale.x >= 10.0f)
	//	{
	//		offsetScale.x = 10.0f;
	//		offsetIncreasing = false;
	//	}
	//}
	//else
	//{
	//	offsetScale.x -= offsetSpeed * elapsedTime;
	//	if (offsetScale.x <= -4.0f)
	//	{
	//		offsetScale.x = -4.0f;
	//		offsetIncreasing = true;
	//	}
	//}

	//// --- X に応じて Y を逆補完 ---
	//float t = (offsetScale.x + 4.0f) / 14.0f; // X: [-4,10] → t: [0,1]
	//offsetScale.y = Mathf::Lerp(10.0f, -4.0f, t); // Y: [10 → -4]
}

void Pause::Render(float elapsedTime, ID3D11DeviceContext* dc)
{
	// SpriteDataへのポインタを持つvectorを作成
	std::vector<std::pair<std::string, SpriteData*>> spriteVec;

	for (auto& pair : sprite)
	{
		spriteVec.emplace_back(pair.first, &pair.second);
	}

	// Z位置でソート（ポインタの中身のposition.zを比較）
	std::sort(spriteVec.begin(), spriteVec.end(),
		[](const auto& a, const auto& b) {
			return a.second->position.z < b.second->position.z;
		});

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
}


void Pause::DrawDebugGUI()
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
		ImGui::DragFloat("Interval", &pulseInterval, 0.1f);
		ImGui::DragFloat("Strength", &pulseStrength, 0.1f);
		ImGui::DragFloat("expandDuration", &expandDuration, 0.1f);
		ImGui::DragFloat("shrinkDuration", &shrinkDuration, 0.1f);
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

void Pause::UpdatePulse(float elapsedTime)
{
	switch (pulseState) {
	case PulseState::Waiting:
		// サイズを通常に戻すだけ
		sprite["SelectBo1"].size.x = baseScale.x;
		sprite["SelectBo1"].size.y = baseScale.y;
		break;

	case PulseState::Expanding:
		expandTimer += elapsedTime;
		{
			float t = std::clamp(expandTimer / expandDuration, 0.0f, 1.0f);
			float offset = Mathf::Lerp(0.0f, pulseStrength, t);
			sprite["SelectBo1"].size.x = baseScale.x + offset;
			sprite["SelectBo1"].size.y = baseScale.y + offset;

			if (expandTimer >= expandDuration) {
				shrinkTimer = 0.0f;
				pulseState = PulseState::Shrinking;
			}
		}
		break;

	case PulseState::Shrinking:
		shrinkTimer += elapsedTime;
		{
			float t = std::clamp(shrinkTimer / shrinkDuration, 0.0f, 1.0f);
			float offset = Mathf::Lerp(pulseStrength, 0.0f, t);
			sprite["SelectBo1"].size.x = baseScale.x + offset;
			sprite["SelectBo1"].size.y = baseScale.y + offset;

			if (shrinkTimer >= shrinkDuration) {
				pulseState = PulseState::Waiting;
			}
		}
		break;
	}
}


