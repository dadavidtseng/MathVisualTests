//----------------------------------------------------------------------------------------------------
// GameCommon.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameCommon.hpp"

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Renderer/Renderer.hpp"

//-----------------------------------------------------------------------------------------------
// DebugRender color-related
//
Rgba8 const LIGHT_BLUE        = Rgba8(100, 150, 255);
Rgba8 const BLUE              = Rgba8(50, 80, 150);
Rgba8 const WHITE             = Rgba8(255, 255, 255);
Rgba8 const TRANSLUCENT_WHITE = Rgba8(255, 255, 255, 30);
Rgba8 const ORANGE            = Rgba8(255, 127, 0);
Rgba8 const CYAN            = Rgba8(0, 255, 255);
Rgba8 const GREY            = Rgba8(100, 100, 100);

//-----------------------------------------------------------------------------------------------
void DebugDrawRing(Vec2 const& center, const float radius, const float thickness, Rgba8 const& color)
{
	const float   halfThickness = 0.5f * thickness;
	const float   innerRadius   = radius - halfThickness;
	const float   outerRadius   = radius + halfThickness;
	constexpr int NUM_SIDES     = 32;
	constexpr int NUM_TRIS      = 2 * NUM_SIDES;
	constexpr int NUM_VERTS     = 3 * NUM_TRIS;
	Vertex_PCU    verts[NUM_VERTS];

	constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

	for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
	{
		// Compute angle-related terms
		const float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		const float endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float       cosStart     = CosDegrees(startDegrees);
		float       sinStart     = SinDegrees(startDegrees);
		float       cosEnd       = CosDegrees(endDegrees);
		float       sinEnd       = SinDegrees(endDegrees);

		// Compute inner & outer positions
		Vec3 innerStartPos(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 outerEndPos(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);
		Vec3 innerEndPos(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);

		// Trapezoid is made of two triangles; ABC and DEF
		// A is inner end; B is inner start; C is outer start
		// D is inner end; E is outer start; F is outer end
		int vertIndexA = 6 * sideNum + 0;
		int vertIndexB = 6 * sideNum + 1;
		int vertIndexC = 6 * sideNum + 2;
		int vertIndexD = 6 * sideNum + 3;
		int vertIndexE = 6 * sideNum + 4;
		int vertIndexF = 6 * sideNum + 5;

		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color    = color;
		verts[vertIndexB].m_color    = color;
		verts[vertIndexC].m_color    = color;

		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color    = color;
		verts[vertIndexE].m_color    = color;
		verts[vertIndexF].m_color    = color;
	}

	g_theRenderer->DrawVertexArray(NUM_VERTS, &verts[0]);
}

//-----------------------------------------------------------------------------------------------
void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color)
{
	Vec2 forward = end - start;
	Vec2 normal  = forward.GetNormalized().GetRotated90Degrees();

	Vec2 halfThicknessOffset = normal * (0.5f * thickness);

	Vec3 vertIndexA = Vec3(start.x - halfThicknessOffset.x, start.y - halfThicknessOffset.y, 0.f);
	Vec3 vertIndexB = Vec3(start.x + halfThicknessOffset.x, start.y + halfThicknessOffset.y, 0.f);
	Vec3 vertIndexC = Vec3(end.x + halfThicknessOffset.x, end.y + halfThicknessOffset.y, 0.f);
	Vec3 vertIndexD = Vec3(end.x - halfThicknessOffset.x, end.y - halfThicknessOffset.y, 0.f);

	Vertex_PCU verts[6];

	verts[0].m_position = vertIndexA;
	verts[1].m_position = vertIndexB;
	verts[2].m_position = vertIndexC;
	verts[0].m_color    = color;
	verts[1].m_color    = color;
	verts[2].m_color    = color;

	verts[3].m_position = vertIndexA;
	verts[4].m_position = vertIndexC;
	verts[5].m_position = vertIndexD;
	verts[3].m_color    = color;
	verts[4].m_color    = color;
	verts[5].m_color    = color;

	g_theRenderer->DrawVertexArray(6, &verts[0]);
}

