-- 斬撃コンボ
--
-- C++ の SlashCombo1Action を Lua に写したもの。
-- どのアニメーションを何本つなぐか、どこでキャンセルして次へ行くかは
-- すべてエディタ側（インスペクタの「アニメーション」）で決められる。
--
-- インスペクタでの設定例
--   アニメーション : Combo_Attack_01_01_Seq_0
--                    Combo_Attack_01_02_Seq_0
--                    Combo_Attack_02_03_Seq_0
--                    Combo_Attack_02_04_Seq_0
--   パラメータ     : approachSeconds (Float, 1.0)   -- 間合い詰めにかける時間
--                    nearX / nearY / nearZ (Float)  -- 「近づいた」とみなす距離

local M = {}

-- パラメータが未設定でも動くように、既定値を用意しておく
local function param(self, name, default)
    local value = self.node.params[name]
    if value == nil then return default end
    return value
end

function M:Run(dt)
    -- プレイヤーを追いながら攻撃判定を更新する
    Boss.SetTargetToPlayer()
    Boss.UpdateAttackCollision()

    if self.step == nil then self:OnReset() end

    local clipCount = Boss.GetClipCount()
    if clipCount == 0 then
        Boss.Log("SlashCombo: アニメーションが設定されていません")
        return "failed"
    end

    if self.step == 0 then
        -- 1 撃目
        self.clipIndex = 1
        Boss.PlayClip(self.clipIndex)
        self.step = 1

    elseif self.step == 1 then
        -- 連撃中。キャンセル受付の終わりまで来たら次の一撃へ差し替える
        Boss.TurnToTarget(dt, Boss.TURN_INSTANT)

        if Boss.IsPastCancelEnd(self.clipIndex) then
            if self.clipIndex >= clipCount then
                self.step = 2
                self.timer = 0.0
            else
                self.clipIndex = self.clipIndex + 1
                Boss.PlayClip(self.clipIndex)
            end
        end

    elseif self.step == 2 then
        -- 最後の一撃のあと、離れていたら少しだけ間合いを詰める
        Boss.TurnToTarget(dt, Boss.TURN_INSTANT)

        local duration = param(self, "approachSeconds", 1.0)
        self.timer = self.timer + dt

        local t = self.timer / duration
        if t > 1.0 then t = 1.0 end

        if Boss.IsPastCancelStart(self.clipIndex) then
            local nearX = param(self, "nearX", 2.5)
            local nearY = param(self, "nearY", 1.0)
            local nearZ = param(self, "nearZ", 2.5)

            if Boss.IsNearPlayer(nearX, nearY, nearZ) then
                self.step = 3
            else
                Boss.LerpTowardPlayer(t)
            end
        end

    elseif self.step == 3 then
        -- 再生し切ったら成功
        if not Boss.IsPlayingAnimation() then
            return "complete"
        end
    end

    -- 被弾・死亡で中断
    if Boss.IsInterrupted() then
        return "failed"
    end

    return "run"
end

function M:OnReset()
    self.step = 0
    self.clipIndex = 1
    self.timer = 0.0
end

return M
