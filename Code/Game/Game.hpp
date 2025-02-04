//-----------------------------------------------------------------------------------------------
// Game.hpp
//

//-----------------------------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------------------------
class Game
{
public:
	virtual ~Game();
	
	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;
};
