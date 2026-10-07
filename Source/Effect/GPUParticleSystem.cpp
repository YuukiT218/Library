
#include "GpuParticleSystem.h"
#include "Graphics/GpuResourceUtils.h"

namespace
{
    // コンピュートシェーダーの1スレッドグループあたりのスレッド数（GPUParticleCS.hlsl の numthreads と合わせる）
    constexpr UINT THREAD_GROUP_SIZE = 256;

    // パーティクルにかかる重力
    const DirectX::XMFLOAT3 PARTICLE_GRAVITY = { 0.0f, -0.1f, 0.0f };
}
#include <vector>
#include <fstream>

HRESULT GpuParticleSystem::Initialize(ID3D11Device* device, int maxParticles)
{
    this->maxParticles = maxParticles;
    HRESULT hr = S_OK;

    // 1. パーティクルの初期データを生成（最初はすべて非アクティブにする）
    std::vector<GpuParticleData> initialData(maxParticles);
    for (int i = 0; i < maxParticles; ++i) {
        initialData[i].lifeTime = -1.0f;
        initialData[i].position = { 0.0f, 0.0f, 0.0f };
        initialData[i].velocity = { 0.0f, 0.0f, 0.0f };
        initialData[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
        initialData[i].size = 1.0f;
    }

    // 2. Structured Buffer (UAV & SRV) の作成
    hr = GpuResourceUtils::CreateStructuredBuffer(
        device,
        sizeof(GpuParticleData),
        maxParticles,
        initialData.data(),
        particleBuffer.GetAddressOf(),
        particleSRV.GetAddressOf(),
        particleUAV.GetAddressOf()
    );
    if (FAILED(hr)) return hr;

    // 3. Compute Shader用 定数バッファの作成
    hr = GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(CbGpuParticleUpdate),
        cbUpdate.GetAddressOf()
    );
    if (FAILED(hr)) return hr;

    hr = GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(CbCamera),
        cbCamera.GetAddressOf()
    );
    if (FAILED(hr)) return hr;

    // 4. シェーダーの読み込み
    // ※ 実際のパスはプロジェクトの設定 (Data/Shader/...cso等) に合わせてください
    GpuResourceUtils::LoadComputeShader(device, "Data/Shader/GpuParticleCS.cso", computeShader.GetAddressOf());
    GpuResourceUtils::LoadVertexShader(device, "Data/Shader/GpuParticleVS.cso", nullptr, 0, nullptr, vertexShader.GetAddressOf()); // 入力レイアウトなし(SV_VertexIDで処理)
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/GpuParticlePS.cso", pixelShader.GetAddressOf());
    GpuResourceUtils::LoadGeometryShader(device, "Data/Shader/GpuParticleGS.cso", geometryShader.GetAddressOf());

    // 5. 加算合成ブレンドステートの作成 (パーティクルをキラキラさせるため)
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE; // 加算
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, blendStateAdd.GetAddressOf());

    // 6. 深度ステートの作成 (パーティクル同士でZファイトしないようにDepth書き込みをオフ)
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 書き込みオフ
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    device->CreateDepthStencilState(&dsDesc, depthStencilState.GetAddressOf());

    // 7. テクスチャの読み込み
    GpuResourceUtils::LoadTexture(device, "Data/Effect/Texture/Particle.png", texture.GetAddressOf());

    // 8. サンプラーステートの作成（バイリニア補間、クランプ）
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());

    return S_OK;
}

void GpuParticleSystem::Update(ID3D11DeviceContext* context, float deltaTime)
{
    if (!computeShader || !particleUAV) return;

    // 定数バッファの更新 (時間や重力の設定)
    CbGpuParticleUpdate cbData;
    cbData.deltaTime = deltaTime;
    cbData.totalTime = totalTime;
    cbData.gravity = PARTICLE_GRAVITY;
    cbData.isRespawn = respawnEnable ? 1 : 0;
    context->UpdateSubresource(cbUpdate.Get(), 0, nullptr, &cbData, 0, 0);

    // Compute Shaderのセットアップ
    context->CSSetShader(computeShader.Get(), nullptr, 0);
    context->CSSetConstantBuffers(0, 1, cbUpdate.GetAddressOf());
    context->CSSetUnorderedAccessViews(0, 1, particleUAV.GetAddressOf(), nullptr);

    // GPUに計算を命令（切り上げて全パーティクルを覆うグループ数にする）
    UINT threadGroupsX = (maxParticles + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE;
    context->Dispatch(threadGroupsX, 1, 1);

    // UAVのバインド解除 (これをしないと次の描画処理でSRVとして読み込めない)
    ID3D11UnorderedAccessView* nullUAV = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
}

void GpuParticleSystem::Render(ID3D11DeviceContext* context, const RenderContext& rc)
{
    if (!vertexShader || !particleSRV) return;

    CbCamera cbCam;
    cbCam.view = rc.camera->GetView();             
    cbCam.projection = rc.camera->GetProjection(); 
    cbCam.cameraPosition = rc.camera->GetEye();    
    cbCam.padding = 0.0f;

    // バッファを更新
    context->UpdateSubresource(cbCamera.Get(), 0, nullptr, &cbCam, 0, 0);

    // ジオメトリシェーダーの定数バッファスロット1 (b1) にセット
    context->GSSetConstantBuffers(1, 1, cbCamera.GetAddressOf());

    // ステートの設定
    context->OMSetBlendState(blendStateAdd.Get(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilState.Get(), 0);

    // トポロジーをポイントリストに設定（GSで四角形に広げます）
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    context->IASetInputLayout(nullptr); // SRVからデータを直接引くので不要

    // シェーダーとSRVのセット
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->GSSetShader(geometryShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);

    // 頂点シェーダーにパーティクルデータを渡す
    context->VSSetShaderResources(0, 1, particleSRV.GetAddressOf());

    // ピクセルシェーダーにテクスチャとサンプラーをセット
    if (texture) {
        context->PSSetShaderResources(0, 1, texture.GetAddressOf());
        context->PSSetSamplers(0, 1, samplerState.GetAddressOf());
    }

    context->Draw(maxParticles, 0);

    // 描画後のクリーンアップ
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->VSSetShaderResources(0, 1, &nullSRV);
    context->GSSetShader(nullptr, nullptr, 0);
}

void GpuParticleSystem::Emit(ID3D11DeviceContext* context, const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT3& velocity, const DirectX::XMFLOAT4& color,
    float size, float lifeTime, GpuParticleBehavior behavior)
{
    if (!particleBuffer) return;

    // GPUに送る新しいパーティクルのデータを作る
    GpuParticleData newData;
    newData.position = position;
    newData.velocity = velocity;
    newData.color = color;
    newData.size = size;
    newData.lifeTime = lifeTime;
    newData.maxLifeTime = lifeTime;
    newData.behaviorType = static_cast<UINT>(behavior);
    newData.padding = { 0.0f, 0.0f };

    // バッファ内のどの部分（何バイト目）を更新するかを指定する
    D3D11_BOX box;
    box.left = emitIndex * sizeof(GpuParticleData);
    box.right = box.left + sizeof(GpuParticleData);
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;

    // GPU上のバッファをピンポイントで上書き！
    context->UpdateSubresource(particleBuffer.Get(), 0, &box, &newData, 0, 0);

    // インデックスを次に進める（最大数に達したら0に戻る）
    emitIndex = (emitIndex + 1) % maxParticles;
}