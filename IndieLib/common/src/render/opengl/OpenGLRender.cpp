/*****************************************************************************************
 * File: OpenGLRender.cpp
 * Desc: Initialization / Destruction using OpenGL
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


#include "Defines.h"

#ifdef INDIERENDER_OPENGL

// ----- Includes -----

#include "Global.h"
#include "IND_Math.h"
#include "OpenGLRender.h"
#include "IND_Window.h"
#include "IND_FontManager.h"
#include "IND_SurfaceManager.h"
#include "IND_AnimationManager.h"
#include "IND_FontManager.h"
#include "IND_Timer.h"
#include "IND_Font.h"
#include "IND_Animation.h"
#include "IND_Camera2d.h"

#if defined (PLATFORM_WIN32) || defined(PLATFORM_LINUX) || defined (PLATFORM_OSX)
#include "platform/OSOpenGLManager.h"
#endif

//Constants
#define MINIMUM_OPENGL_VERSION_STRING "GL_VERSION_1_5"  //The minimum GL version supported by this renderer  

/** @cond DOCUMENT_PRIVATEAPI */
// --------------------------------------------------------------------------------
//							  Initialization / Destruction
// --------------------------------------------------------------------------------

IND_Window* OpenGLRender::initRenderAndWindow(IND_WindowProperties& props) {
	if(props._bpp <= 0 || props._height <= 0 || props._width <= 0) {
		g_debug->header("Error initializing window: Invalid parameters provided", DebugApi::LogHeaderError);
		return 0;
	}
	end();
	initVars();

	// Window creation
	_window = new IND_Window();
	if (!_window) {
		freeVars();
		return NULL;
	}
    
    g_debug->header("Requested Window Height:", DebugApi::LogHeaderInfo);
    g_debug->dataInt(props._height, true);
    g_debug->header("Requested Window Width: ", DebugApi::LogHeaderInfo);
    g_debug->dataInt(props._width, true);
    g_debug->header("Requested Window Bpp: ", DebugApi::LogHeaderInfo);
    g_debug->dataInt(props._bpp, true);
    g_debug->header("Requested Window FullScreen: ", DebugApi::LogHeaderInfo);
    g_debug->dataInt(props._fullscreen, true);
    g_debug->header("Requested Window Double buffer: ", DebugApi::LogHeaderInfo);
    g_debug->dataInt(props._doubleBuffer, true);
	
	if(!_osOpenGLMgr) {
		_osOpenGLMgr = new OSOpenGLManager(_window);  
	}
	_doubleBuffer = props._doubleBuffer;
    
	//Initialize OpenGL parameters for SDL before creating the window for OpenGL
	_osOpenGLMgr->setOpenGLContextParams(IND_RGBA, //Color format. HARDCODED, AND HAS TO MATCH WINDOW HARDCODED FORMAT!
										 props._bpp/4, //Color depth (Bpp) /  num colors
	                                     2, //Depth Buffer bits
	                                     1,  //Stencil Buffer bits
	                                     props._doubleBuffer //Double buffering
	                                    );

	if(!_window->create(props)) {
		g_debug->header("Error creating window: Not supported params provided", DebugApi::LogHeaderError);
		freeVars();
		return NULL;
	}

	g_debug->header("Creating OpenGL Render", DebugApi::LogHeaderBegin);
	_ok = initializeOpenGLRender();
	if (!_ok) {
		g_debug->header("Finalizing OpenGL", DebugApi::LogHeaderWarning);
		freeVars();
		g_debug->header("OpenGL finalized", DebugApi::LogHeaderEnd);
		return NULL;
	}

	g_debug->header("Checking created OpenGL context pixel format", DebugApi::LogHeaderOk);
	if(!_osOpenGLMgr->checkOpenGLSDLContextProps()) {
		g_debug->header("Different GL context pixel format used", DebugApi::LogHeaderWarning);
	} else {
		g_debug->header("Same GL context pixel format used", DebugApi::LogHeaderOk);
	}

	writeInfo();

	g_debug->header("OpenGL Render Created", DebugApi::LogHeaderEnd);
	return _window;
}

