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

エディタ機能はすべて **`SceneEdit`**（メニューバーの Scene Selector → Edit Scene）に集約されている。
`SceneGame` は素のゲームで、エディタ関連の表示は出ない。

開くと最初から Unity 風のドックレイアウトになる。

| 位置 | ウィンドウ | 役割 |
|---|---|---|
| 中央 | **Game View** | ゲーム画面。上に再生操作が並ぶ |
| 下 | **Behavior Tree** | ノードグラフ。ツリーの形を作る。実行中のノードが緑で光る |
| 下（タブ） | **Behavior Script** | Lua のコードエディタ。保存すると即反映。エラーとログもここ |
| 右 | **Behavior Inspector** | 選択中ノードの設定。判定・行動・アニメーション・キャンセル区間・パラメータ |
| 右（タブ） | Debug Menu | 既存のデバッグメニュー |

配置は `imgui.ini` に保存される。崩れたら Game View 上部の「配置リセット」で組み直せる。

### 編集中と再生中

| 状態 | 更新される内容 |
|---|---|
| **編集中** | アニメーション（`EditUpdate`）とエフェクトのみ。ビヘイビアツリーは止まっている |
| **再生中** | ゲームと同じ更新。ツリー・当たり判定・発射物・ヒットストップがすべて回る |

カメラもモードに合わせて切り替わる。

| 状態 | カメラ |
|---|---|
| 編集中 | `EditCameraController`。選択中のキャラを中心に回り込んで見られる |
| 再生中 | `CameraController`。ゲーム本編と同じ追従カメラ（ロックオンも効く） |

切り替え時は `InitCamera()` で今の視点を引き継ぐので画面が飛ばない。
再生中のロックオン切り替えは右ショルダー / マウス中ボタン（Game View にフォーカスがあるときのみ）。

Game View 上部のボタン:

| ボタン | 動作 |
|---|---|
| 再生 / 停止 | 押すたびにキャラクターを作り直し、初期状態から始める |
| 一時停止 / 再開 | 再生中に止める。決めの瞬間を確認したいとき |
| リセット | 位置・体力・行動を初期状態へ戻す |
| 速度 | 0.05x 〜 2.0x。遅くすると繋ぎのフレームを追いやすい |

再生開始・停止・リセットのタイミングで、**エディタで編集中のツリーが作り直したボスへ自動で反映される**ので、
保存していない編集内容もそのまま試せる。

#### プレイヤーを操作するには Game View をクリックする

`Player::Update` は ImGui がマウス／キーボードを掴んでいる間は入力を無視する。
エディタは画面全体がドックスペースなのでカーソルは常に ImGui の上にあり、そのままでは永久に操作できない。

そのため **Game View にフォーカスがある間だけ入力を通す**ようにしてある（Unity と同じ感覚）。
操作したいときは Game View を一度クリックする。ツールバーの「操作:」欄で今どちらかを確認できる。

#### 「戦闘開始」ボタン

ボスはプレイヤーが索敵範囲（20.0）に入るまで `Battle` に入らず、`Scout → Idle` で立ち尽くす。
初期配置ではプレイヤーとの距離が約 21.5 あるため、**再生しただけでは何も起きない**。

近づいて戦闘を始めてもよいが、攻撃行動をすぐ試したいときは「戦闘開始」を押すと
索敵を飛ばして戦闘状態に入る。ツールバーには実行中のノード名と距離も出るので、
何も起きないときはそこを見れば理由が分かる。

カメラは再生中も `EditCameraController` のままなので、選択中のキャラクターを中心に自由に見回せる。

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

見本は `Data/Script/Enemy/EnemyBoss/` の 3 本。

| ファイル | 内容 |
|---|---|
| `SlashCombo.lua` | 連撃とキャンセルの繋ぎ |
| `SlashWave.lua` | 決まったタイミングで飛び道具を撃つ |
| `CloseTeleport.lua` | プレイヤーのそばへ瞬間移動する |

### Lua の最低限

| 書き方 | 意味 |
|---|---|
| `local x = 1` | 局所変数。`local` を付け忘れるとグローバルになるので必ず付ける |
| `nil` | 未設定。未定義の変数を読むとこれになる |
| `if a then ... elseif b then ... else ... end` | 条件分岐。`end` で閉じる |
| `a ~= b` | 「等しくない」。C 系の != ではない |
| `not` / `and` / `or` | 論理演算。C 系の記号は使えない |
| `false` と `nil` だけが偽 | **0 も空文字も真**。C++ と違うので注意 |
| `t = { x = 1 }` から `t.x` | テーブル（連想配列）。オブジェクト代わりに使う |
| `t = { 10, 20 }` から `t[1]` | 配列も同じテーブル。添字は **1 始まり** |
| `#t` | テーブルの要素数 |
| `function f() return 1, 2 end` | 戻り値を複数返せる。`local a, b = f()` で受ける |
| `function M:Run(dt)` | コロンを使うと第 1 引数に `self` が自動で入る |
| `-- コメント` | 行コメント |

`self` はこの行動インスタンス専用のテーブル。`self.step` のように好きな名前で状態を持てて、
その値は次のフレームまで残る。行動が終わったら `OnReset` で初期化する、という使い方をする。

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

### テレポート

| 関数 | 説明 |
|---|---|
| `Boss.CalculateTeleportPosition(distance, bakeY)` | プレイヤーの周囲 `distance` から、画面内に入る位置を 1 つ選んで x,y,z を返す |
| `Boss.StartTeleport(x, y, z, fadeSeconds)` | その位置へテレポートを開始する |
| `Boss.IsTeleporting()` | テレポート演出中か |

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
