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

//----------------------------------------------------------------------------------------------------
struct EasingFunctionInfo
{
    String                      easeFunctionName;
    std::function<float(float)> easeFunction;
};

//----------------------------------------------------------------------------------------------------
class GameCurves2D final : public Game
{
public:
    GameCurves2D();

    void Update() override;
    void Render() const override;

    static std::vector<EasingFunctionInfo> s_easingFunctions;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    void GenerateRandomShapes();
    void GenerateAABB2s();
    void GenerateEaseFunction();
    void GenerateCubicBezierCurves();
    void GenerateCubicHermiteCurves();
    Vec2 GenerateRandomPointInBounds(AABB2 const& aabb2) const;

    void RenderShapes() const;
    void RenderAABB2s() const;
    void RenderEaseFunctions() const;
    void RenderCubicBezierCurves() const;
    void RenderCubicHermiteCurves() const;

    // Common
    AABB2 m_boundA;
    AABB2 m_boundAChild;
    AABB2 m_boundB;
    AABB2 m_boundC;
    float m_lineThickness        = 3.f;
    float m_discRadius           = 6.f;
    int   m_numSubDivisions      = 2;
    int   m_fixedNumSubDivisions = 64;
    float m_loopDuration         = 2.f;

    // EaseFunctions
    AABB2 m_easeFunctionBound;
    Vec2  m_easeFunctionStartPosition;
    Vec2  m_easeFunctionEndPosition;
    int   m_easeIndex = 0;

    // CubicBezierCurve
    CubicBezierCurve2D m_cubicBezierCurve2D;

    // CubicHermiteCurve
    CubicHermiteCurve2D m_cubicHermiteCurve2D;
    CatmullRomSpline2D  m_catmullRomSpline2D;
    std::vector<Vec2>   m_points;
};
