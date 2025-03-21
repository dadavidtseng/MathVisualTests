//----------------------------------------------------------------------------------------------------
// GameShapes3D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/Game.hpp"

struct Rgba8;

//----------------------------------------------------------------------------------------------------
enum class eTestShapeType : int8_t
{
    NONE = -1,
    AABB3,
    SPHERE3,
    CYLINDER3,
    PLAYER
};

//----------------------------------------------------------------------------------------------------
enum class eTestShapeState : int8_t
{
    IDLE,
    GRABBED
};

//----------------------------------------------------------------------------------------------------
struct TestShape3D
{
    eTestShapeType  m_type          = eTestShapeType::NONE;
    eTestShapeState m_state         = eTestShapeState::IDLE;
    Vec3            m_centerPosition = Vec3::ZERO;
    // Vec3            m_endPosition   = Vec3::ZERO;
    EulerAngles     m_orientation   = EulerAngles::ZERO;
    float           m_radius        = 0.f;
    Rgba8           m_currentColor  = Rgba8::WHITE;
    Rgba8           m_targetColor   = Rgba8::WHITE;
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
    void UpdateShapes(float deltaSeconds);

    void RenderShapes() const;
    void RenderTest() const;
    void RenderPlayerBasis() const;

    void GenerateRandomShapes();
    void GenerateTest();

    void ToggleTestShapeState(TestShape3D& testShape);

    Camera*     m_worldCamera    = nullptr;
    Texture*    m_texture        = nullptr;
    TestShape3D m_testShapes[15] = {};
    TestShape3D m_test;

    int  m_grabbedShapeIndex                    = -1;
    Vec3 m_grabbedShapeCameraSpaceStartPosition = Vec3::ZERO;
    Vec3 m_grabbedShapeCameraSpaceEndPosition   = Vec3::ZERO;
};
