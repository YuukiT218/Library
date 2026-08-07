#include "SceneManager.h"

// 更新処理
void SceneManager::Update(float elapsedTime)
{
	if (nextScene != nullptr)
	{
		Clear();

		currentScene = std::move(nextScene);

		if (!currentScene->IsReady())
		{
			currentScene->Initialize();
		}
	}

	if (currentScene != nullptr)
	{
		currentScene->Update(elapsedTime);
	}
}

// 描画処理
void SceneManager::Render(float elapsedTime)
{
	if (currentScene != nullptr)
	{
		currentScene->Render(elapsedTime);
	}
}

// シーンクリア
void SceneManager::Clear()
{
	if (currentScene != nullptr)
	{
		currentScene->Finalize();
		currentScene.reset();
	}
}

// シーン切り替え
void SceneManager::ChangeScene(Scene* scene)
{
	// すでに保留中のシーンがあれば unique_ptr の reset により確実に破棄される。
	nextScene.reset(scene);
}