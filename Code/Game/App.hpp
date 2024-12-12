//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EventSystem.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
class Game;
class UIHandler;

//----------------------------------------------------------------------------------------------------
class App
{
public:
	App()  = default;
	~App() = default;
	void Startup();
	void Shutdown();
	void RunFrame();

	void RunMainLoop();
	// bool IsQuitting() const;
	

private:
	void BeginFrame() const;
	void Update(float deltaSeconds);
	void Render() const;
	void EndFrame() const;

	void HandleKeyPressed();
	void HandleKeyReleased();
	void HandleQuitRequested();
	void AdjustForPauseAndTimeDistortion(float& deltaSeconds) const;
	void DeleteAndCreateNewGame();

	bool     m_isPaused           = false;
	bool     m_isSlowMo           = false;
	float    m_timeLastFrameStart = 0.f;
	Game*    m_theGame            = nullptr;
	GameMode m_currentGameMode{GameMode::GAME_MODE_RAYCAST_VS_DISCS};
};

static bool OnWindowClose(EventArgs& arg);
static void RequestQuit();
static bool m_isQuitting;
