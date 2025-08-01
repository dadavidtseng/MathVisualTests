//----------------------------------------------------------------------------------------------------
// App.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/App.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Platform/Window.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameCurves2D.hpp"
#include "Game/GameNearestPoint.hpp"
#include "Game/GamePachinkoMachine2D.hpp"
#include "Game/GameRaycastVsAABBs.hpp"
#include "Game/GameRaycastVsDiscs.hpp"
#include "Game/GameRaycastVsLineSegments.hpp"
#include "Game/GameShapes3D.hpp"

//----------------------------------------------------------------------------------------------------
App*                   g_theApp        = nullptr;      // Created and owned by Main_Windows.cpp
BitmapFont*            g_theBitmapFont = nullptr;      // Created and owned by the App
Game*                  g_theGame       = nullptr;      // Created and owned by the App
Renderer*              g_theRenderer   = nullptr;      // Created and owned by the App
RandomNumberGenerator* g_theRNG        = nullptr;      // Created and owned by the App
Window*                g_theWindow     = nullptr;      // Created and owned by the App

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
    [] { DeleteAndCreateNewGame<GamePachinkoMachine2D>(); }
};

//----------------------------------------------------------------------------------------------------
void App::Startup()
{
    LoadGameConfig("Data/GameConfig.xml");

    sEventSystemConfig eventSystemConfig;
    g_theEventSystem = new EventSystem(eventSystemConfig);
    g_theEventSystem->SubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);
    g_theEventSystem->SubscribeEventCallbackFunction("quit", OnCloseButtonClicked);

    sInputSystemConfig inputConfig;
    g_theInput = new InputSystem(inputConfig);

    sWindowConfig windowConfig;
    windowConfig.m_aspectRatio = 2.f;
    windowConfig.m_inputSystem = g_theInput;

    windowConfig.m_consoleTitle[0]  = " .----------------.  .----------------.  .----------------.\n";
    windowConfig.m_consoleTitle[1]  = "| .--------------. || .--------------. || .--------------. |\n";
    windowConfig.m_consoleTitle[2]  = "| | ____    ____ | || | ____   ____  | || |  _________   | |\n";
    windowConfig.m_consoleTitle[3]  = "| ||_   \\  /   _|| || ||_  _| |_  _| | || | |  _   _  |  | |\n";
    windowConfig.m_consoleTitle[4]  = "| |  |   \\/   |  | || |  \\ \\   / /   | || | |_/ | | \\_|  | |\n";
    windowConfig.m_consoleTitle[5]  = "| |  | |\\  /| |  | || |   \\ \\ / /    | || |     | |      | |\n";
    windowConfig.m_consoleTitle[6]  = "| | _| |_\\/_| |_ | || |    \\ ' /     | || |    _| |_     | |\n";
    windowConfig.m_consoleTitle[7]  = "| ||_____||_____|| || |     \\_/      | || |   |_____|    | |\n";
    windowConfig.m_consoleTitle[8]  = "| |              | || |              | || |              | |\n";
    windowConfig.m_consoleTitle[9]  = "| '--------------' || '--------------' || '--------------' |\n";
    windowConfig.m_consoleTitle[10] = " '----------------'  '----------------'  '----------------' \n";

    windowConfig.m_windowTitle = "Math Visual Tests";
    g_theWindow                = new Window(windowConfig);

    sRenderConfig renderConfig;
    renderConfig.m_window = g_theWindow;
    g_theRenderer         = new Renderer(renderConfig);

    sDebugRenderConfig debugConfig;
    debugConfig.m_renderer = g_theRenderer;
    debugConfig.m_fontName = "SquirrelFixedFont";

    // Initialize devConsoleCamera
    m_devConsoleCamera = new Camera();

    Vec2 const bottomLeft = Vec2::ZERO;

    float const screenSizeX    = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY    = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    Vec2 const  screenTopRight = Vec2(screenSizeX, screenSizeY);

    m_devConsoleCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);


    sDevConsoleConfig devConsoleConfig;
    devConsoleConfig.m_defaultRenderer = g_theRenderer;
    devConsoleConfig.m_defaultFontName = "SquirrelFixedFont";
    devConsoleConfig.m_defaultCamera   = m_devConsoleCamera;
    g_theDevConsole                    = new DevConsole(devConsoleConfig);

    g_theEventSystem->Startup();
    g_theWindow->Startup();
    g_theRenderer->Startup();
    DebugRenderSystemStartup(debugConfig);
    g_theDevConsole->StartUp();
    g_theInput->Startup();

    g_theBitmapFont = g_theRenderer->CreateOrGetBitmapFontFromFile("Data/Fonts/SquirrelFixedFont"); // DO NOT SPECIFY FILE .EXTENSION!!  (Important later on.)
    g_theRNG        = new RandomNumberGenerator();
    g_theGame       = new GameNearestPoint();
    m_devConsoleCamera->SetNormalizedViewport(AABB2(Vec2::ZERO, Vec2::ONE));
    // m_gameClock = new Clock(Clock::GetSystemClock());
}

