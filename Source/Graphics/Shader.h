#pragma once

#include "RenderContext.h"
#include "Model/Model.h"

class Shader
{
public:
	Shader() {}
	virtual ~Shader() {}

	// 開始処理
	virtual void Begin(const RenderContext& rc) = 0;

	// 更新処理
	virtual void Update(const RenderContext& rc, const ModelResource::Mesh& mesh) = 0;
	virtual void Update(const RenderContext& rc, const std::shared_ptr<Model> model) = 0;

	// 終了処理
	virtual void End(const RenderContext& rc) = 0;

protected:
	void ClearShaderResourceViews(int startSlot, ID3D11DeviceContext* dc)
	{
		ID3D11ShaderResourceView* clear_shader_resource_view[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{};
		dc->VSSetShaderResources(startSlot, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
		dc->HSSetShaderResources(startSlot, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
		dc->DSSetShaderResources(startSlot, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
		dc->GSSetShaderResources(startSlot, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
		dc->PSSetShaderResources(startSlot, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
		dc->CSSetShaderResources(startSlot, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
	}

	void ClearShaderSlots(ID3D11DeviceContext* dc)
	{
		dc->VSSetShader(nullptr, nullptr, 0);
		dc->HSSetShader(nullptr, nullptr, 0);
		dc->DSSetShader(nullptr, nullptr, 0);
		dc->GSSetShader(nullptr, nullptr, 0);
		dc->PSSetShader(nullptr, nullptr, 0);
		dc->CSSetShader(nullptr, nullptr, 0);
	}

	bool	IsDraw = false;
};
