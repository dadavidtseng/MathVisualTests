//----------------------------------------------------------------------------------------------------
// GameCurves2D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Math/Curve2D.hpp"
#include "Game/Game.hpp"

class CubicBezierCurve2D;
struct Rgba8;


//----------------------------------------------------------------------------------------------------
class GameCurves2D final : public Game
{
public:
    GameCurves2D();

    void UpdateShapes();
    void Update() override;
    void RenderShapes() const;
    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;


    Camera* m_worldCamera = nullptr;

    Vec2 startPos;
    Vec2 guide1;
    Vec2 guide2;
    Vec2 endPos;

    CubicBezierCurve2D m_cubicBezierCurve2D;
    CubicHermiteCurve2D m_cubicHermiteCurve2D;
    CatmullRomSpline2D m_catmullRomSpline2D;
};
