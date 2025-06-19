#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d11.h>
#include "ModelResource.h"
#include <unordered_map>

class Model
{
public:
	Model(ID3D11Device* device, const char* filename, float sampleRate = 60);

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


	// ノードデータ取得
	const std::vector<Node>& GetNodes() const { return nodes; }
	std::vector<Node>& GetNodes() { return nodes; }

	// ルートノード取得
	Node* GetRootNode() { return nodes.data(); }

	// ノード検索
	Node* FindNode(const char* name);

	// トランスフォーム更新処理
	void UpdateTransform(const DirectX::XMFLOAT4X4& worldTransform);

	// アニメーション再生
	void PlayAnimation(int index, bool loop, float blendSeconds = 0);

	// アニメーション再生中か
	bool IsPlayAnimation() const;

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
	void UpdateAnimation(float elapsedTime);

	//// アニメーション計算処理
	void ComputeAnimation(float elapsedTime);

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

private:
	int currentAnimationIndex = -1;
	float currentAnimationSeconds = 0;
	float oldAnimationSeconds = 0;
	float animationSpeed = 1.0f;
	float baseAnimationSpeed = 1.0f;

	bool animationPlaying = false;
	bool animationLoop = false;

	float currentAnimationBlendSeconds = 0.0f;
	float animationBlendSecondsLength = -1.0f;
	bool animationBlending = false;

	const char* filename = nullptr;

private:
	std::shared_ptr<ModelResource> resource;
	std::vector<Node>		nodes;
	std::vector<std::string>	nodeNames; // ノード名キャッシュ

};