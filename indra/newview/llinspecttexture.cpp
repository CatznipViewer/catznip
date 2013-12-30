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
#include "llviewerprecompiledheaders.h"

#include "llfloaterreg.h"
#include "llinspect.h"
#include "llinspecttexture.h"
#include "llinventorymodel.h"
#include "llpreviewnotecard.h"
#include "llpreviewtexture.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewerinventory.h"
#include "llviewertexteditor.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"

// ============================================================================
// LLInspectTexture class
//

class LLInspectTexture : public LLInspect
{
	friend class LLFloaterReg;
public:
	LLInspectTexture(const LLSD& sdKey);
	/*virtual*/ ~LLInspectTexture();

	/*
	 * LLView overrides
	 */
public:
	/*virtual*/ void onOpen(const LLSD& sdData);
	/*virtual*/ BOOL postBuild();

	/*
	 * Member functions
	 */
public:
	const LLUUID& getAssetId() const { return m_idAsset; }
	const LLUUID& getItemId() const  { return m_idItem; }
	bool          hasNotecardInfo() const;
protected:
	void onClickOpen();
	void onClickCopyToInv();

	/*
	 * Member variables
	 */
protected:
	LLUUID         m_idAsset;
	LLUUID         m_idItem;		// Item UUID relative to gInventoryModel (or null if not displaying an inventory texture
	LLUUID         m_idNotecard;
	LLTextureCtrl* m_pTextureCtrl;
	LLTextBox*     m_pTextureName;
};

// ============================================================================
// LLInspectTexture class
//

LLInspectTexture::LLInspectTexture(const LLSD& sdKey)
	: LLInspect(LLSD())
	, m_pTextureCtrl(NULL)
	, m_pTextureName(NULL)
{
	mCommitCallbackRegistrar.add("InspectTexture.Open", boost::bind(&LLInspectTexture::onClickOpen, this));
	mEnableCallbackRegistrar.add("InspectTexture.CanOpen", boost::bind(&LLInspectTexture::hasNotecardInfo, this));
	mCommitCallbackRegistrar.add("InspectTexture.CopyToInv", boost::bind(&LLInspectTexture::onClickCopyToInv, this));	
	mEnableCallbackRegistrar.add("InspectTexture.CanCopyToInv", boost::bind(&LLInspectTexture::hasNotecardInfo, this));	
}

LLInspectTexture::~LLInspectTexture()
{
}

void LLInspectTexture::onOpen(const LLSD& sdData)
{
	// Start fade animation
	LLInspect::onOpen(sdData);

	bool fIsAsset = sdData.has("asset_id");
	bool fIsInventory = sdData.has("item_id");

	// Skip if we're being asked to display the same thing
	const LLUUID idAsset = (fIsAsset) ? sdData["asset_id"].asUUID() : LLUUID::null;
	const LLUUID idItem = (fIsInventory) ? sdData["item_id"].asUUID() : LLUUID::null;
	if ( (getVisible()) && ( ((fIsAsset) && (idAsset == m_idAsset)) || ((fIsInventory) && (idItem == m_idItem)) ) )
	{
		return;
	}

	// Position the inspector relative to the mouse cursor
	// Similar to how tooltips are positioned [see LLToolTipMgr::createToolTip()]
	if (sdData.has("pos"))
		LLUI::positionViewNearMouse(this, sdData["pos"]["x"].asInteger(), sdData["pos"]["y"].asInteger());
	else
		LLUI::positionViewNearMouse(this);

	std::string strName = sdData["name"].asString();
	if (fIsAsset)
	{
		m_idAsset = idAsset;
		m_idItem = idItem;		// Will be non-null in the case of a notecard
		m_idNotecard = sdData["notecard_id"].asUUID();
	}
	else if (fIsInventory)
	{
		const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
		if ( (pItem) && (LLAssetType::AT_TEXTURE == pItem->getType()) )
		{
			if (strName.empty())
				strName = pItem->getName();
			m_idAsset = pItem->getAssetUUID();
			m_idItem = idItem;
		}
		else
		{
			m_idAsset.setNull();
			m_idItem.setNull();
		}
		m_idNotecard = LLUUID::null;
	}

	m_pTextureCtrl->setImageAssetID(m_idAsset);
	m_pTextureName->setText(strName);
}

BOOL LLInspectTexture::postBuild()
{
	m_pTextureCtrl = getChild<LLTextureCtrl>("texture_ctrl");
	m_pTextureName = getChild<LLTextBox>("texture_name");

	return TRUE;
}

