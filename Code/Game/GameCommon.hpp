//----------------------------------------------------------------------------------------------------
// GameCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//-Forward-Declaration--------------------------------------------------------------------------------
class App;
class BitmapFont;
class Game;
class Renderer;
class RandomNumberGenerator;
class Window;

// one-time declaration
extern App*                   g_theApp;
extern BitmapFont*            g_theBitmapFont;
extern Game*                  g_theGame;
extern Renderer*              g_theRenderer;
extern RandomNumberGenerator* g_theRNG;
extern Window*                g_theWindow;

//----------------------------------------------------------------------------------------------------
template <typename T>
void GAME_SAFE_RELEASE(T*& pointer)
{
    delete pointer;
    pointer = nullptr;
}
