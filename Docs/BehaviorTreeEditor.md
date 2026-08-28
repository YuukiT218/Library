# 敵の行動パターンエディタ

敵（EnemyBoss）のビヘイビアツリーを、ノードを繋ぐ操作で組み立てるためのエディタ。
行動の中身は Lua スクリプトで書けて、保存するとゲームを止めずに差し替わる。

---

## 全体像

```
Data/Json/BehaviorTree_EnemyBoss.json     ツリーの形・各ノードの設定
Data/Script/Enemy/EnemyBoss/*.lua         行動の中身
Data/Json/AnimationConfigs_EnemyBoss.json アニメーションごとの速度カーブ・イベント・属性（既存）
```

起動時に `EnemyBoss` が JSON を読んでツリーを組み立てる。
JSON が無ければ、これまで C++ に直接書かれていた構成が書き出される（初回起動で自動生成）。
JSON が壊れていた場合も、既定の構成へ戻して棒立ちにならないようにしてある。

---

## ウィンドウ

`SceneGame` と `SceneEdit` のどちらでも開く。ドッキング可能な 3 枚組。

| ウィンドウ | 役割 |
|---|---|
| **Behavior Tree** | ノードグラフ。ツリーの形を作る。実行中のノードが緑で光る |
| **Behavior Inspector** | 選択中のノードの設定。判定・行動・アニメーション・キャンセル区間・パラメータ |
| **Behavior Script** | Lua のコードエディタ。保存すると即反映。エラーとログもここ |

### ツールバー

| ボタン | 動作 |
|---|---|
| 適用 | 編集内容で実行中のツリーを組み直す（ファイルには書かない） |
| 保存 | 検証 → JSON へ保存 → 適用 |
| ファイルから再読込 | JSON を読み直して適用。編集を捨てたいとき |
| 既定の構成に戻す | C++ 由来の初期構成に戻す（未保存状態になる） |

「適用」と「保存」を分けてあるのは、壊れた編集途中の状態がそのままゲームに流れ込まないようにするため。

---

## 操作

- **ノードを繋ぐ** … 親の `out ->` から子の `-> in` へドラッグ。輪になる繋ぎ方は弾かれる
- **繋ぎを切る** … リンクを選んで Delete。親なしノードになるので、検証で「ルートが複数」と出る
- **ノード追加** … ノードを選んでツールバーの「ノード追加」、またはノード右クリック
- **ノード削除** … 選んで Delete。子孫ごと消える
- **位置** … ドラッグした位置は JSON に保存される

### 選択ルール（子の選び方）

| ルール | 動作 |
|---|---|
| `Non` | 末端。子を持たないノード |
| `Priority` | 実行できる子のうち、優先度の数値が **小さい** ものを選ぶ |
| `Sequence` | 子を上から順に 1 つずつ実行する。最後まで行くと終了 |
| `SequentialLooping` | `Sequence` と同じだが、最後まで行くと先頭へ戻る |
| `Random` | 実行できる子からランダムに選ぶ（直前と同じものは避ける） |

`Sequence` の実行順は **グラフ上の位置ではなく JSON の並び順** で決まる。
並びは「そのノードを子として追加した順」なので、順番を変えたいときは一度切って繋ぎ直す。

---

## 行動の種類

インスペクタの「行動 > 種類」で選ぶ。

- **なし（中間ノード）** … 子ノードを選ぶだけ。行動は持たない
- **Native (C++)** … 既存の C++ 行動クラス。`EnemyBossBehavior::RegisterBehaviors()` に登録されたものが並ぶ
- **Lua** … スクリプトで書いた行動

C++ 行動をそのまま使い続けながら、一部だけ Lua に置き換えられる。

新しい C++ 行動クラスを増やしたら、
`Source/Character/Enemy/BehaviorTree/EnemyBossBehavior.cpp` の `RegisterBehaviors()` に
`REGISTER_BEHAVIOR_ACTION(EnemyBoss, クラス名);` を 1 行足すとエディタの一覧に出る。

---

## アニメーションとキャンセル区間

インスペクタの「アニメーション」で、そのノードが再生するクリップを**順番に**並べる。
**この設定が効くのは Lua 行動のみ**（C++ 行動は自前でアニメーション番号を持っているため）。

クリップごとに設定できるもの:

| 項目 | 意味 |
|---|---|
| クリップ | アニメーション名 |
| ループ | ループ再生するか |
| ルートモーション | ルートモーションを適用するか |
| ブレンド時間 | 直前のアニメーションからの繋ぎ時間（秒） |
| 再生速度 | 再生速度の倍率 |

### キャンセル受付区間

連続行動で「今のアニメーションをどこで切って次へ移るか」を決める部分。

既定では、そのアニメーションの `AnimationConfig`
（＝イベントシーケンサーの先行入力受付 `advanceInputStartFrame` / `advanceInputEndFrame`）を使う。

