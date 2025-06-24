#pragma once

#include <windows.h>
#include "HighResolutionTimer.h"
#include "Scene/Scene.h"
#include "Input/Input.h"

class Framework
{
public:
	Framework(HWND hWnd);
	~Framework();

private:
	void Update(float elapsedTime);
	void Render(float elapsedTime);

	template<class T>
	void ChangeSceneButtonGUI(const char* name);

	void SceneSelectGUI();

	void CalculateFrameStats();
	void ResizeSceneFramebufferToWindow();
public:
	int Run();
	LRESULT CALLBACK HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	const HWND				hWnd;
	HighResolutionTimer		timer;
	Input					input;
	std::unique_ptr<Scene>	scene;

	enum class SceneType {
		Title,
		Game,
		Edit,
		Result
	};

	SceneType currentSceneType = SceneType::Game;
	SceneType selectedSceneType = SceneType::Game;
};