//------------------------------------------------------------------------------------------------
void DebugDrawGlowCircle(Vec2 const& center, float radius, Rgba8 const& color, float glowIntensity)
{
	constexpr int NUM_SIDES = 32;           // Controls the smoothness of the circle
	constexpr int NUM_TRIS  = NUM_SIDES;    // One triangle for each segment
	constexpr int NUM_VERTS = 3 * NUM_TRIS; // Each triangle has 3 vertices
	Vertex_PCU    verts[NUM_VERTS];

	constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

	for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
	{
		// Calculate the start and end angles
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart     = CosDegrees(startDegrees);
		float sinStart     = SinDegrees(startDegrees);
		float cosEnd       = CosDegrees(endDegrees);
		float sinEnd       = SinDegrees(endDegrees);

		// Calculate the positions of the center and the edge vertices of the circle
		Vec3 centerPos(center.x, center.y, 0.f);                                        // Center of the circle
		Vec3 startPos(center.x + radius * cosStart, center.y + radius * sinStart, 0.f); // Starting point
		Vec3 endPos(center.x + radius * cosEnd, center.y + radius * sinEnd, 0.f);       // End point

		// The triangle is formed by (centerPos, startPos, endPos)
		int vertIndexA = 3 * sideNum + 0;
		int vertIndexB = 3 * sideNum + 1;
		int vertIndexC = 3 * sideNum + 2;

		verts[vertIndexA].m_position = centerPos;
		verts[vertIndexB].m_position = startPos;
		verts[vertIndexC].m_position = endPos;

		// The center uses a solid color, while the edges have a glow effect
		Rgba8 glowColor = color;
		glowColor.a     = static_cast<unsigned char>(glowIntensity * 255); // Set the alpha based on glowIntensity

		// Center color has no glow effect
		verts[vertIndexA].m_color = color;

		// Edge colors have a glow effect
		verts[vertIndexB].m_color = glowColor;
		verts[vertIndexC].m_color = glowColor;
	}

	g_theRenderer->DrawVertexArray(NUM_VERTS, &verts[0]);
}

void DebugDrawGlowBox(Vec2 const& center, Vec2 const& dimensions, Rgba8 const& color, float glowIntensity)
{
	// Calculate the four corners of the rectangle
	float halfWidth  = dimensions.x * 0.5f;
	float halfHeight = dimensions.y * 0.5f;

	Vec3 topLeft(center.x - halfWidth, center.y + halfHeight, 0.f);
	Vec3 topRight(center.x + halfWidth, center.y + halfHeight, 0.f);
	Vec3 bottomLeft(center.x - halfWidth, center.y - halfHeight, 0.f);
	Vec3 bottomRight(center.x + halfWidth, center.y - halfHeight, 0.f);

	// A rectangle is made of two triangles, each having 3 vertices, total of 6 vertices
	constexpr int NUM_VERTS = 6;
	Vertex_PCU    verts[NUM_VERTS];

	// Set the vertices of triangle 1 (bottomLeft, bottomRight, topLeft)
	verts[0].m_position = bottomLeft;
	verts[1].m_position = bottomRight;
	verts[2].m_position = topLeft;

	// Set the vertices of triangle 2 (topLeft, bottomRight, topRight)
	verts[3].m_position = topLeft;
	verts[4].m_position = bottomRight;
	verts[5].m_position = topRight;

	// Glow color, edge vertices will have a glow effect
	Rgba8 glowColor = color;
	glowColor.a     = static_cast<unsigned char>(glowIntensity * 255); // Adjust alpha based on glowIntensity

	// Set the color of the vertices
	for (int i = 0; i < NUM_VERTS; ++i)
	{
		// Center color (in the middle part of the rectangle) does not have a glow effect
		if (i == 2 || i == 3)
		{
			verts[i].m_color = color; // Top left is shared by both triangles
		}
		else
		{
			verts[i].m_color = glowColor;
		}
	}

	// Draw the vertex array
	g_theRenderer->DrawVertexArray(NUM_VERTS, &verts[0]);
}


void DebugDrawBoxRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
	float halfThickness = 0.5f * thickness;
	float innerRadius   = radius - halfThickness;
	float outerRadius   = radius + halfThickness;

	// Define the inner and outer box corners
	Vec3 innerBottomLeft(center.x - innerRadius, center.y - innerRadius, 0.f);
	Vec3 innerBottomRight(center.x + innerRadius, center.y - innerRadius, 0.f);
	Vec3 innerTopLeft(center.x - innerRadius, center.y + innerRadius, 0.f);
	Vec3 innerTopRight(center.x + innerRadius, center.y + innerRadius, 0.f);

	Vec3 outerBottomLeft(center.x - outerRadius, center.y - outerRadius, 0.f);
	Vec3 outerBottomRight(center.x + outerRadius, center.y - outerRadius, 0.f);
	Vec3 outerTopLeft(center.x - outerRadius, center.y + outerRadius, 0.f);
	Vec3 outerTopRight(center.x + outerRadius, center.y + outerRadius, 0.f);

	// Define 8 triangles to form the ring (each side of the square gets 2 triangles)
	Vertex_PCU verts[24]; // 8 triangles * 3 vertices = 24

	// Bottom side (outerBottomLeft -> innerBottomLeft -> innerBottomRight -> outerBottomRight)
	verts[0].m_position = outerBottomLeft;
	verts[1].m_position = innerBottomLeft;
	verts[2].m_position = innerBottomRight;

	verts[3].m_position = outerBottomLeft;
	verts[4].m_position = innerBottomRight;
	verts[5].m_position = outerBottomRight;

	// Top side (outerTopLeft -> innerTopRight -> innerTopLeft -> outerTopRight)
	verts[6].m_position = outerTopLeft;
	verts[7].m_position = innerTopRight;
	verts[8].m_position = innerTopLeft;

	verts[9].m_position  = outerTopLeft;
	verts[10].m_position = innerTopRight;
	verts[11].m_position = outerTopRight;

	// Left side (outerBottomLeft -> innerBottomLeft -> innerTopLeft -> outerTopLeft)
	verts[12].m_position = outerBottomLeft;
	verts[13].m_position = innerBottomLeft;
	verts[14].m_position = innerTopLeft;

	verts[15].m_position = outerBottomLeft;
	verts[16].m_position = innerTopLeft;
	verts[17].m_position = outerTopLeft;

	// Right side (outerBottomRight -> innerTopRight -> innerBottomRight -> outerTopRight)
	verts[18].m_position = outerBottomRight;
	verts[19].m_position = innerTopRight;
	verts[20].m_position = innerBottomRight;

	verts[21].m_position = outerBottomRight;
	verts[22].m_position = innerTopRight;
	verts[23].m_position = outerTopRight;

	// Set color for all vertices
	for (int i = 0; i < 24; ++i)
	{
		verts[i].m_color = color;
	}

	g_theRenderer->DrawVertexArray(24, &verts[0]);
}

