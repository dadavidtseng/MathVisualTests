//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

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
class App
{
public:
    App()  = default;
    ~App() = default;

    void Startup();
    void Shutdown();
    void RunFrame();
    void RunMainLoop();
    //----------------------------------------------------------------------------------------------------
    template <typename T>
    static void                               DeleteAndCreateNewGame();
    static bool                               OnCloseButtonClicked(EventArgs& arg);
    static void                               RequestQuit();
    static bool                               m_isQuitting;
    static std::vector<std::function<void()>> s_gameModeConstructors;

private:
    void BeginFrame() const;
    void Update();
    void Render() const;
    void EndFrame() const;

    void LoadGameConfig(char const* gameConfigXmlFilePath);
    void UpdateFromFromKeyboard();
    void UpdateFromController();
    void UpdateCursorMode();

    // Clock*    m_gameClock        = nullptr;
    eGameMode m_currentGameMode  = eGameMode::NEAREST_POINT;
    Camera*   m_devConsoleCamera = nullptr;
};

