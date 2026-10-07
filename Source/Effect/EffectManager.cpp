#include "Graphics/Graphics.h"
#include "EffectManager.h"
#include "GpuParticleSystem.h"

#include <stdlib.h>

namespace
{
	// Effekseerで同時に描画・再生できる最大数
	constexpr int EFFEKSEER_MAX_SPRITE_COUNT = 2048;
	constexpr int EFFEKSEER_MAX_INSTANCE_COUNT = 2048;

	// アクション用GPUパーティクルの最大数
	constexpr int ACTION_PARTICLE_MAX_COUNT = 100000;

	// Effekseerは60fps基準のフレーム数で時間を進める
	constexpr float EFFEKSEER_FRAMES_PER_SECOND = 60.0f;
}

// 初期化
void EffectManager::Initialize()
{
	Graphics& graphics = Graphics::Instance();

	// Effekseerレンダラー生成
	effekseerRenderer = EffekseerRendererDX11::Renderer::Create(
		graphics.GetDevice(),
		graphics.GetDeviceContext(),
		EFFEKSEER_MAX_SPRITE_COUNT);

	// Effekseerマネージャー生成
	effekseerManager = Effekseer::Manager::Create(EFFEKSEER_MAX_INSTANCE_COUNT);

	// Effekseerレンダラー設定
	effekseerManager->SetSpriteRenderer(
		effekseerRenderer->CreateSpriteRenderer());
	effekseerManager->SetRibbonRenderer(
		effekseerRenderer->CreateRibbonRenderer());
	effekseerManager->SetRingRenderer(
		effekseerRenderer->CreateRingRenderer());
	effekseerManager->SetTrackRenderer(
		effekseerRenderer->CreateTrackRenderer());
	effekseerManager->SetModelRenderer(
		effekseerRenderer->CreateModelRenderer());

	// Effekseerローダー設定
	effekseerManager->SetTextureLoader(
		effekseerRenderer->CreateTextureLoader());
	effekseerManager->SetModelLoader(
		effekseerRenderer->CreateModelLoader());
	effekseerManager->SetMaterialLoader(
		effekseerRenderer->CreateMaterialLoader());

	// 左手座標系
	effekseerManager->SetCoordinateSystem(
		Effekseer::CoordinateSystem::LH);

	actionParticles = std::make_unique<GpuParticleSystem>();
	actionParticles->Initialize(
		Graphics::Instance().GetDevice(),
		ACTION_PARTICLE_MAX_COUNT);
	actionParticles->SetRespawnEnable(false);
}

// 終了化
void EffectManager::Finalize()
{
	// シーン側の Effect が先に破棄される前提。
	// 念のため再生中のエフェクトを停止する。
	if (effekseerManager != nullptr)
	{
		effekseerManager->StopAllEffects();
	}

	// D3D11 リソースを、Graphics の破棄前に明示的に解放する。
	actionParticles.reset();
	ambientParticles.reset();

	// Manager が保持している各 Renderer / Loader を先に解放する。
	effekseerManager.Reset();

	// 最後に DX11 レンダラー本体を解放する。
	effekseerRenderer.Reset();
}

// 更新処理
void EffectManager::Update(float elapsedTime)
{
	if (effekseerManager != nullptr)
	{
		effekseerManager->Update(elapsedTime * EFFEKSEER_FRAMES_PER_SECOND);
	}

	if (ambientParticles)
	{
		ambientParticles->Update(
			Graphics::Instance().GetDeviceContext(),
			elapsedTime);
	}

	if (actionParticles)
	{
		actionParticles->Update(
			Graphics::Instance().GetDeviceContext(),
			elapsedTime);
	}
}

// 描画処理
void EffectManager::Render(const RenderContext& rc)
{
	if (effekseerManager != nullptr && effekseerRenderer != nullptr)
	{
		effekseerRenderer->SetCameraMatrix(
			*reinterpret_cast<const Effekseer::Matrix44*>(
				&rc.camera->GetView()));

		effekseerRenderer->SetProjectionMatrix(
			*reinterpret_cast<const Effekseer::Matrix44*>(
				&rc.camera->GetProjection()));

		effekseerRenderer->BeginRendering();
		effekseerManager->Draw();
		effekseerRenderer->EndRendering();
	}

	if (ambientParticles)
	{
		ambientParticles->Render(rc.deviceContext, rc);
	}

	if (actionParticles)
	{
		actionParticles->Render(rc.deviceContext, rc);
	}
}

void EffectManager::StopAllEffects()
{
	if (effekseerManager != nullptr)
	{
		effekseerManager->StopAllEffects();
	}
}

void EffectManager::EmitGpuParticle(
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& velocity,
	const DirectX::XMFLOAT4& color,
	float size,
	float lifeTime,
	GpuParticleBehavior behavior)
{
	if (ambientParticles)
	{
		ambientParticles->Emit(
			Graphics::Instance().GetDeviceContext(),
			position,
			velocity,
			color,
			size,
			lifeTime,
			behavior);
	}

	if (actionParticles)
	{
		actionParticles->Emit(
			Graphics::Instance().GetDeviceContext(),
			position,
			velocity,
			color,
			size,
			lifeTime,
			behavior);
	}
}