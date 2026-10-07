#pragma once

#include <DirectXMath.h>
#include "Model/Model.h"

// カメラコントローラ
class EditCameraController
{
public:
    EditCameraController() {}
    ~EditCameraController() {}

    // 毎フレームの更新処理（カメラの位置や角度などを計算）
    void Update(float elapsedTime);

    // カメラ操作（マウスやキー入力による制御処理）
    void ControlUpdate(float elapsedTime);

    // カメラが注視するターゲットの位置を設定
    void SetTarget(const DirectX::XMFLOAT3& target) { this->target = target; }

    // ターゲットに対する相対オフセット（例：頭上に注視させたい場合など）を設定
    void SetOffsetTarget(const DirectX::XMFLOAT3& offset) { this->offsetTarget = offset; }

    // カメラの回転角度を設定（X:上下, Y:左右, Z:ローリング）
    void SetAngle(const DirectX::XMFLOAT3& angle) { this->angle = angle; }

    // カメラとターゲットの距離（ズーム距離）を設定
    void SetRange(float r) { this->range = r; }

    // 実際のターゲット位置を取得（ベース位置 + オフセット）
    DirectX::XMFLOAT3 GetTargetPosition() const {
        return {
            target.x + offsetTarget.x,
            target.y + offsetTarget.y,
            target.z + offsetTarget.z
        };
    }

    // オフセットターゲットを取得
    const DirectX::XMFLOAT3& GetOffsetTarget() const { return offsetTarget; }

    // カメラの距離（ズーム）を取得
    float GetRange() const { return range; }

    // ターゲットから見たカメラの方向ベクトルを取得（未実装）
    DirectX::XMFLOAT3 GetDirection() const;

    // カメラの視点座標を取得（未実装）
    DirectX::XMFLOAT3 GetEyePosition() const;

    // 現在のカメラ角度を取得
    DirectX::XMFLOAT3 GetAngle() const { return angle; }

    // ImGuiでのデバッグ用UI描画
    void DrawDebugGUI();

private:
    // ターゲットの中心位置（注視点の基準）
    DirectX::XMFLOAT3 target = { 0, 0, 0 };

    // ターゲットへの相対オフセット（視線をずらすために使用）
    DirectX::XMFLOAT3 offsetTarget = { 0, 0, 0 };

    // カメラの回転角度（X:ピッチ、Y:ヨー、Z:ロール）
    DirectX::XMFLOAT3 angle = { 0, DirectX::XMConvertToRadians(110), 0 };

    // カメラ回転の速さ（ラジアン毎秒）
    float rollSpeed = DirectX::XMConvertToRadians(200);

    // カメラとターゲット間の距離
    float range = 7.0f;

    // 上方向に向ける角度の最大値（ピッチ）
    float maxAngleX = DirectX::XMConvertToRadians(80);

    // 下方向に向ける角度の最小値（ピッチ）
    float minAngleX = DirectX::XMConvertToRadians(-30);

    bool isFirstRangeFrame = true;
private:
    void RangeOnlyEvent(AnimationConfig* config, float animationSeconds, float secondsLength);
    void HandleCameraEvent(AnimationConfig* config, float animationSeconds, float secondsLength, float currentCharacterYaw);
};

