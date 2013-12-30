/** 
 *
 * Copyright (c) 2010-2013, Kitty Barnett
 * 
 * The source code in this file is provided to you under the terms of the 
 * GNU Lesser General Public License, version 2.1, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. Terms of the LGPL can be found in doc/LGPL-licence.txt 
 * in this distribution, or online at http://www.gnu.org/licenses/lgpl-2.1.txt
 * 
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to 
 * abide by those obligations.
 * 
 */
#ifndef LLINSPECTTEXTURE_H
#define LLINSPECTTEXTURE_H

#include "lltooltip.h"

// ============================================================================
// Forward declarations
//

class LLViewerFetchedTexture;

// ============================================================================
// Helper functions
//

namespace LLInspectTextureUtil
{
	// Register with LLFloaterReg
	void registerFloater();
}

LLToolTip* createInventoryToolTip(LLToolTip::Params p);

// ============================================================================
// LLTexturePreviewView helper class
//

class LLTexturePreviewView : public LLView
{
public:
	LLTexturePreviewView(const LLView::Params& p);
	/*virtual*/ ~LLTexturePreviewView();

	/*
	 * LLView overrides
	 */
public:
	/*virtual*/ void draw();	

	/*
	 * Member functions
	 */
public:
	void setImageFromAssetId(const LLUUID& idAsset);
	void setImageFromItemId(const LLUUID& idItem);

	/*
	 * Member variables
	 */
protected:
	LLPointer<LLViewerFetchedTexture> m_Image;
	S32         m_nImageBoostLevel;
	std::string m_strLoading;
};

// ============================================================================
// LLTextureToolTip class
//

class LLTextureToolTip : public LLToolTip
{
public:
	LLTextureToolTip(const LLToolTip::Params& p);
	/*virtual*/ ~LLTextureToolTip();

	/*
	 * LLToolTip overrides
	 */
public:
	/*virtual*/ void initFromParams(const LLToolTip::Params& p);

	/*
	 * Member variables
	 */
protected:
	LLTexturePreviewView* m_pPreview;
	S32                   m_nPreviewSize;
};

// ============================================================================

#endif // LLINSPECTTEXTURE_H
