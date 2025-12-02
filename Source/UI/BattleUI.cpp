#include <imgui.h>

#include "BattleUI.h"
#include "Graphics/Graphics.h"
#include "Character/player.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Camera/Camera.h"
#include "Camera/CameraParam.h"
#include "Graphics/GpuResourceUtils.h"
#include "Math/Mathf.h"
#include "Input/Input.h"

void BattleUI::Initialize(float maxhealth, int maxag)
{
    ID3D11Device* device = Graphics::Instance().GetDevice();

    maxHealth = currentHealth = maxhealth;
    maxAg = currentAg = maxag;

    lockonEnemy = nullptr;

    //本来は０
    currentOverLimit = 1.0f;

    InitializeSprites();

    // sprite用デフォルト描画シェーダー
    D3D11_INPUT_ELEMENT_DESC input_element_desc[]
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    GpuResourceUtils::LoadVertexShader(device, "Data/Shader/SpriteDissolveVS.cso", input_element_desc, _countof(input_element_desc), sprite_input_layout.GetAddressOf(), sprite_vertex_shader.GetAddressOf());
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/SpriteDissolvePS.cso", sprite_pixel_shader.GetAddressOf());
}

void BattleUI::InitializeSprites()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();

    lockonpoint = std::make_unique<Sprite>(device, "Data/Sprite/Lockon.png");
}

void BattleUI::Update(float elapsedTime, float currenthealth, float currentag)
{
    UpdateUIData(elapsedTime);
    currentHealth = currenthealth;

    lockonSinTimer += elapsedTime * lockonSinSpeed;

    // サイン波で透明度を変化させる
    lockonSinOffset = (std::sin(lockonSinTimer - DirectX::XM_PIDIV2) * 0.5f) + 0.5f;

    lockonEnemy = CameraParam::Instance().GetLockOnEnemy();
}

void BattleUI::Render(float elapsedTime)
{
    Graphics& graphics = Graphics::Instance();
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = graphics.GetRenderState();
    ID3D11SamplerState* samplers[] =
    {
        renderState->GetSamplerState(SamplerState::PointClamp)
    };
    dc->PSSetSamplers(0, _countof(samplers), samplers);

    FLOAT blendFactor[4] = { 1.0f,1.0f,1.0f,1.0f };
    UINT sampleMask = 0xFFFFFFFF;
    dc->VSSetShader(sprite_vertex_shader.Get(), nullptr, 0);
    dc->PSSetShader(sprite_pixel_shader.Get(), nullptr, 0);

    // ブレンドステート
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), blendFactor, sampleMask);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

    float screenWidth = graphics.GetScreenWidth();
    float screenHeight = graphics.GetScreenHeight();

    if (CameraParam::Instance().GetIsLockOn())
    {
        if (lockonEnemy)
        {
            lockonpoint->Render(dc, uiMap["RockOnPoint"].position.x - scale.x * 0.5, uiMap["RockOnPoint"].position.y, 0, scale.x, scale.y, 0, 0, 256, 256, lockonAngle, 1, 1, 1, 0.7f);
        }
    }
}

DirectX::XMFLOAT2 BattleUI::ConvertWorldToScreen(const DirectX::XMFLOAT3& worldPosition, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection, float screenWidth, float screenHeight)
{
    DirectX::XMVECTOR worldVec = DirectX::XMLoadFloat3(&worldPosition);

    DirectX::XMVECTOR screenVec = DirectX::XMVector3Project(
        worldVec,
        0.0f, 0.0f,
        screenWidth, screenHeight,
        0.0f, 1.0f,
        projection,
        view,
        DirectX::XMMatrixIdentity()
    );

    DirectX::XMFLOAT2 screenPosition;
    DirectX::XMStoreFloat2(&screenPosition, screenVec);

    return screenPosition;
}

