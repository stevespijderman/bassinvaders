/*
 * Sprite.cpp
 *
 *  Created on: 19-Apr-2009
 *      Author: spijderman
 */

#include "Sprite.h"
#include "toolkit.h"

Sprite::Sprite(ResourceBundle * resources/*, BassInvaders * game*/) {
	/* take a text file as a parameter containing all the data for all the states
	 * then pass file into a function which populates an AnimationState_t
	  * note: in real game, will need to store checksums of all data files to confirm vanilla operation*/
	loadSpriteData(resources);
	forceStateChange = 0;
	currentState = AS_IDLE;
	pendingState = AS_IDLE;
}

Sprite::~Sprite() {
	/* DON'T free up the surfaces in here.
	 * It's possible that the object was created on the stack
	 * then copied into a container class.
	 * If the owner of the container behaves nicely they will
	 * call the destroy() function when they are done with the sprite
	 */

}

void Sprite::destroy()
{
}

void Sprite::changeState(AnimationState_t newState)
{
	if ((currentState == newState)
		|| (pendingState == newState))
	{
		return;
	}

	if (animationStateData[newState].state == 0)
	{
		DebugPrint(("Can't transition to state with no data\n"));
		return;
	}
	forceStateChange = 1;
	switch(currentState)
	{
		/* JG TODO: do we need any further logic?*/
		case AS_IDLE:
		{
			pendingState = newState;

		}break;

		case AS_DAMAGED:
		{
			pendingState = newState;
		} break;

		case AS_DYING:
		case AS_DEAD:
		default:
		{
			//there's no return from '86...
		}break;
	}
}

void Sprite::renderSprite(SDL_Surface *pScreen)
{
	AnimationStateData_t* pTempState;

	updateStates();

	switch(currentState)
	{
		case AS_IDLE:
		{
			pTempState = &(animationStateData[AS_IDLE]);
		} break;
		case AS_DAMAGED:
		{
			pTempState = &(animationStateData[AS_DAMAGED]);
		} break;
		case AS_DYING:
		{
			pTempState = &(animationStateData[AS_DYING]);
		} break;
		case AS_DEAD:
		default:
		{
			/* dead sprites (or bad states) do not get rendered */
			return;
		}break;
	}

	if (pTempState->state == 0)
	{
		DebugPrint(("No data for this state %x\n", currentState));
		return;
	}

	//this cuts the appropriate frame out of the sprite sheet
	SDL_Rect spriteRect;
	spriteRect.x = pTempState->sheetStartsAt.x
					+ (pTempState->currentAnimationStep * pTempState->spriteWidth)
					+ pTempState->currentAnimationStep;
	spriteRect.y = pTempState->sheetStartsAt.y;
	spriteRect.w = pTempState->spriteWidth;
	spriteRect.h = pTempState->spriteHeight;

	DrawToSurface(xpos,
				  ypos,
				  pTempState->spriteSheet,
				  pScreen,
				  &spriteRect);
}

uint8_t Sprite::getNextAnimationStep(const AnimationStateData_t* pStateData)
{
	if (pStateData->currentAnimationStep == (pStateData->numberOfAnimationSteps - 1))
	{
		return 0;
	}
	else
	{
		return (pStateData->currentAnimationStep+1);
	}
}

void Sprite::updateStates()
{
	AnimationStateData_t* pCurrentState = &animationStateData[currentState];

	/* this may be the last frame in a single pass state. if so , setup the next state here*/
	if (pCurrentState->currentAnimationStep == (pCurrentState->numberOfAnimationSteps-1) && !forceStateChange)
	{
		pendingState = pCurrentState->nextState;
	}
	forceStateChange = 0;

	/* change state if we are not in the one we
	 * should be in
	 */
	if (pendingState != currentState)
	{
		currentState = pendingState;

		pCurrentState = &animationStateData[currentState];
		pCurrentState->currentAnimationStep = 0;
		pCurrentState->lastAnimTickCount = SDL_GetTicks();
	}

	/* special case - dead state
	 * we do nothing if the sprite is meant to be dead*/
	if (currentState == AS_DEAD)
	{
		return;
	}

	/* figure out which animation step we are in */
	uint32_t now = SDL_GetTicks();
	uint32_t delta = now - pCurrentState->lastAnimTickCount;
	if (delta > pCurrentState->ticksPerStep)
	{
		pCurrentState->currentAnimationStep = getNextAnimationStep(pCurrentState);
		pCurrentState->lastAnimTickCount = now;
	}
}

