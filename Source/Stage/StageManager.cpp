#include "StageManager.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

// 更新処理
void StageManager::Update(float elapsedTime)
{
    for (Stage* stage : stages)
    {
        stage->Update(elapsedTime);
    }
}

// 描画処理
void StageManager::Render(const RenderContext& rc, ShaderId shaderId)
{
    for (Stage* stage : stages)
    {
        stage->Render(rc, shaderId);
    }
}

void StageManager::Debug(const RenderContext& rc)
{
    if (isDebugRender)
    {
        for (Stage* stage : stages)
        {
            stage->Debug(rc);
        }
    }
}

void StageManager::DebugImGui()
{
    // トランスフォーム
    if (ImGui::CollapsingHeader("Stage", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox(u8"空間分割レイキャスト", &isDivision);
        if (isDivision)
        {
            ImGui::Checkbox(u8"ステージポリゴン可視化", &isDebugRender);
        }
        else
        {
            isDebugRender = false;
        }
    }
    for (Stage* stage : stages)
    {
        // typeid を使ってクラス名を取得
        std::string className = typeid(*stage).name();
        if (ImGui::CollapsingHeader(className.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            stage->DebugImGui();
        }
    }

}

void StageManager::ShadowRender(const RenderContext& rc, ShadowMap* shadowMap)
{
    for (Stage* stage : stages)
    {
        shadowMap->Draw(rc, stage->GetModel());
    }
}

// ステージ登録
void StageManager::Register(Stage* stage)
{
    stages.emplace_back(stage);
}

// ステージ全削除
void StageManager::Clear()
{
    for (Stage* stage : stages)
    {
        if (stage != nullptr)
        {
            delete stage;
            stage = nullptr;
        }
    }
    stages.clear();
}

// レイキャスト
bool StageManager::RayCast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit)
{
    bool result = false;

    float closestDistance = FLT_MAX;    HitResult tempHit;

    DirectX::XMVECTOR startVec = DirectX::XMLoadFloat3(&start);

    // 全てのステージオブジェクトに対してレイキャストを行い、衝突した交点が一番近い情報を取得する
    for (Stage* stage : stages)
    {
        //空間分割フラグに基づいて処理を分岐
        if (isDivision && stage->IsDivisionEnabled())
        {
            if (stage->SpaceDivisionRaycast(start, end, tempHit))
            {
                DirectX::XMVECTOR hitPointVec = DirectX::XMLoadFloat3(&tempHit.position);
                DirectX::XMVECTOR distanceVec = DirectX::XMVectorSubtract(hitPointVec, startVec);

                float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(distanceVec));

                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    hit = tempHit;
                    result = true;
                }
            }
        }
        else
        {
            if (stage->RayCast(start, end, tempHit))
            {
                DirectX::XMVECTOR hitPointVec = DirectX::XMLoadFloat3(&tempHit.position);
                DirectX::XMVECTOR distanceVec = DirectX::XMVectorSubtract(hitPointVec, startVec);

                float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(distanceVec));

                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    hit = tempHit;
                    result = true;
                }
            }
        }
    }

    return result;
}

