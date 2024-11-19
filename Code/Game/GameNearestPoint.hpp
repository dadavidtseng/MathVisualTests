//----------------------------------------------------------------------------------------------------
// GameNearestPoint.hpp
//----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Game/Game.hpp"

//-----------------------------------------------------------------------------------------------
class Camera;

//-----------------------------------------------------------------------------------------------
class GameNearestPoint : public Game
{
public:
	GameNearestPoint();
	~GameNearestPoint() override;

	void Update(float deltaSeconds) override;
	void Render() const override;
	Vec2 GetMouseWorldPos() const;

private:
	void UpdateFromKeyBoard( float deltaSeconds);
	void UpdateFromController( float deltaSeconds);

	void RenderShapes() const;
	void GenerateRandomShapes();

	Vec2 GenerateRandomPointInScreen() const;
	void GenerateRandomDisc2();
	void GenerateRandomLineSegment2();
	void GenerateRandomInfiniteLine2();
	void GenerateRandomTriangle2D();
	void GenerateRandomAABB2();
	void GenerateRandomOBB2();
	void GenerateRandomCapsule2D();
	Vec2 ClampPointToScreen(const Vec2& point, float radius) const;
	Vec2 ClampPointToScreen(const Vec2& point, float halfWidth, float halfHeight) const;
	void RenderDisc2() const;
	void RenderLineSegment2() const;
	void RenderLineInfinite2() const;
	void RenderTriangle2D() const;
	void RenderAABB2() const;
	void RenderOBB2() const;
	void RenderCapsule2D() const;

	void RenderReferencePoint() const;

	Camera*      m_screenCamera  = nullptr;
	Vec2         m_baseCameraPos = Vec2(0.f, 0.f);
	Vec2         m_referencePoint;
	float        m_moveSpeed = 500.f;
	Disc2        m_randomDisc;
	LineSegment2 m_randomLineSegment;
	LineSegment2 m_randomInfiniteLine;
	Triangle2    m_randomTriangle;
	AABB2        m_randomAABB2;
	OBB2         m_randomOBB2;
	Capsule2     m_randomCapsule2;
};
