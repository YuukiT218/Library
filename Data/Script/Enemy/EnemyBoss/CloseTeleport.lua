-- CloseTeleport : プレイヤーのすぐそばへ瞬間移動する
--
-- 流れ
--   1. 行き先（プレイヤーの周囲）を決めて、消える前のモーションを再生
--   2. キャンセル受付の終わりまで来たらテレポート開始
--   3. 出現しきったら完了
--
-- インスペクタでの設定
--   アニメーション : Dodge_Combat_B_Seq_0
--                    （未設定なら同じモーションを自動で探して使う）
--   パラメータ     : distance    (Float, 3.0)  プレイヤーからどれだけ離れて出るか
--                    fadeSeconds (Float, 0.3)  消えるまでの時間
--                    turnAfter   (Bool,  true) 出現後にプレイヤーの方を向くか

local M = {}

-- パラメータが未設定でも動くように、既定値を返す小さな関数。
-- local を付けるとこのファイルの中だけで使える関数になる。
local function param(self, name, default)
    local value = self.node.params[name]
    if value == nil then return default end
    return value
end

function M:Run(dt)
    Boss.SetTargetToPlayer()

    -- 初回だけ初期化する。Lua では未設定の変数は nil になる。
    if self.step == nil then self:OnReset() end

    if self.step == 0 then
        -- 行き先を先に決めておく。
        -- CalculateTeleportPosition はプレイヤーの周囲を 10 度刻みで調べて、
        -- 画面内に入る位置の中からひとつ選んでくれる。
        -- Lua の関数は値を複数返せるので、x/y/z をそのまま受け取れる。
        local distance = param(self, "distance", 3.0)
        self.destX, self.destY, self.destZ = Boss.CalculateTeleportPosition(distance, true)

        -- 消える前の動作。クリップが設定されていればそれを使う。
        if Boss.GetClipCount() > 0 then
            Boss.PlayClip(1)
            self.useClip = true
        else
            -- 未設定のときは既存の回避モーションで代用する
            local fallback = Boss.GetAnimationIndex("Dodge_Combat_B_Seq_0")
            if fallback >= 0 then
                Boss.PlayRootMotion(fallback, false)
            end
            self.useClip = false
        end

        self.step = 1

    elseif self.step == 1 then
        -- 消える直前までプレイヤーを追い続ける
        Boss.TurnToTarget(dt, Boss.TURN_FAST)

        -- キャンセル受付の終わりまで来たら消え始める。
        -- クリップを使っているならノード側の設定、
        -- 代用モーションならアニメーション共通の設定を見る。
        local ready
        if self.useClip then
            ready = Boss.IsPastCancelEnd(1)
        else
            ready = Boss.GetAnimationSeconds() >= Boss.GetCurrentCancelEnd()
        end

        if ready and not Boss.IsTeleporting() then
            Boss.StartTeleport(
                self.destX, self.destY, self.destZ,
                param(self, "fadeSeconds", 0.3))
            self.step = 2
        end

    elseif self.step == 2 then
        -- 出現しきるまで待つ
        if not Boss.IsTeleporting() then
            if param(self, "turnAfter", true) then
                Boss.TurnToTarget(dt, Boss.TURN_INSTANT)
            end
            return "complete"
        end
    end

    -- 消えている間は無敵なので、中断を見るのは消える前だけでよい
    if self.step < 2 and Boss.IsInterrupted() then
        return "failed"
    end

    return "run"
end

function M:OnReset()
    self.step = 0
    self.destX, self.destY, self.destZ = 0, 0, 0
    self.useClip = false
end

return M
