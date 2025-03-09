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
#include "Engine/Renderer/Window.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameNearestPoint.hpp"
#include "Game/GameRaycastVsAABBs.hpp"
#include "Game/GameRaycastVsDiscs.hpp"
#include "Game/GameRaycastVsLineSegments.hpp"

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
void App::Startup()
{
    LoadGameConfig("Data/GameConfig.xml");

    EventSystemConfig eventSystemConfig;
    g_theEventSystem = new EventSystem(eventSystemConfig);
    g_theEventSystem->SubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);
    g_theEventSystem->SubscribeEventCallbackFunction("quit", OnCloseButtonClicked);

    InputSystemConfig inputConfig;
    g_theInput = new InputSystem(inputConfig);

    WindowConfig windowConfig;
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

    RenderConfig renderConfig;
    renderConfig.m_window = g_theWindow;
    g_theRenderer         = new Renderer(renderConfig);

    DebugRenderConfig debugConfig;
    debugConfig.m_renderer = g_theRenderer;
    debugConfig.m_fontName = "SquirrelFixedFont";

    // Initialize devConsoleCamera
    m_devConsoleCamera = new Camera();

    Vec2 const bottomLeft = Vec2::ZERO;

    float const screenSizeX    = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY    = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    Vec2 const  screenTopRight = Vec2(screenSizeX, screenSizeY);

    m_devConsoleCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);

    DevConsoleConfig devConsoleConfig;
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

    m_gameClock = new Clock(Clock::GetSystemClock());
}

//----------------------------------------------------------------------------------------------------
// All Destroy and ShutDown process should be reverse order of the StartUp
//
void App::Shutdown()
{
    // Destroy all Engine Subsystem
    delete g_theGame;
    g_theGame = nullptr;

    delete g_theRNG;
    g_theRNG = nullptr;

    delete g_theBitmapFont;
    g_theBitmapFont = nullptr;

    g_theInput->Shutdown();
    g_theDevConsole->Shutdown();

    delete m_devConsoleCamera;
    m_devConsoleCamera = nullptr;

    DebugRenderSystemShutdown();
    g_theRenderer->Shutdown();
    g_theWindow->Shutdown();
    g_theEventSystem->Shutdown();

    delete g_theRenderer;
    g_theRenderer = nullptr;

    delete g_theWindow;
    g_theWindow = nullptr;

    delete g_theInput;
    g_theInput = nullptr;
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
    g_theInput->SetCursorMode(CursorMode::POINTER);
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

    AABB2 const box = AABB2(Vec2::ZERO, Vec2(1600.f, 30.f));

    g_theDevConsole->Render(box);
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
void App::LoadGameConfig(char const* gameConfigXmlFilePath)
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
    if (g_theInput->WasKeyJustPressed(KEYCODE_F6))
    {
        // Cycle through game modes backward
        m_currentGameMode = static_cast<eGameMode>((static_cast<int>(m_currentGameMode) +3) % 4);

        if (m_currentGameMode == eGameMode::RAYCAST_VS_DISCS) DeleteAndCreateNewGame<GameRaycastVsDiscs>();
        if (m_currentGameMode == eGameMode::NEAREST_POINT) DeleteAndCreateNewGame<GameNearestPoint>();
        if (m_currentGameMode == eGameMode::RAYCAST_VS_LINESEGMENTS) DeleteAndCreateNewGame<GameRaycastVsLineSegments>();
        if (m_currentGameMode == eGameMode::RAYCAST_VS_AABBS) DeleteAndCreateNewGame<GameRaycastVsAABBs>();
    }

    if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
    {
        // Cycle through game modes forward
        m_currentGameMode = static_cast<eGameMode>((static_cast<int>(m_currentGameMode) + 1) % 4);

        if (m_currentGameMode == eGameMode::RAYCAST_VS_DISCS) DeleteAndCreateNewGame<GameRaycastVsDiscs>();
        if (m_currentGameMode == eGameMode::NEAREST_POINT) DeleteAndCreateNewGame<GameNearestPoint>();
        if (m_currentGameMode == eGameMode::RAYCAST_VS_LINESEGMENTS) DeleteAndCreateNewGame<GameRaycastVsLineSegments>();
        if (m_currentGameMode == eGameMode::RAYCAST_VS_AABBS) DeleteAndCreateNewGame<GameRaycastVsAABBs>();
    }
}

//----------------------------------------------------------------------------------------------------
void App::UpdateFromController()
{
    XboxController const& controller = g_theInput->GetController(0);

    UNUSED(controller)
}

//----------------------------------------------------------------------------------------------------
template <typename T>
void App::DeleteAndCreateNewGame() const
{
    static_assert(std::is_base_of_v<Game, T>, "T must be a subclass of Game");

    delete g_theGame;
    g_theGame = nullptr;

    g_theGame = new T();
}

//----------------------------------------------------------------------------------------------------
void App::UpdateCursorMode()
{
    bool const doesWindowHasFocus   = GetActiveWindow() == g_theWindow->GetWindowHandle();
    bool const shouldUsePointerMode = !doesWindowHasFocus || g_theDevConsole->IsOpen();

    if (shouldUsePointerMode == true)
    {
        g_theInput->SetCursorMode(CursorMode::POINTER);
    }
    else
    {
        g_theInput->SetCursorMode(CursorMode::FPS);
    }
}
