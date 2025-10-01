#include "System/Misc.h"
#include "FrameBuffer.h"
#include "Graphics.h"

// コンストラクタ
FrameBuffer::FrameBuffer(ID3D11Device* device, IDXGISwapChain* swapchain)
{
	HRESULT hr = S_OK;

	UINT width, height;

	// レンダーターゲットビューの生成
	{
		// スワップチェーンからバックバッファテクスチャを取得する。
		// ※スワップチェーンに内包されているバックバッファテクスチャは'色'を書き込むテクスチャ。
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		hr = swapchain->GetBuffer(
			0,
			__uuidof(ID3D11Texture2D),
			reinterpret_cast<void**>(texture2d.GetAddressOf())
		);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// バックバッファテクスチャへの書き込みの窓口となるレンダーターゲットビューを生成する。
		// テクスチャに直接書き込みはできないのでビューとして呼ばれる窓口を通して描く必要がある
		hr = device->CreateRenderTargetView(texture2d.Get(), nullptr, renderTargetView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// バックバッファテクスチャからサイズ情報を取得1
		D3D11_TEXTURE2D_DESC texture2dDesc;
		texture2d->GetDesc(&texture2dDesc);

		width = texture2dDesc.Width;
		height = texture2dDesc.Height;
		format = texture2dDesc.Format;
	}

	// ビューポート
	{
		viewport.Width = static_cast<float>(width);
		viewport.Height = static_cast<float>(height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
	}

	// 深度ステンシルビューの生成
	{
		// 深度ステンシル情報を書き込むためのテクスチャを作成する。
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		D3D11_TEXTURE2D_DESC texture2dDesc;
		texture2dDesc.Width = width;
		texture2dDesc.Height = height;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = 1;
		texture2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;
		hr = device->CreateTexture2D(&texture2dDesc, nullptr, texture2d.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// 深度ステンシルテクスチャへの書き込みに窓口になる深度ステンシルビューを作成する。
		hr = device->CreateDepthStencilView(texture2d.Get(), nullptr, depthStencilView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
}

FrameBuffer::FrameBuffer(ID3D11Device* device, UINT width, UINT height)
{
	HRESULT hr = S_OK;

	// レンダーターゲット
	{
		// テクスチャ生成
		Microsoft::WRL::ComPtr<ID3D11Texture2D> renderTragetBuffer;
		D3D11_TEXTURE2D_DESC texture2dDesc{};
		texture2dDesc.Width = width;
		texture2dDesc.Height = height;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = 1;
		// ブルームは光が溢れる演出をするため、色の値が1.0を超える可能性があるので、HDRフォーマットにする
		texture2dDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		// シェーダーリソースビューとして扱えるようにテクスチャを生成する
		texture2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;
		hr = device->CreateTexture2D(&texture2dDesc, 0, renderTragetBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		format = texture2dDesc.Format;

		// レンダーターゲットビュー作成
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
		renderTargetViewDesc.Format = texture2dDesc.Format;
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		hr = device->CreateRenderTargetView(renderTragetBuffer.Get(), &renderTargetViewDesc, renderTargetView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// シェーダーリソースビュー作成
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = texture2dDesc.Format;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		hr = device->CreateShaderResourceView(renderTragetBuffer.Get(), &shaderResourceViewDesc, colorMap.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// デプスステンシル
	{
		// テクスチャ作成
		Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
		D3D11_TEXTURE2D_DESC texture2dDesc{};
		texture2dDesc.Width = width;
		texture2dDesc.Height = height;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = 1;
		texture2dDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;
		hr = device->CreateTexture2D(&texture2dDesc, 0, depthStencilBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// デプスステンシルビュー生成
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Flags = 0;
		hr = device->CreateDepthStencilView(depthStencilBuffer.Get(), &depthStencilViewDesc, depthStencilView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// ビューポート
	{
		viewport.Width = static_cast<float>(width);
		viewport.Height = static_cast<float>(height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
	}
}

void FrameBuffer::Clear(ID3D11DeviceContext* dc, DirectX::XMFLOAT4 colors)
{
	float color[4]{ colors.x, colors.y, colors.z, colors.w };
	// レンダーターゲットビューを通してバックバッファの色をクリアする
	dc->ClearRenderTargetView(renderTargetView.Get(), color);

	// 深度値は1.0にクリアする
	dc->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

// レンダーターゲット設定
void FrameBuffer::SetRenderTargets(ID3D11DeviceContext* dc)
{
	dc->RSSetViewports(1, &viewport);
	// レンダーターゲットビューを通してバックバッファにCGを描く
	dc->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), nullptr);

	// ビューポート＆レンダーターゲットを設定
	dc->RSSetViewports(1, &viewport);
	// 深度ステンシルビューを通して深度テクスチャに深度を書き込む
	dc->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
}
void FrameBuffer::Resize(int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		return;
	}

	// 古いリソースを解放
	renderTargetView.Reset();
	depthStencilView.Reset();
	colorMap.Reset();

	// 新しくテクスチャ作成
	ID3D11Device* device = Graphics::Instance().GetDevice();  // ※必要に応じて参照取得方法を変更

	// カラーマップ用のテクスチャ
	D3D11_TEXTURE2D_DESC texDesc{};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf());
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create texture for color map.");
	}

	// RTV 作成
	hr = device->CreateRenderTargetView(tex.Get(), nullptr, renderTargetView.GetAddressOf());
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create render target view.");
	}

	// SRV 作成
	hr = device->CreateShaderResourceView(tex.Get(), nullptr, colorMap.GetAddressOf());
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create shader resource view.");
	}

	// DepthStencil テクスチャ
	D3D11_TEXTURE2D_DESC depthDesc = texDesc;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTex;
	hr = device->CreateTexture2D(&depthDesc, nullptr, depthTex.GetAddressOf());
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create depth stencil texture.");
	}

	hr = device->CreateDepthStencilView(depthTex.Get(), nullptr, depthStencilView.GetAddressOf());
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create depth stencil view.");
	}

	// ビューポート更新
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

