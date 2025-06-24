#include <filesystem>
#include <fstream>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include "System/Misc.h"
#include "GLTFImporter.h"
#include "Graphics/GpuResourceUtils.h"
#include "Model.h"
#include "ResourceManager.h"

const std::vector<D3D11_INPUT_ELEMENT_DESC> Model::InputElementDescs =
{
	{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

};

// コンストラクタ
Model::Model(ID3D11Device* device, const char* filename, float sampleRate)
{
	resource = ResourceManager::Instance().LoadModelResource(filename);

	// ノード
	const std::vector<ModelResource::Node>& resNodes = resource->GetNodes();

	nodes.resize(resNodes.size());
	// すべてのノードに対してループ実行
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		// 対応するノードデータをresNodesから取得する(ソースノード)
		auto&& src = resNodes.at(nodeIndex);
		// 対応するノードをnodesから取得する(対象ノード)
		auto&& dst = nodes.at(nodeIndex);

		// ノード名をsrcからdstにコピー
		// nameはstd::stringと仮定し、c_str()でC文字列を取得
		dst.name = src.name.c_str();
		nodeNames.push_back(src.name);
		// src.parentIndexが有効(0以上)であれば、親ノードを設定
		dst.parent = src.parentIndex >= 0 ? &nodes.at(src.parentIndex) : nullptr;
		// スケールの値をsrcからdstにコピー
		dst.scale = src.scale;
		// 回転の値をsrcからdstにコピー
		dst.rotation = src.rotation;
		dst.position = src.position;

		if (dst.parent != nullptr)
		{
			// 親ノードが存在する場合、その親ノードの子リストに現在のノードを追加
			dst.parent->children.emplace_back(&dst);
		}
	}

	// ノードキャッシュ
	nodePose.resize(nodes.size());

	// 行列初期化
	DirectX::XMFLOAT4X4 worldTransform;
	DirectX::XMStoreFloat4x4(&worldTransform, DirectX::XMMatrixIdentity());
	UpdateTransform(worldTransform);
}

Model::Node* Model::FindNode(const char* name)
{
	// 全てのノードを総当たりで名前比較する
	int result;
	for (auto& node : nodes)
	{
		// 名前比較（名前はnull終端文字列として扱う）
		result = strcmp(node.name.c_str(), name);
		if (result == 0)
		{
			// ノードが見つかった場合、そのポインタを返す
			return &node;
		}
	}

	// 見つからなかった
	return nullptr;
}

// アニメーションインデックス取得
int Model::GetAnimationIndex(const char* name) const
{
	for (size_t animationIndex = 0; animationIndex < resource->GetAnimations().size(); ++animationIndex)
	{
		if (resource->GetAnimations().at(animationIndex).name == name)
		{
			return static_cast<int>(animationIndex);
		}
	}
	return -1;
}

// トランスフォーム更新処理
void Model::UpdateTransform(const DirectX::XMFLOAT4X4& worldTransform)
{
	DirectX::XMMATRIX ParentWorldTransform = DirectX::XMLoadFloat4x4(&worldTransform);

	for (Node& node : nodes)
	{
		// ローカル行列算出
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&node.rotation));
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.position.x, node.position.y, node.position.z);
		DirectX::XMMATRIX LocalTransform = S * R * T;

		// グローバル行列算出
		DirectX::XMMATRIX ParentGlobalTransform;
		if (node.parent != nullptr)
		{
			ParentGlobalTransform = DirectX::XMLoadFloat4x4(&node.parent->globalTransform);
		}
		else
		{
			ParentGlobalTransform = DirectX::XMMatrixIdentity();
		}
		DirectX::XMMATRIX GlobalTransform = LocalTransform * ParentGlobalTransform;

		// ワールド行列算出
		DirectX::XMMATRIX WorldTransform = GlobalTransform * ParentWorldTransform;

		// 計算結果を格納
		DirectX::XMStoreFloat4x4(&node.localTransform, LocalTransform);
		DirectX::XMStoreFloat4x4(&node.globalTransform, GlobalTransform);
		DirectX::XMStoreFloat4x4(&node.worldTransform, WorldTransform);
	}
}