//-----------------------------------------------------------------------------------------------
void DrawDisc2(Vec2 const& center, float radius, Rgba8 const& color)
{
	std::vector<Vertex_PCU> verts;
	AddVertsForDisc2D(verts, center, radius, color);
	g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//-----------------------------------------------------------------------------------------------
void DrawDisc2(Disc2 const& disc, Rgba8 const& color)
{
	DrawDisc2(disc.m_position, disc.m_radius, color);
}

//-----------------------------------------------------------------------------------------------
void DrawLineSegment2D(Vec2 const& start, Vec2 const& end, Rgba8 const& color, float thickness, bool isInfinite)
{
	Vec2 forward = end - start;
	Vec2 normal  = forward.GetNormalized().GetRotated90Degrees();

	Vec2 halfThicknessOffset = normal * (0.5f * thickness);

	Vec3 vertIndexA;
	Vec3 vertIndexB;
	Vec3 vertIndexC;
	Vec3 vertIndexD;

	if (isInfinite)
	{
		// Calculate the infinite extension points
		float extensionFactor = 10000.0f; // This value can be adjusted based on screen size

		// Extend start and end points
		Vec2 extendedStart = start - forward.GetNormalized() * extensionFactor;
		Vec2 extendedEnd   = end + forward.GetNormalized() * extensionFactor;

		vertIndexA = Vec3(extendedStart.x - halfThicknessOffset.x, extendedStart.y - halfThicknessOffset.y, 0.f);
		vertIndexB = Vec3(extendedStart.x + halfThicknessOffset.x, extendedStart.y + halfThicknessOffset.y, 0.f);
		vertIndexC = Vec3(extendedEnd.x + halfThicknessOffset.x, extendedEnd.y + halfThicknessOffset.y, 0.f);
		vertIndexD = Vec3(extendedEnd.x - halfThicknessOffset.x, extendedEnd.y - halfThicknessOffset.y, 0.f);
	}
	else
	{
		vertIndexA = Vec3(start.x - halfThicknessOffset.x, start.y - halfThicknessOffset.y, 0.f);
		vertIndexB = Vec3(start.x + halfThicknessOffset.x, start.y + halfThicknessOffset.y, 0.f);
		vertIndexC = Vec3(end.x + halfThicknessOffset.x, end.y + halfThicknessOffset.y, 0.f);
		vertIndexD = Vec3(end.x - halfThicknessOffset.x, end.y - halfThicknessOffset.y, 0.f);
	}

	Vertex_PCU verts[6];

	// First triangle
	verts[0].m_position = vertIndexA;
	verts[1].m_position = vertIndexB;
	verts[2].m_position = vertIndexC;
	verts[0].m_color    = color;
	verts[1].m_color    = color;
	verts[2].m_color    = color;

	// Second triangle
	verts[3].m_position = vertIndexA;
	verts[4].m_position = vertIndexC;
	verts[5].m_position = vertIndexD;
	verts[3].m_color    = color;
	verts[4].m_color    = color;
	verts[5].m_color    = color;

	// Draw the vertex array for the line segment
	g_theRenderer->DrawVertexArray(6, verts);
}

//-----------------------------------------------------------------------------------------------
void DrawLineSegment2D(LineSegment2 const& lineSegment, Rgba8 const& color, float thickness, bool isInfinite)
{
	DrawLineSegment2D(lineSegment.m_start, lineSegment.m_end, color, thickness, isInfinite);
}

//-----------------------------------------------------------------------------------------------
void DrawTriangle2D(Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2, Rgba8 const& color)
{
	std::vector<Vertex_PCU> verts;
	AddVertsForTriangle2D(verts, ccw0, ccw1, ccw2, color);
	g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//-----------------------------------------------------------------------------------------------
void DrawTriangle2D(const Triangle2& triangle, Rgba8 const& color)
{
	DrawTriangle2D(triangle.m_positionCounterClockwise[0], triangle.m_positionCounterClockwise[1],
	               triangle.m_positionCounterClockwise[2], color);
}

//-----------------------------------------------------------------------------------------------
void DrawAABB2D(const AABB2& aabb2, Rgba8 const& color)
{
	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, aabb2, color);

	g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void DrawOBB2D(const OBB2& obb2, Rgba8 const& color)
{
	std::vector<Vertex_PCU> verts;
	AddVertsForOBB2D(verts, obb2, color);
	g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void DrawCapsule2D(Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color)
{
	std::vector<Vertex_PCU> verts;
	AddVertsForCapsule2D(verts, boneStart, boneEnd, radius, color);

	g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void DrawCapsule2D(const Capsule2& capsule, Rgba8 const& color)
{
	DrawCapsule2D(capsule.m_start, capsule.m_end, capsule.m_radius, color);
}
void DrawArrow2D(Vec2 const& tailPos, Vec2 const& tipPos, float radius, float thickness, Rgba8 const& color)
{
	std::vector<Vertex_PCU> verts;

	AddVertsForArrow2D(verts, tailPos, tipPos, radius, thickness, color);

	g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}