bool OpenGLRender::reset(IND_WindowProperties& props) {
	if(props._bpp <= 0 || props._height <= 0 || props._width <= 0) {
		g_debug->header("Error resetting window: Invalid parameters provided", DebugApi::LogHeaderError);
		return 0;
	}

    bool viewPortWasFullWindow = (_window->getWidth() == _info._viewPortWidth) && (_window->getHeight() == _info._viewPortHeight);
    
	if (!_window->reset(props)) {
		g_debug->header("Error resetting SDL window", DebugApi::LogHeaderError);
		return 0;
	}
    
    _info._fbWidth = _window->getWidth();
    _info._fbHeight = _window->getHeight();
    
    bool ok = 1;
    if (viewPortWasFullWindow) {
        ok = resetViewport(_window->getWidth(),_window->getHeight());
    }
	
    return ok;
}

bool OpenGLRender::toggleFullScreen() {

	g_debug->header("Changing To/From Full Screen", DebugApi::LogHeaderBegin);

	if (!_window->toggleFullScreen()) return false;

	return true;
}

void OpenGLRender::beginScene() {

	if (!_ok)
		return;

	//Clear buffers
	glClearColor(0, 0, 0, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void OpenGLRender::endScene() {
	if (!_ok)
		return;

	//Swap memory buffers (OS-dependant)
	_osOpenGLMgr->presentBuffer();

#ifdef _DEBUG
    GLenum glerror = glGetError();
	if (glerror) {
        g_debug->header("OpenGLRenderer::endScene() OpenGL error flag!", DebugApi::LogHeaderError);
		printf("OpenGLRenderer::endScene() Error at end of scene! :%i\n",glerror);
	}
#endif	
}

void OpenGLRender::showFpsInWindowTitle(char *pFPSString) {
	if (!_ok)   return;

	_window->setTitle(pFPSString);
}

void OpenGLRender::setPointPixelScale (float pNewScale) {
    _info._pointPixelScale = pNewScale;
}

void OpenGLRender::getNumrenderedObjectsString(char *pBuffer)      {
	IND_Math::itoa(_numrenderedObjects, pBuffer);
}

void OpenGLRender::getNumDiscardedObjectsString(char *pBuffer)      {
	IND_Math::itoa(_numDiscardedObjects, pBuffer);
}

void OpenGLRender::end() {
	if (_ok) {
		g_debug->header("Finalizing OpenGL", DebugApi::LogHeaderBegin);
		_osOpenGLMgr->endOpenGLContext();
		freeVars();
		g_debug->header("OpenGL finalized ", DebugApi::LogHeaderEnd);
		_ok = false;
	}
}

// --------------------------------------------------------------------------------
//							        Private methods
// --------------------------------------------------------------------------------

/*
==================
Init vars
==================
*/
void OpenGLRender::initVars() {
	_numrenderedObjects = 0;
	_numDiscardedObjects = 0;
	_window = NULL;
	_math.init();
	_osOpenGLMgr = NULL;

}

/*
==================
Init OpenGL
==================
*/
bool OpenGLRender::initializeOpenGLRender() {

	//Check dependency of window initialization
	if (!_window->isOK()) {
		// Window error
		g_debug->header("This operation can not be done:", DebugApi::LogHeaderInfo);
		g_debug->dataChar("", 1);
		g_debug->header("Invalid Id or IND_Window not correctly initialized.", DebugApi::LogHeaderError);

		return false;
	}

	//OpenGL context creation
	_osOpenGLMgr->createOpenGLSDLContext();

	//Check for GL extensions
	if (!checkGLExtensions())
		return false;

	//Get all graphics device information
	getInfo();
    
    //Window params
    _info._fbWidth = _window->getWidth();
	_info._fbHeight = _window->getHeight();

	// ViewPort initialization
	return resetViewport(_window->getWidth(),_window->getHeight());
}

/*
==================
Check OpenGL extensions
==================
*/
bool OpenGLRender::checkGLExtensions() {

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		g_debug->header("Extensions loading (GLEW) failed!", DebugApi::LogHeaderError);
		return false;
	}

	//Check system support for minimum targeted version of library
	if (!glewIsSupported(MINIMUM_OPENGL_VERSION_STRING)) {
		g_debug->header("Minimum OPENGL version is not available!", DebugApi::LogHeaderError);
		return false;
	}

	strcpy(_info._version, MINIMUM_OPENGL_VERSION_STRING);

	//TODO: Other extensions

	return true;
}

/*
==================
Free memory
==================
*/
void OpenGLRender::freeVars() {
	DISPOSE(_osOpenGLMgr);
	DISPOSE(_window);
	_ok = false;
}

/*
==================
Hardware information
==================
*/
void OpenGLRender::getInfo() {

    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxTextureSize);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &_info._textureUnits);
    _info._maxTextureSize = maxTextureSize;

}

