/*****************************************************************************************
 * File: IND_Input.h
 * Desc: Input class (wrapping SDL input)
 *****************************************************************************************/

/*********************************** The zlib License ************************************
 *
 * Copyright (c) 2013 Indielib-crossplatform Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 *
 *****************************************************************************************/


// ----- Includes -----

#include "Global.h"
#include "Defines.h"
#include "IND_Input.h"
#include "IND_Render.h"
#include "dependencies/SDL-2.0/include/SDL.h"

// --------------------------------------------------------------------------------
//							  Initialization / Destruction
// --------------------------------------------------------------------------------

/**
 * Returns 1 (true) if the adminstrator is successfully initialized.
 * Must be called before using any method.
 */
bool IND_Input::init(IND_Render *pRender) {
	end();
	freeVars();
    
	initVars();
    
	_ok = true;
    _keyboardActive = false;

	_render = pRender;
    
	return _ok;
}


/**
 * Frees the manager and all the objects that it contains.
 */
void IND_Input::end() {
	if (_ok) {
		freeVars();
        
		_ok = false;
	}
}

/**
 Returns state of initialization.
 @return  Will give true if object initialized correctly, false otherwise
 */
bool IND_Input::isOK() {
    return _ok;
}

// --------------------------------------------------------------------------------
//									  Public methods
// --------------------------------------------------------------------------------

/**
 * Update input function. This method has to be called everytime in the game loop.
 */
