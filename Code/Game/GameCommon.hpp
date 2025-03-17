//-----------------------------------------------------------------------------------------------
// GameCommon.hpp
//

//-----------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Renderer/Window.hpp"

// struct Capsule2;
struct AABB2;
struct OBB2;
// struct Triangle2;
struct LineSegment2;
struct Disc2;
//-----------------------------------------------------------------------------------------------
struct Rgba8;
struct Vec2;
class App;
// class AudioSystem;
class BitmapFont;
class Game;
class Renderer;
class RandomNumberGenerator;
class Window;

// one-time declaration
extern App* g_theApp;
// extern AudioSystem*           g_theAudio;
extern BitmapFont*            g_theBitmapFont;
extern Game*                  g_theGame;
extern Renderer*              g_theRenderer;
extern RandomNumberGenerator* g_theRNG;
extern Window*                g_theWindow;

//-----------------------------------------------------------------------------------------------
// DebugRender-related
//
void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color);
void DebugDrawGlowCircle(Vec2 const& center, float radius, Rgba8 const& color, float glowIntensity);
void DebugDrawGlowBox(Vec2 const& center, Vec2 const& dimensions, Rgba8 const& color, float glowIntensity);
void DebugDrawBoxRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);

void DrawDisc2D(Vec2 const& discCenter, float discRadius, Rgba8 const& color);
void DrawDisc2D(Disc2 const& disc, Rgba8 const& color);
void DrawLineSegment2D(Vec2 const& startPosition, Vec2 const& endPosition, float thickness, bool isInfinite, Rgba8 const& color);
void DrawLineSegment2D(LineSegment2 const& lineSegment, float thickness, bool isInfinite, Rgba8 const& color);
// void DrawTriangle2D(Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2, Rgba8 const& color);
// void DrawTriangle2D(Triangle2 const& triangle, Rgba8 const& color);
void DrawAABB2D(AABB2 const& aabb2, Rgba8 const& color);
void DrawOBB2D(OBB2 const& obb2, Rgba8 const& color);
// void DrawCapsule2D(Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color);
// void DrawCapsule2D(Capsule2 const& capsule, Rgba8 const& color);
void DrawArrow2D(Vec2 const& tailPos, Vec2 const& tipPos, float radius, float thickness, Rgba8 const& color);

//----------------------------------------------------------------------------------------------------
template <typename T>
void SafeDelete(T*& ptr)
{
    delete ptr;
    ptr = nullptr;
}
