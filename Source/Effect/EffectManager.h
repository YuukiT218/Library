#pragma once

#include <DirectXMath.h>
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>
#include "GPUParticleSystem.h"
#include <memory>

// エフェクトマネージャー
class EffectManager
{
private:
	EffectManager() {}
	~EffectManager() {}

public:
	// 唯一のインスタンス取得
	static EffectManager& Instance()
	{
		static EffectManager instance;
		return instance;
	}

	// 初期化
	void Initialize();

	// 終了化
	void Finalize();

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc);

	void StopAllEffects();

	// パーティクル発生 
	void EmitGpuParticle(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity,
		const DirectX::XMFLOAT4& color, float size, float lifeTime, GpuParticleBehavior behavior);

	// Effekseerマネージャーの取得
	Effekseer::ManagerRef GetEffekseerManager() { return effekseerManager; }

private:
	Effekseer::ManagerRef effekseerManager;
	EffekseerRenderer::RendererRef effekseerRenderer;
	std::unique_ptr<GpuParticleSystem> ambientParticles;
	std::unique_ptr<GpuParticleSystem> actionParticles;
};