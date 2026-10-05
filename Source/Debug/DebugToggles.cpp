#include "Debug/DebugToggles.h"

#include <imgui.h>

void DebugToggles::DrawDebugGUI()
{
	if (!ImGui::CollapsingHeader(u8"比較用トグル", ImGuiTreeNodeFlags_DefaultOpen)) return;

	// --- ビヘイビアツリー ---

	ImGui::Checkbox(u8"シーケンス判定による行動キャンセル", &sequenceCancelEnabled);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(
			u8"ON  : シーケンス途中の行動は次へ繋ぐために途中で打ち切る\n"
			u8"OFF : どの行動も最後まで再生してから次へ進む");
	}

	ImGui::Separator();

	// --- テレポート ---

	ImGui::Text(u8"テレポート演出");

	int mode = static_cast<int>(teleportMode);
	ImGui::RadioButton(u8"Full（残像＋軌跡）", &mode, static_cast<int>(TeleportMode::Full));
	ImGui::RadioButton(u8"パーティクルなし（タイミングはそのまま）", &mode, static_cast<int>(TeleportMode::NoParticle));
	ImGui::RadioButton(u8"瞬間移動（演出なし）", &mode, static_cast<int>(TeleportMode::Instant));
	teleportMode = static_cast<TeleportMode>(mode);

	ImGui::Checkbox(u8"テレポート先を画面内に限定", &teleportOnScreenOnly);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(
			u8"ON  : プレイヤーの周囲のうち、画面内に収まる位置だけを選ぶ\n"
			u8"OFF : 画面内判定をせず無条件に選ぶ（画面外へ飛ぶことがある）");
	}

	ImGui::Separator();

	// --- ロックオン中のカメラ ---

	ImGui::Checkbox(u8"位置判定によるカメラ制御", &cameraPositionJudge);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(u8"OFF にすると下の三つの判定をまとめて無効にする");
	}

	ImGui::Indent();

	ImGui::Checkbox(u8"接地状態による切り替え", &cameraAirStateJudge);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(u8"OFF のときは常に「両方地上」の距離ベース補間を使う");
	}

	ImGui::Checkbox(u8"高低差による見上げ／ズームアウト", &cameraHeightDiffJudge);

	ImGui::Checkbox(u8"手前・奥による注視点の寄せ", &cameraFocusOrderJudge);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(u8"OFF のときは単純に二人の中間を見る");
	}

	ImGui::Unindent();
}
