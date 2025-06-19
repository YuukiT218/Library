#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <memory>
#include "FrameBuffer.h"
#include "RenderState.h"
#include "Debug/PrimitiveRenderer.h"
#include "Debug/ShapeRenderer.h"
#include "Model/ModelRenderer.h"
#include "Shader.h"

enum class FrameBufferId
{
	Display,
	Scene,
	Luminance,
	Bloom,
	GaussianBlur,
	ToneMapping,
	RadialBlur,
	Chromatic,

	EnumCount
};

// グラフィックス
class Graphics
{
private:
	Graphics() = default;
	~Graphics() = default;

public:
	// インスタンス取得
	static Graphics& Instance()
	{
		static Graphics instance;
		return instance;
	}

	// 初期化
	void Initialize(HWND hWnd);

	// クリア
	void Clear(float r, float g, float b, float a);

	// レンダーターゲット設定
	void SetRenderTargets();

	// 画面表示
	void Present(UINT syncInterval);

	// ウインドウハンドル取得
	HWND GetWindowHandle() { return hWnd; }

	// デバイス取得
	ID3D11Device* GetDevice() { return device.Get(); }

	// デバイスコンテキスト取得
	ID3D11DeviceContext* GetDeviceContext() { return immediateContext.Get(); }

	ID3D11RenderTargetView* GetRenderTargetView() { return renderTargetView.Get(); }

	ID3D11DepthStencilView* GetDepthStencilView() { return depthStencilView.Get(); }

	// スクリーン幅取得
	float GetScreenWidth() const { return screenWidth; }

	// スクリーン高さ取得
	float GetScreenHeight() const { return screenHeight; }

	// フレームバッファ取得
	FrameBuffer* GetFrameBuffer(FrameBufferId frameBufferId)
	{
		return frameBuffers[static_cast<int>(frameBufferId)].get();
	}

	// レンダーステート取得
	RenderState* GetRenderState() { return renderState.get(); }

	// シェーダー取得
	Shader* GetShader(ShaderId shaderId) { return shaders[static_cast<int>(shaderId)].get(); }

	// プリミティブレンダラ取得
	PrimitiveRenderer* GetPrimitiveRenderer() const { return primitiveRenderer.get(); }

	// シェイプレンダラ取得
	ShapeRenderer* GetShapeRenderer() const { return shapeRenderer.get(); }

	// モデルレンダラ取得
	ModelRenderer* GetModelRenderer() const { return modelRenderer.get(); }

private:
	HWND											hWnd = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Device>			device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		immediateContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain>			swapchain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	depthStencilView;
	D3D11_VIEWPORT									viewport;

	std::unique_ptr<FrameBuffer> frameBuffers[static_cast<int>(FrameBufferId::EnumCount)];

	float	screenWidth = 0;
	float	screenHeight = 0;

	std::unique_ptr<Shader> shaders[static_cast<int>(ShaderId::EnumCount)];
 	std::unique_ptr<RenderState>					renderState;
	std::unique_ptr<PrimitiveRenderer>				primitiveRenderer;
	std::unique_ptr<ShapeRenderer>					shapeRenderer;
	std::unique_ptr<ModelRenderer>					modelRenderer;
};