void BattleUI::UpdateUIPosition(const std::string& key, const DirectX::XMFLOAT3& worldPosition, std::unordered_map<std::string, HPUIData>& uiMap, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection, float screenWidth, float screenHeight)
{
    DirectX::XMFLOAT2 screenPosition = ConvertWorldToScreen(worldPosition, view, projection, screenWidth, screenHeight);
    uiMap[key].position = { screenPosition.x, screenPosition.y, 0.0f }; // Zは0に固定
}

// スプライトの初期設定
void BattleUI::SpriteSetUp(std::string name, DirectX::XMFLOAT3 position, float width, float height, DirectX::XMFLOAT4 size, float angle, DirectX::XMFLOAT4 color)
{
    spriteUI[name].position = { position.x, position.y, position.z };
    spriteUI[name].width = width;
    spriteUI[name].height = height;
    spriteUI[name].size = { size.x, size.y, size.z, size.w };
    spriteUI[name].angle = angle;
    spriteUI[name].color = { color.x, color.y, color.z, color.w };
}

void BattleUI::UpdateUIData(float elapsedTime)
{
    DirectX::XMFLOAT3 playerPosition = Player::Instance().GetPosition();

    // スクリーンサイズ取得
    float screenWidth = Graphics::Instance().GetScreenWidth();
    float screenHeight = Graphics::Instance().GetScreenHeight();

    Camera& camera = Camera::Instance();

    DirectX::XMMATRIX View = DirectX::XMLoadFloat4x4(&camera.GetView());
    DirectX::XMMATRIX Projection = DirectX::XMLoadFloat4x4(&camera.GetProjection());

    //// 各UIの位置を更新
    //UpdateUIPosition("RadGauge", playerPosition, uiMap, View, Projection, screenWidth, screenHeight);
    //UpdateUIPosition("GreenGauge", playerPosition, uiMap, View, Projection, screenWidth, screenHeight);
    //UpdateUIPosition("BackGauge", playerPosition, uiMap, View, Projection, screenWidth, screenHeight);

    //UpdateUIPosition("OverLimit", playerPosition, uiMap, View, Projection, screenWidth, screenHeight);
    //UpdateUIPosition("OverLimit_Back", playerPosition, uiMap, View, Projection, screenWidth, screenHeight);

    if (lockonEnemy)
    { 
        DirectX::XMFLOAT3 lockonEnemyPosition = {lockonEnemy->GetModel()->FindNode("spine_03")->worldTransform._41,
                                                 lockonEnemy->GetModel()->FindNode("spine_03")->worldTransform._42,
                                                 lockonEnemy->GetModel()->FindNode("spine_03")->worldTransform._43 };

        // 位置ベクトルの差を計算
        DirectX::XMVECTOR playerVec = DirectX::XMLoadFloat3(&playerPosition);
        DirectX::XMVECTOR enemyVec = DirectX::XMLoadFloat3(&lockonEnemyPosition);
        DirectX::XMVECTOR distanceVec = DirectX::XMVectorSubtract(playerVec, enemyVec);
        float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(distanceVec));

        // アングルをゆっくり回す
        lockonAngle += elapsedTime * 90.0f;

        // 距離に基づいてスケールを変更
        if (distance <= 5.0f)
        {
            scale.x = 40.0f; // 距離が5以下なら最大スケール
            scale.y = 40.0f; // 距離が5以下なら最大スケール
        }
        else if (distance <= 30.0f)
        {
            // 距離が5〜30の間でスケールを線形補間
            scale.x = 40.0f - ((distance - 5.0f) / 25.0f) * (40.0f - 30.0f);
            scale.y = 40.0f - ((distance - 5.0f) / 25.0f) * (40.0f - 30.0f);
        }
        else
        {
            scale.x = 30.0f; // 距離が30を超えるなら最小スケール
            scale.y = 30.0f; // 距離が30を超えるなら最小スケール
        }

        UpdateUIPosition("RockOnPoint", lockonEnemyPosition, uiMap, View, Projection, screenWidth, screenHeight);
    }
}