bool LLInspectTexture::hasNotecardInfo() const
{
	return (m_idNotecard.notNull()) && (LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", m_idNotecard));
}

void LLInspectTexture::onClickOpen()
{
	if (m_idNotecard.notNull())
	{
		LLPreviewNotecard* pPreview = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", m_idNotecard);
		LLViewerTextEditor* pEditor = (pPreview) ? pPreview->getEditor() : NULL;
		if (pEditor)
		{
			pEditor->openEmbeddedItem(m_idItem);
		}
	}
	else
	{
		LLFloaterReg::showTypedInstance<LLPreviewTexture>("preview_texture", m_idAsset, TAKE_FOCUS_YES);
	}
}

void LLInspectTexture::onClickCopyToInv()
{
	if (m_idNotecard.notNull())
	{
		LLPreviewNotecard* pPreview = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", m_idNotecard);
		LLViewerTextEditor* pEditor = (pPreview) ? pPreview->getEditor() : NULL;
		if (pEditor)
		{
			pEditor->showCopyToInvDialog(m_idItem);
		}
	}
}

// ============================================================================
// Helper functions
//

void LLInspectTextureUtil::registerFloater()
{
	LLFloaterReg::add("inspect_texture", "inspect_texture.xml", &LLFloaterReg::build<LLInspectTexture>);
}

// ============================================================================
// LLTexturePreviewView helper class
//

LLTexturePreviewView::LLTexturePreviewView(const LLView::Params& p)
	: LLView(p)
	, m_nImageBoostLevel(LLGLTexture::BOOST_NONE)
{
	m_strLoading = LLTrans::getString("texture_loading");
}

LLTexturePreviewView::~LLTexturePreviewView()
{
	if (m_Image)
	{
		m_Image->setBoostLevel(m_nImageBoostLevel);
		m_Image = NULL;
	}
}

void LLTexturePreviewView::draw()
{
	if (m_Image)
	{
		LLRect rctClient = getLocalRect();

		gl_rect_2d(rctClient, LLColor4::black);
		rctClient.stretch(-2);
		if (4 == m_Image->getComponents())
			gl_rect_2d_checkerboard(rctClient);
		gl_draw_scaled_image(rctClient.mLeft, rctClient.mBottom, rctClient.getWidth(), rctClient.getHeight(), m_Image);

		m_Image->addTextureStats((F32)(rctClient.getWidth() * rctClient.getHeight()));

		if ( (!m_Image->isFullyLoaded()) && (m_Image->getDiscardLevel() > 1) )
		{
			LLFontGL::getFontSansSerif()->renderUTF8(m_strLoading, 0, llfloor(rctClient.mLeft + 3),  llfloor(rctClient.mTop - 25), LLColor4::white, LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);
		}
	}
}

void LLTexturePreviewView::setImageFromAssetId(const LLUUID& idAsset)
{
	// Initialize the texture
	m_Image = LLViewerTextureManager::getFetchedTexture(idAsset, FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	if (m_Image)
	{
		m_nImageBoostLevel = m_Image->getBoostLevel();
		m_Image->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
		m_Image->forceToSaveRawImage(0);
	}
}

void LLTexturePreviewView::setImageFromItemId(const LLUUID& idItem)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
	setImageFromAssetId( (pItem) ? pItem->getAssetUUID() : LLUUID::null );
}

// ============================================================================
// LLTextureToolTip class
//

LLTextureToolTip::LLTextureToolTip(const LLToolTip::Params& p)
	: LLToolTip(p)
	, m_pPreview(NULL)
	, m_nPreviewSize(256)
{
	mMaxWidth = llmax(mMaxWidth, m_nPreviewSize);
}

LLTextureToolTip::~LLTextureToolTip()
{
}

void LLTextureToolTip::initFromParams(const LLToolTip::Params& p)
{
	LLToolTip::initFromParams(p);

	// Create and add the preview control
	LLView::Params p_preview;
	p_preview.name = "texture_preview";
	LLRect rctPreview;
	rctPreview.setOriginAndSize(mPadding, mTextBox->getRect().mTop, m_nPreviewSize, m_nPreviewSize);
	p_preview.rect = rctPreview;
	m_pPreview = LLUICtrlFactory::create<LLTexturePreviewView>(p_preview);
	addChild(m_pPreview);

	// Parse the control params
	const LLSD& sdTextureParams = p.create_params;
	if (sdTextureParams.has("asset_id"))
		m_pPreview->setImageFromAssetId(sdTextureParams["asset_id"].asUUID());
	else if (sdTextureParams.has("item_id"))
		m_pPreview->setImageFromItemId(sdTextureParams["item_id"].asUUID());

	snapToChildren();
}

// ============================================================================
