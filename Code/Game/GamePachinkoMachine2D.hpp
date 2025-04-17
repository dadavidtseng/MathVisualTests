//----------------------------------------------------------------------------------------------------
// GamePachinkoMachine2D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Game/Game.hpp"

//----------------------------------------------------------------------------------------------------
enum class eBumperType : int8_t
{
    NONE = -1,
    DISC2,
    CAPSULE2,
    OBB2,
    COUNT
};

//----------------------------------------------------------------------------------------------------
struct Ball
{
    Vec2  m_position   = Vec2::ZERO;
    Vec2  m_velocity   = Vec2::ZERO;
    float m_radius     = 0.f;
    float m_elasticity = 0.f;
    Rgba8 m_color      = Rgba8::WHITE;
};

struct Bumper
{
    eBumperType m_type          = eBumperType::NONE;
    Vec2        m_startPosition = Vec2::ZERO;
    Vec2        m_endPosition   = Vec2::ZERO;
    Vec2        m_velocity      = Vec2::ZERO;
    float       m_radius        = 0.f;
    float       m_elasticity    = 0.f;
    Rgba8       m_color         = Rgba8::WHITE;
};

//----------------------------------------------------------------------------------------------------
class GamePachinkoMachine2D final : public Game
{
public:
    GamePachinkoMachine2D();

    void Update() override;

    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;
    void UpdateBall(float timeSteps);

    void GenerateRandomShapes();
    void GenerateRandomLineSegmentInScreen();

    void RenderShapes() const;

    std::vector<Ball>   m_ballList;
    std::vector<Bumper> m_bumperList;
    float               m_ballElasticity = 0.f;
    // Ball m_ball;

    Camera* m_worldCamera = nullptr;

    LineSegment2 m_lineSegment;
    float        m_physicsTimeOwed = 0.f;
    float        m_fixedTimeStep   = 0.f;
};
