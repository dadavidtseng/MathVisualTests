//----------------------------------------------------------------------------------------------------
// GameRaycastVsDiscs.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <vector>

#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Game/Game.hpp"

struct RaycastResult2D;
//----------------------------------------------------------------------------------------------------
class Camera;

//----------------------------------------------------------------------------------------------------
class GameRaycastVsDiscs : public Game
{
public:
	GameRaycastVsDiscs();
	~GameRaycastVsDiscs() override;

	void Update() override;
	void Render() const override;
	Vec2 GetMouseWorldPos() const;

private:
	void UpdateFromKeyboard(float deltaSeconds);
	void UpdateFromController(float deltaSeconds);
	Vec2 GenerateRandomPointInScreen() const;
	void GenerateRandomLineSegmentInScreen();
	void GenerateRandomDiscs();
	void RenderDisc2() const;
	void RenderRaycastResult() const;
	Vec2 ClampPointToScreen(const Vec2& point, float radius) const;
	bool IsTailPosInsideDisc(Vec2 const& startPos) const;


	Camera*      m_screenCamera  = nullptr;
	Vec2         m_baseCameraPos = Vec2(0.f, 0.f);
	Disc2        m_randomDisc[8] = {};
	LineSegment2 m_lineSegment;
	float        m_moveSpeed = 500.f;
};