//----------------------------------------------------------------------------------------------------
// All Destroy and ShutDown process should be reverse order of the StartUp
//
void App::Shutdown()
{
    // Destroy all Engine Subsystem
    GAME_SAFE_RELEASE(g_theGame);
    GAME_SAFE_RELEASE(g_theRNG);
    GAME_SAFE_RELEASE(g_theBitmapFont);
    GAME_SAFE_RELEASE(m_devConsoleCamera);

    g_theInput->Shutdown();
    g_theDevConsole->Shutdown();

    DebugRenderSystemShutdown();
    g_theRenderer->Shutdown();

    g_theWindow->Shutdown();
    g_theEventSystem->Shutdown();

    GAME_SAFE_RELEASE(g_theInput);
    GAME_SAFE_RELEASE(g_theRenderer);
    GAME_SAFE_RELEASE(g_theWindow);
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

    if (g_theGame != nullptr)
    {
        delete g_theGame;
        g_theGame = nullptr;
    }

    g_theGame = new T();
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
    g_theEventSystem->BeginFrame();
    g_theWindow->BeginFrame();
    g_theRenderer->BeginFrame();
    DebugRenderBeginFrame();
    g_theDevConsole->BeginFrame();
    g_theInput->BeginFrame();
}

//----------------------------------------------------------------------------------------------------
void App::Update()
{
    Clock::TickSystemClock();
    g_theInput->SetCursorMode(eCursorMode::POINTER);
    UpdateFromFromKeyboard();
    UpdateFromController();
    g_theGame->Update();
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

    g_theRenderer->ClearScreen(clearColor);
    g_theGame->Render();
    g_theDevConsole->Render(AABB2(Vec2::ZERO, Vec2(1600.f, 30.f)));
}

//----------------------------------------------------------------------------------------------------
void App::EndFrame() const
{
    g_theEventSystem->EndFrame();
    g_theWindow->EndFrame();
    g_theRenderer->EndFrame();
    DebugRenderEndFrame();
    g_theDevConsole->EndFrame();
    g_theInput->EndFrame();
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

    if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
    {
        // Backward
        m_currentGameMode = static_cast<eGameMode>((static_cast<int>(m_currentGameMode) + gameModeCount - 1) % gameModeCount);
        s_gameModeConstructors[static_cast<int>(m_currentGameMode)]();
    }

    if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
    {
        // Forward
        m_currentGameMode = static_cast<eGameMode>((static_cast<int>(m_currentGameMode) + 1) % gameModeCount);
        s_gameModeConstructors[static_cast<int>(m_currentGameMode)]();
    }
}

//----------------------------------------------------------------------------------------------------
void App::UpdateFromController()
{
    XboxController const& controller = g_theInput->GetController(0);

    UNUSED(controller)
}

//----------------------------------------------------------------------------------------------------
void App::UpdateCursorMode()
{
    bool const doesWindowHasFocus   = GetActiveWindow() == g_theWindow->GetWindowHandle();
    bool const shouldUsePointerMode = !doesWindowHasFocus || g_theDevConsole->IsOpen();

    if (shouldUsePointerMode == true)
    {
        g_theInput->SetCursorMode(eCursorMode::POINTER);
    }
    else
    {
        g_theInput->SetCursorMode(eCursorMode::FPS);
    }
}
