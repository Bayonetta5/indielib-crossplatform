/*****************************************************************************************
 * File: IND_FontManager.cpp
 * Desc: Font manager.
 *****************************************************************************************/

/*
IndieLib 2d library Copyright (C) 2005 Javier L�pez L�pez (javierlopezpro@gmail.com)

This library is free software; you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this library; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA 02111-1307 USA
*/

// ----- Includes -----

#include "Global.h"
#include "Defines.h"
#include "dependencies/tinyxml/tinyxml.h"
#include "IND_FontManager.h"
#include "IND_Font.h"
#include "IND_Surface.h"
#include "IND_Image.h"
#include "IND_ImageManager.h"
#include "IND_Math.h"

// --------------------------------------------------------------------------------
//							  Initialization / Destruction
// --------------------------------------------------------------------------------

/**
 * Returns 1 (true) if the adminstrator is successfully initialized.
 * Must be called before using any method.
 * @param pImageManager					Pointer to the manager IND_ImageManager correctly initialized.
 * @param pSurfaceManager				Pointer to the manager IND_SurfaceManager correctly initialized.
 */
bool IND_FontManager::init(IND_ImageManager *pImageManager, IND_SurfaceManager *pSurfaceManager) {
	end();
	initVars();

	g_debug->header("Initializing FontManager", DebugApi::LogHeaderBegin);

	// Checking IND_SurfaceManager
	if (pSurfaceManager->isOK()) {
		g_debug->header("SurfaceManager Ok", DebugApi::LogHeaderOk);
		_surfaceManager = pSurfaceManager;

		g_debug->header("ImageManager Ok", DebugApi::LogHeaderOk);
		_imageManager = pImageManager;

		_ok = true;

		g_debug->header("FontManager OK", DebugApi::LogHeaderEnd);
	} else {
		g_debug->header("SurfaceManager is not correctly initalized", DebugApi::LogHeaderError);
		_ok = false;
	}

	return _ok;
}

/**
 * Frees the manager and all the objects that it contains.
 */
void IND_FontManager::end() {
	if (_ok) {
		// If the object is loaded, we free it
		g_debug->header("Finalizing FontManager", DebugApi::LogHeaderBegin);
		g_debug->header("Freeing fonts" , DebugApi::LogHeaderBegin);
		freeVars();
		g_debug->header("Fonts freed", DebugApi::LogHeaderEnd);
		g_debug->header("FontManager finalized", DebugApi::LogHeaderEnd);

		_ok = false;
	}
}

/**
 Returns state of initialization.
 @return  Will give true if object initialized correctly, false otherwise
 */
bool IND_FontManager::isOK()            {
    return _ok;
}

// --------------------------------------------------------------------------------
//									Public methods
// --------------------------------------------------------------------------------

/**
 * Returns 1 (true) if the font object passed as a parameter
 * exists and is added successfully to the manager loading the font directly from
 * a graphic file and a configuration file, both generated with MudFont
 * (modified version for IndieLib that can be found in the tools section ).
 * @param pNewFont					Pointer to a font.
 * @param pName                   			Name of the graphic file that contains the font generated by MudFont.
 * @param pFile                   			Name of the configuration file of the font generated by MudFont.
 * @param pType						Font type (see ::IND_Type).
 * @param pQuality					Font quality (see ::IND_Quality).
 */
bool IND_FontManager::add(IND_Font		*pNewFont,
                          const char	*pName,
                          const char	*pFile,
                          IND_Type		pType,
                          IND_Quality	pQuality) {
	// Image loading
	IND_Image *mNewImage = new IND_Image;
    
    bool noError(true);
    noError = _imageManager->add(mNewImage, pName);

	// IND_Surface creation
	if (noError) {
        add(pNewFont, mNewImage, pFile, pType, pQuality);
    }

	// Free the image
    if (!noError) {
        DISPOSE(mNewImage);
    }
    
	_imageManager->remove(mNewImage);

    
	return noError;
}

/**
 * Returns 1 (true) if the font object type 1 passed as a parameter
 * exists and is added successfully to the manager loading the font directly from
 * ::IND_Image object and a configuration file, both generated with MudFont
 * (modified version for IndieLib can be found in the section tools).
 *
 * The posibility of changing the font from an ::IND_Image object is offered in case
 * that you want to change the original font with any modification or filter from
 * ::IND_ImageManager.
 *
 * @param pNewFont					Pointer to a new object type 1 font.
 * @param pImage					Pointer to an object ::IND_Image that contains a previously loaded font from a graphic file generated by MudFont (see tools section).
 * @param pFile						Name of the configuration file of the font generated by MudFont (see tools section).
 * @param pType						Font type (see ::IND_Type).
 * @param pQuality					Font quality (see ::IND_Quality).
 */
