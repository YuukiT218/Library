#pragma once

#include <memory>

#include "Sprite/Sprite.h"
#include "Model/Model.h"
#include "Scene.h"

class SceneTitle : public Scene
{
public:
	SceneTitle();
	~SceneTitle() override {}

	//‰Šú‰»
	void Initialize() override;

	//I—¹‰»
	void Finalize() override;

	//XVˆ—
	void Update(float elapsedTime) override;

	//•`‰æˆ—
	void Render(float elapsedTime) override;

private:
	std::unique_ptr<Sprite> sprite = nullptr;
	std::shared_ptr<Model> character = nullptr;
};

