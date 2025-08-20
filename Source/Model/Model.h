#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d11.h>
#include "ModelResource.h"
#include <unordered_map>
#include <imgui.h>

class Character;

class Model
{
public:
	Model(ID3D11Device* device, const char* filename, float scale);

	static const std::vector<D3D11_INPUT_ELEMENT_DESC> InputElementDescs;

	struct Node
	{
		std::string			name;
		int					parentIndex = -1;
		DirectX::XMFLOAT3	position = { 0, 0, 0 };
		DirectX::XMFLOAT4	rotation = { 0, 0, 0, 1 };
		DirectX::XMFLOAT3	scale = { 1, 1, 1 };

		DirectX::XMFLOAT4X4	localTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
		DirectX::XMFLOAT4X4	globalTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
		DirectX::XMFLOAT4X4	worldTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

		Node*				parent = nullptr; 
		std::vector<Node*>	children;

		template<class Archive>
		void serialize(Archive& archive);
	};

	struct NodePose
	{
		DirectX::XMFLOAT3	position = { 0, 0, 0 };
		DirectX::XMFLOAT4	rotation = { 0, 0, 0, 1 };
		DirectX::XMFLOAT3	scale = { 1, 1, 1 };
	};
	std::vector<NodePose> nodePose; // ノードポーズキャッシュ

	struct  EmissiveColors
	{
		float emissiveFactor = 1.0f;//発光度
		DirectX::XMFLOAT4 adjustColor{ 1.0f,1.0f,1.0f,1.0f };//色の調整
	};

	struct  DissolveConstants
	{
		float emissivedissolve = -0.1f;//エミッシブテクスチャ用ディゾルブ
		float dissolve = -0.1f;	//ディゾルブ
		float alphaFactor = 1.0f;//アルファ値調整
		DirectX::XMFLOAT4 OverwriteColor = { 1.0f,1.0f,1.0f,1.0f };//モデルの色を変化
	};

	void DrawGui();

	// ノードデータ取得
	const std::vector<Node>& GetNodes() const { return nodes; }
	std::vector<Node>& GetNodes() { return nodes; }

	// ルートノード取得
	Node* GetRootNode() { return nodes.data(); }

	// ノード検索
	Node* FindNode(const char* name);

	// トランスフォーム更新処理
	void UpdateTransform(const DirectX::XMFLOAT4X4& worldTransform);

	// 移動値取得
	const DirectX::XMFLOAT3& GetMove() const { return move; }
	void ClearMove() { move = { 0, 0, 0 }; }

	// アニメーション再生
	void PlayAnimation(int index, bool loop, float blendSeconds = 0);

	// ルートモーション再生
	void PlayRootMotion(int index, bool loop, bool bakeY, float blendSeconds = 0, const char* rootName = nullptr);

	// アニメーション再生中か
	bool IsPlayAnimation() const;

	// ルートモーション再生中か
	bool IsPlayRootMotion() const { return isRootMotion; }

	// 現在のアニメーション再生時間取得
	float GetCurrentAnimationSeconds() const { return currentAnimationSeconds; }
	void SetCurrentAnimationSeconds(float seconds) { currentAnimationSeconds = seconds; }

	// アニメーションインデックス取得
	int GetAnimationIndex(const char* name) const;

	// アニメーション名取得
	const char* GetAnimationName(int animationIndex) const;

	// アニメーション再生時間取得
	float GetAnimationLength(int animationIndex) const;

	//
	int GetCurrentAnimationIndex() { return currentAnimationIndex; }

	// アニメーション更新処理
	void UpdateAnimation(float elapsedTime, Character* character);

	// アニメーション計算処理
	void ComputeAnimation(float elapsedTime);
	void ComputeRootAnimation(float elapsedTime, Character* character);

	// ブレンディング計算処理
	void ComputeBlending(float elapsedTime);

	// リソース取得
	const ModelResource* GetResource() const { return resource.get(); }

	// アニメーション計算
	void ComputeAnimation(int animationIndex, int nodeIndex, float time, NodePose& nodePose) const;
	void ComputeAnimation(int animationIndex, float time, std::vector<NodePose>& nodePoses) const;

	// ノードポーズ設定
	void SetNodePoses(const std::vector<NodePose>& nodePoses);

	// ノードポーズ取得
	void GetNodePoses(std::vector<NodePose>& nodePoses) const;

	//金属質感補正値取得
	void SetAdMetalness(const float metalness) { adjustMetalness = metalness; }
	//質感粗さ補正値取得
	void SetAdRoughness(const float roughness) { adjustRoughness = roughness; }

	//金属質感補正値取得
	float GetAdMetalness() const { return adjustMetalness; }
	//質感粗さ補正値取得
	float GetAdRoughness() const { return adjustRoughness;; }

	//ディゾルブ
	void SetEmissiveDissolve(const float emidissolve) { this->dissolveConstants.emissivedissolve = emidissolve; }
	void SetEmissiveConstants(const DissolveConstants dissolveConstants) { this->dissolveConstants = dissolveConstants; }
	DissolveConstants GetEmissiveConstants() const { return dissolveConstants; }

	//エミッシブ色設定
	void SetEmissiveColors(const EmissiveColors colors) { emissive = colors; }

	//エミッシブ色取得
	EmissiveColors GetEmissiveColors() const { return emissive; }

	//質感調整用ImGui
	void DebugGui(const char* name);

private:
	//model事の質感補正値
	float adjustMetalness = 0; //  金属質調整
	float adjustRoughness = 0; //  粗さ調整
	//model事の発光補正値
	EmissiveColors emissive;
	//ディゾルブ
	DissolveConstants dissolveConstants;

	int currentAnimationIndex = -1;
	float currentAnimationSeconds = 0;
	float oldAnimationSeconds = 0;
	float animationSpeed = 1.0f;
	float baseAnimationSpeed = 1.0f;
	float scaling = 1.0f;

	bool animationPlaying = false;
	bool animationLoop = false;
	bool isRootMotion = false;
	bool bakeMoveY = false;

	float currentAnimationBlendSeconds = 0.0f;
	float animationBlendSecondsLength = -1.0f;
	bool animationBlending = false;

	const char* filename = nullptr;
	const char* rootNodeName = nullptr;

private:
	std::shared_ptr<ModelResource> resource;
	std::vector<Node>		nodes;
	std::vector<NodePose> nodePoses;
	std::vector<std::string>	nodeNames; // ノード名キャッシュ
	NodePose beginPose, oldPose, newPose;
	NodePose endPose;
protected:
	DirectX::XMFLOAT3 move = { 0,0,0 };
};