//----------------------------------------------------------------------------------------------------
// GameShapes3D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/Game.hpp"

struct TestShapes
{
    Vec3 m_position = Vec3::ZERO;
    Vec3 m_velocity = Vec3::ZERO;
    EulerAngles m_orientation  = EulerAngles::ZERO;
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

    Camera* m_worldCamera = nullptr;
    TestShapes m_testShapes;
    TestShapes m_testShapes2;
    Texture* m_texture = nullptr;
};