「このノードだけ別の値にする」を入れると、**そのノード専用の値**で上書きできる。
同じ斬りモーションでも、単発で使うときと 4 連コンボの 2 段目で使うときとで
繋ぐタイミングを変えたい、という場合はここを使う。

スクリプト側からは

- `Boss.IsPastCancelStart(i)` … i 番目のクリップのキャンセル受付**開始**を過ぎたか
- `Boss.IsPastCancelEnd(i)` … 受付**終了**を過ぎたか（＝次の一撃へ差し替える合図）

で参照する。

### イベント / 属性

インスペクタの「イベント / 属性」を開くと、選択中クリップの `AnimationConfig` の
イベントと属性（攻撃判定・無敵など）を確認・追加・削除できる。
細かい時間調整は既存の**イベントシーケンサー**の方がやりやすい。
どちらもアニメーション単位の設定なので、同じ実体を編集している。

---

## パラメータ

ノードごとに Float / Int / Bool / String の値を持たせられる。
スクリプトからは `self.node.params.<名前>` で読める。

同じスクリプトを複数ノードで使い回して、数値だけノードごとに変える、という使い方をする。

---

## Lua スクリプトの書き方

テーブルを 1 つ `return` するファイルにする。

```lua
local M = {}

function M:Run(dt)
    -- self       : この行動インスタンス専用の作業領域
    -- self.node  : エディタで設定した内容（name / params / clips）
    -- Boss.*     : EnemyBoss の操作

    if self.step == nil then self:OnReset() end

    if self.step == 0 then
        Boss.PlayClip(1)          -- インスペクタの 1 番目のクリップ
        self.step = 1

    elseif self.step == 1 then
        Boss.TurnToTarget(dt, Boss.TURN_INSTANT)
        Boss.UpdateAttackCollision()
        if Boss.IsPastCancelEnd(1) then
            self.step = 2
        end

    elseif self.step == 2 then
        if not Boss.IsPlayingAnimation() then
            return "complete"
        end
    end

    if Boss.IsInterrupted() then
        return "failed"
    end
    return "run"
end

function M:OnReset()
    self.step = 0
end

return M
```

- 戻り値は `"run"`（続行） / `"complete"`（成功） / `"failed"`（中断）。省略すると `"run"`
- `M:OnReset()` は行動が終わって片付けるときに C++ 側から呼ばれる（任意）
- `self` にはスクリプト本体がメタテーブル経由で繋がっているので、`self:自分の関数()` が使える
- クリップ番号は Lua に合わせて **1 始まり**

見本は `Data/Script/Enemy/EnemyBoss/SlashCombo.lua` と `SlashWave.lua`。

---

## Boss API

### アニメーション

| 関数 | 説明 |
|---|---|
| `Boss.PlayClip(i)` | i 番目のクリップを再生（ループ・ブレンド・速度も反映） |
| `Boss.GetClipCount()` | クリップ本数 |
| `Boss.PlayAnimation(名前 or 番号, loop, blend)` | 直接指定して再生 |
| `Boss.PlayRootMotion(名前 or 番号, loop, blend)` | ルートモーション付きで再生 |
| `Boss.GetAnimationIndex(名前)` | アニメーション番号 |
| `Boss.GetAnimationSeconds()` | 再生位置（秒） |
| `Boss.IsPlayingAnimation()` | 再生中か |
| `Boss.SetAnimationSpeed(倍率)` | 再生速度 |

### キャンセル受付

| 関数 | 説明 |
|---|---|
| `Boss.GetCancelStart(i)` / `GetCancelEnd(i)` | i 番目のクリップの受付区間（秒） |
| `Boss.IsPastCancelStart(i)` / `IsPastCancelEnd(i)` | その時間を過ぎたか |
| `Boss.GetCurrentCancelStart()` / `GetCurrentCancelEnd()` | 今再生中のアニメーション自身の設定値 |

### 位置・向き

| 関数 | 説明 |
|---|---|
| `Boss.GetPosition()` / `SetPosition(x,y,z)` | 位置（取得は 3 値を返す） |
| `Boss.GetAngle()` | 角度 |
| `Boss.GetPlayerPosition()` / `GetDistanceToPlayer()` | プレイヤーの位置・距離 |
| `Boss.SetTargetToPlayer()` / `SetTargetPosition(x,y,z)` | 追従対象 |
| `Boss.TurnToTarget(dt, speed)` | 旋回。speed は `Boss.TURN_INSTANT` / `TURN_FAST` / `TURN_SLOW` |
| `Boss.MoveToTarget(dt, speedRate)` | 目標地点へ移動 |
| `Boss.IsNearPlayer(ex,ey,ez)` / `LerpTowardPlayer(t)` | 間合い判定・間合い詰め |
| `Boss.GetForward()` / `GetBack()` / `GetLeft()` / `GetRight()` | 各方向ベクトル |
| `Boss.SetVerticalVelocity(v)` / `SetWarpPosition()` | 落下速度・ワープ位置 |

### 戦闘・状態

