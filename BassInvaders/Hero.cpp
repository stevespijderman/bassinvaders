/*
 * Hero.cpp
 *
 *  Created on: 19-Apr-2009
 *      Author: spijderman
 */

#include "Hero.h"
#include "toolkit.h"
#include <fstream>
#include <iostream>

Hero::Hero(ResourceBundle* resource)
{
	loadHeroData(resource);
	score = 0;
	xvelocity = 0;
	yvelocity = 0;
	xpos = 100;
	ypos = 100;
	type = RT_FRIENDLY;
	currentState = RS_ACTIVE;
	pendingState = RS_ACTIVE;

	this->velocityTicks = 10;
	/* JG TODO: load in sprites:
	 * ship/damaged ship
	 * thrusters all around
	 * charge attack? */
}

/* hero manages it's own sprites */
Hero::~Hero()
{
	std::vector<Sprite>::iterator pos;

		for (pos = sprites.begin(); pos !=sprites.end(); ++pos)
		{
			pos->destroy();
		}
}

void Hero::loadHeroData(ResourceBundle* resource)
{
	this->health = *((int*)(*resource)["health"]);
	this->attackDamage = *((int*)(*resource)["attackdamage"]);
	Sprite heroBody(((ResourceBundle**)(*resource)["bodysprite"])[0]);

	sprites.push_back(heroBody);

}

bool Hero::isOffScreen(int32_t screenWidth, int32_t screenHeight)
{
	if ( (ypos < -50) || (ypos > (int32_t)screenHeight) )
		return true;

	return false;
}

void Hero::render(SDL_Surface* pScreen)
{
	doActions();
	updateStates();

	/* set the sprite for the main body to the same position
	 * as the Hero class
	 * we'll then, (when they exist) render all the other sprites  (e.g. thrusters)
	 * in their appropriate relative positions
	 */
	sprites[BODYSPRITE].setLocation(this->xpos, this->ypos);

	/* render all the sprites in the vector*/
	std::vector<Sprite>::iterator pos;

	uint8_t i = 0;
	for (pos = sprites.begin(); pos!=sprites.end(); ++pos)
	{
		pos->renderSprite(pScreen);
		++i;
	}
}

void Hero::setActions(ACTIONMASK actions)
{
	if (actions & ACTION_MOVE_DOWN)
	{
		yvelocity = HERO_Y_SPEED;
	}
	else if (actions & ACTION_MOVE_UP)
	{
		yvelocity = -(HERO_Y_SPEED);
	}
	else
	{
		yvelocity = 0;
	}

	if (actions & ACTION_MOVE_LEFT)
	{
		xvelocity = -(HERO_X_SPEED);
	}
	else if (actions & ACTION_MOVE_RIGHT)
	{
		xvelocity = HERO_X_SPEED;
	}
	else
	{
		xvelocity = 0;
	}

	if (actions & ACTION_SHOOT)
	{
	}
}

void Hero::doActions()
{
	uint32_t now = SDL_GetTicks();
	uint32_t delta = now - lastTickCount;

	if (delta > velocityTicks)
	{
		xpos += xvelocity;
		ypos += yvelocity;

		lastTickCount = now;
	}
}

bool Hero::canBeRemoved()
{
	return false;
}

void Hero::updateStates()
{
	currentState = pendingState;

	switch(currentState)
	{
		case RS_ACTIVE:
		{
			sprites[BODYSPRITE].changeState(AS_IDLE);
		}break;

		case RS_DEAD:
		{
			sprites[BODYSPRITE].changeState(AS_DEAD);
		}break;
	}
}

void Hero::doCollision(Renderable* pOther)
{
	/* read type of Renderable.
	 * if neutral/friend:
	 *  return
	 * if enemy/powerup:
	 *  find out if collision
	 *   if true, call reactToCollision on both Renderables
	 */

	switch(pOther->getType())
	{
		case RT_ENEMY:
		case RT_POWERUP:
		{
			if (isCollidingWith(pOther))
			{
				this->reactToCollision(pOther);
				pOther->reactToCollision(this);
			}
		}break;

		case RT_FRIENDLY:
		case RT_NEUTRAL:
		default:
		{
			return;
		}
	}
}

std::vector<Sprite> Hero::getActiveSpriteList()
{
	std::vector<Sprite> ret;
	ret.push_back(sprites[BODYSPRITE]);
	return ret;
}

void Hero::reactToCollision(Renderable* pOther)
{
	switch(pOther->getType())
	{
		case RT_ENEMY:
		{

			//hero has hit a monster!
			health -= pOther->getAttackDamage();
			if (health <= DEAD_HEALTH)
			{
				changeState(RS_DEAD);
				health = DEAD_HEALTH;
				// JG TODO: maybe kick off game over logic from this point...
			}
			else if (health < DAMAGED_HEALTH)
			{
				sprites[BODYSPRITE].changeState(AS_DAMAGED);
			}

			DebugPrint(("Hero hit by enemy, down to %d health\n", health));

		}break;

		case RT_POWERUP:
		case RT_FRIENDLY:
		case RT_NEUTRAL:
		default:
		{
			// so far nothing, but if we want to have health pickups, we can do it here
			return;
		}
	}
}
