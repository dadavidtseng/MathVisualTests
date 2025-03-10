//----------------------------------------------------------------------------------------------------
// GameShapes3D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/Game.hpp"

//----------------------------------------------------------------------------------------------------
struct TestShape3D
{
    Vec3        m_position        = Vec3::ZERO;
    Vec3        m_velocity        = Vec3::ZERO;
    EulerAngles m_orientation     = EulerAngles::ZERO;
    EulerAngles m_angularVelocity = EulerAngles::ZERO;
};

//----------------------------------------------------------------------------------------------------
class GameShapes3D final : public Game
{
public:
    GameShapes3D();

    void Update() override;
    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    void RenderEntities() const;
    void RenderPlayerBasis() const;

    Camera*     m_worldCamera = nullptr;
    Texture*    m_texture = nullptr;
    TestShape3D m_player;
    TestShape3D m_sphere;
};