| 関数 | 説明 |
|---|---|
| `Boss.UpdateAttackCollision()` | 武器の攻撃判定更新 |
| `Boss.IsInterrupted()` | 被弾か死亡で中断すべきか |
| `Boss.IsAnyDamage()` / `IsDead()` | 被弾中・死亡 |
| `Boss.GetHealth()` / `GetMaxHealth()` / `IsHealthBelowRate(r)` | 体力 |
| `Boss.SetSuperArmor(b)` / `IsSuperArmor()` | スーパーアーマー |
| `Boss.GetRunTimer()` / `SetRunTimer(t)` | 汎用タイマー |
| `Boss.GetRevengeValue()` / `ResetRevengeValue()` / `SetRevengeState(b)` / `GetRevengeState()` | 反撃値 |
| `Boss.SetSpecialReady(b)` / `GetSpecialReady()` | 必殺技の使用可否 |
| `Boss.SetPlayedEffect(b)` / `GetPlayedEffect()` | エフェクト再生済みフラグ |
| `Boss.GetAttackRange()` / `GetMoveSpeed()` / `GetBlendSeconds()` | 各種設定値 |

### シーケンス（連続行動）

| 関数 | 説明 |
|---|---|
| `Boss.IsInSequence()` | シーケンス実行中か |
| `Boss.IsLastNodeInSequence()` | シーケンスの最後のノードか |
| `Boss.IsInSequenceAndNotLast()` | シーケンス中で、まだ後続があるか |

### エフェクト・発射物

| 関数 | 説明 |
|---|---|
| `Boss.PlayEffect(名前, x, y, z, scale)` | `LightBall` / `Teleport` / `AttackSign` / `MagicCircle` |
| `Boss.LaunchSlashWave{ speed=, offset=, homing=, ... }` | 斬撃波を撃つ |
| `Boss.SpawnLightPillarCircle{ count=, radius=, ... }` | 光柱を円状に配置（行動側に溜まる） |
| `Boss.SpawnSideLightPillars()` | 左右後方に光柱を 2 本 |
| `Boss.FireStoredProjectiles(speed, homing)` | 溜めた発射物をまとめて撃つ |
| `Boss.GetStoredProjectileCount()` | 溜まっている数 |

### その他

| 関数 | 説明 |
|---|---|
| `Boss.Log(文字列)` | Behavior Script ウィンドウのログへ出す |

API を増やしたいときは `Source/Script/LuaBossBindings.cpp` に関数を書いて、
末尾の `BOSS_FUNCTIONS` テーブルに 1 行足す。

---

## ホットリロード

「ホットリロード」を有効にしておくと、0.25 秒ごとに更新を見に行く。

- **.lua** … 更新されると読み直される。エディタで保存した場合はその場で即反映
  - `self`（行動の途中経過）は保持されるので、動作中でも差し替わる
  - 文法エラーのときはエラーを表示して**直前の動作を止める**（その行動は `failed` を返す）。直せば次のフレームで復帰する
- **BehaviorTree_EnemyBoss.json** … 外部エディタで書き換えた場合も取り込んで組み直す
  - ただしエディタ側に未保存の変更があるときは、上書きせず編集内容を優先する

---

## ソース構成

| ファイル | 役割 |
|---|---|
| `Source/Character/Enemy/BehaviorTree/BehaviorTreeAsset.*` | JSON に落とす形のツリー定義と検証 |
| `Source/Character/Enemy/BehaviorTree/BehaviorRegistry.h` | 名前 → C++ 行動／判定クラス |
| `Source/Character/Enemy/BehaviorTree/BehaviorTreeBuilder.h` | アセット → 実行時ツリー |
| `Source/Character/Enemy/BehaviorTree/EnemyBossBehavior.*` | EnemyBoss 用の登録と組み立て |
| `Source/Character/Enemy/BehaviorTree/EnemyBossDefaultTree.cpp` | 既定のツリー構成（データのみ） |
| `Source/Script/LuaScriptSystem.*` | Lua の読み込み・ホットリロード・エラー収集 |
| `Source/Script/LuaAction.*` | Lua で書かれた行動 |
| `Source/Script/LuaBossBindings.*` | `Boss.*` API |
| `Source/Editor/BehaviorTreeEditor.*` | エディタ UI |
| `External/lua-5.4.7/` | Lua 本体（C++ としてビルド） |
| `External/imgui-node-editor/` | ノードグラフ UI |

### 外部ライブラリについて

- **Lua 5.4.7** は C ではなく **C++ としてコンパイル**している。
  こうするとスクリプトのエラーが longjmp ではなく C++ 例外で巻き戻るので、
  バインディング側の C++ オブジェクトが正しく破棄される。
  そのため `lua.hpp`（`extern "C"` 付き）は使わず、`lua.h` を直接 include すること。
- **imgui-node-editor** はこのプロジェクトの ImGui 1.80 に合わせて 3 か所だけ手を入れてある
  （`imgui_node_editor.cpp` 内、`[PATCH: ImGui 1.80 compatibility]` のコメント付き）。
  ライブラリを更新するときはそこを見直す。
