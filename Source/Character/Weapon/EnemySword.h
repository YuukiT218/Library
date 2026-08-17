#pragma once

#include "Weapon.h"

class EnemySword : public Weapon
{
public:
	EnemySword(ID3D11Device* device, const char* filename);

	void Update(float elapsedTime) override;

	void Render(const RenderContext& rc, ShaderId shaderId) override;

	void DrawDebugGUI();
};