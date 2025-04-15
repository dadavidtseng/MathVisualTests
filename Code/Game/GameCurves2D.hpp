//----------------------------------------------------------------------------------------------------
// GameCurves2D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <functional>

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Curve2D.hpp"
#include "Game/Game.hpp"

// class CubicBezierCurve2D;
// struct Rgba8;

//----------------------------------------------------------------------------------------------------
struct EasingFunctionInfo
{
    String                      easeFunctionName;
    std::function<float(float)> easeFunction;
};

//----------------------------------------------------------------------------------------------------
class GameCurves2D : public Game
{
public:
    GameCurves2D();

    void Update() override;
    void RenderShapes() const;

    void                                   Render() const override;
    static std::vector<EasingFunctionInfo> s_easingFunctions;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;
    void UpdateEaseFunction();

    void GenerateAABB2s();
    void GenerateEaseFunction();

    void RenderAABB2s() const;
    void RenderEaseFunctions() const;

    Camera* m_worldCamera = nullptr;

    Vec2 startPos;
    Vec2 guide1;
    Vec2 guide2;
    Vec2 endPos;


    CubicHermiteCurve2D m_cubicHermiteCurve2D;
    CatmullRomSpline2D  m_catmullRomSpline2D;

    // AABB2 box           = AABB2(Vec2(100.f, 100.f), Vec2(500.f, 500.f));
    // Vec2  startPosition = box.m_mins;
    // Vec2  endPosition   = box.m_maxs;

    // EaseFunctions
    AABB2 m_easeFunctionBound;
    Vec2  m_easeFunctionStartPosition;
    Vec2  m_easeFunctionEndPosition;
    int   m_easeIndex = 0;
    AABB2 boundA;
    AABB2 boundAChild;
    AABB2 boundB;
    AABB2 boundC;

    // CubicBezierCurve
    CubicBezierCurve2D m_cubicBezierCurve2D;

};
