#pragma once

#include <memory>

#include "Scene.h"

// シーンマネージャー
class SceneManager
{
private:
	SceneManager() = default;
	~SceneManager() = default;

public:
	static SceneManager& Instance()
	{
		static SceneManager instance;
		return instance;
	}

	void Update(float elapsedTime);
	void Render(float elapsedTime);

	// 現在のシーンだけを終了・破棄する
	void Clear();

	// scene の所有権を SceneManager に移す
	void ChangeScene(Scene* scene);

	Scene* GetCurrentScene()
	{
		return currentScene.get();
	}

private:
	std::unique_ptr<Scene> currentScene;
	std::unique_ptr<Scene> nextScene;
};