/*
==================
Write hardware information to debug log
==================
*/
void OpenGLRender::writeInfo() {
	g_debug->header("Hardware information" , DebugApi::LogHeaderBegin);

	// ----- D3D version -----

	g_debug->header("OpenGL version:" , DebugApi::LogHeaderInfo);
	g_debug->dataChar(_info._version, 1);

	// ----- Vendor -----

	g_debug->header("Mark:" , DebugApi::LogHeaderInfo);
	g_debug->dataChar(_info._vendor, 1);

	// ----- Renderer -----

	g_debug->header("Chip:" , DebugApi::LogHeaderInfo);
	g_debug->dataChar(_info._renderer, 1);

	// ----- Antialiasing -----

	g_debug->header("Primitive antialising:", DebugApi::LogHeaderInfo);
	if (_info._antialiasing)
		g_debug->dataChar("Yes", 1);
	else
		g_debug->dataChar("No", 1);

	// ----- Max texture size -----

	g_debug->header("Maximum texture size:" , DebugApi::LogHeaderInfo);
	g_debug->dataInt(_info._maxTextureSize, 0);
	g_debug->dataChar("x", 0);
	g_debug->dataInt(_info._maxTextureSize, 1);
	g_debug->header("Texture units:" , DebugApi::LogHeaderInfo);
	g_debug->dataInt(_info._textureUnits, 0);


	// ----- Vertex Shader version  -----

	/*g_debug->Header ("Vertex Shader:" , DebugApi::LogHeaderInfo);
	g_debug->DataInt (D3DSHADER_VERSION_MAJOR (_info._vertexShaderVersion), 0);
	g_debug->DataChar (".", 0);
	g_debug->DataInt (D3DSHADER_VERSION_MINOR (_info._vertexShaderVersion), 0);

	if (_info._softwareVertexProcessing)
	    g_debug->DataChar ("(Software)", 1);
	else
	    g_debug->DataChar ("", 1);*/

	// ----- Pixel Shader version -----

	/*g_debug->Header ("Pixel Shader:" , DebugApi::LogHeaderInfo);
	g_debug->DataInt (D3DSHADER_VERSION_MAJOR (_info._pixelShaderVersion), 0);
	g_debug->DataChar (".", 0);
	g_debug->DataInt (D3DSHADER_VERSION_MINOR (_info._pixelShaderVersion), 1);*/

	g_debug->header("Hardware Ok" , DebugApi::LogHeaderEnd);
}

/*
==================
Resets the viewport to all window width/height
==================
*/
bool OpenGLRender::resetViewport(int pWitdh, int pHeight) {
    if (pHeight == 0 || pWitdh == 0) return false;
    
	if (!setViewPort2d(0, 0, pWitdh, pHeight))
		return false;

	IND_Camera2d mCamera2d(static_cast<float>(_info._viewPortWidth/2),
						   static_cast<float>(_info._viewPortHeight/2));   //Default 2D camera in center of viewport
	setCamera2d(&mCamera2d);
	clearViewPort(0, 0, 0);

	return true;
}
/** @endcond */
#endif //INDIERENDER_OPENGL
