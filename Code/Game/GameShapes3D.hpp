//----------------------------------------------------------------------------------------------------
// GameShapes3D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/Game.hpp"

//----------------------------------------------------------------------------------------------------
enum class eTestShapeType : int8_t
{
    NONE = -1,
    AABB3,
    SPHERE3,
    CYLINDER3,
    OBB3,
    PLANE3,
    COUNT
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
    eTestShapeType  m_type               = eTestShapeType::NONE;
    eTestShapeState m_state              = eTestShapeState::IDLE;
    Vec3            m_centerPosition     = Vec3::ZERO;
    EulerAngles     m_orientation        = EulerAngles::ZERO;
    float           m_radius             = 0.f;
    float           m_distanceFromOrigin = 0.f;
    Vec3            m_halfDimensions     = Vec3::ZERO;
    Rgba8           m_currentColor       = Rgba8::WHITE;
    Rgba8           m_targetColor        = Rgba8::WHITE;
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
    void UpdateShapes();

    void RenderRaycastResult() const;
    void RenderNearestPoint() const;
    void RenderStoredRaycastResult() const;
    void RenderShapes() const;
    void RenderPlayerBasis() const;

    void GenerateRandomShapes();

    // Utils
    Vec3        RollVec3InRange(FloatRange const& rangeX, FloatRange const& rangeY, FloatRange const& rangeZ) const;
    EulerAngles RollEulerAngleInRange(FloatRange const& rangeX, FloatRange const& rangeY, FloatRange const& rangeZ) const;

    Texture*    m_texture        = nullptr;
    TestShape3D m_testShapes[25] = {};

    int    m_grabbedShapeIndex                    = -1;
    Vec3   m_grabbedShapeCameraSpaceStartPosition = Vec3::ZERO;
    Ray3*  m_storedRay                            = nullptr;
    String m_raycastResultText                    = "space=lock raycast; ";
    String m_grabbedShapeText;
};
