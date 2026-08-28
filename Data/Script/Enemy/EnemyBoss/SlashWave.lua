-- 斬撃波
--
-- 連続でアニメーションを再生しつつ、決まったタイミングで飛び道具を撃つ例。
-- 「1 本ごとに 1 発撃つ」ような行動の書き方の見本。
--
-- インスペクタでの設定例
--   アニメーション : Combo_Attack_Wave_05_01_Seq_0
--                    Combo_Attack_Wave_05_02_Seq_0
--                    Combo_Attack_Wave_05_03_Seq_0
--                    Combo_Attack_Wave_05_04_Seq_0
--   パラメータ     : waveSpeed  (Float, 20.0) -- 斬撃波の速さ
--                    waveOffset (Float, 1.5)  -- 自分の前方どれだけ先に出すか

local M = {}

local function param(self, name, default)
    local value = self.node.params[name]
    if value == nil then return default end
    return value
end

function M:Run(dt)
    Boss.SetTargetToPlayer()
    Boss.TurnToTarget(dt, Boss.TURN_INSTANT)
    Boss.UpdateAttackCollision()

    if self.step == nil then self:OnReset() end

    local clipCount = Boss.GetClipCount()
    if clipCount == 0 then
        Boss.Log("SlashWave: アニメーションが設定されていません")
        return "failed"
    end

    if self.step == 0 then
        -- 次の一振りを再生する
        Boss.PlayClip(self.clipIndex)
        self.hasShot = false
        self.step = 1

    elseif self.step == 1 then
        -- 振りかぶりが終わるあたり（キャンセル受付の開始）で 1 発撃つ
        if not self.hasShot and Boss.IsPastCancelStart(self.clipIndex) then
            Boss.LaunchSlashWave{
                speed  = param(self, "waveSpeed", 20.0),
                offset = param(self, "waveOffset", 1.5),
                homing = false,
            }
            self.hasShot = true
        end

        -- 受付の終わりまで来たら次の一振りへ
        if Boss.IsPastCancelEnd(self.clipIndex) then
            if self.clipIndex >= clipCount then
                self.step = 2
            else
                self.clipIndex = self.clipIndex + 1
                self.step = 0
            end
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
    self.clipIndex = 1
    self.hasShot = false
end

return M
