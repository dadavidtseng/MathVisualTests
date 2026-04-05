//----------------------------------------------------------------------------------------------------
// App.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/App.hpp"
//----------------------------------------------------------------------------------------------------
#include "Game/GameCommon.hpp"
#include "Game/GameCurves2D.hpp"
#include "Game/GameNearestPoint.hpp"
#include "Game/GamePachinkoMachine2D.hpp"
#include "Game/GameRaycastVsAABBs.hpp"
#include "Game/GameRaycastVsDiscs.hpp"
#include "Game/GameRaycastVsLineSegments.hpp"
#include "Game/GameConvexScene.hpp"
#include "Game/GameShapes3D.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"

//----------------------------------------------------------------------------------------------------
App*                   g_app        = nullptr;      // Created and owned by Main_Windows.cpp
Game*                  g_game       = nullptr;      // Created and owned by the App

//----------------------------------------------------------------------------------------------------
STATIC bool App::m_isQuitting = false;

//----------------------------------------------------------------------------------------------------
std::vector<std::function<void()>> App::s_gameModeConstructors =
{
    [] { DeleteAndCreateNewGame<GameNearestPoint>(); },
    [] { DeleteAndCreateNewGame<GameRaycastVsDiscs>(); },
    [] { DeleteAndCreateNewGame<GameRaycastVsLineSegments>(); },
    [] { DeleteAndCreateNewGame<GameRaycastVsAABBs>(); },
    [] { DeleteAndCreateNewGame<GameShapes3D>(); },
    [] { DeleteAndCreateNewGame<GameCurves2D>(); },
    [] { DeleteAndCreateNewGame<GamePachinkoMachine2D>(); },
    [] { DeleteAndCreateNewGame<GameConvexScene>(); }
};

//----------------------------------------------------------------------------------------------------
App::App()
{
    GEngine::Get().Construct();
}

//----------------------------------------------------------------------------------------------------
App::~App()
{
    GEngine::Get().Destruct();
}

//----------------------------------------------------------------------------------------------------
void App::Startup()
{
    GEngine::Get().Startup();

    LoadGameConfig("Data/GameConfig.xml");

    g_eventSystem->SubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);
    g_eventSystem->SubscribeEventCallbackFunction("quit", OnCloseButtonClicked);

    g_game       = new GameNearestPoint();
}

//----------------------------------------------------------------------------------------------------
// All Destroy and ShutDown process should be reverse order of the StartUp
//
void App::Shutdown()
{
    GAME_SAFE_RELEASE(g_game);

    g_eventSystem->UnsubscribeEventCallbackFunction("quit", OnCloseButtonClicked);
    g_eventSystem->UnsubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);

    GEngine::Get().Shutdown();
}

//----------------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
//
void App::RunFrame()
{
    BeginFrame();           // Engine pre-frame stuff
    Update();               // Game updates / moves / spawns / hurts / kills stuff
    Render();               // Game draws current state of things
    EndFrame();             // Engine post-frame stuff
}

//----------------------------------------------------------------------------------------------------
void App::RunMainLoop()
{
    // Program main loop; keep running frames until it's time to quit
    while (!m_isQuitting)
    {
        // Sleep(16); // Temporary code to "slow down" our app to ~60Hz until we have proper frame timing in
        RunFrame();
    }
}


template <typename T>
void App::DeleteAndCreateNewGame()
{
    static_assert(std::is_base_of_v<Game, T>, "T must be a subclass of Game");

    if (g_game != nullptr)
    {
        delete g_game;
        g_game = nullptr;
    }

    g_game = new T();
}

//----------------------------------------------------------------------------------------------------
bool App::OnCloseButtonClicked(EventArgs& arg)
{
    UNUSED(arg)

    RequestQuit();
    return true;
}

//----------------------------------------------------------------------------------------------------
void App::RequestQuit()
{
    m_isQuitting = true;
}

//----------------------------------------------------------------------------------------------------
void App::BeginFrame() const
{
    g_eventSystem->BeginFrame();
    g_window->BeginFrame();
    g_renderer->BeginFrame();
    DebugRenderBeginFrame();
    g_devConsole->BeginFrame();
    g_input->BeginFrame();
}

//----------------------------------------------------------------------------------------------------
void App::Update()
{
    Clock::TickSystemClock();
    g_input->SetCursorMode(eCursorMode::POINTER);
    UpdateFromFromKeyboard();
    UpdateFromController();
    g_game->Update();
}

//----------------------------------------------------------------------------------------------------
// Some simple OpenGL example drawing code.
// This is the graphical equivalent of printing "Hello, world."
//
// Ultimately this function (App::Render) will only call methods on Renderer (like Renderer::DrawVertexArray)
//	to draw things, never calling OpenGL (nor DirectX) functions directly.
//
void App::Render() const
{
    Rgba8 const clearColor = Rgba8::BLACK;

    g_renderer->ClearScreen(clearColor);
    g_game->Render();
    g_devConsole->Render(AABB2(Vec2::ZERO, Vec2(1600.f, 30.f)));
}

//----------------------------------------------------------------------------------------------------
void App::EndFrame() const
{
    g_eventSystem->EndFrame();
    g_window->EndFrame();
    g_renderer->EndFrame();
    DebugRenderEndFrame();
    g_devConsole->EndFrame();
    g_input->EndFrame();
}

//----------------------------------------------------------------------------------------------------
void App::LoadGameConfig(char const* gameConfigXmlFilePath) const
{
    XmlDocument     gameConfigXml;
    XmlResult const result = gameConfigXml.LoadFile(gameConfigXmlFilePath);

    if (result == XmlResult::XML_SUCCESS)
    {
        if (XmlElement const* rootElement = gameConfigXml.RootElement())
        {
            g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
        }
        else
        {
            printf("WARNING: game config from file \"%s\" was invalid (missing root element)\n", gameConfigXmlFilePath);
        }
    }
    else
    {
        printf("WARNING: failed to load game config from file \"%s\"\n", gameConfigXmlFilePath);
    }
}

//----------------------------------------------------------------------------------------------------
void App::UpdateFromFromKeyboard()
{
    int constexpr gameModeCount = static_cast<int>(eGameMode::COUNT);

    if (g_input->WasKeyJustPressed(KEYCODE_F6))
    {
        // Backward
        m_currentGameMode = static_cast<eGameMode>((static_cast<int>(m_currentGameMode) + gameModeCount - 1) % gameModeCount);
        s_gameModeConstructors[static_cast<int>(m_currentGameMode)]();
    }

    if (g_input->WasKeyJustPressed(KEYCODE_F7))
    {
        // Forward
        m_currentGameMode = static_cast<eGameMode>((static_cast<int>(m_currentGameMode) + 1) % gameModeCount);
        s_gameModeConstructors[static_cast<int>(m_currentGameMode)]();
    }
}

//----------------------------------------------------------------------------------------------------
void App::UpdateFromController()
{
    XboxController const& controller = g_input->GetController(0);

    UNUSED(controller)
}

//----------------------------------------------------------------------------------------------------
void App::UpdateCursorMode()
{
    bool const doesWindowHasFocus   = GetActiveWindow() == g_window->GetWindowHandle();
    bool const shouldUsePointerMode = !doesWindowHasFocus || g_devConsole->IsOpen();

    if (shouldUsePointerMode == true)
    {
        g_input->SetCursorMode(eCursorMode::POINTER);
    }
    else
    {
        g_input->SetCursorMode(eCursorMode::FPS);
    }
}
