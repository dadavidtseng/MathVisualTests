//----------------------------------------------------------------------------------------------------
// App.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/App.hpp"

#include "GameRaycastVsAABBs.hpp"
#include "GameRaycastVsLineSegments.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameNearestPoint.hpp"
#include "Game/GameRaycastVsDiscs.hpp"

//----------------------------------------------------------------------------------------------------
App*                   g_theApp        = nullptr;      // Created and owned by Main_Windows.cpp
BitmapFont*            g_theBitmapFont = nullptr;      // Created and owned by the App
Game*                  g_theGame       = nullptr;      // Created and owned by the App
Renderer*              g_theRenderer   = nullptr;      // Created and owned by the App
RandomNumberGenerator* g_theRNG        = nullptr;      // Created and owned by the App
Window*                g_theWindow     = nullptr;      // Created and owned by the App

//----------------------------------------------------------------------------------------------------
void App::Startup()
{
    EventSystemConfig eventSystemConfig;
    g_theEventSystem = new EventSystem(eventSystemConfig);
    g_theEventSystem->SubscribeEventCallbackFunction("OnCloseButtonClicked", OnCloseButtonClicked);

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

    // Initialize devConsoleCamera
    m_devConsoleCamera = new Camera();

    Vec2 const bottomLeft     = Vec2::ZERO;
    Vec2 const screenTopRight = Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y);

    m_devConsoleCamera->SetOrthoGraphicView(bottomLeft, screenTopRight);

    DevConsoleConfig devConsoleConfig;
    devConsoleConfig.m_defaultRenderer = g_theRenderer;
    devConsoleConfig.m_defaultFontName = "SquirrelFixedFont";
    devConsoleConfig.m_defaultCamera   = m_devConsoleCamera;
    g_theDevConsole                    = new DevConsole(devConsoleConfig);

    g_theEventSystem->Startup();
    g_theWindow->Startup();
    g_theRenderer->Startup();
    g_theDevConsole->StartUp();
    g_theInput->Startup();

    g_theBitmapFont = g_theRenderer->CreateOrGetBitmapFontFromFile("Data/Fonts/SquirrelFixedFont"); // DO NOT SPECIFY FILE .EXTENSION!!  (Important later on.)
    g_theRNG        = new RandomNumberGenerator();
    g_theGame       = new GameRaycastVsDiscs();
}

//-----------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
//
void App::RunFrame()
{
    BeginFrame();         // Engine pre-frame stuff
    Update(); // Game updates / moves / spawns / hurts / kills stuff
    Render();             // Game draws current state of things
    EndFrame();           // Engine post-frame stuff
}

// //-----------------------------------------------------------------------------------------------
// bool App::IsQuitting() const
// {
// 	return m_isQuitting;
// }

void App::RunMainLoop()
{
    // Program main loop; keep running frames until it's time to quit
    while (!m_isQuitting)
    {
        // Sleep(16); // Temporary code to "slow down" our app to ~60Hz until we have proper frame timing in
        RunFrame();
    }
}

//-----------------------------------------------------------------------------------------------
void App::BeginFrame() const
{
    g_theEventSystem->BeginFrame();
    g_theWindow->BeginFrame();
    g_theRenderer->BeginFrame();
    g_theDevConsole->BeginFrame();
    g_theInput->BeginFrame();
    // g_theNetwork->BeginFrame();
}

//-----------------------------------------------------------------------------------------------
void App::Update()
{
    Clock::TickSystemClock();

    g_theInput->SetCursorMode(CursorMode::POINTER);
    HandleKeyPressed();
    HandleKeyReleased();
    g_theGame->Update();


    // AdjustForPauseAndTimeDistortion();
    // m_theGame->Update();
}

//-----------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------
void App::EndFrame() const
{
    g_theEventSystem->EndFrame();
    g_theInput->EndFrame();
    g_theWindow->EndFrame();
    g_theRenderer->EndFrame();
    g_theDevConsole->EndFrame();
}

//-----------------------------------------------------------------------------------------------
void App::HandleKeyPressed()
{
    XboxController const& controller = g_theInput->GetController(0);

    if (g_theInput->WasKeyJustPressed('O'))
    {
        m_isPaused = true;
        // m_theGame->Update(1.f / 60.f);
    }

    if (g_theInput->WasKeyJustPressed('T')) m_isSlowMo = true;

    if (g_theInput->WasKeyJustPressed('P')) m_isPaused = !m_isPaused;

    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
    {
        m_isQuitting = true;
    }

    if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
    {
        delete g_theGame; // Clean up the current game mode

        // Cycle through game modes
        if (m_currentGameMode == eGameMode::RAYCAST_VS_DISCS)
        {
            g_theGame         = new GameNearestPoint();
            m_currentGameMode = eGameMode::NEAREST_POINT;
        }
        else if (m_currentGameMode == eGameMode::NEAREST_POINT)
        {
            g_theGame         = new GameRaycastVsLineSegments();
            m_currentGameMode = eGameMode::RAYCAST_VS_LINESEGMENTS;
        }
        else if (m_currentGameMode == eGameMode::RAYCAST_VS_LINESEGMENTS)
        {
            g_theGame         = new GameRaycastVsAABBs();
            m_currentGameMode = eGameMode::RAYCAST_VS_AABBS;
        }
        else if (m_currentGameMode == eGameMode::RAYCAST_VS_AABBS)
        {
            g_theGame         = new GameRaycastVsDiscs();
            m_currentGameMode = eGameMode::RAYCAST_VS_DISCS;
        }
    }
}

//-----------------------------------------------------------------------------------------------
void App::HandleKeyReleased()
{
    XboxController const& controller = g_theInput->GetController(0);

    if (g_theInput->WasKeyJustReleased('T') ||
        controller.WasButtonJustReleased(XBOX_BUTTON_DPAD_UP))
        m_isSlowMo = false;
}

//-----------------------------------------------------------------------------------------------
void App::HandleQuitRequested()
{
    m_isQuitting = true;
}

void App::AdjustForPauseAndTimeDistortion(float& deltaSeconds) const
{
    if (m_isPaused) deltaSeconds = 0.f;

    if (m_isSlowMo) deltaSeconds *= 1 / 10.f;
}

//----------------------------------------------------------------------------------------------------
void App::DeleteAndCreateNewGame()
{
    delete g_theGame;
    g_theGame = nullptr;

    g_theGame = new GameNearestPoint();
}

//-----------------------------------------------------------------------------------------------
bool OnCloseButtonClicked(EventArgs& arg)
{
    UNUSED(arg)

    RequestQuit();
    return true;
}

//----------------------------------------------------------------------------------------------------
void RequestQuit()
{
    m_isQuitting = true;
}
