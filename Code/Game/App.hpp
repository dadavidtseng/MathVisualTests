//----------------------------------------------------------------------------------------------------
// App.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/EventSystem.hpp"
#include "Game/Game.hpp"

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

    static bool OnCloseButtonClicked(EventArgs& arg);
    static void RequestQuit();
    static bool m_isQuitting;

private:
    void BeginFrame() const;
    void Update();
    void Render() const;
    void EndFrame() const;

    void LoadGameConfig(char const* gameConfigXmlFilePath);
    void UpdateFromFromKeyboard();
    void UpdateFromController();
    void UpdateCursorMode();

    template <class T>
    void DeleteAndCreateNewGame() const;

    Clock*    m_gameClock        = nullptr;
    eGameMode m_currentGameMode  = eGameMode::NEAREST_POINT;
    Camera*   m_devConsoleCamera = nullptr;
};