bool IND_FontManager::add(IND_Font		*pNewFont,
                          IND_Image		*pImage,
                          const char	*pFile,
                          IND_Type		pType,
                          IND_Quality	pQuality) {
	g_debug->header("Parsing and loading font", DebugApi::LogHeaderBegin);
	g_debug->header("File name:", DebugApi::LogHeaderInfo);
	g_debug->dataChar(pFile, 1);

	if (!_ok) {
		writeMessage();
		return 0;
	}

	char stringTemp[128];
	char *pFileCharTemp = strcpy(stringTemp, pFile);

	// ----- Width and height of the bitmap font MUST be power of two -----

	IND_Math mMath;

	if (!mMath.isPowerOfTwo(pImage->getWidth()) ||
	        !mMath.isPowerOfTwo(pImage->getHeight())) {
		g_debug->header("This operation can not be done", DebugApi::LogHeaderInfo);
		g_debug->dataChar("", 1);
		g_debug->header("The height and width of the font must be power of 2", DebugApi::LogHeaderError);
		return 0;
	}

	// ----- Bitmap (IND_Surface object) creation -----

	IND_Surface *mNewSurface = new IND_Surface;
	if (!_surfaceManager->add(mNewSurface, pImage, pType, pQuality))
		return 0;

	// IND_Surface object MUST have one block ONLY
	if (mNewSurface->getNumBlocks() > 1) {
		_surfaceManager->remove(mNewSurface);
		return 0;
	}

	pNewFont->setSurface(mNewSurface);

	// ----- XML font parsing -----	

	if (!parseFont(pNewFont, pFileCharTemp)) {
		g_debug->header("Fatal error, cannot load the font xml file", DebugApi::LogHeaderError);
		return 0;
	}

	pNewFont->setFileName(pFileCharTemp);

	// ----- Puts the object into the manager -----

	addToList(pNewFont);

	// ----- g_debug -----

	g_debug->header("Font parsed and loaded", DebugApi::LogHeaderEnd);

	return 1;
}

/**
 * Returns 1 (true) if the font object type 1 passed as a parameter exists
 * and it is deleted from the manager successfully.
 * @param pFo						Pointer to font object type 1
 */
bool IND_FontManager::remove(IND_Font  *pFo) {
	g_debug->header("Freeing font", DebugApi::LogHeaderBegin);

	if (!_ok) {
		writeMessage();
		return 0;
	}

	// Search object
	bool mIs = 0;
	list <IND_Font *>::iterator mFontListIter;
	for (mFontListIter  = _listFonts->begin();
	        mFontListIter != _listFonts->end();
	        mFontListIter++) {
		if ((*mFontListIter) == pFo) {
			mIs = 1;
			break;
		}
	}

	// Not found
	if (!mIs) {
		writeMessage();
		return 0;
	}

	// ----- Free object -----

	g_debug->header("File name:", DebugApi::LogHeaderInfo);
	g_debug->dataChar(pFo->getFileName(), 1);

    // Free bitmap IND_Surface
	_surfaceManager->remove(pFo->getSurface());
    
	// Quit from list
	delFromlist(pFo);

	g_debug->header("Ok", DebugApi::LogHeaderEnd);

	return 1;
}

// --------------------------------------------------------------------------------
//									Private methods
// --------------------------------------------------------------------------------

/** @cond DOCUMENT_PRIVATEAPI */