void BattleUI::DrawDebugGUI()
{
    if (ImGui::CollapsingHeader(u8"プレイヤーHP背景", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3(u8"プレイヤーHP背景UI位置", &spriteUI["playerHPBack"].position.x, 0.1f);
        ImGui::DragFloat(u8"プレイヤーHP背景UI幅", &spriteUI["playerHPBack"].width, 0.1f, 0.0f);
        ImGui::DragFloat(u8"プレイヤーHP背景UI高さ", &spriteUI["playerHPBack"].height, 0.1f, 0.0f);
        ImGui::DragFloat4(u8"プレイヤーHP背景UI大きさ", &spriteUI["playerHPBack"].size.x, 0.1f, 0.0f);
        ImGui::DragFloat(u8"プレイヤーHP背景UI回転", &spriteUI["playerHPBack"].angle, 0.1f, 0.0f);
        ImGui::ColorEdit4(u8"プレイヤーHP背景UI色", &spriteUI["playerHPBack"].color.x);
    }

    if (ImGui::CollapsingHeader(u8"プレイヤーHP赤ゲージ", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3(u8"プレイヤーHP赤ゲージUI位置", &spriteUI["playerHPRed"].position.x, 0.1f);
        ImGui::DragFloat(u8"プレイヤーHP赤ゲージUI幅", &spriteUI["playerHPRed"].width, 0.1f, 0.0f);
        ImGui::DragFloat(u8"プレイヤーHP赤ゲージUI高さ", &spriteUI["playerHPRed"].height, 0.1f, 0.0f);
        ImGui::DragFloat4(u8"プレイヤーHP赤ゲージUI大きさ", &spriteUI["playerHPRed"].size.x, 0.1f, 0.0f);
        ImGui::DragFloat(u8"プレイヤーHP赤ゲージUI回転", &spriteUI["playerHPRed"].angle, 0.1f, 0.0f);
        ImGui::ColorEdit4(u8"プレイヤーHP赤ゲージUI色", &spriteUI["playerHPRed"].color.x);
    }

    if (ImGui::CollapsingHeader(u8"プレイヤーHP緑ゲージ", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3(u8"プレイヤーHP緑ゲージUI位置", &spriteUI["playerHPGreen"].position.x, 0.1f);
        ImGui::DragFloat(u8"プレイヤーHP緑ゲージUI幅", &spriteUI["playerHPGreen"].width, 0.1f, 0.0f);
        ImGui::DragFloat(u8"プレイヤーHP緑ゲージUI高さ", &spriteUI["playerHPGreen"].height, 0.1f, 0.0f);
        ImGui::DragFloat4(u8"プレイヤーHP緑ゲージUI大きさ", &spriteUI["playerHPGreen"].size.x, 0.1f, 0.0f);
        ImGui::DragFloat(u8"プレイヤーHP緑ゲージUI回転", &spriteUI["playerHPGreen"].angle, 0.1f, 0.0f);
        ImGui::ColorEdit4(u8"プレイヤーHP緑ゲージUI色", &spriteUI["playerHPGreen"].color.x);
    }

    if (ImGui::CollapsingHeader(u8"エルダードラゴン", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3(u8"エルダードラゴンUI位置", &spriteUI["elderDragon"].position.x, 0.1f);
        ImGui::DragFloat(u8"エルダードラゴンUI幅", &spriteUI["elderDragon"].width, 0.1f, 0.0f);
        ImGui::DragFloat(u8"エルダードラゴンUI高さ", &spriteUI["elderDragon"].height, 0.1f, 0.0f);
        ImGui::DragFloat(u8"エルダードラゴンUI回転", &spriteUI["elderDragon"].angle, 0.1f, 0.0f);
        ImGui::ColorEdit4(u8"エルダードラゴンUI色", &spriteUI["elderDragon"].color.x);
    }

    if (ImGui::CollapsingHeader(u8"エルダードラゴンHP背景", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3(u8"エルダードラゴンHP背景UI位置", &spriteUI["elderDragonHPBack"].position.x, 0.1f);
        ImGui::DragFloat(u8"エルダードラゴンHP背景UI幅", &spriteUI["elderDragonHPBack"].width, 0.1f, 0.0f);
        ImGui::DragFloat(u8"エルダードラゴンHP背景UI高さ", &spriteUI["elderDragonHPBack"].height, 0.1f, 0.0f);
        ImGui::DragFloat4(u8"エルダードラゴンHP背景UI大きさ", &spriteUI["elderDragonHPBack"].size.x, 0.1f, 0.0f);
        ImGui::DragFloat(u8"エルダードラゴンHP背景UI回転", &spriteUI["elderDragonHPBack"].angle, 0.1f, 0.0f);
        ImGui::ColorEdit4(u8"エルダードラゴンHP背景UI色", &spriteUI["elderDragonHPBack"].color.x);
    }

    if (ImGui::CollapsingHeader(u8"エルダードラゴンHPゲージ", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3(u8"エルダードラゴンHPゲージUI位置", &spriteUI["elderDragonHPGauge"].position.x, 0.1f);
        ImGui::DragFloat(u8"エルダードラゴンHPゲージUI幅", &spriteUI["elderDragonHPGauge"].width, 0.1f, 0.0f);
        ImGui::DragFloat(u8"エルダードラゴンHPゲージUI高さ", &spriteUI["elderDragonHPGauge"].height, 0.1f, 0.0f);
        ImGui::DragFloat4(u8"エルダードラゴンHPゲージUI大きさ", &spriteUI["elderDragonHPGauge"].size.x, 0.1f, 0.0f);
        ImGui::DragFloat(u8"エルダードラゴンHPゲージUI回転", &spriteUI["elderDragonHPGauge"].angle, 0.1f, 0.0f);
        ImGui::ColorEdit4(u8"エルダードラゴンHPゲージUI色", &spriteUI["elderDragonHPGauge"].color.x);
    }

    //if (ImGui::CollapsingHeader("HP UI", ImGuiTreeNodeFlags_DefaultOpen))
    //{
    //    ImGui::DragFloat("OffsetPositionX", &offsetPositionX, 0.1f);
    //    ImGui::DragFloat("OffsetPositionY", &offsetPositionY, 0.1f);

    //    ImGui::DragFloat("OverLimit", &currentOverLimit, 0.01f, 0, 1.0f);

    //    ImGui::DragFloat2("Scale", &scale.x);
    //}

    if (ImGui::CollapsingHeader(u8"コントローラー操作UI", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3(u8"コントローラー操作UI位置", &spriteUI["operationGamePad"].position.x, 0.1f);
        ImGui::DragFloat(u8"コントローラー操作UI幅", &spriteUI["operationGamePad"].width, 0.1f, 0.0f);
        ImGui::DragFloat(u8"コントローラー操作UI高さ", &spriteUI["operationGamePad"].height, 0.1f, 0.0f);
        ImGui::DragFloat(u8"コントローラー操作UI回転", &spriteUI["operationGamePad"].angle, 0.1f, 0.0f);
        ImGui::ColorEdit4(u8"コントローラー操作UI色", &spriteUI["operationGamePad"].color.x);
    }

    if (ImGui::CollapsingHeader(u8"キーボード&マウス操作UI", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3(u8"キーボード&マウス操作UI位置", &spriteUI["operation"].position.x, 0.1f);
        ImGui::DragFloat(u8"キーボード&マウス操作UI幅", &spriteUI["operation"].width, 0.1f, 0.0f);
        ImGui::DragFloat(u8"キーボード&マウス操作UI高さ", &spriteUI["operation"].height, 0.1f, 0.0f);
        ImGui::DragFloat(u8"キーボード&マウス操作UI回転", &spriteUI["operation"].angle, 0.1f, 0.0f);
        ImGui::ColorEdit4(u8"キーボード&マウス操作UI色", &spriteUI["operation"].color.x);
    }
}