#pragma once

#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>

class FrameBuffer
{
public:
	FrameBuffer(ID3D11Device* device, IDXGISwapChain* swapchain);

	FrameBuffer(ID3D11Device* device, UINT width, UINT height);

	// カラーマップ取得
	ID3D11ShaderResourceView* GetColorMap() const { return colorMap.Get(); }

	// クリア
	void Clear(ID3D11DeviceContext* dc, DirectX::XMFLOAT4 color);

	// レンダーターゲット設定
	void SetRenderTargets(ID3D11DeviceContext* dc);

	void Resize(int width, int height);

private:
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		depthStencilView;
	D3D11_VIEWPORT										viewport;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	colorMap;
	DXGI_FORMAT                                         format = DXGI_FORMAT_R8G8B8A8_UNORM;
};