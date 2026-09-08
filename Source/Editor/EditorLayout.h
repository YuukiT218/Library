#pragma once

#include <d3d11.h>
#include <functional>

// ---------------------------------------------------------------------------
// エディタ画面の土台。
//
//   ・画面全体にドックスペースを敷き、各ウィンドウをタイル状に並べられるようにする
//   ・ゲームの描画結果を "Game View" ウィンドウへ ImGui::Image で貼る
//
// 初回は Unity 風の配置を自動で組む。以降の配置は imgui.ini に残る。
// ---------------------------------------------------------------------------
class EditorLayout
{
public:
	static EditorLayout& Instance();

	// コピー禁止
	EditorLayout(const EditorLayout&) = delete;
	void operator=(const EditorLayout&) = delete;

	// ドックスペースを敷く。
	// 他のどのウィンドウよりも先に呼ぶこと（初回の自動配置がここで決まる）。
	void BeginDockSpace();

	// 次のフレームで既定の配置に組み直す
	void RequestDefaultLayout() { rebuildLayout = true; }

	// ゲーム画面をパネルとして描く。
	// sourceWidth / sourceHeight は元の描画解像度で、縦横比の維持に使う。
	// toolbar を渡すと、画面の上に 1 行ぶんの操作列を差し込める（再生ボタンなど）。
	//
	// エディタの主役になる画面なので、閉じるボタンは出していない。
	void DrawGameView(
		ID3D11ShaderResourceView* texture,
		float sourceWidth,
		float sourceHeight,
		const std::function<void()>& toolbar = nullptr);

	// Game View にフォーカスがあるか。
	// ここが真のときだけ、ゲーム側の操作を受け付けるようにする。
	bool IsGameViewFocused() const { return gameViewFocused; }

private:
	EditorLayout() = default;

	// Unity 風の初期配置を組む
	void BuildDefaultLayout(unsigned int dockspaceId);

	bool rebuildLayout = false;
	bool gameViewFocused = false;

	// imgui.ini に配置が残っているかを 1 度だけ確かめるための目印
	bool initialLayoutChecked = false;
};
