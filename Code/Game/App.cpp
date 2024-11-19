//-----------------------------------------------------------------------------------------------
// App.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Game/App.hpp"

#include "Engine/Core/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameNearestPoint.hpp"
#include "Game/GameRaycastVsDiscs.hpp"

//-----------------------------------------------------------------------------------------------
App *                  g_theApp      = nullptr; // Created and owned by Main_Windows.cpp
InputSystem *          g_theInput    = nullptr;
Renderer *             g_theRenderer = nullptr; // Created and owned by the App
RandomNumberGenerator *g_theRNG      = nullptr; // Created and owned by the App
Window *               g_theWindow   = nullptr;

//-----------------------------------------------------------------------------------------------
void App::Startup()
{
	// Create All Engine Subsystems
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
	g_theRenderer         = new Renderer(renderConfig); // Create render

	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();

	g_theRNG  = new RandomNumberGenerator();
	m_theGame = new GameRaycastVsDiscs();
}

//-----------------------------------------------------------------------------------------------
// All Destroy and ShutDown process should be reverse order of the StartUp
//
void App::Shutdown()
{
	delete m_theGame;
	m_theGame = nullptr;

	g_theRenderer->Shutdown();
	g_theWindow->Shutdown();
	g_theInput->Shutdown();

	// Destroy all Engine Subsystem


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
	const float timeNow      = static_cast<float>(GetCurrentTimeSeconds());
	const float deltaSeconds = timeNow - m_timeLastFrameStart;
	m_timeLastFrameStart     = timeNow;

	// DebuggerPrintf("TimeNow = %.06f\n", timeNow);

	BeginFrame();         // Engine pre-frame stuff
	Update(deltaSeconds); // Game updates / moves / spawns / hurts / kills stuff
	Render();             // Game draws current state of things
	EndFrame();           // Engine post-frame stuff
}

//-----------------------------------------------------------------------------------------------
bool App::IsQuitting() const
{
	return m_isQuitting;
}

void App::RunMainLoop()
{
	// Program main loop; keep running frames until it's time to quit
	while (!IsQuitting())
	{
		// Sleep(16); // Temporary code to "slow down" our app to ~60Hz until we have proper frame timing in
		RunFrame();
	}
}

//-----------------------------------------------------------------------------------------------
void App::BeginFrame() const
{
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	// g_theNetwork->BeginFrame();
	// g_theWindow->BeginFrame();
	// g_theDevConsole->BeginFrame();
	// g_theEventSystem->BeginFrame();
	// g_theNetwork->BeginFrame();
}

//-----------------------------------------------------------------------------------------------
void App::Update(float deltaSeconds)
{
	HandleKeyPressed();
	HandleKeyReleased();
	AdjustForPauseAndTimeDistortion(deltaSeconds);
	m_theGame->Update(deltaSeconds);
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
	const Rgba8 clearColor = Rgba8(0, 0, 0);
	g_theRenderer->ClearScreen(clearColor);
	m_theGame->Render();
}

//-----------------------------------------------------------------------------------------------
void App::EndFrame() const
{
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
	g_theInput->EndFrame();
}

//-----------------------------------------------------------------------------------------------
void App::HandleKeyPressed()
{
	XboxController const& controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_isPaused = true;
		m_theGame->Update(1.f / 60.f);
	}

	if (g_theInput->WasKeyJustPressed('T'))
		m_isSlowMo = true;

	if (g_theInput->WasKeyJustPressed('P'))
		m_isPaused = !m_isPaused;

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		m_isQuitting = true;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F7))
	{
		delete m_theGame; // Clean up the current game mode

		// Cycle through game modes
		if (m_currentGameMode == GameMode::GAME_MODE_NEAREST_POINT)
		{
			m_theGame         = new GameRaycastVsDiscs();
			m_currentGameMode = GameMode::GAME_MODE_RAYCAST_VS_DISCS;
		}
		else
		{
			m_theGame         = new GameNearestPoint();
			m_currentGameMode = GameMode::GAME_MODE_NEAREST_POINT;
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
	if (m_isPaused)
		deltaSeconds = 0.f;

	if (m_isSlowMo)
		deltaSeconds *= 1 / 10.f;
}

void App::DeleteAndCreateNewGame()
{
	delete m_theGame;
	m_theGame = nullptr;

	m_theGame = new GameNearestPoint();
}
