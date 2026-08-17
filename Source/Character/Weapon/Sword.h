#pragma once

#include "Weapon.h"

class Sword : public Weapon
{
public:
	Sword(ID3D11Device* device, const char* filename);

	void Update(float elapsedTime) override;

	void Render(const RenderContext& rc, ShaderId shaderId) override;

	void DrawDebugGUI();
};