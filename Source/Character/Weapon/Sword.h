#pragma once

#include "Weapon.h"

class Sword : public Weapon
{
public:
	Sword(ID3D11Device* device, const char* filename);
	~Sword();

	void Update(float elapsedTime) override;

	void Render(const RenderContext& rc, Shader* shader) override;

	void DrawDebugImGUi();
};