/*
==================
Parses and XML font file
Uses Tinyxml
==================
*/
bool IND_FontManager::parseFont(IND_Font *pNewFont, char *pFontName) {
	TiXmlDocument   *mXmlDoc = new TiXmlDocument(pFontName);

	// Fatal error, cannot load
	if (!mXmlDoc->LoadFile()) {
        DISPOSE(mXmlDoc);
     	return 0;
    }

	// Document root
	TiXmlElement *mXFont = 0;
	mXFont = mXmlDoc->FirstChildElement("font");

	if (!mXFont) {
		g_debug->header("Invalid name for document root, should be <font>", DebugApi::LogHeaderError);
		mXmlDoc->Clear();
		delete mXmlDoc;
		return 0;
	}

	if (mXFont->Attribute("num_characters")) {
		pNewFont->_font._numChars = atoi(mXFont->Attribute("num_characters"));
		pNewFont->setLetters(new IND_Font::LETTER [pNewFont->getNumChars()]);
	} else {
		g_debug->header("The font doesn't have a \"num_characters\" attribute", DebugApi::LogHeaderError);
		mXmlDoc->Clear();
		delete mXmlDoc;
		return 0;
	}

	TiXmlElement *mXChar = 0;
	mXChar = mXFont->FirstChildElement("char");

	if (!mXChar) {
		g_debug->header("There are no chars to parse", DebugApi::LogHeaderError);
		mXmlDoc->Clear();
		delete mXmlDoc;
		return 0;
	}

	// Parse all the chars
	int mCont = 0;
	while (mXChar) {
		// Id
		if (mXChar->Attribute("id")) {
			pNewFont->getLetters() [mCont]._letter = static_cast<unsigned char>(atoi(mXChar->Attribute("id")));
		} else {
			g_debug->header("The char doesn't have a \"id\" attribute", DebugApi::LogHeaderError);
			mXmlDoc->Clear();
			delete mXmlDoc;
			return 0;
		}

		// x
		if (mXChar->Attribute("x")) {
			pNewFont->getLetters() [mCont]._offsetX = atoi(mXChar->Attribute("x"));
		} else {
			g_debug->header("The char doesn't have a \"x\" attribute", DebugApi::LogHeaderError);
			mXmlDoc->Clear();
			delete mXmlDoc;
			return 0;
		}

		// y
		if (mXChar->Attribute("y")) {
			pNewFont->getLetters() [mCont]._offsetY = atoi(mXChar->Attribute("y"));
		} else {
			g_debug->header("The char doesn't have a \"y\" attribute", DebugApi::LogHeaderError);
			mXmlDoc->Clear();
			delete mXmlDoc;
			return 0;
		}

		// width
		if (mXChar->Attribute("width")) {
			pNewFont->getLetters() [mCont]._widthChar = atoi(mXChar->Attribute("width"));
		} else {
			g_debug->header("The char doesn't have a \"width\" attribute", DebugApi::LogHeaderError);
			mXmlDoc->Clear();
			delete mXmlDoc;
			return 0;
		}

		// height
		if (mXChar->Attribute("height")) {
			pNewFont->getLetters() [mCont]._heightChar = atoi(mXChar->Attribute("height"));
		} else {
			g_debug->header("The char doesn't have a \"height\" attribute", DebugApi::LogHeaderError);
			mXmlDoc->Clear();
			delete mXmlDoc;
			return 0;
		}

		// Move to the next char declaration
		mXChar = mXChar->NextSiblingElement("char");

		mCont++;
	}

	mXmlDoc->Clear();
	delete mXmlDoc;

	return 1;
}

/*
==================
Inserts the font into a list
==================
*/
void IND_FontManager::addToList(IND_Font *pNewFont) {
	_listFonts->push_back(pNewFont);
}

/*
==================
Deletes object from the manager
==================
*/
void IND_FontManager::delFromlist(IND_Font *pFo) {
	_listFonts->remove(pFo);
	DISPOSE(pFo);
}


/*
==================
Initialization error message
==================
*/
void IND_FontManager::writeMessage() {
	g_debug->header("This operation can not be done", DebugApi::LogHeaderInfo);
	g_debug->dataChar("", 1);
	g_debug->header("Invalid Id or FontManager not correctly initialized", DebugApi::LogHeaderError);
}

/*
==================
Init manager vars
==================
*/
void IND_FontManager::initVars() {
	_listFonts = new list <IND_Font *>;
}

/*
==================
Free manager memory
==================
*/
void IND_FontManager::freeVars() {
	// Deletes all the manager entities
	list <IND_Font *>::iterator mFontListIter;
	for (mFontListIter  = _listFonts->begin();
	        mFontListIter != _listFonts->end();
	        mFontListIter++) {
		g_debug->header("Freeing font:", DebugApi::LogHeaderInfo);
		g_debug->dataChar((*mFontListIter)->getFileName(), 1);

		// Free bitmap IND_Surface
		_surfaceManager->remove((*mFontListIter)->getSurface());

        DISPOSE(*mFontListIter);
	}

	// Clear list
	_listFonts->clear();

	// Free list
	DISPOSE(_listFonts);
}

/** @endcond */
