#pragma once

// ---------------------------------------------------------------------------
// 比較動画の撮影用に、ゲームを起動したまま演出や制御を切り替えるためのフラグ置き場。
//
// 「入れる前」と「入れた後」を同じプレイの中で撮り比べたいので、
// ビルドを分けずに ImGui から切り替えられるようにしている。
// ---------------------------------------------------------------------------
class DebugToggles
{
public:
	// テレポートの見せ方
	enum class TeleportMode
	{
		Full,        // 残像の粒子分解＋軌跡パーティクル（本来の演出）
		NoParticle,  // フェーズのタイミングはそのまま、パーティクルだけ止める
		Instant,     // フェーズを踏まず、その場で目的地へ飛ぶ
	};

	// 唯一のインスタンス取得
	static DebugToggles& Instance()
	{
		static DebugToggles instance;
		return instance;
	}

	// シーケンスノード判定による行動キャンセルを行うか
	//
	// OFF にすると BehaviorData::IsInSequenceAndNotLast() が常に false を返すようになり、
	// シーケンス途中のノードでも行動が打ち切られず最後まで再生される。
	bool IsSequenceCancelEnabled() const { return sequenceCancelEnabled; }
	void SetSequenceCancelEnabled(bool enable) { sequenceCancelEnabled = enable; }

	TeleportMode GetTeleportMode() const { return teleportMode; }
	void SetTeleportMode(TeleportMode mode) { teleportMode = mode; }

	// 残像・軌跡パーティクルを出すか
	bool IsTeleportParticleEnabled() const { return teleportMode == TeleportMode::Full; }

	// フェーズを踏まずに瞬間移動するか
	bool IsTeleportInstant() const { return teleportMode == TeleportMode::Instant; }

	// テレポート先を画面内に収まる位置だけから選ぶか
	//
	// OFF にすると画面内判定を行わず、プレイヤーの周囲から無条件に選ぶため、
	// 画面外へ飛んでボスを見失うことがある。
	bool IsTeleportDestinationOnScreenOnly() const { return teleportOnScreenOnly; }
	void SetTeleportDestinationOnScreenOnly(bool enable) { teleportOnScreenOnly = enable; }

	// --- ロックオン中のカメラ制御 ---
	//
	// 敵とプレイヤーの位置関係を見てカメラを寄せる／引く処理を、
	// 判定の種類ごとに個別に切れるようにしている。
	// 親を OFF にすると三つまとめて無効になる。

	// 位置判定によるカメラ制御を行うか（親スイッチ）
	bool IsCameraPositionJudgeEnabled() const { return cameraPositionJudge; }
	void SetCameraPositionJudgeEnabled(bool enable) { cameraPositionJudge = enable; }

	// 接地状態（どちらが空中か）でカメラパラメータを切り替えるか
	// OFF のときは常に「両方地上」の距離ベース補間を使う
	bool IsCameraAirStateJudgeEnabled() const { return cameraPositionJudge && cameraAirStateJudge; }

	// 高低差に応じた見上げ／見下ろしとズームアウトを行うか
	bool IsCameraHeightDiffJudgeEnabled() const { return cameraPositionJudge && cameraHeightDiffJudge; }

	// カメラから見た手前・奥を判定して注視点を寄せるか
	// OFF のときは単純に二人の中間を見る
	bool IsCameraFocusOrderJudgeEnabled() const { return cameraPositionJudge && cameraFocusOrderJudge; }

	// デバッグ ImGui 描画
	void DrawDebugGUI();

private:
	bool sequenceCancelEnabled = true;
	TeleportMode teleportMode = TeleportMode::Full;
	bool teleportOnScreenOnly = true;

	bool cameraPositionJudge = true;
	bool cameraAirStateJudge = true;
	bool cameraHeightDiffJudge = true;
	bool cameraFocusOrderJudge = true;
};
