#pragma once

#include <d3d11.h>

// GPUリソースユーティリティ
class GpuResourceUtils
{
public:
	// 頂点シェーダー読み込み
	static HRESULT LoadVertexShader(
		ID3D11Device* device,
		const char* filename,
		const D3D11_INPUT_ELEMENT_DESC inputElementDescs[],
		UINT inputElementCount,
		ID3D11InputLayout** inputLayout,
		ID3D11VertexShader** vertexShader);

	// ピクセルシェーダー読み込み
	static HRESULT LoadPixelShader(
		ID3D11Device* device,
		const char* filename,
		ID3D11PixelShader** pixelShader);

	// コンピュートシェーダー読み込み
	static HRESULT LoadComputeShader(
		ID3D11Device* device,
		const char* filename,
		ID3D11ComputeShader** computeShader);

	// ジオメトリシェーダー読み込み
	static HRESULT LoadGeometryShader(
		ID3D11Device* device,
		const char* filename,
		ID3D11GeometryShader** geometryShader);

	// テクスチャ読み込み
	static HRESULT LoadTexture(
		ID3D11Device* device,
		const char* filename,
		ID3D11ShaderResourceView** shaderResourceView,
		D3D11_TEXTURE2D_DESC* texture2dDesc = nullptr);

	// テクスチャ読み込み
	static HRESULT LoadTexture(
		ID3D11Device* device,
		const void* data,
		size_t size,
		ID3D11ShaderResourceView** shaderResourceView,
		D3D11_TEXTURE2D_DESC* texture2dDesc = nullptr);

	// ダミーテクスチャ作成
	static HRESULT CreateDummyTexture(
		ID3D11Device* device,
		UINT color,
		ID3D11ShaderResourceView** shaderResourceView,
		D3D11_TEXTURE2D_DESC* texture2dDesc = nullptr);

	// 定数バッファ作成
	static HRESULT CreateConstantBuffer(
		ID3D11Device* device,
		UINT bufferSize,
		ID3D11Buffer** constantBuffer);

	// 構造化バッファ (Structured Buffer) と UAV / SRV の作成
	static HRESULT CreateStructuredBuffer(
		ID3D11Device* device,
		UINT elementSize,
		UINT elementCount,
		const void* initData,
		ID3D11Buffer** buffer,
		ID3D11ShaderResourceView** srv,
		ID3D11UnorderedAccessView** uav);
};