void IND_Input::update() {
	static SDL_Event mEvent;
    
	// ----- Flags initialization -----
    
	for (int i = 0; i < 132; i++) {
		_keys [i]._pressed = 0;
		_keys [i]._released = 0;
	}
    
	_mouse._mouseMotion = false;
	_mouse._mouseButtons [IND_MBUTTON_LEFT]._pressed = 0;
	_mouse._mouseButtons [IND_MBUTTON_LEFT]._released = 0;
	_mouse._mouseButtons [IND_MBUTTON_RIGHT]._pressed = 0;
	_mouse._mouseButtons [IND_MBUTTON_RIGHT]._released = 0;
	_mouse._mouseButtons [IND_MBUTTON_MIDDLE]._pressed = 0;
	_mouse._mouseButtons [IND_MBUTTON_MIDDLE]._released = 0;
	_mouse._mouseScroll = false;
	_mouse._mouseScrollX = 0;
	_mouse._mouseScrollY = 0;
    
    // Touches marked for deletion from previous update, to be removed
    clearOldTouches();
    
	// ----- Update -----
    
	while (SDL_PollEvent(&mEvent)) {
		// ----- Mouse ------
        
		SDL_GetMouseState(&_mouse._mouseX, &_mouse._mouseY);
                
		switch (mEvent.type) {
                // ----- Window status ------
                
            case SDL_QUIT:  //Window closed.
                _quit = 1;
                break;
                // ----- App states ------
                // TODO: App states
            case SDL_APP_TERMINATING:
            case SDL_APP_LOWMEMORY:
            case SDL_APP_WILLENTERBACKGROUND:
            case SDL_APP_DIDENTERBACKGROUND:
            case SDL_APP_WILLENTERFOREGROUND:
            case SDL_APP_DIDENTERFOREGROUND:
                break;
                
                // ----- Window focus changed -----
                
               
                // ----- Window event -----
			case SDL_WINDOWEVENT:
				//Handle minimize by locking event thread and reset timers when coming back
				if (mEvent.window.event == SDL_WINDOWEVENT_MINIMIZED ||
					mEvent.window.event == SDL_WINDOWEVENT_HIDDEN) {
						SDL_WaitEvent(&mEvent);   //Will lock thread.
						_render->resetTimer();
				}
				break;
                // ----- Mouse motion -----
                
            case SDL_MOUSEMOTION:
                _mouse._mouseMotion = 1;
                break;
                
                // ----- Mouse buttons -----
                
            case SDL_MOUSEBUTTONDOWN:
                // SDL sends a mousebuton down when a touchscreen fingerdown is triggered.
                // We don't want double events, so we must distinguish it from a 'real' mouse event
                if (mEvent.button.which != SDL_TOUCH_MOUSEID) {
                    switch (mEvent.button.button) {
                        case SDL_BUTTON_LEFT:
                            _mouse._mouseButtons [IND_MBUTTON_LEFT].setState(IND_MBUTTON_PRESSED);
                            break;
                        case SDL_BUTTON_RIGHT:
                            _mouse._mouseButtons [IND_MBUTTON_RIGHT].setState(IND_MBUTTON_PRESSED);
                            break;
                        case SDL_BUTTON_MIDDLE:
                            _mouse._mouseButtons [IND_MBUTTON_MIDDLE].setState(IND_MBUTTON_PRESSED);
                            break;
                            
                    }
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                // SDL sends a mousebuton down when a touchscreen fingerup is triggered.
                // We don't want double events, so we must distinguish it from a 'real' mouse event
                if ( mEvent.button.which != SDL_TOUCH_MOUSEID) {
                    switch (mEvent.button.button) {
                        case SDL_BUTTON_LEFT:
                            _mouse._mouseButtons [IND_MBUTTON_LEFT].setState(IND_MBUTTON_NOT_PRESSED);
                            break;
                        case SDL_BUTTON_RIGHT:
                            _mouse._mouseButtons [IND_MBUTTON_RIGHT].setState(IND_MBUTTON_NOT_PRESSED);
                            break;
                        case SDL_BUTTON_MIDDLE:
                            _mouse._mouseButtons [IND_MBUTTON_MIDDLE].setState(IND_MBUTTON_NOT_PRESSED);
                            break;
                            
                    }
                }
                break;
                
            case SDL_MOUSEWHEEL:
				_mouse._mouseScroll = true;
				_mouse._mouseScrollX = mEvent.wheel.x;
				_mouse._mouseScrollY = mEvent.wheel.y;

                break;
            
            case SDL_FINGERDOWN: {
                IND_Touch* touch = new IND_Touch();
                touch->identifier = mEvent.tfinger.touchId;
                touch->state = IND_TouchStateDown;
                touch->position = IND_NormalizedPoint(mEvent.tfinger.x,mEvent.tfinger.y);
                _touches[touch->identifier] = touch;
            }
                break;
            case SDL_FINGERMOTION: {
                TouchesMapIterator it = _touches.find(mEvent.tfinger.touchId);
                if (it != _touches.end()) {
                    (*it).second->state = IND_TouchStateMoved;
                    (*it).second->position = IND_NormalizedPoint(mEvent.tfinger.x,mEvent.tfinger.y);
                }
            }
                break;
            case SDL_FINGERUP: {
                TouchesMapIterator it = _touches.find(mEvent.tfinger.touchId);
                if (it != _touches.end()) {
                    (*it).second->state = IND_TouchStateUp;
                    (*it).second->position = IND_NormalizedPoint(mEvent.tfinger.x,mEvent.tfinger.y);
                    _oldTouches[(*it).second->identifier] = (*it).second;
                }
            }
                break;
                
            default: {
                
            }
                break;
                
		}
        
		// ----- Keyboard ------
		switch (mEvent.key.keysym.sym) {  //TODO: MFK valgrind is unhappy about this "Conditional jump or move depends on uninitialised value(s)".
			case SDLK_a:
				_keys [IND_A].setState(mEvent.key.state);
				break;
			case SDLK_b:
				_keys [IND_B].setState(mEvent.key.state);
				break;
			case SDLK_c:
				_keys [IND_C].setState(mEvent.key.state);
				break;
			case SDLK_d:
				_keys [IND_D].setState(mEvent.key.state);
				break;
			case SDLK_e:
				_keys [IND_E].setState(mEvent.key.state);
				break;
			case SDLK_f:
				_keys [IND_F].setState(mEvent.key.state);
				break;
			case SDLK_g:
				_keys [IND_G].setState(mEvent.key.state);
				break;
			case SDLK_h:
				_keys [IND_H].setState(mEvent.key.state);
				break;
			case SDLK_i:
				_keys [IND_I].setState(mEvent.key.state);
				break;
			case SDLK_j:
				_keys [IND_J].setState(mEvent.key.state);
				break;
			case SDLK_k:
				_keys [IND_K].setState(mEvent.key.state);
				break;
			case SDLK_l:
				_keys [IND_L].setState(mEvent.key.state);
				break;
			case SDLK_m:
				_keys [IND_M].setState(mEvent.key.state);
				break;
			case SDLK_n:
				_keys [IND_N].setState(mEvent.key.state);
				break;
			case SDLK_o:
				_keys [IND_O].setState(mEvent.key.state);
				break;
			case SDLK_p:
				_keys [IND_P].setState(mEvent.key.state);
				break;
			case SDLK_q:
				_keys [IND_Q].setState(mEvent.key.state);
				break;
			case SDLK_r:
				_keys [IND_R].setState(mEvent.key.state);
				break;
			case SDLK_s:
				_keys [IND_S].setState(mEvent.key.state);
				break;
			case SDLK_t:
				_keys [IND_T].setState(mEvent.key.state);
				break;
			case SDLK_u:
				_keys [IND_U].setState(mEvent.key.state);
				break;
			case SDLK_v:
				_keys [IND_V].setState(mEvent.key.state);
				break;
			case SDLK_w:
				_keys [IND_W].setState(mEvent.key.state);
				break;
			case SDLK_x:
				_keys [IND_X].setState(mEvent.key.state);
				break;
			case SDLK_y:
				_keys [IND_Y].setState(mEvent.key.state);
				break;
			case SDLK_z:
				_keys [IND_Z].setState(mEvent.key.state);
				break;
                
			case SDLK_BACKSPACE:
				_keys [IND_BACKSPACE].setState(mEvent.key.state);
				break;
			case SDLK_TAB:
				_keys [IND_TAB].setState(mEvent.key.state);
				break;
			case SDLK_CLEAR:
				_keys [IND_CLEAR].setState(mEvent.key.state);
				break;
			case SDLK_RETURN:
				_keys [IND_RETURN].setState(mEvent.key.state);
				break;
			case SDLK_PAUSE:
				_keys [IND_PAUSE].setState(mEvent.key.state);
				break;
			case SDLK_ESCAPE:
				_keys [IND_ESCAPE].setState(mEvent.key.state);
				break;
			case SDLK_SPACE:
				_keys [IND_SPACE].setState(mEvent.key.state);
				break;
			case SDLK_EXCLAIM:
				_keys [IND_EXCLAIM].setState(mEvent.key.state);
				break;
			case SDLK_QUOTEDBL:
				_keys [IND_QUOTEDBL].setState(mEvent.key.state);
				break;
			case SDLK_HASH:
				_keys [IND_HASH].setState(mEvent.key.state);
				break;
            case SDLK_PERCENT:
                _keys [IND_HASH].setState(mEvent.key.state);
			case SDLK_DOLLAR:
				_keys [IND_DOLLAR].setState(mEvent.key.state);
				break;
			case SDLK_AMPERSAND:
				_keys [IND_AMPERSAND].setState(mEvent.key.state);
				break;
			case SDLK_QUOTE:
				_keys [IND_QUOTE].setState(mEvent.key.state);
				break;
			case SDLK_LEFTPAREN:
				_keys [IND_LEFTPAREN].setState(mEvent.key.state);
				break;
			case SDLK_RIGHTPAREN:
				_keys [IND_RIGHTPAREN].setState(mEvent.key.state);
				break;
			case SDLK_ASTERISK:
				_keys [IND_ASTERISK].setState(mEvent.key.state);
				break;
			case SDLK_PLUS:
				_keys [IND_PLUS].setState(mEvent.key.state);
				break;
			case SDLK_COMMA:
				_keys [IND_COMMA].setState(mEvent.key.state);
				break;
			case SDLK_MINUS:
				_keys [IND_MINUS].setState(mEvent.key.state);
				break;
			case SDLK_PERIOD:
				_keys [IND_PERIOD].setState(mEvent.key.state);
				break;
			case SDLK_SLASH:
				_keys [IND_SLASH].setState(mEvent.key.state);
				break;
                
			case SDLK_0:
				_keys [IND_0].setState(mEvent.key.state);
				break;
			case SDLK_1:
				_keys [IND_1].setState(mEvent.key.state);
				break;
			case SDLK_2:
				_keys [IND_2].setState(mEvent.key.state);
				break;
			case SDLK_3:
				_keys [IND_3].setState(mEvent.key.state);
				break;
			case SDLK_4:
				_keys [IND_4].setState(mEvent.key.state);
				break;
			case SDLK_5:
				_keys [IND_5].setState(mEvent.key.state);
				break;
			case SDLK_6:
				_keys [IND_6].setState(mEvent.key.state);
				break;
			case SDLK_7:
				_keys [IND_7].setState(mEvent.key.state);
				break;
			case SDLK_8:
				_keys [IND_8].setState(mEvent.key.state);
				break;
			case SDLK_9:
				_keys [IND_9].setState(mEvent.key.state);
				break;
                
			case SDLK_COLON:
				_keys [IND_COLON].setState(mEvent.key.state);
				break;
			case SDLK_SEMICOLON:
				_keys [IND_SEMICOLON].setState(mEvent.key.state);
				break;
			case SDLK_LESS:
				_keys [IND_LESS].setState(mEvent.key.state);
				break;
			case SDLK_EQUALS:
				_keys [IND_EQUALS].setState(mEvent.key.state);
				break;
			case SDLK_GREATER:
				_keys [IND_GREATER].setState(mEvent.key.state);
				break;
			case SDLK_QUESTION:
				_keys [IND_QUESTION].setState(mEvent.key.state);
				break;
			case SDLK_AT:
				_keys [IND_AT].setState(mEvent.key.state);
				break;
			case SDLK_LEFTBRACKET:
				_keys [IND_LEFTBRACKET].setState(mEvent.key.state);
				break;
			case SDLK_BACKSLASH:
				_keys [IND_BACKSLASH].setState(mEvent.key.state);
				break;
			case SDLK_RIGHTBRACKET:
				_keys [IND_RIGHTBRACKET].setState(mEvent.key.state);
				break;
			case SDLK_CARET:
				_keys [IND_CARET].setState(mEvent.key.state);
				break;
			case SDLK_UNDERSCORE:
				_keys [IND_UNDERSCORE].setState(mEvent.key.state);
				break;
			case SDLK_BACKQUOTE:
				_keys [IND_BACKQUOTE].setState(mEvent.key.state);
				break;
                
			case SDLK_DELETE:
				_keys [IND_DELETE].setState(mEvent.key.state);
				break;		
			case SDLK_KP_0:
				_keys [IND_K0].setState(mEvent.key.state);
				break;
			case SDLK_KP_1:
				_keys [IND_K1].setState(mEvent.key.state);
				break;
			case SDLK_KP_2:
				_keys [IND_K2].setState(mEvent.key.state);
				break;
			case SDLK_KP_3:
				_keys [IND_K3].setState(mEvent.key.state);
				break;
			case SDLK_KP_4:
				_keys [IND_K4].setState(mEvent.key.state);
				break;
			case SDLK_KP_5:
				_keys [IND_K5].setState(mEvent.key.state);
				break;
			case SDLK_KP_6:
				_keys [IND_K6].setState(mEvent.key.state);
				break;
			case SDLK_KP_7:
				_keys [IND_K7].setState(mEvent.key.state);
				break;
			case SDLK_KP_8:
				_keys [IND_K8].setState(mEvent.key.state);
				break;
			case SDLK_KP_9:
				_keys [IND_K9].setState(mEvent.key.state);
				break;
			case SDLK_KP_PERIOD:
				_keys [IND_KPERIOD].setState(mEvent.key.state);
				break;
			case SDLK_KP_DIVIDE:
				_keys [IND_KDIVIDE].setState(mEvent.key.state);
				break;
			case SDLK_KP_MULTIPLY:
				_keys [IND_KMULTIPLY].setState(mEvent.key.state);
				break;
			case SDLK_KP_MINUS:
				_keys [IND_KMINUS].setState(mEvent.key.state);
				break;
			case SDLK_KP_PLUS:
				_keys [IND_KPLUS].setState(mEvent.key.state);
				break;
			case SDLK_KP_ENTER:
				_keys [IND_KENTER].setState(mEvent.key.state);
				break;
			case SDLK_KP_EQUALS:
				_keys [IND_KEQUALS].setState(mEvent.key.state);
				break;
                
			case SDLK_UP:
				_keys [IND_KEYUP].setState(mEvent.key.state);
				break;
			case SDLK_DOWN:
				_keys [IND_KEYDOWN].setState(mEvent.key.state);
				break;
			case SDLK_RIGHT:
				_keys [IND_KEYRIGHT].setState(mEvent.key.state);
				break;
			case SDLK_LEFT:
				_keys [IND_KEYLEFT].setState(mEvent.key.state);
				break;
			case SDLK_INSERT:
				_keys [IND_INSERT].setState(mEvent.key.state);
				break;
			case SDLK_HOME:
				_keys [IND_HOME].setState(mEvent.key.state);
				break;
			case SDLK_END:
				_keys [IND_END].setState(mEvent.key.state);
				break;
			case SDLK_PAGEUP:
				_keys [IND_PAGEUP].setState(mEvent.key.state);
				break;
			case SDLK_PAGEDOWN:
				_keys [IND_PAGEDOWN].setState(mEvent.key.state);
				break;
                
			case SDLK_F1:
				_keys [IND_F1].setState(mEvent.key.state);
				break;
			case SDLK_F2:
				_keys [IND_F2].setState(mEvent.key.state);
				break;
			case SDLK_F3:
				_keys [IND_F3].setState(mEvent.key.state);
				break;
			case SDLK_F4:
				_keys [IND_F4].setState(mEvent.key.state);
				break;
			case SDLK_F5:
				_keys [IND_F5].setState(mEvent.key.state);
				break;
			case SDLK_F6:
				_keys [IND_F6].setState(mEvent.key.state);
				break;
			case SDLK_F7:
				_keys [IND_F7].setState(mEvent.key.state);
				break;
			case SDLK_F8:
				_keys [IND_F8].setState(mEvent.key.state);
				break;
			case SDLK_F9:
				_keys [IND_F9].setState(mEvent.key.state);
				break;
			case SDLK_F10:
				_keys [IND_F10].setState(mEvent.key.state);
				break;
			case SDLK_F11:
				_keys [IND_F11].setState(mEvent.key.state);
				break;
			case SDLK_F12:
				_keys [IND_F12].setState(mEvent.key.state);
				break;
			case SDLK_F13:
				_keys [IND_F13].setState(mEvent.key.state);
				break;
			case SDLK_F14:
				_keys [IND_F14].setState(mEvent.key.state);
				break;
			case SDLK_F15:
				_keys [IND_F15].setState(mEvent.key.state);
				break;	
			case SDLK_NUMLOCKCLEAR:
				_keys [IND_NUMLOCK].setState(mEvent.key.state);
				break;
			case SDLK_CAPSLOCK:
				_keys [IND_CAPSLOCK].setState(mEvent.key.state);
				break;
			case SDLK_SCROLLLOCK:
				_keys [IND_SCROLLOCK].setState(mEvent.key.state);
				break;
			case SDLK_RSHIFT:
				_keys [IND_RSHIFT].setState(mEvent.key.state);
				break;
			case SDLK_LSHIFT:
				_keys [IND_LSHIFT].setState(mEvent.key.state);
				break;
			case SDLK_RCTRL:
				_keys [IND_RCTRL].setState(mEvent.key.state);
				break;
			case SDLK_LCTRL:
				_keys [IND_LCTRL].setState(mEvent.key.state);
				break;
			case SDLK_RALT:
				_keys [IND_RALT].setState(mEvent.key.state);
				break;
			case SDLK_LALT:
				_keys [IND_LALT].setState(mEvent.key.state);
				break;
            case SDLK_RGUI:
                 _keys [IND_RMETA].setState(mEvent.key.state);
                 break;
            case SDLK_LGUI:
                 _keys [IND_LMETA].setState(mEvent.key.state);
                 break;
			case SDLK_MODE:
				_keys [IND_MODE].setState(mEvent.key.state);
				break;
			case SDLK_HELP:
				_keys [IND_HELP].setState(mEvent.key.state);
				break;
			case SDLK_PRINTSCREEN:
				_keys [IND_PRINT].setState(mEvent.key.state);
				break;
			case SDLK_SYSREQ:
				_keys [IND_SYSREQ].setState(mEvent.key.state);
				break;
            case SDLK_RETURN2:
                 _keys [IND_BREAK].setState(mEvent.key.state);
                 break;
			case SDLK_MENU:
				_keys [IND_MENU].setState(mEvent.key.state);
				break;
			case SDLK_POWER:
				_keys [IND_POWER].setState(mEvent.key.state);
				break;
            case SDLK_CURRENCYUNIT:
                _keys [IND_CURRENCYUNIT].setState(mEvent.type);
                break;  //KEYDOWN OR KEYUP
                
            default:
                break; 
		}
	}
}

/**
 * Returns true if the user closes the window.
 */
bool IND_Input::quit() {
	return _quit;
}

/**
 * Used for touch screen devices. 
 * 
 * If the system does not have on-screen keyboards, this is ignored. Use it if you want to show the keyboard 
 * on screen, for example in iOS
 */
void IND_Input::beginKeyboardEvents() {
    SDL_StartTextInput();
    _keyboardActive = true;
}

/**
 * Used for touch screen devices.
 *
 * If the system does not have on-screen keyboards, this is ignored. Use it to dismiss on-screen keyboard
 * Use it for on-screen compatible keyboards, like iOS
 */
void IND_Input::endKeyboardEvents() {
    SDL_StopTextInput();
    _keyboardActive = false;
}

/**
 * Queries if a call to beginKeyboardEvents was done, and not rest by endKeyboardEvents
 * 
 * Use this to track your calls to begin/end keyboardEvents API. In no way it reflects if the system is actually receiving
 * keyboard events. Many devices can share a keyboard + a touch screen, so it is wise you don't abstract this and consider into
 * your app design how you handle those.
 */
bool IND_Input::isAcceptingKeyboardEvents() {
    return _keyboardActive;
}

/**
 * Returns true if the key passed as @b pKey parameter is pressed.
 * @param pKey						Key that we want to check. See ::IND_Key.
 */
bool IND_Input::onKeyPress(IND_Key pKey) {
	return _keys [pKey]._pressed;
}

/**
 * Returns true if the key passed as @b pKey parameter is released.
 * @param pKey						Key that we want to check. See ::IND_Key.
 */
bool IND_Input::onKeyRelease(IND_Key pKey) {
	return _keys [pKey]._released;
}

/**
 * Returns true if the key passed as @b pKey parameter is being pressed.
 * @param pKey						Key that we want to check. See ::IND_Key.
 */
bool IND_Input::isKeyPressed(IND_Key pKey) {
	if (_keys [pKey]._keyState == IND_KEY_PRESSED) {
		return 1;
	}
    
	return 0;
}

/**
 * Returns true if the key passed as pKey parameter has been pressed pTime milliseconds.
 * @param pKey						Key that we want to check. See ::IND_Key.
 * @param pTime						Time that has to pass in milliseconds.
 */
bool IND_Input::isKeyPressed(IND_Key pKey, unsigned long pTime) {
	if (_keys [pKey]._keyState == IND_KEY_PRESSED && _keys [pKey]._timer.getTicks() > pTime) {
		_keys [pKey]._timer.start();
		return 1;
	}
    
	return 0;
}

/**
 * Returns the true if the mouse is moving.
 */
bool IND_Input::isMouseMotion() {
	return _mouse._mouseMotion;
}

/**
 * Returns the horizontal position of the mouse in screen pixels.
 */
int IND_Input::getMouseX() {
	return _mouse._mouseX;
}

/**
 * Returns the vertical position of the mouse in screen pixels.
 */
int IND_Input::getMouseY() {
	return _mouse._mouseY;
}

/**
 * Returns true if mouse is scrolling. Mouse scroll concept in this case groups
 * events from 'mouse wheel' in traditional mouses, and also scroll in X and Y from modern
 * multitouch mouses, like Apple's 'magic mouse'.
 */
bool IND_Input::isMouseScroll() {
	return _mouse._mouseScroll;
}

/**
 * Returns the amount of units, mouse scrolled in X. This value will be fixed every 
 * step for tradional mousewheels.
 */
int IND_Input::getMouseScrollX() {
	return _mouse._mouseScrollX;
}

/**
 * Returns the amount of units, mouse scrolled in Y. This value will be fixed every 
 * step for tradional mousewheels.
 */
int IND_Input::getMouseScrollY() {
	return _mouse._mouseScrollY;
}

/**
 * Returns true if the mouse button passed as pMouseButton is pressed. It will return true again only if the button
 * is released and pressed again.
 * @param pMouseButton					Mouse button that we want to check. See ::IND_MouseButton.
 */
bool IND_Input::onMouseButtonPress(IND_MouseButton pMouseButton) {
	return _mouse._mouseButtons [pMouseButton]._pressed;
}

/**
 * Returns true if the mouse button passed as pMouseButton is released. It will return true again only if the button
 * is pressed and released again.
 * @param pMouseButton					Mouse button that we want to check. See ::IND_MouseButton.
 */
bool IND_Input::onMouseButtonRelease(IND_MouseButton pMouseButton) {
	return _mouse._mouseButtons [pMouseButton]._released;
}

/**
 * Returns true if the mouse button passed as pMouseButton is being pressed.
 * @param pMouseButton					Mouse button that we want to check. See ::IND_MouseButton.
 */
bool IND_Input::isMouseButtonPressed(IND_MouseButton pMouseButton) {
	if (_mouse._mouseButtons [pMouseButton]._buttonState == IND_MBUTTON_PRESSED) {
		return 1;
	}
    
	return 0;
}

/**
 * Returns true if the mouse button passed as @b pMouseButton parameter has been pressed @b pTime milliseconds.
 * @param pMouseButton					Mouse button that we want to check. See ::IND_MouseButton.
 * @param pTime						Time that has to pass in milliseconds.
 */
bool IND_Input::isMouseButtonPressed(IND_MouseButton pMouseButton, unsigned long pTime) {
	if (_mouse._mouseButtons [pMouseButton]._buttonState == IND_MBUTTON_PRESSED && _mouse._mouseButtons [pMouseButton]._timer.getTicks() > pTime) {
		_mouse._mouseButtons [pMouseButton]._timer.start();
		return 1;
	}
    
	return 0;
}

/**
 * Returns Returns any active touch from a finger, having the identifier as requested
 * @param pIdentifier               Identifier of the touch to search for
 * @return A valid touch, if existing, or NULL in case no touch is found
 */
IND_Touch* IND_Input::touchWithIdentifier(unsigned int pIdentifier) {
    
    TouchesMapIterator it = _touches.find(pIdentifier);
    
    if (it != _touches.end()) {
        return it->second;
    }
    
    return NULL;
}

/**
 * Returns Returns a map containing a set of touches, queryable by id, and having the state requested
 *
 * Do not delete the touches contained in the map, as they are owned by IND_Input
 * @param pState The state the touches are
 * @return Always returns a map, which may be empty if no touches with that state are found
 */
IND_Input::TouchesMap IND_Input::touchesWithState(IND_TouchState pState) {
    
    TouchesMap toReturn;
    
    TouchesMapIterator it;
    for (it = _touches.begin(); it != _touches.end() ; ++it) {
        if (it->second->state == pState) {
            toReturn[it->first] = it->second;
        }
    }
    return toReturn;
    
}

// --------------------------------------------------------------------------------
//									 Private methods
// --------------------------------------------------------------------------------

/** @cond DOCUMENT_PRIVATEAPI */

/*
==================
 Initialize the input flags
==================
*/
void IND_Input::initFlags() {
	for (int i = 0; i < 132; i++) {
		_keys [i]._key = i;
		_keys [i]._pressed = 0;
		_keys [i]._released = 0;
	}
    
	_mouse._mouseButtons [IND_MBUTTON_LEFT]._pressed = 0;
	_mouse._mouseButtons [IND_MBUTTON_LEFT]._released = 0;
	_mouse._mouseButtons [IND_MBUTTON_RIGHT]._pressed = 0;
	_mouse._mouseButtons [IND_MBUTTON_RIGHT]._released = 0;
	_mouse._mouseButtons [IND_MBUTTON_MIDDLE]._pressed = 0;
	_mouse._mouseButtons [IND_MBUTTON_MIDDLE]._released = 0;
}

/*
==================
 Init vars
==================
*/
void IND_Input::initVars() {
	_mouse._mouseMotion = false;
	_mouse._mouseScroll = false;
	initFlags();
	_quit = 0;
}

/*
==================
 Free memory
 ==================
*/
void IND_Input::freeVars() {
    
}

void IND_Input::clearOldTouches() {
    TouchesMapIterator it;
    for (it = _oldTouches.begin(); it != _oldTouches.end(); ++it) {
        _touches.erase((*it).first);
        DISPOSE((*it).second);
    }
    
    _oldTouches.clear();
}

/** @endcond */
