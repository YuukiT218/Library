#pragma once

#include <string>
#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <filesystem>

class ModelResource
{
public:
	ModelResource() {}
	virtual ~ModelResource() {}

	static const std::vector<D3D11_INPUT_ELEMENT_DESC> InputElementDescs;

	using NodeId = UINT64;

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

		Node* parent = nullptr;
		std::vector<Node*>	children;

		template<class Archive>
		void serialize(Archive& archive);
	};

	enum class AlphaMode
	{
		Opaque,
		Mask,
		Blend
	};

	struct Material
	{
		std::string			name;
		std::string			baseTextureFileName;
		std::string			normalTextureFileName;
		std::string			emissiveTextureFileName;
		std::string			occlusionTextureFileName;
		std::string			metalnessRoughnessTextureFileName;
		DirectX::XMFLOAT4	baseColor = { 1, 1, 1, 1 };
		DirectX::XMFLOAT3	emissiveColor = { 1, 1, 1 };
		float				metalness = 0.0f;
		float				roughness = 0.0f;
		float				occlusionStrength = 0.0f;
		float				alphaCutoff = 0.5f;
		AlphaMode			alphaMode = AlphaMode::Opaque;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	baseMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	normalMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	emissiveMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	occlusionMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	metalnessRoughnessMap;

		template<class Archive>
		void serialize(Archive& archive);
	};

	struct Vertex
	{
		DirectX::XMFLOAT3		position = { 0, 0, 0 };
		DirectX::XMFLOAT3		normal = { 0, 0, 0 };
		DirectX::XMFLOAT4		tangent = { 0, 0, 0, 1 };
		DirectX::XMFLOAT2		texcoord = { 0, 0 };
		DirectX::XMFLOAT4		boneWeight = { 1, 0, 0, 0 };
		DirectX::XMUINT4		boneIndex = { 0, 0, 0, 0 };

		template<class Archive>
		void serialize(Archive& archive);
	};

	struct Bone
	{
		int						nodeIndex;
		DirectX::XMFLOAT4X4		offsetTransform;
		Node* node = nullptr;

		template<class Archive>
		void serialize(Archive& archive);
	};

	struct Mesh
	{
		std::vector<Vertex>		vertices;
		std::vector<uint32_t>	indices;
		std::vector<Bone>		bones;
		int			nodeIndex = 0;
		int			materialIndex = 0;
		Material* material = nullptr;
		Node* node = nullptr;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>	indexBuffer;

		template<class Archive>
		void serialize(Archive& archive);
	};

	struct VectorKeyframe
	{
		float					seconds;
		DirectX::XMFLOAT3		value;

		template<class Archive>
		void serialize(Archive& archive);
	};

	struct QuaternionKeyframe
	{
		float					seconds;
		DirectX::XMFLOAT4		value;

		template<class Archive>
		void serialize(Archive& archive);
	};

	struct NodeAnim
	{
		std::vector<VectorKeyframe>		positionKeyframes;
		std::vector<QuaternionKeyframe>	rotationKeyframes;
		std::vector<VectorKeyframe>		scaleKeyframes;

		template<class Archive>
		void serialize(Archive& archive);
	};

	struct Animation
	{
		std::string					name;
		float						secondsLength;
		std::vector<NodeAnim>		nodeAnims;

		template<class Archive>
		void serialize(Archive& archive);
	};

	// 各種データ取得
	const std::vector<Mesh>& GetMeshes() const { return meshes; }
	const std::vector<Node>& GetNodes() const { return nodes; }
	const std::vector<Animation>& GetAnimations() const { return animations; }
	const std::vector<Material>& GetMaterials() const { return materials; }

	// 読み込み
	void Load(ID3D11Device* device, const char* filename, float sampleRate);

	// アニメーション追加読み込み
	void AppendAnimations(const char* filename);

	// アニメーションインデックス取得
	int GetAnimationIndex(const char* animationName) const;

	// アニメーション名取得
	const char* GetAnimationName(int animationIndex) const { return animations.at(animationIndex).name.c_str(); }

	// アニメーション最終フレーム取得
	float GetAnimationLength(int animationIndex) const { return animations.at(animationIndex).secondsLength; }

	// ノードインデックス取得
	int GetNodeIndex(const char* name) const;

protected:
	// モデルセットアップ
	void BuildModel(ID3D11Device* device, const char* dirname, std::filesystem::path dirpath);

	// シリアライズ
	void Serialize(const char* filename);

	// デシリアライズ
	void Deserialize(const char* filename);

	// ノードインデックスを取得する
	int FindNodeIndex(NodeId nodeId) const;

protected:
	std::vector<Node> nodes;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<Animation> animations;
};

