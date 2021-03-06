/*****************************************************************************************
 * File: Main.cpp
 * Desc: Tileless editor tutorial using Indielib
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

#include "CIndieLib.h"
#include "Resources.h"
#include "EditorMap.h"
#include "Listener.h"
#include "WorkingPath.h"

/*
==================
Main
==================
*/
Indielib_Main		
{
	// ----- IndieLib intialization -----

	CIndieLib *mI = CIndieLib::instance();
	if (!mI->init ()) return 0;	

	const char* resourcesPath = WorkingPathSetup::resourcesDirectory();
	if (!WorkingPathSetup::setWorkingPath(resourcesPath)) {
		std::cout<<"\nUnable to Set the working path !";
	}
	
	// ----- Editor classes initialization -----
	// Resource loading
	Resources mResources;

	char mPath [MAX_PATH];
	strcpy(mPath, resourcesPath);
	strcat (mPath, "/editor/tileset_01.xml");		// Add the name of the tileset to the path
	
	if (!mResources.LoadResources (mPath)) exit (0);

	// Map initialization
	EditorMap mMap;

	// Listener initialization
	Listener mListener (&mResources, &mMap);

	// ----- Main Loop -----

	while (!mI->_input->onKeyPress (IND_ESCAPE) && !mI->_input->quit())
	{
		// ----- Input Update ----

		mI->_input->update ();

		// -------- Atributes -------

		mListener.Listen ();

		// -------- Render -------

		// Reset the counting of rendered and discarded objects
		mI->_render->resetNumDiscardedObjects();
		mI->_render->resetNumrenderedObject();

		mI->_render->beginScene ();

		// Render banckground (two triangles)
		mI->_render->blitColoredTriangle (0, 0, mI->_window->getWidth(), 0, 0, mI->_window->getHeight(), 
										255, 128, 128, 
										255, 128, 128, 
										27, 27, 204,
										255);

		mI->_render->blitColoredTriangle (0, mI->_window->getHeight(), mI->_window->getWidth(), 0, mI->_window->getWidth(), mI->_window->getHeight(), 
										27, 27, 204, 
										255, 128, 128, 
										27, 27, 204,
										255);

		// --- Render parallax layer B ---

		mI->_render->setCamera2d (mListener.GetCameraB());
		mI->_entity2dManager->renderEntities2d (0);

		// --- Render parallax layer N ---

		mI->_render->setCamera2d (mListener.GetCameraN());
		mI->_entity2dManager->renderEntities2d (1);

		// --- Render parallax layer M ---

		mI->_render->setCamera2d (mListener.GetCameraM());
		mI->_entity2dManager->renderEntities2d (2);

		// --- Render backdrop elements of Layers from 1 to 9 ---

		mI->_render->setCamera2d (mListener.GetCameraLayers());

		for (int i = 3; i < NUM_EDITOR_LAYERS; i++)
			mI->_entity2dManager->renderEntities2d (i);

		// --- Render editor elements (like the brush and areas) ---

		// If editing mode
		if (mListener.GetMode())
		{
			switch (mListener.GetCurrentLayer())
			{
				case 0:		mI->_render->setCamera2d (mListener.GetCameraB());		break;
				case 1:		mI->_render->setCamera2d (mListener.GetCameraN());		break;
				case 2:		mI->_render->setCamera2d (mListener.GetCameraM());		break;
				default:	mI->_render->setCamera2d (mListener.GetCameraLayers());		break;
			}

			// Render
			mI->_entity2dManager->renderEntities2d (BRUSH_LAYER);

			// Render the collision areas of the working layer
			mI->_entity2dManager->renderCollisionAreas (mListener.GetCurrentLayer(), 255, 255, 255, 30);
			mI->_entity2dManager->renderGridAreas (mListener.GetCurrentLayer(), 255, 0, 0, 255);
		}

		// --- Render texts ---

		// Render gui elements (text, mouse cursor)
		mI->_render->setCamera2d (mListener.GetCameraGui());
		mI->_entity2dManager->renderEntities2d (GUI_LAYER);	

		// --- End Scene ---

		mI->_render->endScene ();
	}

	// ----- Free memory (we don't use destructors becase IndieLib pointers would be pointing to null -----
	// FIXME: This is a hack. Proper memory management please.
	mListener.Free ();
	mResources.Free ();
	mMap.Free ();

	// ----- Indielib End -----

	mI->end ();

	return 0;
}
