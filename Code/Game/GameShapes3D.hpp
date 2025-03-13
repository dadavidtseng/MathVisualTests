//----------------------------------------------------------------------------------------------------
// GameShapes3D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Cylinder3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Sphere3.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/Game.hpp"

//----------------------------------------------------------------------------------------------------
enum class TestShapeType
{
    AABB3,
    SPHERE3,
    CYLINDER3,
    PLAYER
};

//----------------------------------------------------------------------------------------------------
struct TestShape3D
{
    TestShapeType m_type;
    Vec3          m_startPosition   = Vec3::ZERO;
    Vec3          m_endPosition     = Vec3::ZERO;
    EulerAngles   m_orientation     = EulerAngles::ZERO;
    float         m_radius          = 0.f;
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

    void RenderShapes() const;
    void RenderTest() const;
    void RenderPlayerBasis() const;

    void GenerateRandomShapes();
    void GenerateTest();

    Camera*     m_worldCamera = nullptr;
    Texture*    m_texture     = nullptr;
    TestShape3D m_testShapes[15] = {};
    TestShape3D m_test;
};
