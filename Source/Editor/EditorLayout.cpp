#include "EditorLayout.h"

#include "imgui.h"
#include "imgui_internal.h"	// DockBuilder 系（初期配置を組むのに必要）

namespace
{
	// 既定配置で並べるウィンドウ名。ImGui::Begin に渡している名前と一致させること。
	constexpr const char* WINDOW_GAME_VIEW = "Game View";
	constexpr const char* WINDOW_TREE = "Behavior Tree";
	constexpr const char* WINDOW_SCRIPT = "Behavior Script";
	constexpr const char* WINDOW_INSPECTOR = "Behavior Inspector";
	constexpr const char* WINDOW_DEBUG_MENU = "Debug Menu";
}

EditorLayout& EditorLayout::Instance()
{
	static EditorLayout instance;
	return instance;
}

void EditorLayout::BeginDockSpace()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// 画面いっぱいの土台ウィンドウ。背景は描かず、下のゲーム画面を透かす。
	ImGui::SetNextWindowPos(viewport->GetWorkPos());
	ImGui::SetNextWindowSize(viewport->GetWorkSize());
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("##EditorDockHost", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoSavedSettings);

	ImGui::PopStyleVar(3);

	const ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");

	// 組み直しは DockSpace を呼ぶ前に済ませる（ImGui が想定している順序）
	if (rebuildLayout)
	{
		BuildDefaultLayout(dockspaceId);
		rebuildLayout = false;
	}

	// 中央ノードを空のままにできるようにしておくと、
	// Game View を閉じたときに下のゲーム画面がそのまま見える。
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	// imgui.ini に保存された配置は DockSpace が呼ばれて初めて反映されるので、
	// 判定はここ（1 回目の DockSpace のあと）で行う。
	// 何も入っていなければ、次のフレームで初期配置を組む。
	if (!initialLayoutChecked)
	{
		initialLayoutChecked = true;

		const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
		if (node == nullptr || node->IsEmpty()) rebuildLayout = true;
	}

	ImGui::End();
}

void EditorLayout::BuildDefaultLayout(unsigned int dockspaceId)
{
	const ImGuiID root = static_cast<ImGuiID>(dockspaceId);

	ImGui::DockBuilderRemoveNode(root);
	ImGui::DockBuilderAddNode(root, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::DockBuilderSetNodeSize(root, ImGui::GetMainViewport()->GetWorkSize());

	// 中央にゲーム画面、右にインスペクタ、下にツリーとスクリプト
	ImGuiID center = root;
	const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.26f, nullptr, &center);
	const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.42f, nullptr, &center);

	ImGui::DockBuilderDockWindow(WINDOW_GAME_VIEW, center);
	ImGui::DockBuilderDockWindow(WINDOW_TREE, bottom);
	ImGui::DockBuilderDockWindow(WINDOW_SCRIPT, bottom);
	ImGui::DockBuilderDockWindow(WINDOW_INSPECTOR, right);
	ImGui::DockBuilderDockWindow(WINDOW_DEBUG_MENU, right);

	ImGui::DockBuilderFinish(root);
}

void EditorLayout::DrawGameView(
	ID3D11ShaderResourceView* texture,
	float sourceWidth,
	float sourceHeight,
	const std::function<void()>& toolbar)
{
	ImGui::SetNextWindowSize(ImVec2(720.0f, 405.0f), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin(WINDOW_GAME_VIEW, nullptr,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		gameViewFocused = false;
		ImGui::End();
		return;
	}

	// クリックしてフォーカスを当てている間だけ、ゲーム操作を通す（Unity と同じ感覚）
	gameViewFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (toolbar)
	{
		toolbar();
		ImGui::Separator();
	}

	const ImVec2 avail = ImGui::GetContentRegionAvail();

	if (texture == nullptr || avail.x < 8.0f || avail.y < 8.0f || sourceHeight <= 0.0f)
	{
		ImGui::TextDisabled("Game view is too small");
		ImGui::End();
		return;
	}

	// 縦横比を保ったまま、パネルに収まる最大サイズを求める
	const float sourceAspect = sourceWidth / sourceHeight;
	const float panelAspect = avail.x / avail.y;

	ImVec2 drawSize = avail;
	if (panelAspect > sourceAspect)
	{
		drawSize.x = avail.y * sourceAspect;
	}
	else
	{
		drawSize.y = avail.x / sourceAspect;
	}

	// 余った分だけ中央に寄せる
	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	ImGui::SetCursorScreenPos(ImVec2(
		cursor.x + (avail.x - drawSize.x) * 0.5f,
		cursor.y + (avail.y - drawSize.y) * 0.5f));

	ImGui::Image(reinterpret_cast<void*>(texture), drawSize);

	ImGui::End();
}
