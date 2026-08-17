#pragma once

#include "Camera/Camera.h"
#include "RenderState.h"
#include "Light.h"
#include "ShadowMap.h"

struct RenderContext
{
	ID3D11DeviceContext*	deviceContext;
	const RenderState*		renderState;
	const Camera*			camera;
	const LightManager*		lightManager = nullptr;
	const ShadowMap*		shadowMap;

	float timer;
	DirectX::XMFLOAT3 targetPosition;
	bool enableWallTransparency;
};
