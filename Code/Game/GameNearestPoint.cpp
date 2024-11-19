//----------------------------------------------------------------------------------------------------
// GameNearestPoint.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameNearestPoint.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/SimpleTriangleFont.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameNearestPoint::GameNearestPoint()
{
	m_screenCamera = new Camera();
	m_screenCamera->SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	m_referencePoint = Vec2(SCREEN_SIZE_X / 2.f, SCREEN_SIZE_Y / 2.f);

	GenerateRandomShapes();
}

//----------------------------------------------------------------------------------------------------
GameNearestPoint::~GameNearestPoint()
{
	delete m_screenCamera;
	m_screenCamera = nullptr;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::Update(const float deltaSeconds)
{
	UpdateFromKeyBoard(deltaSeconds);
	UpdateFromController(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::Render() const
{
	g_theRenderer->BeginCamera(*m_screenCamera);
	RenderShapes();

	//TODO: The current mode, and relevant keys & controls, are printed in text near the top of the screen.
	std::vector<Vertex_PCU> titleVerts;
	AddVertsForTextTriangles2D(titleVerts,
							   "CURRENT MODE: NearestPoint",
							   Vec2(10.f, SCREEN_SIZE_Y-60.f),
							   50.f,
							   WHITE,
							   1.f,
							   true,
							   0.1f);
	g_theRenderer->DrawVertexArray(static_cast<int>(titleVerts.size()), titleVerts.data());
	
	g_theRenderer->EndCamera(*m_screenCamera);
}

//------------------------------------------------------------------------------------------------
Vec2 GameNearestPoint::GetMouseWorldPos() const
{
	Vec2 const mouseUV = g_theWindow->GetNormalizedMouseUV();
	Vec2 bottomLeft = m_screenCamera->GetOrthoBottomLeft();
	Vec2 topRight = m_screenCamera->GetOrthoTopRight();
	AABB2 const	orthoBounds( bottomLeft, topRight );
	return orthoBounds.GetPointAtUV( mouseUV );
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::UpdateFromKeyBoard(const float deltaSeconds)
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		GenerateRandomShapes();
		m_referencePoint = Vec2(SCREEN_SIZE_X / 2.f, SCREEN_SIZE_Y / 2.f);
	}

	if (g_theInput->IsKeyDown(KEYCODE_W))
		m_referencePoint.y += m_moveSpeed * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_S))
		m_referencePoint.y -= m_moveSpeed * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_A))
		m_referencePoint.x -= m_moveSpeed * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_D))
		m_referencePoint.x += m_moveSpeed * deltaSeconds;

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
		m_referencePoint = GetMouseWorldPos();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::UpdateFromController(const float deltaSeconds)
{
	UNUSED(deltaSeconds)
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderShapes() const
{
	RenderDisc2();
	RenderLineSegment2();
	RenderLineInfinite2();
	RenderTriangle2D();
	RenderAABB2();
	RenderOBB2();
	RenderCapsule2D();
	RenderReferencePoint();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomShapes()
{
	GenerateRandomDisc2();
	GenerateRandomLineSegment2();
	GenerateRandomInfiniteLine2();
	GenerateRandomTriangle2D();
	GenerateRandomAABB2();
	GenerateRandomOBB2();
	GenerateRandomCapsule2D();
}

//----------------------------------------------------------------------------------------------------
Vec2 GameNearestPoint::GenerateRandomPointInScreen() const
{
	const float randomX = g_theRNG->RollRandomFloatInRange(0, SCREEN_SIZE_X);
	const float randomY = g_theRNG->RollRandomFloatInRange(0, SCREEN_SIZE_Y);

	return Vec2(randomX, randomY);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomDisc2()
{
	const float randomRadius = g_theRNG->RollRandomFloatInRange(10.f, 200.f);
	Vec2        center       = GenerateRandomPointInScreen();
	center                   = ClampPointToScreen(center, randomRadius);

	m_randomDisc = Disc2(center, randomRadius);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomLineSegment2()
{
	Vec2        start           = GenerateRandomPointInScreen();
	Vec2        end             = GenerateRandomPointInScreen();
	const float randomThickness = g_theRNG->RollRandomFloatInRange(1.f, 5.f);

	// Ensure the line is within screen bounds
	start = ClampPointToScreen(start, randomThickness);
	end   = ClampPointToScreen(end, randomThickness);

	m_randomLineSegment = LineSegment2(start, end, randomThickness, false);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomInfiniteLine2()
{
	Vec2        start        = GenerateRandomPointInScreen();
	Vec2        end          = GenerateRandomPointInScreen();
	const float randomRadius = g_theRNG->RollRandomFloatInRange(1.f, 5.f);

	// Ensure the line is within screen bounds
	start = ClampPointToScreen(start, randomRadius);
	end   = ClampPointToScreen(end, randomRadius);

	m_randomInfiniteLine = LineSegment2(start, end, randomRadius, true);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomTriangle2D()
{
	const Vec2 randomA = GenerateRandomPointInScreen();
	const Vec2 randomB = GenerateRandomPointInScreen();
	const Vec2 randomC = GenerateRandomPointInScreen();

	m_randomTriangle = Triangle2(randomA, randomB, randomC);

	// Ensure the triangle is valid
	const Vec2 edge1 = randomB - randomA;
	const Vec2 edge2 = randomC - randomA;

	if (DotProduct2D(edge1, edge2) < 0.f)
	{
		m_randomTriangle = Triangle2(randomA, randomC, randomB);
	}
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomAABB2()
{
	const float randomWidth  = g_theRNG->RollRandomFloatInRange(0.1f, SCREEN_SIZE_X / 2);
	const float randomHeight = g_theRNG->RollRandomFloatInRange(0.1f, SCREEN_SIZE_Y / 2);

	Vec2 center     = GenerateRandomPointInScreen();
	Vec2 randomMins = ClampPointToScreen(center - Vec2(randomWidth / 2, randomHeight / 2), randomWidth / 2,
	                                     randomHeight / 2);
	Vec2 randomMaxs = ClampPointToScreen(center + Vec2(randomWidth / 2, randomHeight / 2), randomWidth / 2,
	                                     randomHeight / 2);

	m_randomAABB2 = AABB2(randomMins, randomMaxs);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomOBB2()
{
	const Vec2 center         = GenerateRandomPointInScreen();
	const Vec2 halfDimensions = Vec2(g_theRNG->RollRandomFloatInRange(10.f, SCREEN_SIZE_X / 4),
	                                 g_theRNG->RollRandomFloatInRange(10.f, SCREEN_SIZE_Y / 4));
	m_randomOBB2 = OBB2(center, Vec2(1.f, 0.f), halfDimensions); // Angle is set to 0 for simplicity
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomCapsule2D()
{
	Vec2        start        = GenerateRandomPointInScreen();
	Vec2        end          = GenerateRandomPointInScreen();
	const float randomRadius = g_theRNG->RollRandomFloatInRange(10.f, 200.f);

	start = ClampPointToScreen(start, randomRadius);
	end   = ClampPointToScreen(end, randomRadius);

	m_randomCapsule2 = Capsule2(start, end, randomRadius);
}

//----------------------------------------------------------------------------------------------------
Vec2 GameNearestPoint::ClampPointToScreen(const Vec2& point, float radius) const
{
	Vec2 clampedPoint = point;
	clampedPoint.x    = GetClamped(clampedPoint.x, radius, SCREEN_SIZE_X - radius);
	clampedPoint.y    = GetClamped(clampedPoint.y, radius, SCREEN_SIZE_Y - radius);
	return clampedPoint;
}

//----------------------------------------------------------------------------------------------------
Vec2 GameNearestPoint::ClampPointToScreen(const Vec2& point, float halfWidth, float halfHeight) const
{
	Vec2 clampedPoint = point;
	clampedPoint.x    = GetClamped(clampedPoint.x, halfWidth, SCREEN_SIZE_X - halfWidth);
	clampedPoint.y    = GetClamped(clampedPoint.y, halfHeight, SCREEN_SIZE_Y - halfHeight);
	return clampedPoint;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderDisc2() const
{
	const Vec2 nearestPoint = m_randomDisc.GetNearestPoint(m_referencePoint);
	DrawDisc2(m_randomDisc, m_randomDisc.IsPointInside(m_referencePoint) ? LIGHT_BLUE : BLUE);
	DrawLineSegment2D(m_referencePoint, nearestPoint, TRANSLUCENT_WHITE, 3.0f, false);
	DrawDisc2(nearestPoint, 5.0f, ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderLineSegment2() const
{
	const Vec2 nearestPoint = m_randomLineSegment.GetNearestPoint(m_referencePoint);
	DrawLineSegment2D(m_randomLineSegment, BLUE, m_randomLineSegment.m_thickness, m_randomLineSegment.m_isInfinite);
	DrawLineSegment2D(m_referencePoint, nearestPoint, TRANSLUCENT_WHITE, 3.0f, false);
	DrawDisc2(nearestPoint, 5.0f, ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderLineInfinite2() const
{
	const Vec2 nearestPoint = m_randomInfiniteLine.GetNearestPoint(m_referencePoint);
	DrawLineSegment2D(m_randomInfiniteLine, BLUE, m_randomInfiniteLine.m_thickness, m_randomInfiniteLine.m_isInfinite);
	DrawLineSegment2D(m_referencePoint, nearestPoint, TRANSLUCENT_WHITE, 3.0f, false);
	DrawDisc2(nearestPoint, 5.0f, ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderTriangle2D() const
{
	const Vec2 nearestPoint = m_randomTriangle.GetNearestPoint(m_referencePoint);
	DrawTriangle2D(m_randomTriangle, m_randomTriangle.IsPointInside(m_referencePoint) ? LIGHT_BLUE : BLUE);
	DrawLineSegment2D(m_referencePoint, nearestPoint, TRANSLUCENT_WHITE, 3.0f, false);
	DrawDisc2(nearestPoint, 5.0f, ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderAABB2() const
{
	const Vec2 nearestPoint = m_randomAABB2.GetNearestPoint(m_referencePoint);
	DrawAABB2D(m_randomAABB2, m_randomAABB2.IsPointInside(m_referencePoint) ? LIGHT_BLUE : BLUE);
	DrawLineSegment2D(m_referencePoint, nearestPoint, TRANSLUCENT_WHITE, 3.0f, false);
	DrawDisc2(nearestPoint, 5.0f, ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderOBB2() const
{
	const Vec2 nearestPoint = m_randomOBB2.GetNearestPoint(m_referencePoint);
	DrawOBB2D(m_randomOBB2, m_randomOBB2.IsPointInside(m_referencePoint) ? LIGHT_BLUE : BLUE);
	DrawLineSegment2D(m_referencePoint, nearestPoint, TRANSLUCENT_WHITE, 3.0f, false);
	DrawDisc2(nearestPoint, 5.0f, ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderCapsule2D() const
{
	const Vec2 nearestPoint = m_randomCapsule2.GetNearestPoint(m_referencePoint);
	DrawCapsule2D(m_randomCapsule2, m_randomCapsule2.IsPointInside(m_referencePoint) ? LIGHT_BLUE : BLUE);
	DrawLineSegment2D(m_referencePoint, nearestPoint, TRANSLUCENT_WHITE, 3.0f, false);
	DrawDisc2(nearestPoint, 5.0f, ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderReferencePoint() const
{
	DrawDisc2(m_referencePoint, 3.f, WHITE);
}
