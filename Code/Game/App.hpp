//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Game.hpp"
#include "Engine/Core/EventSystem.hpp"

class Camera;
//----------------------------------------------------------------------------------------------------

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

private:
    void BeginFrame() const;
    void Update();
    void Render() const;
    void EndFrame() const;

    void HandleKeyPressed();
    void HandleKeyReleased();
    void HandleQuitRequested();
    void AdjustForPauseAndTimeDistortion(float& deltaSeconds) const;
    void DeleteAndCreateNewGame();

    bool  m_isPaused           = false;
    bool  m_isSlowMo           = false;
    float m_timeLastFrameStart = 0.f;
    // Game*    m_theGame            = nullptr;
    eGameMode m_currentGameMode  = eGameMode::RAYCAST_VS_DISCS;
    Camera*   m_devConsoleCamera = nullptr;
};

static bool OnCloseButtonClicked(EventArgs& arg);
static void RequestQuit();
static bool m_isQuitting;