void Sprite::loadSpriteData(ResourceBundle * resource)
{
	/*read first line, should be number of states
	 * then loop through file parsing each line until
	 * we read all the data we expected
	 * you should probably add some error handling code at some point*/
	AnimationStateData_t* pData;
	uint32_t numberOfStates;
	uint32_t R=0;
	uint32_t G=0;
	uint32_t B=0;
	uint32_t numberOfCollisionRects;
	AnimationState_t state;

	numberOfStates = GET_RESOURCE(int32_t, *resource, "numberofstates", 0);

	memset(animationStateData, 0, (sizeof(AnimationStateData_t) * AS_STATES_SIZE));
	ResourceBundle * currentState;
	for (uint32_t i = 0; i<numberOfStates; i++)
	{
		currentState = GET_RESOURCE(ResourceBundle*, *resource, "statefiles", i);
		state = GET_RESOURCE(AnimationState_t, *currentState, "state", 0);

		pData = &(animationStateData[state]);
		pData->state = state;

		DebugPrint((" loading state 0x%x\n", state));

		R = GET_RESOURCE(int32_t, *currentState, "colorkey", 0);
		G = GET_RESOURCE(int32_t, *currentState, "colorkey", 1);
		B = GET_RESOURCE(int32_t, *currentState, "colorkey", 2);

		pData->nextState = GET_RESOURCE(AnimationState_t, *currentState, "nextstate", 0);
		pData->numberOfAnimationSteps = GET_RESOURCE(int32_t, *currentState, "numberofanimationsteps", 0);
		pData->ticksPerStep = GET_RESOURCE(int32_t, *currentState, "ticksperstep", 0);

		pData->sheetStartsAt.x = GET_RESOURCE(int32_t, *currentState, "sheetstartsat", 0);
		pData->sheetStartsAt.y = GET_RESOURCE(int32_t, *currentState, "sheetstartsat", 1);

		pData->spriteWidth = GET_RESOURCE(int32_t, *currentState, "spritesize", 0);
		pData->spriteHeight = GET_RESOURCE(int32_t, *currentState, "spritesize", 1);

		numberOfCollisionRects = GET_RESOURCE(int32_t, *currentState, "numberofrects", 0);

		for (uint32_t j = 0; j<numberOfCollisionRects; ++j)
		{
			CollisionRect_t rect = {0,0,0,0};

			rect.x = GET_RESOURCE(int32_t, *currentState, "rect", 0);
			rect.y = GET_RESOURCE(int32_t, *currentState, "rect", 1);
			rect.w = GET_RESOURCE(int32_t, *currentState, "rect", 2);
			rect.h = GET_RESOURCE(int32_t, *currentState, "rect", 3);
			pData->collisionRects.push_back(rect);
		}

		// this doesn't work with GET_RESOURCE, I guess because filename doesn't return an array maybe?
		pData->spriteSheet = (SDL_Surface*)((*currentState)["filename"]);
		uint32_t colorkey = SDL_MapRGB( pData->spriteSheet->format, R, G, B );
		SDL_SetColorKey( pData->spriteSheet, SDL_SRCCOLORKEY, colorkey );
	}
}

void Sprite::setLocation(uint32_t xpos, uint32_t ypos)
{
	this->xpos = xpos;
	this->ypos = ypos;
}

std::vector<CollisionRect_t> Sprite::getCollisionRects()
{
	return animationStateData[currentState].collisionRects;
}
