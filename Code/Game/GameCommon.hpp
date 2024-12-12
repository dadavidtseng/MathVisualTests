//-----------------------------------------------------------------------------------------------
// GameCommon.hpp
//

//-----------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Renderer/Window.hpp"


class Arrow;
struct Capsule2;
struct AABB2;
struct OBB2;
struct Triangle2;
struct LineSegment2;
struct Disc2;
//-----------------------------------------------------------------------------------------------
struct Rgba8;
struct Vec2;
class App;
// class AudioSystem;
class BitmapFont;
class Game;
class InputSystem;
class Renderer;
class RandomNumberGenerator;
class Window;

// one-time declaration
extern App*                   g_theApp;
// extern AudioSystem*           g_theAudio;
extern BitmapFont*            g_theBitmapFont;
extern Game*                  g_theGame;
extern InputSystem*           g_theInput;
extern Renderer*              g_theRenderer;
extern RandomNumberGenerator* g_theRNG;
extern Window*                g_theWindow;

//-----------------------------------------------------------------------------------------------
// initial settings
//
constexpr float SCREEN_SIZE_X = 1600.f;
constexpr float SCREEN_SIZE_Y = 800.f;
constexpr float SCREEN_CENTER_X = SCREEN_SIZE_X / 2.f;
constexpr float SCREEN_CENTER_Y = SCREEN_SIZE_Y / 2.f;

//-----------------------------------------------------------------------------------------------
// DebugRender-related
//
void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color);
void DebugDrawGlowCircle(Vec2 const& center, float radius, Rgba8 const& color, float glowIntensity);
void DebugDrawGlowBox(Vec2 const& center, Vec2 const& dimensions, Rgba8 const& color, float glowIntensity);
void DebugDrawBoxRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);

void DrawDisc2(Vec2 const& center, float radius, Rgba8 const& color);
void DrawDisc2(Disc2 const& disc, Rgba8 const& color);
void DrawLineSegment2D(Vec2 const& start, Vec2 const& end, Rgba8 const& color, float thickness, bool isInfinite);
void DrawLineSegment2D(LineSegment2 const& lineSegment, Rgba8 const& color, float thickness, bool isInfinite);
void DrawTriangle2D(Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2, Rgba8 const& color);
void DrawTriangle2D(const Triangle2& triangle, Rgba8 const& color);
void DrawAABB2D(const AABB2& aabb2, Rgba8 const& color);
void DrawOBB2D(const OBB2& obb2, Rgba8 const& color);
void DrawCapsule2D(Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color);
void DrawCapsule2D(const Capsule2& capsule, Rgba8 const& color);
void DrawArrow2D(Vec2 const& tailPos, Vec2 const& tipPos, float radius, float thickness, Rgba8 const& color);

extern Rgba8 const LIGHT_BLUE;
extern Rgba8 const BLUE;
extern Rgba8 const WHITE;
extern Rgba8 const TRANSLUCENT_WHITE;
extern Rgba8 const ORANGE;
extern Rgba8 const CYAN;
extern Rgba8 const GREY;

enum class GameMode
{
	GAME_MODE_NEAREST_POINT,
	GAME_MODE_RAYCAST_VS_DISCS
};