// アニメーション再生
void Model::PlayAnimation(int index, bool loop, float blendSeconds)
{
	currentAnimationIndex = index;
	currentAnimationSeconds = 0;
	animationLoop = loop;
	animationPlaying = true;

	// ブレンドパラメータ
	animationBlending = blendSeconds > 0.0f;
	currentAnimationBlendSeconds = 0.0f;
	animationBlendSecondsLength = blendSeconds;

	// 現在の姿勢をキャッシュする
	for (size_t i = 0; i < nodes.size(); ++i)
	{
		const Node& src = nodes.at(i);
		NodePose& dst = nodePose.at(i);

		dst.position = src.position;
		dst.rotation = src.rotation;
		dst.scale = src.scale;
	}
}

// アニメーション再生中か
bool Model::IsPlayAnimation() const
{
	if (currentAnimationIndex < 0) return false;
	if (currentAnimationIndex >= resource->GetAnimations().size()) return false;
	return animationPlaying;
}

// アニメーション更新処理
void Model::UpdateAnimation(float elapsedTime)
{
	ComputeAnimation(elapsedTime * animationSpeed * baseAnimationSpeed);

	ComputeBlending(elapsedTime * animationSpeed * baseAnimationSpeed);
}

// アニメーション計算処理
void Model::ComputeAnimation(float elapsedTime)
{
	if (!IsPlayAnimation()) return;

	// 指定のアニメーションデータを取得
	const ModelResource::Animation& animation = resource->GetAnimations().at(currentAnimationIndex);

	// ノード毎のアニメーション処理
	for (size_t nodeIndex = 0; nodeIndex < animation.nodeAnims.size(); ++nodeIndex)
	{
		Node& node = nodes.at(nodeIndex);
		const ModelResource::NodeAnim& nodeAnim = animation.nodeAnims.at(nodeIndex);

		// 位置
		for (size_t index = 0; index < nodeAnim.positionKeyframes.size() - 1; ++index)
		{
			// 現在の時間がどのキーフレームの間にいるか判定する
			const ModelResource::VectorKeyframe& keyframe0 = nodeAnim.positionKeyframes.at(index);
			const ModelResource::VectorKeyframe& keyframe1 = nodeAnim.positionKeyframes.at(index + 1);
			if (currentAnimationSeconds >= keyframe0.seconds && currentAnimationSeconds < keyframe1.seconds)
			{
				// 再生時間とキーフレームの時間から補完率を算出する
				float rate = (currentAnimationSeconds - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

				// 前のキーフレームと次のキーフレームの姿勢を補完
				DirectX::XMVECTOR V0 = DirectX::XMLoadFloat3(&keyframe0.value);
				DirectX::XMVECTOR V1 = DirectX::XMLoadFloat3(&keyframe1.value);
				DirectX::XMVECTOR V = DirectX::XMVectorLerp(V0, V1, rate);
				// 計算結果をノードに格納
				DirectX::XMStoreFloat3(&node.position, V);
			}
		}
		// 回転
		for (size_t index = 0; index < nodeAnim.rotationKeyframes.size() - 1; ++index)
		{
			// 現在の時間がどのキーフレームの間にいるか判断する
			const ModelResource::QuaternionKeyframe& keyframe0 = nodeAnim.rotationKeyframes.at(index);
			const ModelResource::QuaternionKeyframe& keyframe1 = nodeAnim.rotationKeyframes.at(index + 1);
			if (currentAnimationSeconds >= keyframe0.seconds && currentAnimationSeconds < keyframe1.seconds)
			{
				// 再生時間とキーフレームの時間から補完率を算出する
				float rate = (currentAnimationSeconds - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

				// 前のキーフレームと次のキーフレームの姿勢を補完
				DirectX::XMVECTOR Q0 = DirectX::XMLoadFloat4(&keyframe0.value);
				DirectX::XMVECTOR Q1 = DirectX::XMLoadFloat4(&keyframe1.value);
				DirectX::XMVECTOR Q = DirectX::XMQuaternionSlerp(Q0, Q1, rate);
				// 計算結果をノードに格納
				DirectX::XMStoreFloat4(&node.rotation, Q);
			}
		}
		// スケール
		for (size_t index = 0; index < nodeAnim.scaleKeyframes.size() - 1; ++index)
		{
			// 現在の時間がどのキーフレームの間にいるか
			const ModelResource::VectorKeyframe& keyframe0 = nodeAnim.scaleKeyframes.at(index);
			const ModelResource::VectorKeyframe& keyframe1 = nodeAnim.scaleKeyframes.at(index + 1);
			if (currentAnimationSeconds >= keyframe0.seconds && currentAnimationSeconds < keyframe1.seconds)
			{
				// 再生時間とキーフレームの時間から補完率を算出する
				float rate = (currentAnimationSeconds - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

				// 前のキーフレームと次のキーフレームの姿勢を補完
				DirectX::XMVECTOR V0 = DirectX::XMLoadFloat3(&keyframe0.value);
				DirectX::XMVECTOR V1 = DirectX::XMLoadFloat3(&keyframe1.value);
				DirectX::XMVECTOR V = DirectX::XMVectorLerp(V0, V1, rate);
				// 計算結果をノードに格納
				DirectX::XMStoreFloat3(&node.scale, V);
			}
		}
	}

	// 経過時間
	currentAnimationSeconds += elapsedTime;

	// 再生時間が終端時間を超えたら
	if (currentAnimationSeconds >= animation.secondsLength)
	{
		if (animationLoop)
		{
			// 再生時間を巻き戻す
			currentAnimationSeconds -= animation.secondsLength;
		}
		else
		{
			// 再生終了時間にする
			currentAnimationSeconds = animation.secondsLength;
			animationPlaying = false;
		}
	}
}

// ブレンディング計算処理
void Model::ComputeBlending(float elapsedTime)
{
	if (!animationBlending)
	{
		return;
	}

	// ブレンド率の計算
	float rate = currentAnimationSeconds / animationBlendSecondsLength;

	// ブレンド計算
	int count = static_cast<int>(nodes.size());
	for (int i = 0; i < count; ++i)
	{
		const NodePose& cache = nodePose.at(i);
		Node& node = nodes.at(i);

		DirectX::XMVECTOR S0 = DirectX::XMLoadFloat3(&cache.scale);
		DirectX::XMVECTOR S1 = DirectX::XMLoadFloat3(&node.scale);
		DirectX::XMVECTOR R0 = DirectX::XMLoadFloat4(&cache.rotation);
		DirectX::XMVECTOR R1 = DirectX::XMLoadFloat4(&node.rotation);
		DirectX::XMVECTOR T0 = DirectX::XMLoadFloat3(&cache.position);
		DirectX::XMVECTOR T1 = DirectX::XMLoadFloat3(&node.position);

		DirectX::XMVECTOR S = DirectX::XMVectorLerp(S0, S1, rate);
		DirectX::XMVECTOR R = DirectX::XMQuaternionSlerp(R0, R1, rate);
		DirectX::XMVECTOR T = DirectX::XMVectorLerp(T0, T1, rate);

		DirectX::XMStoreFloat3(&node.scale, S);
		DirectX::XMStoreFloat4(&node.rotation, R);
		DirectX::XMStoreFloat3(&node.position, T);
	}

	// 時間経過
	currentAnimationBlendSeconds += elapsedTime;
	if (currentAnimationBlendSeconds >= animationBlendSecondsLength)
	{
		currentAnimationBlendSeconds = animationBlendSecondsLength;
		animationBlending = false;
	}
}

void Model::ComputeAnimation(int animationIndex, int nodeIndex, float time, NodePose& nodePose) const
{
	const ModelResource::Animation& animation = resource->GetAnimations().at(animationIndex);
	const ModelResource::NodeAnim& nodeAnim = animation.nodeAnims.at(nodeIndex);

	// 位置
	for (size_t index = 0; index < nodeAnim.positionKeyframes.size() - 1; ++index)
	{
		// 現在の時間がどのキーフレームの間にいるか判定する
		const ModelResource::VectorKeyframe& keyframe0 = nodeAnim.positionKeyframes.at(index);
		const ModelResource::VectorKeyframe& keyframe1 = nodeAnim.positionKeyframes.at(index + 1);
		if (time >= keyframe0.seconds && time <= keyframe1.seconds)
		{
			// 再生時間とキーフレームの時間から補完率を算出する
			float rate = (time - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

			// 前のキーフレームと次のキーフレームの姿勢を補完
			DirectX::XMVECTOR V0 = DirectX::XMLoadFloat3(&keyframe0.value);
			DirectX::XMVECTOR V1 = DirectX::XMLoadFloat3(&keyframe1.value);
			DirectX::XMVECTOR V = DirectX::XMVectorLerp(V0, V1, rate);
			// 計算結果をノードに格納
			DirectX::XMStoreFloat3(&nodePose.position, V);
		}
	}
	// 回転
	for (size_t index = 0; index < nodeAnim.rotationKeyframes.size() - 1; ++index)
	{
		// 現在の時間がどのキーフレームの間にいるか判定する
		const ModelResource::QuaternionKeyframe& keyframe0 = nodeAnim.rotationKeyframes.at(index);
		const ModelResource::QuaternionKeyframe& keyframe1 = nodeAnim.rotationKeyframes.at(index + 1);
		if (time >= keyframe0.seconds && time <= keyframe1.seconds)
		{
			// 再生時間とキーフレームの時間から補完率を算出する
			float rate = (time - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

			// 前のキーフレームと次のキーフレームの姿勢を補完
			DirectX::XMVECTOR Q0 = DirectX::XMLoadFloat4(&keyframe0.value);
			DirectX::XMVECTOR Q1 = DirectX::XMLoadFloat4(&keyframe1.value);
			DirectX::XMVECTOR Q = DirectX::XMQuaternionSlerp(Q0, Q1, rate);
			// 計算結果をノードに格納
			DirectX::XMStoreFloat4(&nodePose.rotation, Q);
		}
	}
	// スケール
	for (size_t index = 0; index < nodeAnim.scaleKeyframes.size() - 1; ++index)
	{
		// 現在の時間がどのキーフレームの間にいるか判定する
		const ModelResource::VectorKeyframe& keyframe0 = nodeAnim.scaleKeyframes.at(index);
		const ModelResource::VectorKeyframe& keyframe1 = nodeAnim.scaleKeyframes.at(index + 1);
		if (time >= keyframe0.seconds && time <= keyframe1.seconds)
		{
			// 再生時間とキーフレームの時間から補完率を算出する
			float rate = (time - keyframe0.seconds) / (keyframe1.seconds - keyframe0.seconds);

			// 前のキーフレームと次のキーフレームの姿勢を補完
			DirectX::XMVECTOR V0 = DirectX::XMLoadFloat3(&keyframe0.value);
			DirectX::XMVECTOR V1 = DirectX::XMLoadFloat3(&keyframe1.value);
			DirectX::XMVECTOR V = DirectX::XMVectorLerp(V0, V1, rate);
			// 計算結果をノードに格納
			DirectX::XMStoreFloat3(&nodePose.scale, V);
		}
	}
}

// アニメーション計算
void Model::ComputeAnimation(int animationIndex, float time, std::vector<NodePose>& nodePoses) const
{
	if (nodePoses.size() != nodes.size())
	{
		nodePoses.resize(nodes.size());
	}
	for (size_t nodeIndex = 0; nodeIndex < nodePoses.size(); ++nodeIndex)
	{
		ComputeAnimation(animationIndex, static_cast<int>(nodeIndex), time, nodePoses.at(nodeIndex));
	}
}

// ノードポーズ設定
void Model::SetNodePoses(const std::vector<NodePose>& nodePoses)
{
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		const NodePose& pose = nodePoses.at(nodeIndex);
		Node& node = nodes.at(nodeIndex);

		node.position = pose.position;
		node.rotation = pose.rotation;
		node.scale = pose.scale;
	}
}

// ノードポーズ取得
void Model::GetNodePoses(std::vector<NodePose>& nodePoses) const
{
	if (nodePoses.size() != nodes.size())
	{
		nodePoses.resize(nodes.size());
	}
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		const Node& node = nodes.at(nodeIndex);
		NodePose& pose = nodePoses.at(nodeIndex);

		pose.position = node.position;
		pose.rotation = node.rotation;
		pose.scale = node.scale;
	}
}

void Model::DebugGui(const char* name)
{

	std::vector<ModelResource::Material> materials = resource->GetMaterials();
	for (int i = 0; i < materials.size(); i++)
	{
		ModelResource::Material material = materials.at(i);
		ImGui::PushID(i);  // ここでIDスコープを変える

		char textureID[128];
		snprintf(textureID, sizeof(textureID), u8"テクスチャ　%i", i + 1);

		if (ImGui::CollapsingHeader(textureID, ImGuiTreeNodeFlags_Framed))
		{

			if (ImGui::CollapsingHeader("Albedo Map", ImGuiTreeNodeFlags_Framed))
			{
				if (!resource->GetMaterials().at(0).baseTextureFileName.empty()) {//テクスチャの階層
					ImGui::Text("Albedo : %s", resource->GetMaterials().at(0).baseTextureFileName.c_str());
				}
				ImGui::Text("Albedo Texture:");//テクスチャ表示
				if (resource->GetMaterials().at(0).baseMap) {
					ImGui::Image(resource->GetMaterials().at(0).baseMap.Get(), ImVec2(128, 128));
				}
			}

			if (ImGui::CollapsingHeader("Normal Map", ImGuiTreeNodeFlags_Framed))
			{
				if (!resource->GetMaterials().at(0).normalTextureFileName.empty()) {
					ImGui::Text("Normal : %s", resource->GetMaterials().at(0).normalTextureFileName.c_str());
				}
				ImGui::Text("Normal Map:");
				if (resource->GetMaterials().at(0).normalMap) {
					ImGui::Image(resource->GetMaterials().at(0).normalMap.Get(), ImVec2(128, 128));
				}
			}

			if (ImGui::CollapsingHeader("Emmisive Map", ImGuiTreeNodeFlags_Framed))
			{
				if (!resource->GetMaterials().at(0).emissiveTextureFileName.empty()) {
					ImGui::Text("Emmisive : %s", resource->GetMaterials().at(0).emissiveTextureFileName.c_str());
				}
				ImGui::Text("Emmisive Map:");
				if (resource->GetMaterials().at(0).emissiveMap) {
					ImGui::Image(resource->GetMaterials().at(0).emissiveMap.Get(), ImVec2(128, 128));
				}
			}

			if (ImGui::CollapsingHeader("Metallic Map", ImGuiTreeNodeFlags_Framed))
			{
				if (!resource->GetMaterials().at(0).metalnessRoughnessTextureFileName.empty()) {
					ImGui::Text("Metallic : %s", resource->GetMaterials().at(0).metalnessRoughnessTextureFileName.c_str());
				}

				ImGui::Text("Metallic Map:");
				if (resource->GetMaterials().at(0).metalnessRoughnessMap) {
					ImGui::Image(resource->GetMaterials().at(0).metalnessRoughnessMap.Get(), ImVec2(128, 128));
				}
			}

			if (ImGui::CollapsingHeader("Occlusion Map", ImGuiTreeNodeFlags_Framed))
			{
				if (!resource->GetMaterials().at(0).occlusionTextureFileName.empty()) {
					ImGui::Text("Occlusion : %s", resource->GetMaterials().at(0).occlusionTextureFileName.c_str());
				}

				ImGui::Text("Occkusion Map:");
				if (resource->GetMaterials().at(0).occlusionMap) {
					ImGui::Image(resource->GetMaterials().at(0).occlusionMap.Get(), ImVec2(128, 128));
				}
			}

			// その他のテクスチャがあれば追加
		}
		ImGui::PopID();
	}


	// ラベル文字列のバッファを用意
	char labelMetalness[128];
	char labelRoughness[128];
	char labelDissolve[128];
	char labelEmissiveDissolve[128];
	char labelAlpha[128];
	char labelOverColor[128];

	// 引数の name を前に追加
	snprintf(labelMetalness, sizeof(labelMetalness), u8"%s 金属質", name);
	snprintf(labelRoughness, sizeof(labelRoughness), u8"%s 材質の粗さ", name);
	snprintf(labelDissolve, sizeof(labelDissolve), u8"%s ディゾルブ", name);
	snprintf(labelEmissiveDissolve, sizeof(labelEmissiveDissolve), u8"%s エミッシブディゾルブ", name);
	snprintf(labelAlpha, sizeof(labelAlpha), u8"%s アルファ", name);
	snprintf(labelOverColor, sizeof(labelOverColor), u8"%s オーバーカラー", name);

	// ImGui に渡す
	ImGui::DragFloat(labelMetalness, &adjustMetalness, 0.01f, 0, 1.0f);
	ImGui::DragFloat(labelRoughness, &adjustRoughness, 0.01f, 0, 1.0f);
	ImGui::DragFloat(labelDissolve, &dissolveConstants.dissolve, 0.01f, -0.1f, 1.0f);
	ImGui::DragFloat(labelEmissiveDissolve, &dissolveConstants.emissivedissolve, 0.01f, -0.1f, 1.0f);
	ImGui::DragFloat(labelAlpha, &dissolveConstants.alphaFactor, 0.01f, 0.0f, 1.0f);
	ImGui::ColorEdit4(labelOverColor, &dissolveConstants.OverwriteColor.x);
}