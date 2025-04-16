//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
// #include <functional>

#include <functional>

#include "GameCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Game/Game.hpp"

class GameCurves2D;
class GameShapes3D;
class GameRaycastVsAABBs;
class GameRaycastVsLineSegments;
class GameNearestPoint;
class GameRaycastVsDiscs;

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;

//----------------------------------------------------------------------------------------------------
template <typename T>
void DeleteAndCreateNewGame()
{
    static_assert(std::is_base_of_v<Game, T>, "T must be a subclass of Game");

    if (g_theGame != nullptr)
    {
        delete g_theGame;
        g_theGame = nullptr;
    }

    g_theGame = new T();
}

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

    static bool                               OnCloseButtonClicked(EventArgs& arg);
    static void                               RequestQuit();
    static bool                               m_isQuitting;
    static std::function<void()> s_gameModeConstructors[7];

private:
    void BeginFrame() const;
    void Update();
    void Render() const;
    void EndFrame() const;

    void LoadGameConfig(char const* gameConfigXmlFilePath);
    void UpdateFromFromKeyboard();
    void UpdateFromController();
    void UpdateCursorMode();

    Clock*                m_gameClock            = nullptr;
    eGameMode             m_currentGameMode      = eGameMode::NEAREST_POINT;
    Camera*               m_devConsoleCamera     = nullptr;

};





