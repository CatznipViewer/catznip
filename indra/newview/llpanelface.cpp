/** 
 * @file llpanelface.cpp
 * @brief Panel in the tools floater for editing face textures, colors, etc.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

// file include
#include "llpanelface.h"
 
// library includes
#include "llcalc.h"
#include "llerror.h"
#include "llfocusmgr.h"
#include "llrect.h"
#include "llstring.h"
#include "llfontgl.h"

// project includes
#include "llagentdata.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lldrawpoolbump.h"
#include "llface.h"
// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
// [/SL:KB]
#include "lllineeditor.h"
#include "llmaterialmgr.h"
#include "llmediaentry.h"
#include "llnotificationsutil.h"
//#include "llradiogroup.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "lltooldraganddrop.h"
// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
#include "lltoolmgr.h"
// [/SL:KB]
#include "lltrans.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvovolume.h"
#include "lluictrlfactory.h"
#include "llpluginclassmedia.h"
#include "llviewertexturelist.h"// Update sel manager as to which channel we're editing so it can reflect the correct overlay UI

// [SL:KB] - Patch: Build-CopyPasteParams | Checked: 2013-07-28 (Catznip-3.6)
#include "llsdutil_math.h"
#include "material_codes.h"
// [/SL:KB]

//
// Constant definitions for comboboxes
// Must match the commbobox definitions in panel_tools_texture.xml
//
const S32 MATMEDIA_MATERIAL = 0;	// Material
const S32 MATMEDIA_MEDIA = 1;		// Media
const S32 MATTYPE_DIFFUSE = 0;		// Diffuse material texture
const S32 MATTYPE_NORMAL = 1;		// Normal map
const S32 MATTYPE_SPECULAR = 2;		// Specular map
const S32 ALPHAMODE_MASK = 2;		// Alpha masking mode
const S32 BUMPY_TEXTURE = 18;		// use supplied normal map
const S32 SHINY_TEXTURE = 4;		// use supplied specular map

BOOST_STATIC_ASSERT(MATTYPE_DIFFUSE == LLRender::DIFFUSE_MAP && MATTYPE_NORMAL == LLRender::NORMAL_MAP && MATTYPE_SPECULAR == LLRender::SPECULAR_MAP);

//
// "Use texture" label for normal/specular type comboboxes
// Filled in at initialization from translated strings
//
std::string USE_TEXTURE;

LLRender::eTexIndex LLPanelFace::getTextureChannelToEdit()
{
	LLComboBox* combobox_matmedia = getChild<LLComboBox>("combobox matmedia");
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	LLComboBox* combobox_mattype = getChild<LLComboBox>("combobox mattype");
// [/SL:KB]
//	LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");

	LLRender::eTexIndex channel_to_edit = (combobox_matmedia && combobox_matmedia->getCurrentIndex() == MATMEDIA_MATERIAL) ?
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
													  (combobox_mattype ? (LLRender::eTexIndex)combobox_mattype->getCurrentIndex() : LLRender::DIFFUSE_MAP) : LLRender::DIFFUSE_MAP;
// [/SL:KB]
//	                                                    (radio_mat_type ? (LLRender::eTexIndex)radio_mat_type->getSelectedIndex() : LLRender::DIFFUSE_MAP) : LLRender::DIFFUSE_MAP;

	channel_to_edit = (channel_to_edit == LLRender::NORMAL_MAP)		? (getCurrentNormalMap().isNull()		? LLRender::DIFFUSE_MAP : channel_to_edit) : channel_to_edit;
	channel_to_edit = (channel_to_edit == LLRender::SPECULAR_MAP)	? (getCurrentSpecularMap().isNull()		? LLRender::DIFFUSE_MAP : channel_to_edit) : channel_to_edit;
	return channel_to_edit;
}

// Things the UI provides...
//
LLUUID	LLPanelFace::getCurrentNormalMap()			{ return getChild<LLTextureCtrl>("bumpytexture control")->getImageAssetID();	}
LLUUID	LLPanelFace::getCurrentSpecularMap()		{ return getChild<LLTextureCtrl>("shinytexture control")->getImageAssetID();	}
U32		LLPanelFace::getCurrentShininess()			{ return getChild<LLComboBox>("combobox shininess")->getCurrentIndex();			}
U32		LLPanelFace::getCurrentBumpiness()			{ return getChild<LLComboBox>("combobox bumpiness")->getCurrentIndex();			}
U8			LLPanelFace::getCurrentDiffuseAlphaMode()	{ return (U8)getChild<LLComboBox>("combobox alphamode")->getCurrentIndex();	}
U8			LLPanelFace::getCurrentAlphaMaskCutoff()	{ return (U8)getChild<LLUICtrl>("maskcutoff")->getValue().asInteger();			}
U8			LLPanelFace::getCurrentEnvIntensity()		{ return (U8)getChild<LLUICtrl>("environment")->getValue().asInteger();			}
U8			LLPanelFace::getCurrentGlossiness()			{ return (U8)getChild<LLUICtrl>("glossiness")->getValue().asInteger();			}
F32		LLPanelFace::getCurrentBumpyRot()			{ return getChild<LLUICtrl>("bumpyRot")->getValue().asReal();						}
F32		LLPanelFace::getCurrentBumpyScaleU()		{ return getChild<LLUICtrl>("bumpyScaleU")->getValue().asReal();					}
F32		LLPanelFace::getCurrentBumpyScaleV()		{ return getChild<LLUICtrl>("bumpyScaleV")->getValue().asReal();					}
F32		LLPanelFace::getCurrentBumpyOffsetU()		{ return getChild<LLUICtrl>("bumpyOffsetU")->getValue().asReal();					}
F32		LLPanelFace::getCurrentBumpyOffsetV()		{ return getChild<LLUICtrl>("bumpyOffsetV")->getValue().asReal();					}
F32		LLPanelFace::getCurrentShinyRot()			{ return getChild<LLUICtrl>("shinyRot")->getValue().asReal();						}
F32		LLPanelFace::getCurrentShinyScaleU()		{ return getChild<LLUICtrl>("shinyScaleU")->getValue().asReal();					}
F32		LLPanelFace::getCurrentShinyScaleV()		{ return getChild<LLUICtrl>("shinyScaleV")->getValue().asReal();					}
F32		LLPanelFace::getCurrentShinyOffsetU()		{ return getChild<LLUICtrl>("shinyOffsetU")->getValue().asReal();					}
F32		LLPanelFace::getCurrentShinyOffsetV()		{ return getChild<LLUICtrl>("shinyOffsetV")->getValue().asReal();					}

//
// Methods
//

BOOL	LLPanelFace::postBuild()
{
	childSetCommitCallback("combobox shininess",&LLPanelFace::onCommitShiny,this);
	childSetCommitCallback("combobox bumpiness",&LLPanelFace::onCommitBump,this);
	childSetCommitCallback("combobox alphamode",&LLPanelFace::onCommitAlphaMode,this);
// [SL:KB] - Patch: Build-ScaleParamFlip | Checked: Catznip-5.2
	{
		LLUICtrl* pTexScaleUCtrl = getChild<LLSpinCtrl>("TexScaleU");
		pTexScaleUCtrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitTextureScaleX, pTexScaleUCtrl, this));
		getChild<LLButton>("TexScaleUFlip")->setCommitCallback(boost::bind(&LLPanelFace::onCommitScaleFlip, pTexScaleUCtrl));

		LLUICtrl* pTexScaleVCtrl = getChild<LLSpinCtrl>("TexScaleV");
		pTexScaleVCtrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitTextureScaleY, pTexScaleVCtrl, this));
		getChild<LLButton>("TexScaleVFlip")->setCommitCallback(boost::bind(&LLPanelFace::onCommitScaleFlip, pTexScaleVCtrl));
	}
// [/SL:KB]
//	childSetCommitCallback("TexScaleU",&LLPanelFace::onCommitTextureInfo, this);
//	childSetCommitCallback("TexScaleV",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexRot", &LLPanelFace::onCommitTextureRot, this);
	childSetCommitCallback("rptctrl",&LLPanelFace::onCommitRepeatsPerMeter, this);
	childSetCommitCallback("checkbox planar align",&LLPanelFace::onCommitPlanarAlign, this);
	childSetCommitCallback("TexOffsetU",LLPanelFace::onCommitTextureOffsetX, this);
	childSetCommitCallback("TexOffsetV",LLPanelFace::onCommitTextureOffsetY, this);

// [SL:KB] - Patch: Build-ScaleParamFlip | Checked: Catznip-5.2
	{
		LLUICtrl* pBumpyScaleUCtrl = getChild<LLSpinCtrl>("bumpyScaleU");
		pBumpyScaleUCtrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitMaterialBumpyScaleX, pBumpyScaleUCtrl, this));
		getChild<LLButton>("bumpyScaleUFlip")->setCommitCallback(boost::bind(&LLPanelFace::onCommitScaleFlip, pBumpyScaleUCtrl));

		LLUICtrl* pBumpyScaleVCtrl = getChild<LLSpinCtrl>("bumpyScaleV");
		pBumpyScaleVCtrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitMaterialBumpyScaleY, pBumpyScaleVCtrl, this));
		getChild<LLButton>("bumpyScaleVFlip")->setCommitCallback(boost::bind(&LLPanelFace::onCommitScaleFlip, pBumpyScaleVCtrl));
	}
// [/SL:KB]
//	childSetCommitCallback("bumpyScaleU",&LLPanelFace::onCommitMaterialBumpyScaleX, this);
//	childSetCommitCallback("bumpyScaleV",&LLPanelFace::onCommitMaterialBumpyScaleY, this);
	childSetCommitCallback("bumpyRot",&LLPanelFace::onCommitMaterialBumpyRot, this);
	childSetCommitCallback("bumpyOffsetU",&LLPanelFace::onCommitMaterialBumpyOffsetX, this);
	childSetCommitCallback("bumpyOffsetV",&LLPanelFace::onCommitMaterialBumpyOffsetY, this);
// [SL:KB] - Patch: Build-ScaleParamFlip | Checked: Catznip-5.2
	{
		LLUICtrl* pShinyScaleUCtrl = getChild<LLSpinCtrl>("shinyScaleU");
		pShinyScaleUCtrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitMaterialShinyScaleX, pShinyScaleUCtrl, this));
		getChild<LLButton>("shinyScaleUFlip")->setCommitCallback(boost::bind(&LLPanelFace::onCommitScaleFlip, pShinyScaleUCtrl));

		LLUICtrl* pShinyScaleVCtrl = getChild<LLSpinCtrl>("shinyScaleV");
		pShinyScaleVCtrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitMaterialShinyScaleY, pShinyScaleVCtrl, this));
		getChild<LLButton>("shinyScaleVFlip")->setCommitCallback(boost::bind(&LLPanelFace::onCommitScaleFlip, pShinyScaleVCtrl));
	}
// [/SL:KB]
//	childSetCommitCallback("shinyScaleU",&LLPanelFace::onCommitMaterialShinyScaleX, this);
//	childSetCommitCallback("shinyScaleV",&LLPanelFace::onCommitMaterialShinyScaleY, this);
	childSetCommitCallback("shinyRot",&LLPanelFace::onCommitMaterialShinyRot, this);
	childSetCommitCallback("shinyOffsetU",&LLPanelFace::onCommitMaterialShinyOffsetX, this);
	childSetCommitCallback("shinyOffsetV",&LLPanelFace::onCommitMaterialShinyOffsetY, this);
	childSetCommitCallback("glossiness",&LLPanelFace::onCommitMaterialGloss, this);
	childSetCommitCallback("environment",&LLPanelFace::onCommitMaterialEnv, this);
	childSetCommitCallback("maskcutoff",&LLPanelFace::onCommitMaterialMaskCutoff, this);

	childSetAction("button align",&LLPanelFace::onClickAutoFix,this);
	childSetAction("button align textures", &LLPanelFace::onAlignTexture, this);

	LLTextureCtrl*	mTextureCtrl;
	LLTextureCtrl*	mShinyTextureCtrl;
	LLTextureCtrl*	mBumpyTextureCtrl;
	LLColorSwatchCtrl*	mColorSwatch;
	LLColorSwatchCtrl*	mShinyColorSwatch;

	LLComboBox*		mComboTexGen;
	LLComboBox*		mComboMatMedia;
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	LLComboBox*		mComboMatType;
// [/SL:KB]

	LLCheckBoxCtrl	*mCheckFullbright;
	
	LLTextBox*		mLabelColorTransp;
	LLSpinCtrl*		mCtrlColorTransp;		// transparency = 1 - alpha

	LLSpinCtrl*     mCtrlGlow;

	setMouseOpaque(FALSE);

// [SL:KB] - Patch: Build-TexturePipette | Checked: 2013-07-27 (Catznip-3.6)
	mBtnCopyMaterialTypeParams = findChild<LLButton>("copy_mattype_btn");
	mBtnPasteMaterialTypeParams = findChild<LLButton>("paste_mattype_btn");
	mBtnMaterialTypePipette = findChild<LLButton>("pipette_mattype_btn");
	mBtnMaterialTypePipette->setCommitCallback(boost::bind(&LLPanelFace::onClickPipette, this, _1, LLToolPipette::TYPE_MATERIAL_TYPE));
// [/SL:KB]

	mTextureCtrl = getChild<LLTextureCtrl>("texture control");
	if(mTextureCtrl)
	{
		mTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectTexture" )));
		mTextureCtrl->setCommitCallback( boost::bind(&LLPanelFace::onCommitTexture, this, _2) );
		mTextureCtrl->setOnCancelCallback( boost::bind(&LLPanelFace::onCancelTexture, this, _2) );
		mTextureCtrl->setOnSelectCallback( boost::bind(&LLPanelFace::onSelectTexture, this, _2) );
		mTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
		mTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
		mTextureCtrl->setOnCloseCallback( boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2) );

		mTextureCtrl->setFollowsTop();
		mTextureCtrl->setFollowsLeft();
		mTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
		mTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	}

// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
	mBtnTexturePipette = findChild<LLButton>("texture_pipette");
	mBtnTexturePipette->setCommitCallback(boost::bind(&LLPanelFace::onClickPipette, this, _1, LLToolPipette::TYPE_TEXTURE));
// [/SL:KB]

	mShinyTextureCtrl = getChild<LLTextureCtrl>("shinytexture control");
	if(mShinyTextureCtrl)
	{
		mShinyTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectSpecularTexture" )));
		mShinyTextureCtrl->setCommitCallback( boost::bind(&LLPanelFace::onCommitSpecularTexture, this, _2) );
		mShinyTextureCtrl->setOnCancelCallback( boost::bind(&LLPanelFace::onCancelSpecularTexture, this, _2) );
		mShinyTextureCtrl->setOnSelectCallback( boost::bind(&LLPanelFace::onSelectSpecularTexture, this, _2) );
		mShinyTextureCtrl->setOnCloseCallback( boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2) );
		
		mShinyTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
		mShinyTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
		mShinyTextureCtrl->setFollowsTop();
		mShinyTextureCtrl->setFollowsLeft();
		mShinyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
		mShinyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	}

	mBumpyTextureCtrl = getChild<LLTextureCtrl>("bumpytexture control");
	if(mBumpyTextureCtrl)
	{
		mBumpyTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectNormalTexture" )));
		mBumpyTextureCtrl->setBlankImageAssetID(LLUUID( gSavedSettings.getString( "DefaultBlankNormalTexture" )));
		mBumpyTextureCtrl->setCommitCallback( boost::bind(&LLPanelFace::onCommitNormalTexture, this, _2) );
		mBumpyTextureCtrl->setOnCancelCallback( boost::bind(&LLPanelFace::onCancelNormalTexture, this, _2) );
		mBumpyTextureCtrl->setOnSelectCallback( boost::bind(&LLPanelFace::onSelectNormalTexture, this, _2) );
		mBumpyTextureCtrl->setOnCloseCallback( boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2) );

		mBumpyTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
		mBumpyTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
		mBumpyTextureCtrl->setFollowsTop();
		mBumpyTextureCtrl->setFollowsLeft();
		mBumpyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
		mBumpyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	}

	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(mColorSwatch)
	{
		mColorSwatch->setCommitCallback(boost::bind(&LLPanelFace::onCommitColor, this, _2));
		mColorSwatch->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelColor, this, _2));
		mColorSwatch->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectColor, this, _2));
		mColorSwatch->setFollowsTop();
		mColorSwatch->setFollowsLeft();
		mColorSwatch->setCanApplyImmediately(TRUE);
	}

// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
	mBtnColorPipette = findChild<LLButton>("color_pipette");
	mBtnColorPipette->setCommitCallback(boost::bind(&LLPanelFace::onClickPipette, this, _1, LLToolPipette::TYPE_COLOR));
// [/SL:KB]

	mShinyColorSwatch = getChild<LLColorSwatchCtrl>("shinycolorswatch");
	if(mShinyColorSwatch)
	{
		mShinyColorSwatch->setCommitCallback(boost::bind(&LLPanelFace::onCommitShinyColor, this, _2));
		mShinyColorSwatch->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelShinyColor, this, _2));
		mShinyColorSwatch->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectShinyColor, this, _2));
		mShinyColorSwatch->setFollowsTop();
		mShinyColorSwatch->setFollowsLeft();
		mShinyColorSwatch->setCanApplyImmediately(TRUE);
	}

	mLabelColorTransp = getChild<LLTextBox>("color trans");
	if(mLabelColorTransp)
	{
		mLabelColorTransp->setFollowsTop();
		mLabelColorTransp->setFollowsLeft();
	}

	mCtrlColorTransp = getChild<LLSpinCtrl>("ColorTrans");
	if(mCtrlColorTransp)
	{
		mCtrlColorTransp->setCommitCallback(boost::bind(&LLPanelFace::onCommitAlpha, this, _2));
		mCtrlColorTransp->setPrecision(0);
		mCtrlColorTransp->setFollowsTop();
		mCtrlColorTransp->setFollowsLeft();
	}

	mCheckFullbright = getChild<LLCheckBoxCtrl>("checkbox fullbright");
	if (mCheckFullbright)
	{
		mCheckFullbright->setCommitCallback(LLPanelFace::onCommitFullbright, this);
	}

	mComboTexGen = getChild<LLComboBox>("combobox texgen");
	if(mComboTexGen)
	{
		mComboTexGen->setCommitCallback(LLPanelFace::onCommitTexGen, this);
		mComboTexGen->setFollows(FOLLOWS_LEFT | FOLLOWS_TOP);	
	}

	mComboMatMedia = getChild<LLComboBox>("combobox matmedia");
	if(mComboMatMedia)
	{
		mComboMatMedia->setCommitCallback(LLPanelFace::onCommitMaterialsMedia,this);
		mComboMatMedia->selectNthItem(MATMEDIA_MATERIAL);
	}

// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	mComboMatType = getChild<LLComboBox>("combobox mattype");
	if(mComboMatType)
	{
		mComboMatType->setCommitCallback(LLPanelFace::onCommitMaterialType, this);
		mComboMatType->selectNthItem(MATTYPE_DIFFUSE);
	}
// [/SL:KB]
//	LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
//    if(radio_mat_type)
//    {
//        radio_mat_type->setCommitCallback(LLPanelFace::onCommitMaterialType, this);
//        radio_mat_type->selectNthItem(MATTYPE_DIFFUSE);
//    }

	mCtrlGlow = getChild<LLSpinCtrl>("glow");
	if(mCtrlGlow)
	{
		mCtrlGlow->setCommitCallback(LLPanelFace::onCommitGlow, this);
	}
	

	clearCtrls();

// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
	LLToolPipette::getInstance()->setToolSelectCallback(boost::bind(&LLPanelFace::onSelectPipette, this, _1, _2, _3));
// [/SL:KB]

	return TRUE;
}

LLPanelFace::LLPanelFace()
:	LLPanel(),
	mIsAlpha(false)
// [SL:KB] - Patch: Build-TexturePipette | Checked: 2013-07-27 (Catznip-3.6)
,	mBtnColorPipette(NULL)
,	mBtnCopyMaterialTypeParams(NULL)
,	mBtnPasteMaterialTypeParams(NULL)
,	mBtnMaterialTypePipette(NULL)
,	mBtnTexturePipette(NULL)
// [/SL:KB]
{
	USE_TEXTURE = LLTrans::getString("use_texture");
// [SL:KB] - Patch: Build-TexturePipette | Checked: 2013-07-27 (Catznip-3.6)
	mCommitCallbackRegistrar.add("BuildTool.CopyParams", boost::bind(&LLPanelFace::onClickBtnCopyParams, this, _2));
	mCommitCallbackRegistrar.add("BuildTool.PasteParams", boost::bind(&LLPanelFace::onClickBtnPasteParams, this, _2));
// [/SL:KB]
}


LLPanelFace::~LLPanelFace()
{
	// Children all cleaned up by default view destructor.
}

// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
void LLPanelFace::draw()
{
	LLToolPipette::EType typePipette = (LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance()) ? LLToolPipette::getInstance()->getPipetteType() : LLToolPipette::TYPE_NONE;
	mBtnColorPipette->setValue(LLToolPipette::TYPE_COLOR == typePipette);
	mBtnMaterialTypePipette->setValue(LLToolPipette::TYPE_MATERIAL_TYPE == typePipette);
	mBtnTexturePipette->setValue(LLToolPipette::TYPE_TEXTURE == typePipette);

	LLPanel::draw();
}
// [/SL:KB]

void LLPanelFace::sendTexture()
{
	LLTextureCtrl* mTextureCtrl = getChild<LLTextureCtrl>("texture control");
	if(!mTextureCtrl) return;
	if( !mTextureCtrl->getTentative() )
	{
		// we grab the item id first, because we want to do a
		// permissions check in the selection manager. ARGH!
		LLUUID id = mTextureCtrl->getImageItemID();
		if(id.isNull())
		{
			id = mTextureCtrl->getImageAssetID();
		}
		LLSelectMgr::getInstance()->selectionSetImage(id);
	}
}

void LLPanelFace::sendBump(U32 bumpiness)
{	
	LLTextureCtrl* bumpytexture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
	if (bumpiness < BUMPY_TEXTURE)
{	
		LL_DEBUGS("Materials") << "clearing bumptexture control" << LL_ENDL;	
		bumpytexture_ctrl->clear();
		bumpytexture_ctrl->setImageAssetID(LLUUID());		
	}

	updateBumpyControls(bumpiness == BUMPY_TEXTURE, true);

	LLUUID current_normal_map = bumpytexture_ctrl->getImageAssetID();

	U8 bump = (U8) bumpiness & TEM_BUMP_MASK;

	// Clear legacy bump to None when using an actual normal map
	//
	if (!current_normal_map.isNull())
		bump = 0;

	// Set the normal map or reset it to null as appropriate
	//
	LLSelectedTEMaterial::setNormalID(this, current_normal_map);

	LLSelectMgr::getInstance()->selectionSetBumpmap( bump );
}

void LLPanelFace::sendTexGen()
{
	LLComboBox*	mComboTexGen = getChild<LLComboBox>("combobox texgen");
	if(!mComboTexGen)return;
	U8 tex_gen = (U8) mComboTexGen->getCurrentIndex() << TEM_TEX_GEN_SHIFT;
	LLSelectMgr::getInstance()->selectionSetTexGen( tex_gen );
}

void LLPanelFace::sendShiny(U32 shininess)
{
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("shinytexture control");

	if (shininess < SHINY_TEXTURE)
{
		texture_ctrl->clear();
		texture_ctrl->setImageAssetID(LLUUID());		
	}

	LLUUID specmap = getCurrentSpecularMap();

	U8 shiny = (U8) shininess & TEM_SHINY_MASK;
	if (!specmap.isNull())
		shiny = 0;

	LLSelectedTEMaterial::setSpecularID(this, specmap);

	LLSelectMgr::getInstance()->selectionSetShiny( shiny );

	updateShinyControls(!specmap.isNull(), true);
	
}

void LLPanelFace::sendFullbright()
{
	LLCheckBoxCtrl*	mCheckFullbright = getChild<LLCheckBoxCtrl>("checkbox fullbright");
	if(!mCheckFullbright)return;
	U8 fullbright = mCheckFullbright->get() ? TEM_FULLBRIGHT_MASK : 0;
	LLSelectMgr::getInstance()->selectionSetFullbright( fullbright );
}

void LLPanelFace::sendColor()
{
	
	LLColorSwatchCtrl*	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(!mColorSwatch)return;
	LLColor4 color = mColorSwatch->get();

	LLSelectMgr::getInstance()->selectionSetColorOnly( color );
}

void LLPanelFace::sendAlpha()
{	
	LLSpinCtrl*	mCtrlColorTransp = getChild<LLSpinCtrl>("ColorTrans");
	if(!mCtrlColorTransp)return;
	F32 alpha = (100.f - mCtrlColorTransp->get()) / 100.f;

	LLSelectMgr::getInstance()->selectionSetAlphaOnly( alpha );
}


void LLPanelFace::sendGlow()
{
	LLSpinCtrl* mCtrlGlow = getChild<LLSpinCtrl>("glow");
	llassert(mCtrlGlow);
	if (mCtrlGlow)
	{
		F32 glow = mCtrlGlow->get();
		LLSelectMgr::getInstance()->selectionSetGlow( glow );
	}
}

struct LLPanelFaceSetTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetTEFunctor(LLPanelFace* panel) : mPanel(panel) {}
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		BOOL valid;
		F32 value;

//        LLRadioGroup * radio_mat_type = mPanel->getChild<LLRadioGroup>("radio_material_type");
//        std::string prefix;
//        switch (radio_mat_type->getSelectedIndex())
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-5.3
		const LLComboBox* combobox_mattype = mPanel->getChild<LLComboBox>("combobox mattype");
        std::string prefix;
        switch (combobox_mattype->getCurrentIndex())
// [/SL:KB]
        {
        case MATTYPE_DIFFUSE:
            prefix = "Tex";
            break;
        case MATTYPE_NORMAL:
            prefix = "bumpy";
            break;
        case MATTYPE_SPECULAR:
            prefix = "shiny";
            break;
        }
        
        LLSpinCtrl * ctrlTexScaleS = mPanel->getChild<LLSpinCtrl>(prefix + "ScaleU");
        LLSpinCtrl * ctrlTexScaleT = mPanel->getChild<LLSpinCtrl>(prefix + "ScaleV");
        LLSpinCtrl * ctrlTexOffsetS = mPanel->getChild<LLSpinCtrl>(prefix + "OffsetU");
        LLSpinCtrl * ctrlTexOffsetT = mPanel->getChild<LLSpinCtrl>(prefix + "OffsetV");
        LLSpinCtrl * ctrlTexRotation = mPanel->getChild<LLSpinCtrl>(prefix + "Rot");

		LLComboBox*	comboTexGen = mPanel->getChild<LLComboBox>("combobox texgen");
		LLCheckBoxCtrl*	cb_planar_align = mPanel->getChild<LLCheckBoxCtrl>("checkbox planar align");
		bool align_planar = (cb_planar_align && cb_planar_align->get());

		llassert(comboTexGen);
		llassert(object);

		if (ctrlTexScaleS)
		{
			valid = !ctrlTexScaleS->getTentative(); // || !checkFlipScaleS->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexScaleS->get();
				if (comboTexGen &&
				    comboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleS( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalRepeatX(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatX(mPanel, value, te, object->getID());
				}
			}
		}

		if (ctrlTexScaleT)
		{
			valid = !ctrlTexScaleT->getTentative(); // || !checkFlipScaleT->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexScaleT->get();
				//if( checkFlipScaleT->get() )
				//{
				//	value = -value;
				//}
				if (comboTexGen &&
				    comboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleT( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalRepeatY(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatY(mPanel, value, te, object->getID());
				}
			}
		}

		if (ctrlTexOffsetS)
		{
			valid = !ctrlTexOffsetS->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexOffsetS->get();
				object->setTEOffsetS( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalOffsetX(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetX(mPanel, value, te, object->getID());
				}
			}
		}

		if (ctrlTexOffsetT)
		{
			valid = !ctrlTexOffsetT->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexOffsetT->get();
				object->setTEOffsetT( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalOffsetY(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetY(mPanel, value, te, object->getID());
				}
			}
		}

		if (ctrlTexRotation)
		{
			valid = !ctrlTexRotation->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexRotation->get() * DEG_TO_RAD;
				object->setTERotation( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalRotation(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularRotation(mPanel, value, te, object->getID());
				}
			}
		}
		return true;
	}
private:
	LLPanelFace* mPanel;
};

// Functor that aligns a face to mCenterFace
struct LLPanelFaceSetAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetAlignedTEFunctor(LLPanelFace* panel, LLFace* center_face) :
		mPanel(panel),
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return true;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{
			return true;
		}

		bool set_aligned = true;
		if (facep == mCenterFace)
		{
			set_aligned = false;
		}
		if (set_aligned)
		{
			LLVector2 uv_offset, uv_scale;
			F32 uv_rot;
			set_aligned = facep->calcAlignedPlanarTE(mCenterFace, &uv_offset, &uv_scale, &uv_rot);
			if (set_aligned)
			{
				object->setTEOffset(te, uv_offset.mV[VX], uv_offset.mV[VY]);
				object->setTEScale(te, uv_scale.mV[VX], uv_scale.mV[VY]);
				object->setTERotation(te, uv_rot);

				LLPanelFace::LLSelectedTEMaterial::setNormalRotation(mPanel, uv_rot, te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularRotation(mPanel, uv_rot, te, object->getID());

				LLPanelFace::LLSelectedTEMaterial::setNormalOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setNormalOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setNormalRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setNormalRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());

				LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());
			}
		}
		if (!set_aligned)
		{
			LLPanelFaceSetTEFunctor setfunc(mPanel);
			setfunc.apply(object, te);
		}
		return true;
	}
private:
	LLPanelFace* mPanel;
	LLFace* mCenterFace;
};

struct LLPanelFaceSetAlignedConcreteTEFunctor : public LLSelectedTEFunctor
{
    LLPanelFaceSetAlignedConcreteTEFunctor(LLPanelFace* panel, LLFace* center_face, LLRender::eTexIndex map) :
        mPanel(panel),
        mChefFace(center_face),
        mMap(map)
    {}

    virtual bool apply(LLViewerObject* object, S32 te)
    {
        LLFace* facep = object->mDrawable->getFace(te);
        if (!facep)
        {
            return true;
        }

        if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
        {
            return true;
        }

        if (mChefFace != facep)
        {
            LLVector2 uv_offset, uv_scale;
            F32 uv_rot;
            if (facep->calcAlignedPlanarTE(mChefFace, &uv_offset, &uv_scale, &uv_rot, mMap))
            {
                switch (mMap)
                {
                case LLRender::DIFFUSE_MAP:
                        object->setTEOffset(te, uv_offset.mV[VX], uv_offset.mV[VY]);
                        object->setTEScale(te, uv_scale.mV[VX], uv_scale.mV[VY]);
                        object->setTERotation(te, uv_rot);
                    break;
                case LLRender::NORMAL_MAP:
                        LLPanelFace::LLSelectedTEMaterial::setNormalRotation(mPanel, uv_rot, te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setNormalOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setNormalOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setNormalRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setNormalRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());
                    break;
                case LLRender::SPECULAR_MAP:
                        LLPanelFace::LLSelectedTEMaterial::setSpecularRotation(mPanel, uv_rot, te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());
                    break;
                default: /*make compiler happy*/
                    break;
                }
            }
        }
        
        return true;
    }
private:
    LLPanelFace* mPanel;
    LLFace* mChefFace;
    LLRender::eTexIndex mMap;
};

// Functor that tests if a face is aligned to mCenterFace
struct LLPanelFaceGetIsAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceGetIsAlignedTEFunctor(LLFace* center_face) :
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return false;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{ //volume face does not exist, can't be aligned
			return false;
		}

		if (facep == mCenterFace)
		{
			return true;
		}
		
		LLVector2 aligned_st_offset, aligned_st_scale;
		F32 aligned_st_rot;
		if ( facep->calcAlignedPlanarTE(mCenterFace, &aligned_st_offset, &aligned_st_scale, &aligned_st_rot) )
		{
			const LLTextureEntry* tep = facep->getTextureEntry();
			LLVector2 st_offset, st_scale;
			tep->getOffset(&st_offset.mV[VX], &st_offset.mV[VY]);
			tep->getScale(&st_scale.mV[VX], &st_scale.mV[VY]);
			F32 st_rot = tep->getRotation();

            bool eq_offset_x = is_approx_equal_fraction(st_offset.mV[VX], aligned_st_offset.mV[VX], 12);
            bool eq_offset_y = is_approx_equal_fraction(st_offset.mV[VY], aligned_st_offset.mV[VY], 12);
            bool eq_scale_x  = is_approx_equal_fraction(st_scale.mV[VX], aligned_st_scale.mV[VX], 12);
            bool eq_scale_y  = is_approx_equal_fraction(st_scale.mV[VY], aligned_st_scale.mV[VY], 12);
            bool eq_rot      = is_approx_equal_fraction(st_rot, aligned_st_rot, 6);

			// needs a fuzzy comparison, because of fp errors
			if (eq_offset_x && 
				eq_offset_y && 
				eq_scale_x &&
				eq_scale_y &&
				eq_rot)
			{
				return true;
			}
		}
		return false;
	}
private:
	LLFace* mCenterFace;
};

struct LLPanelFaceSendFunctor : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* object)
	{
		object->sendTEUpdate();
		return true;
	}
};

void LLPanelFace::sendTextureInfo()
{
	if ((bool)childGetValue("checkbox planar align").asBoolean())
	{
		LLFace* last_face = NULL;
		bool identical_face =false;
		LLSelectedTE::getFace(last_face, identical_face);		
		LLPanelFaceSetAlignedTEFunctor setfunc(this, last_face);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}
	else
	{
		LLPanelFaceSetTEFunctor setfunc(this);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

void LLPanelFace::alignTestureLayer()
{
    LLFace* last_face = NULL;
    bool identical_face = false;
    LLSelectedTE::getFace(last_face, identical_face);

//    LLRadioGroup * radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
//    LLPanelFaceSetAlignedConcreteTEFunctor setfunc(this, last_face, static_cast<LLRender::eTexIndex>(radio_mat_type->getSelectedIndex()));
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-5.3
	const LLComboBox* combobox_mattype = getChild<LLComboBox>("combobox mattype");
    LLPanelFaceSetAlignedConcreteTEFunctor setfunc(this, last_face, static_cast<LLRender::eTexIndex>(combobox_mattype->getCurrentIndex()));
// [/SL:KB]
    LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
}

void LLPanelFace::getState()
{
	updateUI();
}

void LLPanelFace::updateUI(bool force_set_values /*false*/)
{ //set state of UI to match state of texture entry(ies)  (calls setEnabled, setValue, etc, but NOT setVisible)
	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();

	if( objectp
		&& objectp->getPCode() == LL_PCODE_VOLUME
		&& objectp->permModify())
	{
		BOOL editable = objectp->permModify() && !objectp->isPermanentEnforced();

		// only turn on auto-adjust button if there is a media renderer and the media is loaded
		getChildView("button align")->setEnabled(editable);
		
		LLComboBox* combobox_matmedia = getChild<LLComboBox>("combobox matmedia");
		if (combobox_matmedia)
		{
			if (combobox_matmedia->getCurrentIndex() < MATMEDIA_MATERIAL)
			{
				combobox_matmedia->selectNthItem(MATMEDIA_MATERIAL);
			}
		}
		else
		{
			LL_WARNS() << "failed getChild for 'combobox matmedia'" << LL_ENDL;
		}
		getChildView("combobox matmedia")->setEnabled(editable);

// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
		LLComboBox* combobox_mattype = getChild<LLComboBox>("combobox mattype");
		if (combobox_mattype)
		{
			if (combobox_mattype->getCurrentIndex() < MATTYPE_DIFFUSE)
			{
				combobox_mattype->selectNthItem(MATTYPE_DIFFUSE);
			}
		}
		else
		{
			LL_WARNS("Materials") << "failed getChild for 'combobox mattype'" << LL_ENDL;
		}
		getChildView("combobox mattype")->setEnabled(editable);
// [/SL:KB]
//		LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
//		if(radio_mat_type)
//		{
//		    if (radio_mat_type->getSelectedIndex() < MATTYPE_DIFFUSE)
//		    {
//		        radio_mat_type->selectNthItem(MATTYPE_DIFFUSE);
//		    }
//
//		}
//		else
//		{
//		    LL_WARNS("Materials") << "failed getChild for 'radio_material_type'" << LL_ENDL;
//		}
//
//		getChildView("radio_material_type")->setEnabled(editable);

// [SL:KB] - Patch: Build-CopyPasteParams | Checked: 2011-10-09 (Catznip-3.0)
		mBtnCopyMaterialTypeParams->setEnabled(editable);
		mBtnPasteMaterialTypeParams->setEnabled(editable && mObjectClipboard.has(combobox_mattype->getValue().asString()));
		mBtnMaterialTypePipette->setEnabled(editable);
// [/SL:KB]

		getChildView("checkbox_sync_settings")->setEnabled(editable);
		childSetValue("checkbox_sync_settings", gSavedSettings.getBOOL("SyncMaterialSettings"));
		updateVisibility();

		bool identical				= true;	// true because it is anded below
      bool identical_diffuse	= false;
      bool identical_norm		= false;
      bool identical_spec		= false;

		LLTextureCtrl*	texture_ctrl = getChild<LLTextureCtrl>("texture control");
		LLTextureCtrl*	shinytexture_ctrl = getChild<LLTextureCtrl>("shinytexture control");
		LLTextureCtrl*	bumpytexture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
		
		LLUUID id;
		LLUUID normmap_id;
		LLUUID specmap_id;
		
		// Color swatch
		{
			getChildView("color label")->setEnabled(editable);
		}
		LLColorSwatchCtrl*	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");

		LLColor4 color					= LLColor4::white;
		bool		identical_color	= false;

		if(mColorSwatch)
		{
			LLSelectedTE::getColor(color, identical_color);
			LLColor4 prev_color = mColorSwatch->get();

			mColorSwatch->setOriginal(color);
			mColorSwatch->set(color, force_set_values || (prev_color != color) || !editable);

			mColorSwatch->setValid(editable);
			mColorSwatch->setEnabled( editable );
			mColorSwatch->setCanApplyImmediately( editable );
		}
// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
		mBtnColorPipette->setEnabled(editable);
// [/SL:KB]

		// Color transparency
		getChildView("color trans")->setEnabled(editable);

		F32 transparency = (1.f - color.mV[VALPHA]) * 100.f;
		getChild<LLUICtrl>("ColorTrans")->setValue(editable ? transparency : 0);
		getChildView("ColorTrans")->setEnabled(editable);

		// Specular map
		LLSelectedTEMaterial::getSpecularID(specmap_id, identical_spec);
		
		U8 shiny = 0;
		bool identical_shiny = false;

		// Shiny
		LLSelectedTE::getShiny(shiny, identical_shiny);
		identical = identical && identical_shiny;

		shiny = specmap_id.isNull() ? shiny : SHINY_TEXTURE;

		LLCtrlSelectionInterface* combobox_shininess = childGetSelectionInterface("combobox shininess");
		if (combobox_shininess)
				{
			combobox_shininess->selectNthItem((S32)shiny);
		}

		getChildView("label shininess")->setEnabled(editable);
		getChildView("combobox shininess")->setEnabled(editable);

		getChildView("label glossiness")->setEnabled(editable);			
		getChildView("glossiness")->setEnabled(editable);

		getChildView("label environment")->setEnabled(editable);
		getChildView("environment")->setEnabled(editable);
		getChildView("label shinycolor")->setEnabled(editable);
					
		getChild<LLUICtrl>("combobox shininess")->setTentative(!identical_spec);
		getChild<LLUICtrl>("glossiness")->setTentative(!identical_spec);
		getChild<LLUICtrl>("environment")->setTentative(!identical_spec);			
		getChild<LLUICtrl>("shinycolorswatch")->setTentative(!identical_spec);
					
		LLColorSwatchCtrl*	mShinyColorSwatch = getChild<LLColorSwatchCtrl>("shinycolorswatch");
		if(mShinyColorSwatch)
					{
			mShinyColorSwatch->setValid(editable);
			mShinyColorSwatch->setEnabled( editable );
			mShinyColorSwatch->setCanApplyImmediately( editable );
		}

		U8 bumpy = 0;
		// Bumpy
						{
			bool identical_bumpy = false;
			LLSelectedTE::getBumpmap(bumpy,identical_bumpy);

			LLUUID norm_map_id = getCurrentNormalMap();
			LLCtrlSelectionInterface* combobox_bumpiness = childGetSelectionInterface("combobox bumpiness");

			bumpy = norm_map_id.isNull() ? bumpy : BUMPY_TEXTURE;

			if (combobox_bumpiness)
							{
				combobox_bumpiness->selectNthItem((S32)bumpy);
							}
			else
							{
				LL_WARNS() << "failed childGetSelectionInterface for 'combobox bumpiness'" << LL_ENDL;
							}

			getChildView("combobox bumpiness")->setEnabled(editable);
			getChild<LLUICtrl>("combobox bumpiness")->setTentative(!identical_bumpy);
			getChildView("label bumpiness")->setEnabled(editable);
						}

		// Texture
		{
			LLSelectedTE::getTexId(id,identical_diffuse);

			// Normal map
			LLSelectedTEMaterial::getNormalID(normmap_id, identical_norm);

			mIsAlpha = FALSE;
			LLGLenum image_format = GL_RGB;
			bool identical_image_format = false;
			LLSelectedTE::getImageFormat(image_format, identical_image_format);
            
         mIsAlpha = FALSE;
         switch (image_format)
         {
               case GL_RGBA:
               case GL_ALPHA:
               {
                  mIsAlpha = TRUE;
               }
               break;

               case GL_RGB: break;
               default:
               {
                  LL_WARNS() << "Unexpected tex format in LLPanelFace...resorting to no alpha" << LL_ENDL;
					}
               break;
				}

			if(LLViewerMedia::getInstance()->textureHasMedia(id))
			{
				getChildView("button align")->setEnabled(editable);
			}
			
			// Diffuse Alpha Mode

			// Init to the default that is appropriate for the alpha content of the asset
			//
			U8 alpha_mode = mIsAlpha ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

			bool identical_alpha_mode = false;

			// See if that's been overridden by a material setting for same...
			//
			LLSelectedTEMaterial::getCurrentDiffuseAlphaMode(alpha_mode, identical_alpha_mode, mIsAlpha);

			LLCtrlSelectionInterface* combobox_alphamode = childGetSelectionInterface("combobox alphamode");
			if (combobox_alphamode)
			{
				//it is invalid to have any alpha mode other than blend if transparency is greater than zero ... 
				// Want masking? Want emissive? Tough! You get BLEND!
				alpha_mode = (transparency > 0.f) ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : alpha_mode;

				// ... unless there is no alpha channel in the texture, in which case alpha mode MUST be none
				alpha_mode = mIsAlpha ? alpha_mode : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

				combobox_alphamode->selectNthItem(alpha_mode);
			}
			else
			{
				LL_WARNS() << "failed childGetSelectionInterface for 'combobox alphamode'" << LL_ENDL;
			}

			updateAlphaControls();

			if (texture_ctrl)
				{
				if (identical_diffuse)
				{
					texture_ctrl->setTentative(FALSE);
					texture_ctrl->setEnabled(editable);
					texture_ctrl->setImageAssetID(id);
					getChildView("combobox alphamode")->setEnabled(editable && mIsAlpha && transparency <= 0.f);
					getChildView("label alphamode")->setEnabled(editable && mIsAlpha);
					getChildView("maskcutoff")->setEnabled(editable && mIsAlpha);
					getChildView("label maskcutoff")->setEnabled(editable && mIsAlpha);

					texture_ctrl->setBakeTextureEnabled(TRUE);
				}
				else if (id.isNull())
					{
						// None selected
					texture_ctrl->setTentative(FALSE);
					texture_ctrl->setEnabled(FALSE);
					texture_ctrl->setImageAssetID(LLUUID::null);
					getChildView("combobox alphamode")->setEnabled(FALSE);
					getChildView("label alphamode")->setEnabled(FALSE);
					getChildView("maskcutoff")->setEnabled(FALSE);
					getChildView("label maskcutoff")->setEnabled(FALSE);

					texture_ctrl->setBakeTextureEnabled(false);
					}
					else
					{
						// Tentative: multiple selected with different textures
					texture_ctrl->setTentative(TRUE);
					texture_ctrl->setEnabled(editable);
					texture_ctrl->setImageAssetID(id);
					getChildView("combobox alphamode")->setEnabled(editable && mIsAlpha && transparency <= 0.f);
					getChildView("label alphamode")->setEnabled(editable && mIsAlpha);
					getChildView("maskcutoff")->setEnabled(editable && mIsAlpha);
					getChildView("label maskcutoff")->setEnabled(editable && mIsAlpha);
					
					texture_ctrl->setBakeTextureEnabled(TRUE);
// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
		mBtnTexturePipette->setEnabled(editable);
// [/SL:KB]
				}
				
			}

			if (shinytexture_ctrl)
			{
				shinytexture_ctrl->setTentative( !identical_spec );
				shinytexture_ctrl->setEnabled( editable );
				shinytexture_ctrl->setImageAssetID( specmap_id );
			}

			if (bumpytexture_ctrl)
			{
				bumpytexture_ctrl->setTentative( !identical_norm );
				bumpytexture_ctrl->setEnabled( editable );
				bumpytexture_ctrl->setImageAssetID( normmap_id );
			}
		}

		// planar align
		bool align_planar = false;
		bool identical_planar_aligned = false;
		{
			LLCheckBoxCtrl*	cb_planar_align = getChild<LLCheckBoxCtrl>("checkbox planar align");
			align_planar = (cb_planar_align && cb_planar_align->get());

			bool enabled = (editable && isIdenticalPlanarTexgen());
			childSetValue("checkbox planar align", align_planar && enabled);
			childSetVisible("checkbox planar align", enabled);
			childSetEnabled("checkbox planar align", enabled);
			childSetEnabled("button align textures", enabled && LLSelectMgr::getInstance()->getSelection()->getObjectCount() > 1);

			if (align_planar && enabled)
			{
				LLFace* last_face = NULL;
				bool identical_face = false;
				LLSelectedTE::getFace(last_face, identical_face);

				LLPanelFaceGetIsAlignedTEFunctor get_is_aligend_func(last_face);
				// this will determine if the texture param controls are tentative:
				identical_planar_aligned = LLSelectMgr::getInstance()->getSelection()->applyToTEs(&get_is_aligend_func);
			}
		}
		
		// Needs to be public and before tex scale settings below to properly reflect
		// behavior when in planar vs default texgen modes in the
		// NORSPEC-84 et al
		//
		LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
		bool identical_texgen = true;		
		bool identical_planar_texgen = false;

		{	
			LLSelectedTE::getTexGen(selected_texgen, identical_texgen);
			identical_planar_texgen = (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));
		}

		// Texture scale
		{
			bool identical_diff_scale_s = false;
			bool identical_spec_scale_s = false;
			bool identical_norm_scale_s = false;

			identical = align_planar ? identical_planar_aligned : identical;

			F32 diff_scale_s = 1.f;			
			F32 spec_scale_s = 1.f;
			F32 norm_scale_s = 1.f;

			LLSelectedTE::getScaleS(diff_scale_s, identical_diff_scale_s);			
			LLSelectedTEMaterial::getSpecularRepeatX(spec_scale_s, identical_spec_scale_s);
			LLSelectedTEMaterial::getNormalRepeatX(norm_scale_s, identical_norm_scale_s);

			diff_scale_s = editable ? diff_scale_s : 1.0f;
			diff_scale_s *= identical_planar_texgen ? 2.0f : 1.0f;
			
			norm_scale_s = editable ? norm_scale_s : 1.0f;
			norm_scale_s *= identical_planar_texgen ? 2.0f : 1.0f;

			spec_scale_s = editable ? spec_scale_s : 1.0f;
			spec_scale_s *= identical_planar_texgen ? 2.0f : 1.0f;

			getChild<LLUICtrl>("TexScaleU")->setValue(diff_scale_s);
			getChild<LLUICtrl>("shinyScaleU")->setValue(spec_scale_s);
			getChild<LLUICtrl>("bumpyScaleU")->setValue(norm_scale_s);

			getChildView("TexScaleU")->setEnabled(editable);
			getChildView("shinyScaleU")->setEnabled(editable && specmap_id.notNull());
			getChildView("bumpyScaleU")->setEnabled(editable && normmap_id.notNull());
// [SL:KB] - Patch: Build-ScaleParamFlip | Checked: Catznip-5.2
			getChildView("TexScaleUFlip")->setEnabled(editable);
			getChildView("shinyScaleUFlip")->setEnabled(editable && specmap_id.notNull());
			getChildView("bumpyScaleUFlip")->setEnabled(editable && normmap_id.notNull());
// [/SL:KB]

			BOOL diff_scale_tentative = !(identical && identical_diff_scale_s);
			BOOL norm_scale_tentative = !(identical && identical_norm_scale_s);
			BOOL spec_scale_tentative = !(identical && identical_spec_scale_s);

			getChild<LLUICtrl>("TexScaleU")->setTentative(  LLSD(diff_scale_tentative));			
			getChild<LLUICtrl>("shinyScaleU")->setTentative(LLSD(spec_scale_tentative));			
			getChild<LLUICtrl>("bumpyScaleU")->setTentative(LLSD(norm_scale_tentative));
		}

		{
			bool identical_diff_scale_t = false;
			bool identical_spec_scale_t = false;
			bool identical_norm_scale_t = false;

			F32 diff_scale_t = 1.f;			
			F32 spec_scale_t = 1.f;
			F32 norm_scale_t = 1.f;

			LLSelectedTE::getScaleT(diff_scale_t, identical_diff_scale_t);
			LLSelectedTEMaterial::getSpecularRepeatY(spec_scale_t, identical_spec_scale_t);
			LLSelectedTEMaterial::getNormalRepeatY(norm_scale_t, identical_norm_scale_t);

			diff_scale_t = editable ? diff_scale_t : 1.0f;
			diff_scale_t *= identical_planar_texgen ? 2.0f : 1.0f;

			norm_scale_t = editable ? norm_scale_t : 1.0f;
			norm_scale_t *= identical_planar_texgen ? 2.0f : 1.0f;

			spec_scale_t = editable ? spec_scale_t : 1.0f;
			spec_scale_t *= identical_planar_texgen ? 2.0f : 1.0f;

			BOOL diff_scale_tentative = !identical_diff_scale_t;
			BOOL norm_scale_tentative = !identical_norm_scale_t;
			BOOL spec_scale_tentative = !identical_spec_scale_t;

			getChildView("TexScaleV")->setEnabled(editable);
			getChildView("shinyScaleV")->setEnabled(editable && specmap_id.notNull());
			getChildView("bumpyScaleV")->setEnabled(editable && normmap_id.notNull());
// [SL:KB] - Patch: Build-ScaleParamFlip | Checked: Catznip-5.2
			getChildView("TexScaleVFlip")->setEnabled(editable);
			getChildView("shinyScaleVFlip")->setEnabled(editable && specmap_id.notNull());
			getChildView("bumpyScaleVFlip")->setEnabled(editable && normmap_id.notNull());
// [/SL:KB]

			if (force_set_values)
			{
				getChild<LLSpinCtrl>("TexScaleV")->forceSetValue(diff_scale_t);
			}
			else
			{
				getChild<LLSpinCtrl>("TexScaleV")->setValue(diff_scale_t);
			}
			getChild<LLUICtrl>("shinyScaleV")->setValue(norm_scale_t);
			getChild<LLUICtrl>("bumpyScaleV")->setValue(spec_scale_t);

			getChild<LLUICtrl>("TexScaleV")->setTentative(LLSD(diff_scale_tentative));
			getChild<LLUICtrl>("shinyScaleV")->setTentative(LLSD(norm_scale_tentative));
			getChild<LLUICtrl>("bumpyScaleV")->setTentative(LLSD(spec_scale_tentative));
		}

		// Texture offset
		{
			bool identical_diff_offset_s = false;
			bool identical_norm_offset_s = false;
			bool identical_spec_offset_s = false;

			F32 diff_offset_s = 0.0f;
			F32 norm_offset_s = 0.0f;
			F32 spec_offset_s = 0.0f;

			LLSelectedTE::getOffsetS(diff_offset_s, identical_diff_offset_s);
			LLSelectedTEMaterial::getNormalOffsetX(norm_offset_s, identical_norm_offset_s);
			LLSelectedTEMaterial::getSpecularOffsetX(spec_offset_s, identical_spec_offset_s);

			BOOL diff_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_diff_offset_s);
			BOOL norm_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_norm_offset_s);
			BOOL spec_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_spec_offset_s);

			getChild<LLUICtrl>("TexOffsetU")->setValue(  editable ? diff_offset_s : 0.0f);
			getChild<LLUICtrl>("bumpyOffsetU")->setValue(editable ? norm_offset_s : 0.0f);
			getChild<LLUICtrl>("shinyOffsetU")->setValue(editable ? spec_offset_s : 0.0f);

			getChild<LLUICtrl>("TexOffsetU")->setTentative(LLSD(diff_offset_u_tentative));
			getChild<LLUICtrl>("shinyOffsetU")->setTentative(LLSD(norm_offset_u_tentative));
			getChild<LLUICtrl>("bumpyOffsetU")->setTentative(LLSD(spec_offset_u_tentative));

			getChildView("TexOffsetU")->setEnabled(editable);
			getChildView("shinyOffsetU")->setEnabled(editable && specmap_id.notNull());
			getChildView("bumpyOffsetU")->setEnabled(editable && normmap_id.notNull());
		}

		{
			bool identical_diff_offset_t = false;
			bool identical_norm_offset_t = false;
			bool identical_spec_offset_t = false;

			F32 diff_offset_t = 0.0f;
			F32 norm_offset_t = 0.0f;
			F32 spec_offset_t = 0.0f;

			LLSelectedTE::getOffsetT(diff_offset_t, identical_diff_offset_t);
			LLSelectedTEMaterial::getNormalOffsetY(norm_offset_t, identical_norm_offset_t);
			LLSelectedTEMaterial::getSpecularOffsetY(spec_offset_t, identical_spec_offset_t);
			
			BOOL diff_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_diff_offset_t);
			BOOL norm_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_norm_offset_t);
			BOOL spec_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_spec_offset_t);

			getChild<LLUICtrl>("TexOffsetV")->setValue(  editable ? diff_offset_t : 0.0f);
			getChild<LLUICtrl>("bumpyOffsetV")->setValue(editable ? norm_offset_t : 0.0f);
			getChild<LLUICtrl>("shinyOffsetV")->setValue(editable ? spec_offset_t : 0.0f);

			getChild<LLUICtrl>("TexOffsetV")->setTentative(LLSD(diff_offset_v_tentative));
			getChild<LLUICtrl>("shinyOffsetV")->setTentative(LLSD(norm_offset_v_tentative));
			getChild<LLUICtrl>("bumpyOffsetV")->setTentative(LLSD(spec_offset_v_tentative));

			getChildView("TexOffsetV")->setEnabled(editable);
			getChildView("shinyOffsetV")->setEnabled(editable && specmap_id.notNull());
			getChildView("bumpyOffsetV")->setEnabled(editable && normmap_id.notNull());
		}

		// Texture rotation
		{
			bool identical_diff_rotation = false;
			bool identical_norm_rotation = false;
			bool identical_spec_rotation = false;

			F32 diff_rotation = 0.f;
			F32 norm_rotation = 0.f;
			F32 spec_rotation = 0.f;

			LLSelectedTE::getRotation(diff_rotation,identical_diff_rotation);
			LLSelectedTEMaterial::getSpecularRotation(spec_rotation,identical_spec_rotation);
			LLSelectedTEMaterial::getNormalRotation(norm_rotation,identical_norm_rotation);

			BOOL diff_rot_tentative = !(align_planar ? identical_planar_aligned : identical_diff_rotation);
			BOOL norm_rot_tentative = !(align_planar ? identical_planar_aligned : identical_norm_rotation);
			BOOL spec_rot_tentative = !(align_planar ? identical_planar_aligned : identical_spec_rotation);

			F32 diff_rot_deg = diff_rotation * RAD_TO_DEG;
			F32 norm_rot_deg = norm_rotation * RAD_TO_DEG;
			F32 spec_rot_deg = spec_rotation * RAD_TO_DEG;
			
			getChildView("TexRot")->setEnabled(editable);
			getChildView("shinyRot")->setEnabled(editable && specmap_id.notNull());
			getChildView("bumpyRot")->setEnabled(editable && normmap_id.notNull());

			getChild<LLUICtrl>("TexRot")->setTentative(diff_rot_tentative);
			getChild<LLUICtrl>("shinyRot")->setTentative(LLSD(norm_rot_tentative));
			getChild<LLUICtrl>("bumpyRot")->setTentative(LLSD(spec_rot_tentative));

			getChild<LLUICtrl>("TexRot")->setValue(  editable ? diff_rot_deg : 0.0f);			
			getChild<LLUICtrl>("shinyRot")->setValue(editable ? spec_rot_deg : 0.0f);
			getChild<LLUICtrl>("bumpyRot")->setValue(editable ? norm_rot_deg : 0.0f);
		}

		{
			F32 glow = 0.f;
			bool identical_glow = false;
			LLSelectedTE::getGlow(glow,identical_glow);
			getChild<LLUICtrl>("glow")->setValue(glow);
			getChild<LLUICtrl>("glow")->setTentative(!identical_glow);
			getChildView("glow")->setEnabled(editable);
			getChildView("glow label")->setEnabled(editable);
		}

		{
			LLCtrlSelectionInterface* combobox_texgen = childGetSelectionInterface("combobox texgen");
			if (combobox_texgen)
			{
				// Maps from enum to combobox entry index
				combobox_texgen->selectNthItem(((S32)selected_texgen) >> 1);
			}
			else
				{
				LL_WARNS() << "failed childGetSelectionInterface for 'combobox texgen'" << LL_ENDL;
				}

			getChildView("combobox texgen")->setEnabled(editable);
			getChild<LLUICtrl>("combobox texgen")->setTentative(!identical);
			getChildView("tex gen")->setEnabled(editable);

			}

		{
			U8 fullbright_flag = 0;
			bool identical_fullbright = false;
			
			LLSelectedTE::getFullbright(fullbright_flag,identical_fullbright);

			getChild<LLUICtrl>("checkbox fullbright")->setValue((S32)(fullbright_flag != 0));
			getChildView("checkbox fullbright")->setEnabled(editable);
			getChild<LLUICtrl>("checkbox fullbright")->setTentative(!identical_fullbright);
		}
		
		// Repeats per meter
		{
			F32 repeats_diff = 1.f;
			F32 repeats_norm = 1.f;
			F32 repeats_spec = 1.f;

			bool identical_diff_repeats = false;
			bool identical_norm_repeats = false;
			bool identical_spec_repeats = false;

			LLSelectedTE::getMaxDiffuseRepeats(repeats_diff, identical_diff_repeats);
			LLSelectedTEMaterial::getMaxNormalRepeats(repeats_norm, identical_norm_repeats);
			LLSelectedTEMaterial::getMaxSpecularRepeats(repeats_spec, identical_spec_repeats);			

			LLComboBox*	mComboTexGen = getChild<LLComboBox>("combobox texgen");
			if (mComboTexGen)
		{
				S32 index = mComboTexGen ? mComboTexGen->getCurrentIndex() : 0;
				BOOL enabled = editable && (index != 1);
				BOOL identical_repeats = true;
				F32  repeats = 1.0f;

// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
				U32 material_type = (combobox_matmedia->getCurrentIndex() == MATMEDIA_MATERIAL) ? combobox_mattype->getCurrentIndex() : MATTYPE_DIFFUSE;
// [/SL:KB]
//				U32 material_type = (combobox_matmedia->getCurrentIndex() == MATMEDIA_MATERIAL) ? radio_mat_type->getSelectedIndex() : MATTYPE_DIFFUSE;
				LLSelectMgr::getInstance()->setTextureChannel(LLRender::eTexIndex(material_type));

				switch (material_type)
			{
					default:
					case MATTYPE_DIFFUSE:
				{
						enabled = editable && !id.isNull();
						identical_repeats = identical_diff_repeats;
						repeats = repeats_diff;
				}
					break;

					case MATTYPE_SPECULAR:
			{
						enabled = (editable && ((shiny == SHINY_TEXTURE) && !specmap_id.isNull()));
						identical_repeats = identical_spec_repeats;
						repeats = repeats_spec;
			}
					break;

					case MATTYPE_NORMAL:
			{
						enabled = (editable && ((bumpy == BUMPY_TEXTURE) && !normmap_id.isNull()));
						identical_repeats = identical_norm_repeats;
						repeats = repeats_norm;
					}
					break;
				}

				BOOL repeats_tentative = !identical_repeats;

				getChildView("rptctrl")->setEnabled(identical_planar_texgen ? FALSE : enabled);
				LLSpinCtrl* rpt_ctrl = getChild<LLSpinCtrl>("rptctrl");
				if (force_set_values)
				{
					//onCommit, previosly edited element updates related ones
					rpt_ctrl->forceSetValue(editable ? repeats : 1.0f);
				}
				else
				{
					rpt_ctrl->setValue(editable ? repeats : 1.0f);
				}
				rpt_ctrl->setTentative(LLSD(repeats_tentative));
			}
		}

		// Materials
		{
			LLMaterialPtr material;
			LLSelectedTEMaterial::getCurrent(material, identical);

			if (material && editable)
			{
				LL_DEBUGS("Materials") << material->asLLSD() << LL_ENDL;

				// Alpha
				LLCtrlSelectionInterface* combobox_alphamode =
					childGetSelectionInterface("combobox alphamode");
				if (combobox_alphamode)
				{
					U32 alpha_mode = material->getDiffuseAlphaMode();

					if (transparency > 0.f)
					{ //it is invalid to have any alpha mode other than blend if transparency is greater than zero ... 
						alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
					}

					if (!mIsAlpha)
					{ // ... unless there is no alpha channel in the texture, in which case alpha mode MUST ebe none
						alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
				}

					combobox_alphamode->selectNthItem(alpha_mode);
			}
			else
			{
					LL_WARNS() << "failed childGetSelectionInterface for 'combobox alphamode'" << LL_ENDL;
			}
				getChild<LLUICtrl>("maskcutoff")->setValue(material->getAlphaMaskCutoff());
				updateAlphaControls();

				identical_planar_texgen = isIdenticalPlanarTexgen();

				// Shiny (specular)
				F32 offset_x, offset_y, repeat_x, repeat_y, rot;
				LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("shinytexture control");
				texture_ctrl->setImageAssetID(material->getSpecularID());

				if (!material->getSpecularID().isNull() && (shiny == SHINY_TEXTURE))
			{
					material->getSpecularOffset(offset_x,offset_y);
					material->getSpecularRepeat(repeat_x,repeat_y);

					if (identical_planar_texgen)
			{
						repeat_x *= 2.0f;
						repeat_y *= 2.0f;
			}

					rot = material->getSpecularRotation();
					getChild<LLUICtrl>("shinyScaleU")->setValue(repeat_x);
					getChild<LLUICtrl>("shinyScaleV")->setValue(repeat_y);
					getChild<LLUICtrl>("shinyRot")->setValue(rot*RAD_TO_DEG);
					getChild<LLUICtrl>("shinyOffsetU")->setValue(offset_x);
					getChild<LLUICtrl>("shinyOffsetV")->setValue(offset_y);
					getChild<LLUICtrl>("glossiness")->setValue(material->getSpecularLightExponent());
					getChild<LLUICtrl>("environment")->setValue(material->getEnvironmentIntensity());

					updateShinyControls(!material->getSpecularID().isNull(), true);
		}

				// Assert desired colorswatch color to match material AFTER updateShinyControls
				// to avoid getting overwritten with the default on some UI state changes.
				//
				if (!material->getSpecularID().isNull())
				{
					LLColorSwatchCtrl*	shiny_swatch = getChild<LLColorSwatchCtrl>("shinycolorswatch");
					LLColor4 new_color = material->getSpecularLightColor();
					LLColor4 old_color = shiny_swatch->get();

					shiny_swatch->setOriginal(new_color);
					shiny_swatch->set(new_color, force_set_values || old_color != new_color || !editable);
				}

				// Bumpy (normal)
				texture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
				texture_ctrl->setImageAssetID(material->getNormalID());

				if (!material->getNormalID().isNull())
				{
					material->getNormalOffset(offset_x,offset_y);
					material->getNormalRepeat(repeat_x,repeat_y);

					if (identical_planar_texgen)
					{
						repeat_x *= 2.0f;
						repeat_y *= 2.0f;
					}
			
					rot = material->getNormalRotation();
					getChild<LLUICtrl>("bumpyScaleU")->setValue(repeat_x);
					getChild<LLUICtrl>("bumpyScaleV")->setValue(repeat_y);
					getChild<LLUICtrl>("bumpyRot")->setValue(rot*RAD_TO_DEG);
					getChild<LLUICtrl>("bumpyOffsetU")->setValue(offset_x);
					getChild<LLUICtrl>("bumpyOffsetV")->setValue(offset_y);

					updateBumpyControls(!material->getNormalID().isNull(), true);
				}
			}
		}

		// Set variable values for numeric expressions
		LLCalc* calcp = LLCalc::getInstance();
		calcp->setVar(LLCalc::TEX_U_SCALE, childGetValue("TexScaleU").asReal());
		calcp->setVar(LLCalc::TEX_V_SCALE, childGetValue("TexScaleV").asReal());
		calcp->setVar(LLCalc::TEX_U_OFFSET, childGetValue("TexOffsetU").asReal());
		calcp->setVar(LLCalc::TEX_V_OFFSET, childGetValue("TexOffsetV").asReal());
		calcp->setVar(LLCalc::TEX_ROTATION, childGetValue("TexRot").asReal());
		calcp->setVar(LLCalc::TEX_TRANSPARENCY, childGetValue("ColorTrans").asReal());
		calcp->setVar(LLCalc::TEX_GLOW, childGetValue("glow").asReal());
	}
	else
	{
		// Disable all UICtrls
		clearCtrls();

		// Disable non-UICtrls
		LLTextureCtrl*	texture_ctrl = getChild<LLTextureCtrl>("texture control"); 
		if(texture_ctrl)
		{
			texture_ctrl->setImageAssetID( LLUUID::null );
			texture_ctrl->setEnabled( FALSE );  // this is a LLUICtrl, but we don't want it to have keyboard focus so we add it as a child, not a ctrl.
// 			texture_ctrl->setValid(FALSE);
		}
		LLColorSwatchCtrl* mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(mColorSwatch)
		{
			mColorSwatch->setEnabled( FALSE );			
			mColorSwatch->setFallbackImage(LLUI::getUIImage("locked_image.j2c") );
			mColorSwatch->setValid(FALSE);
		}
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-5.3
		LLComboBox* combobox_mattype = getChild<LLComboBox>("combobox mattype");
		if (combobox_mattype)
		{
			combobox_mattype->setCurrentByIndex(0);
		}
// [/SL:KB]
//		LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
//		if (radio_mat_type)
//		{
//			radio_mat_type->setSelectedIndex(0);
//		}
		getChildView("color trans")->setEnabled(FALSE);
		getChildView("rptctrl")->setEnabled(FALSE);
		getChildView("tex gen")->setEnabled(FALSE);
		getChildView("label shininess")->setEnabled(FALSE);
		getChildView("label bumpiness")->setEnabled(FALSE);
		getChildView("button align")->setEnabled(FALSE);
		//getChildView("has media")->setEnabled(FALSE);
		//getChildView("media info set")->setEnabled(FALSE);
		
		updateVisibility();

		// Set variable values for numeric expressions
		LLCalc* calcp = LLCalc::getInstance();
		calcp->clearVar(LLCalc::TEX_U_SCALE);
		calcp->clearVar(LLCalc::TEX_V_SCALE);
		calcp->clearVar(LLCalc::TEX_U_OFFSET);
		calcp->clearVar(LLCalc::TEX_V_OFFSET);
		calcp->clearVar(LLCalc::TEX_ROTATION);
		calcp->clearVar(LLCalc::TEX_TRANSPARENCY);
		calcp->clearVar(LLCalc::TEX_GLOW);		
	}
}


void LLPanelFace::refresh()
{
	LL_DEBUGS("Materials") << LL_ENDL;
	getState();
}

//
// Static functions
//

// static
F32 LLPanelFace::valueGlow(LLViewerObject* object, S32 face)
{
	return (F32)(object->getTE(face)->getGlow());
}


void LLPanelFace::onCommitColor(const LLSD& data)
{
	sendColor();
}

void LLPanelFace::onCommitShinyColor(const LLSD& data)
{
	LLSelectedTEMaterial::setSpecularLightColor(this, getChild<LLColorSwatchCtrl>("shinycolorswatch")->get());
}

void LLPanelFace::onCommitAlpha(const LLSD& data)
{
	sendAlpha();
}

void LLPanelFace::onCancelColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertColors();
}

void LLPanelFace::onCancelShinyColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertShinyColors();
}

void LLPanelFace::onSelectColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->saveSelectedObjectColors();
	sendColor();
}

void LLPanelFace::onSelectShinyColor(const LLSD& data)
{
	LLSelectedTEMaterial::setSpecularLightColor(this, getChild<LLColorSwatchCtrl>("shinycolorswatch")->get());
	LLSelectMgr::getInstance()->saveSelectedShinyColors();
}

// static
void LLPanelFace::onCommitMaterialsMedia(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	// Force to default states to side-step problems with menu contents
	// and generally reflecting old state when switching tabs or objects
	//
	self->updateShinyControls(false,true);
	self->updateBumpyControls(false,true);
	self->updateUI();
}

// static
void LLPanelFace::updateVisibility()
{	
	LLComboBox* combo_matmedia = getChild<LLComboBox>("combobox matmedia");
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	LLComboBox* combo_mattype = getChild<LLComboBox>("combobox mattype");
// [/SL:KB]
//	LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
	LLComboBox* combo_shininess = getChild<LLComboBox>("combobox shininess");
	LLComboBox* combo_bumpiness = getChild<LLComboBox>("combobox bumpiness");
//	if (!radio_mat_type || !combo_matmedia || !combo_shininess || !combo_bumpiness)
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	if (!combo_mattype || !combo_matmedia || !combo_shininess || !combo_bumpiness)
// [/SL:KB]
	{
		LL_WARNS("Materials") << "Combo box not found...exiting." << LL_ENDL;
		return;
	}
	U32 materials_media = combo_matmedia->getCurrentIndex();
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	U32 material_type = combo_mattype->getCurrentIndex();
// [/SL:KB]
//	U32 material_type = radio_mat_type->getSelectedIndex();
	bool show_media = (materials_media == MATMEDIA_MEDIA) && combo_matmedia->getEnabled();
	bool show_texture = (show_media || ((material_type == MATTYPE_DIFFUSE) && combo_matmedia->getEnabled()));
	bool show_bumpiness = (!show_media) && (material_type == MATTYPE_NORMAL) && combo_matmedia->getEnabled();
	bool show_shininess = (!show_media) && (material_type == MATTYPE_SPECULAR) && combo_matmedia->getEnabled();
//	getChildView("radio_material_type")->setVisible(!show_media);
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	getChildView("combobox mattype")->setVisible(!show_media);
// [/SL:KB]
// [SL:KB] - Patch: Build-TexturePipette | Checked: 2013-07-27 (Catznip-3.6)
	mBtnCopyMaterialTypeParams->setVisible(!show_media);
	mBtnPasteMaterialTypeParams->setVisible(!show_media);
	mBtnMaterialTypePipette->setVisible(!show_media);
// [/SL:KB]

	// Media controls
	getChildView("media_info")->setVisible(show_media);
	getChildView("add_media")->setVisible(show_media);
	getChildView("delete_media")->setVisible(show_media);
	getChildView("button align")->setVisible(show_media);

	// Diffuse texture controls
	getChildView("texture control")->setVisible(show_texture && !show_media);
// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
	mBtnTexturePipette->setVisible( (show_texture) && (!show_media) );
// [/SL:KB]
	getChildView("label alphamode")->setVisible(show_texture && !show_media);
	getChildView("combobox alphamode")->setVisible(show_texture && !show_media);
	getChildView("label maskcutoff")->setVisible(false);
	getChildView("maskcutoff")->setVisible(false);
	if (show_texture && !show_media)
	{
		updateAlphaControls();
	}
	getChildView("TexScaleU")->setVisible(show_texture);
	getChildView("TexScaleV")->setVisible(show_texture);
// [SL:KB] - Patch: Build-ScaleParamFlip | Checked: Catznip-5.2
	getChildView("TexScaleUFlip")->setVisible(show_texture);
	getChildView("TexScaleVFlip")->setVisible(show_texture);
// [/SL:KB]
	getChildView("TexRot")->setVisible(show_texture);
	getChildView("TexOffsetU")->setVisible(show_texture);
	getChildView("TexOffsetV")->setVisible(show_texture);

	// Specular map controls
	getChildView("shinytexture control")->setVisible(show_shininess);
	getChildView("combobox shininess")->setVisible(show_shininess);
	getChildView("label shininess")->setVisible(show_shininess);
	getChildView("label glossiness")->setVisible(false);
	getChildView("glossiness")->setVisible(false);
	getChildView("label environment")->setVisible(false);
	getChildView("environment")->setVisible(false);
	getChildView("label shinycolor")->setVisible(false);
	getChildView("shinycolorswatch")->setVisible(false);
	if (show_shininess)
	{
		updateShinyControls();
	}
	getChildView("shinyScaleU")->setVisible(show_shininess);
	getChildView("shinyScaleV")->setVisible(show_shininess);
// [SL:KB] - Patch: Build-ScaleParamFlip | Checked: Catznip-5.2
	getChildView("shinyScaleUFlip")->setVisible(show_shininess);
	getChildView("shinyScaleVFlip")->setVisible(show_shininess);
// [/SL:KB]
	getChildView("shinyRot")->setVisible(show_shininess);
	getChildView("shinyOffsetU")->setVisible(show_shininess);
	getChildView("shinyOffsetV")->setVisible(show_shininess);

	// Normal map controls
	if (show_bumpiness)
	{
		updateBumpyControls();
	}
	getChildView("bumpytexture control")->setVisible(show_bumpiness);
	getChildView("combobox bumpiness")->setVisible(show_bumpiness);
	getChildView("label bumpiness")->setVisible(show_bumpiness);
	getChildView("bumpyScaleU")->setVisible(show_bumpiness);
	getChildView("bumpyScaleV")->setVisible(show_bumpiness);
// [SL:KB] - Patch: Build-ScaleParamFlip | Checked: Catznip-5.2
	getChildView("bumpyScaleUFlip")->setVisible(show_bumpiness);
	getChildView("bumpyScaleVFlip")->setVisible(show_bumpiness);
// [/SL:KB]
	getChildView("bumpyRot")->setVisible(show_bumpiness);
	getChildView("bumpyOffsetU")->setVisible(show_bumpiness);
	getChildView("bumpyOffsetV")->setVisible(show_bumpiness);


}

// static
void LLPanelFace::onCommitMaterialType(LLUICtrl* ctrl, void* userdata)
{
    LLPanelFace* self = (LLPanelFace*) userdata;
	 // Force to default states to side-step problems with menu contents
	 // and generally reflecting old state when switching tabs or objects
	 //
	 self->updateShinyControls(false,true);
	 self->updateBumpyControls(false,true);
    self->updateUI();
}

// static
void LLPanelFace::onCommitBump(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;

	LLComboBox*	mComboBumpiness = self->getChild<LLComboBox>("combobox bumpiness");
	if(!mComboBumpiness)
		return;

	U32 bumpiness = mComboBumpiness->getCurrentIndex();

	self->sendBump(bumpiness);
}

// static
void LLPanelFace::onCommitTexGen(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTexGen();
}

// static
void LLPanelFace::updateShinyControls(bool is_setting_texture, bool mess_with_shiny_combobox)
{
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("shinytexture control");
	LLUUID shiny_texture_ID = texture_ctrl->getImageAssetID();
	LL_DEBUGS("Materials") << "Shiny texture selected: " << shiny_texture_ID << LL_ENDL;
	LLComboBox* comboShiny = getChild<LLComboBox>("combobox shininess");

	if(mess_with_shiny_combobox)
	{
		if (!comboShiny)
		{
			return;
		}
		if (!shiny_texture_ID.isNull() && is_setting_texture)
		{
			if (!comboShiny->itemExists(USE_TEXTURE))
			{
				comboShiny->add(USE_TEXTURE);
			}
			comboShiny->setSimple(USE_TEXTURE);
		}
		else
		{
			if (comboShiny->itemExists(USE_TEXTURE))
			{
				comboShiny->remove(SHINY_TEXTURE);
				comboShiny->selectFirstItem();
			}
		}
	}
	else
	{
		if (shiny_texture_ID.isNull() && comboShiny && comboShiny->itemExists(USE_TEXTURE))
		{
			comboShiny->remove(SHINY_TEXTURE);
			comboShiny->selectFirstItem();
		}
	}


	LLComboBox* combo_matmedia = getChild<LLComboBox>("combobox matmedia");
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	LLComboBox* combo_mattype = getChild<LLComboBox>("combobox mattype");
// [/SL:KB]
//	LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
	U32 materials_media = combo_matmedia->getCurrentIndex();
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	U32 material_type = combo_mattype->getCurrentIndex();
// [/SL:KB]
//	U32 material_type = radio_mat_type->getSelectedIndex();
	bool show_media = (materials_media == MATMEDIA_MEDIA) && combo_matmedia->getEnabled();
	bool show_shininess = (!show_media) && (material_type == MATTYPE_SPECULAR) && combo_matmedia->getEnabled();
	U32 shiny_value = comboShiny->getCurrentIndex();
	bool show_shinyctrls = (shiny_value == SHINY_TEXTURE) && show_shininess; // Use texture
	getChildView("label glossiness")->setVisible(show_shinyctrls);
	getChildView("glossiness")->setVisible(show_shinyctrls);
	getChildView("label environment")->setVisible(show_shinyctrls);
	getChildView("environment")->setVisible(show_shinyctrls);
	getChildView("label shinycolor")->setVisible(show_shinyctrls);
	getChildView("shinycolorswatch")->setVisible(show_shinyctrls);
}

// static
void LLPanelFace::updateBumpyControls(bool is_setting_texture, bool mess_with_combobox)
{
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
	LLUUID bumpy_texture_ID = texture_ctrl->getImageAssetID();
	LL_DEBUGS("Materials") << "texture: " << bumpy_texture_ID << (mess_with_combobox ? "" : " do not") << " update combobox" << LL_ENDL;
	LLComboBox* comboBumpy = getChild<LLComboBox>("combobox bumpiness");
	if (!comboBumpy)
	{
		return;
	}

	if (mess_with_combobox)
	{
		LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
		LLUUID bumpy_texture_ID = texture_ctrl->getImageAssetID();
		LL_DEBUGS("Materials") << "texture: " << bumpy_texture_ID << (mess_with_combobox ? "" : " do not") << " update combobox" << LL_ENDL;

		if (!bumpy_texture_ID.isNull() && is_setting_texture)
		{
			if (!comboBumpy->itemExists(USE_TEXTURE))
			{
				comboBumpy->add(USE_TEXTURE);
			}
			comboBumpy->setSimple(USE_TEXTURE);
		}
		else
		{
			if (comboBumpy->itemExists(USE_TEXTURE))
			{
				comboBumpy->remove(BUMPY_TEXTURE);
				comboBumpy->selectFirstItem();
			}
		}
	}
}

// static
void LLPanelFace::onCommitShiny(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;


	LLComboBox*	mComboShininess = self->getChild<LLComboBox>("combobox shininess");
	if(!mComboShininess)
		return;
	
	U32 shininess = mComboShininess->getCurrentIndex();

	self->sendShiny(shininess);
}

// static
void LLPanelFace::updateAlphaControls()
{
	LLComboBox* comboAlphaMode = getChild<LLComboBox>("combobox alphamode");
	if (!comboAlphaMode)
	{
		return;
	}
	U32 alpha_value = comboAlphaMode->getCurrentIndex();
	bool show_alphactrls = (alpha_value == ALPHAMODE_MASK); // Alpha masking
    
    LLComboBox* combobox_matmedia = getChild<LLComboBox>("combobox matmedia");
    U32 mat_media = MATMEDIA_MATERIAL;
    if (combobox_matmedia)
    {
        mat_media = combobox_matmedia->getCurrentIndex();
    }
    
	U32 mat_type = MATTYPE_DIFFUSE;
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	LLComboBox* combobox_mattype = getChild<LLComboBox>("combobox mattype");
	if (combobox_mattype)
	{
		mat_type = combobox_mattype->getCurrentIndex();
	}
// [/SL:KB]
//    LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
//    if(radio_mat_type)
//    {
//        mat_type = radio_mat_type->getSelectedIndex();
//    }

    show_alphactrls = show_alphactrls && (mat_media == MATMEDIA_MATERIAL);
    show_alphactrls = show_alphactrls && (mat_type == MATTYPE_DIFFUSE);
    
	getChildView("label maskcutoff")->setVisible(show_alphactrls);
	getChildView("maskcutoff")->setVisible(show_alphactrls);
}

// static
void LLPanelFace::onCommitAlphaMode(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->updateAlphaControls();
	LLSelectedTEMaterial::setDiffuseAlphaMode(self,self->getCurrentDiffuseAlphaMode());
}

// static
void LLPanelFace::onCommitFullbright(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendFullbright();
}

// static
void LLPanelFace::onCommitGlow(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendGlow();
}

// static
BOOL LLPanelFace::onDragTexture(LLUICtrl*, LLInventoryItem* item)
{
	BOOL accept = TRUE;
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* obj = node->getObject();
		if(!LLToolDragAndDrop::isInventoryDropAcceptable(obj, item))
		{
			accept = FALSE;
			break;
		}
	}
	return accept;
}

void LLPanelFace::onCommitTexture( const LLSD& data )
{
	add(LLStatViewer::EDIT_TEXTURE, 1);
	sendTexture();
}

void LLPanelFace::onCancelTexture(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertTextures();
}

void LLPanelFace::onSelectTexture(const LLSD& data)
{
	LLSelectMgr::getInstance()->saveSelectedObjectTextures();
	sendTexture();

	LLGLenum image_format;
	bool identical_image_format = false;
	LLSelectedTE::getImageFormat(image_format, identical_image_format);
    
	LLCtrlSelectionInterface* combobox_alphamode =
		childGetSelectionInterface("combobox alphamode");

	U32 alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
	if (combobox_alphamode)
	{
		switch (image_format)
		{
		case GL_RGBA:
		case GL_ALPHA:
			{
				alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
			}
			break;

		case GL_RGB: break;
		default:
			{
				LL_WARNS() << "Unexpected tex format in LLPanelFace...resorting to no alpha" << LL_ENDL;
			}
			break;
		}

		combobox_alphamode->selectNthItem(alpha_mode);
	}
	LLSelectedTEMaterial::setDiffuseAlphaMode(this, getCurrentDiffuseAlphaMode());
}

void LLPanelFace::onCloseTexturePicker(const LLSD& data)
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	updateUI();
}

void LLPanelFace::onCommitSpecularTexture( const LLSD& data )
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	sendShiny(SHINY_TEXTURE);
}

void LLPanelFace::onCommitNormalTexture( const LLSD& data )
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	LLUUID nmap_id = getCurrentNormalMap();
	sendBump(nmap_id.isNull() ? 0 : BUMPY_TEXTURE);
}

// [SL:KB] - Patch: Build-TexturePipette | Checked: 2012-09-11 (Catznip-3.3)
struct LLSelectedLLSDFunctorHelper
{
	static U8 getDefaultDiffuseMode(const LLViewerObject* pObj, S32 idxTE)
	{
		const LLViewerTexture* pImg = pObj->getTEImage(idxTE);
		LLGLenum formatImg = (pImg) ? pImg->getPrimaryFormat() : GL_RGB;

		if ( (GL_RGBA == formatImg) || (GL_ALPHA == formatImg) )
			return LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
		return LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
	}

	static bool getNeedsMaterial(const LLViewerObject* pObj, S32 idxTE, const LLMaterialPtr& material)
	{
		return 
			(material.notNull()) &&
			( (material->getDiffuseAlphaMode() != getDefaultDiffuseMode(pObj, idxTE)) || (material->getNormalID().notNull()) || (material->getSpecularID().notNull()) );
	}
};

struct LLSelectedTEGetLLSDFunctor : public LLSelectedTEFunctor, public LLSelectedLLSDFunctorHelper
{
	LLSelectedTEGetLLSDFunctor(bool fIdentical) : LLSelectedTEFunctor(), m_fIdentical(fIdentical) {}
	const LLSD& getParams() const { return m_sdParams; }

	/*virtual*/ bool apply(LLViewerObject* pObj, S32 idxTE)
	{
		const LLTextureEntry* pTE = pObj->getTE(idxTE);
		if (pTE)
		{
			LLMaterialPtr pMaterial = pTE->getMaterialParams();

			LLSD& sdFaceParams = m_sdParams[(m_fIdentical) ? 0 : idxTE];
			applyFunc(sdFaceParams, pObj, idxTE, pTE, pMaterial);
		}
		return true;
	}

	static void setCommonParams(LLSD& sdFaceParams, const LLUUID& idTexture, F32 nScaleS, F32 nScaleT, F32 nRotation, F32 nOffsetS, F32 nOffsetT)
	{
		if (find_item_from_asset(idTexture, true, false) != LLUUID::null)
			sdFaceParams["texture_id"] = idTexture;
		else
			sdFaceParams["texture_id"] = LL_DEFAULT_WOOD_UUID;
		sdFaceParams["scale_s"] = nScaleS;
		sdFaceParams["scale_t"] = nScaleT;
		sdFaceParams["rotation"] = nRotation;
		sdFaceParams["offset_s"] = nOffsetS;
		sdFaceParams["offset_t"] = nOffsetT;
	}

protected:
	virtual void applyFunc(LLSD& sdFaceParams, const LLViewerObject* pObj, U8 idxTe, const LLTextureEntry* pTE, LLMaterialPtr& pMaterial) = 0;

	bool m_fIdentical;
	LLSD m_sdParams;
};

struct LLSelectedDiffuseTEGetLLSDFunctor : public LLSelectedTEGetLLSDFunctor
{
	LLSelectedDiffuseTEGetLLSDFunctor(bool fIdentical) : LLSelectedTEGetLLSDFunctor(fIdentical) {}
protected:
	/*virtual*/ void applyFunc(LLSD& sdFaceParams, const LLViewerObject* pObj, U8 idxTe, const LLTextureEntry* pTE, LLMaterialPtr& pMaterial)
	{
		bool fHasDiffuse = (pMaterial) && (pMaterial->getDiffuseAlphaMode() != getDefaultDiffuseMode(pObj, idxTe));
		if (fHasDiffuse)
		{
			sdFaceParams["alphamode"] = pMaterial->getDiffuseAlphaMode();
			sdFaceParams["alphacutoff"] = pMaterial->getAlphaMaskCutoff();
		}
		sdFaceParams["has_diffuse"] = fHasDiffuse;

		setCommonParams(sdFaceParams, pTE->getID(), pTE->getScaleS(), pTE->getScaleT(), pTE->getRotation(), pTE->getOffsetS(), pTE->getOffsetT());
		sdFaceParams["texgen"] = (U8)pTE->getTexGen();
	}
};

struct LLSelectedNormalTEGetLLSDFunctor : public LLSelectedTEGetLLSDFunctor
{
	LLSelectedNormalTEGetLLSDFunctor(bool fIdentical) : LLSelectedTEGetLLSDFunctor(fIdentical) {}
protected:
	/*virtual*/ void applyFunc(LLSD& sdFaceParams, const LLViewerObject* pObj, U8 idxTe, const LLTextureEntry* pTE, LLMaterialPtr& pMaterial)
	{
		bool fHasNormal = (pMaterial) && (pMaterial->getNormalID().notNull());
		if (fHasNormal)
			setCommonParams(sdFaceParams, pMaterial->getNormalID(), pMaterial->getNormalRepeatX(), pMaterial->getNormalRepeatY(), pMaterial->getNormalRotation(), pMaterial->getNormalOffsetX(), pMaterial->getNormalOffsetY());
		else
			sdFaceParams["bumpmap"] = pTE->getBumpmap();
		sdFaceParams["has_normal"] = fHasNormal;
	}
};

struct LLSelectedSpecularTEGetLLSDFunctor : public LLSelectedTEGetLLSDFunctor
{
	LLSelectedSpecularTEGetLLSDFunctor(bool fIdentical) : LLSelectedTEGetLLSDFunctor(fIdentical){}
protected:
	/*virtual*/ void applyFunc(LLSD& sdFaceParams, const LLViewerObject* pObj, U8 idxTe, const LLTextureEntry* pTE, LLMaterialPtr& pMaterial)
	{
		bool fHasSpecular = (pMaterial) && (pMaterial->getSpecularID().notNull());
		if (fHasSpecular)
		{
			setCommonParams(sdFaceParams, pMaterial->getSpecularID(), pMaterial->getSpecularRepeatX(), pMaterial->getSpecularRepeatY(), pMaterial->getSpecularRotation(), pMaterial->getSpecularOffsetX(), pMaterial->getSpecularOffsetY());

			sdFaceParams["color"] = ll_sd_from_color4(pMaterial->getSpecularLightColor());
			sdFaceParams["exponent"] = pMaterial->getSpecularLightExponent();
			sdFaceParams["environment"] = pMaterial->getEnvironmentIntensity();
		}
		else
		{
			sdFaceParams["shiny"] = pTE->getShiny();
		}
		sdFaceParams["has_specular"] = fHasSpecular;
	}
};

LLSD LLPanelFace::objectToLLSD(const std::string& strParamType)
{
	// Allowed use cases:
	//   * selection of multiple faces on multiple objects but they all have identical values => supported (1 face)
	//   * selection of multiple faces on the same object and some/all have different values => supported (save all selected faces, paste on identical faces)
	//   * selection of multiple faces on multiple objects and some/all have different values => unsupported
	bool fIdentical = false;	// The functions won't really tell us if all params are identical, but it will do for now

	LLSelectedTEGetLLSDFunctor* pFunc = NULL;
	if ("diffuse" == strParamType)
	{
		LLUUID idTexture;
		LLSelectedTE::getTexId(idTexture, fIdentical);
		pFunc = new LLSelectedDiffuseTEGetLLSDFunctor(fIdentical);
	}
	else if ("normal" == strParamType)
	{
		LLUUID idNormal;
		LLSelectedTEMaterial::getSpecularID(idNormal, fIdentical);
		pFunc = new LLSelectedNormalTEGetLLSDFunctor(fIdentical);
	}
	else if ("specular" == strParamType)
	{
		LLUUID idSpecular;
		LLSelectedTEMaterial::getSpecularID(idSpecular, fIdentical);
		pFunc = new LLSelectedSpecularTEGetLLSDFunctor(fIdentical);
	}

	LLObjectSelectionHandle hSel = LLSelectMgr::getInstance()->getSelection();
	if ( (hSel.notNull()) && (pFunc) && ((fIdentical) || (hSel->getNumNodes() == 1)) )
	{
		hSel->applyToTEs(pFunc, fIdentical);
		LLSD sdParams = pFunc->getParams();
		delete pFunc;
		return sdParams;
	}
	return LLSD();
}

void LLPanelFace::onClickBtnCopyParams(const LLSD& sdParam)
{
	std::string strParamType = sdParam.asString();

	LLButton* pButton = NULL;
	if ("mattype" == strParamType)
	{
		const LLComboBox* pComboMatType = findChild<LLComboBox>("combobox mattype");
		if (pComboMatType)
		{
			strParamType = pComboMatType->getValue().asString();
			pButton = mBtnPasteMaterialTypeParams;
		}
	}

	if (pButton)
	{
		mObjectClipboard[strParamType] = objectToLLSD(strParamType);
		if (!pButton->getEnabled())
			refresh();
	}
}

struct LLSelectedTEFromLLSDFunctor : public LLSelectedTEMaterialFunctor, public LLSelectedLLSDFunctorHelper
{
	LLSelectedTEFromLLSDFunctor(const LLSD& sdParams) : m_sdParams(sdParams) {}

	/*virtual*/ LLMaterialPtr apply(LLViewerObject* pObj, S32 idxTE, LLTextureEntry* pTE, LLMaterialPtr& material)
	{
		if ( (m_sdParams.size() > 1) && ((idxTE >= m_sdParams.size()) || (m_sdParams[idxTE].isUndefined())) )
		{
			// We don't have any information about this face
			return material;
		}

		LLMaterialPtr pMaterial( (!material.isNull()) ? new LLMaterial(material->asLLSD()) : new LLMaterial());
		if (material.isNull())
		{
			pMaterial->setDiffuseAlphaMode(getDefaultDiffuseMode(pObj, idxTE));
		}

		const LLSD& sdFaceParams = m_sdParams[(1 == m_sdParams.size()) ? 0 : idxTE];
		applyParams(sdFaceParams, pObj, idxTE, pTE, pMaterial);

		if (getNeedsMaterial(pObj, idxTE, pMaterial))
		{
			LLMaterialMgr::getInstance()->put(pObj->getID(), idxTE, *pMaterial);
		}
		else
		{
			LLMaterialMgr::getInstance()->remove(pObj->getID(), idxTE);
			pMaterial = NULL;
		}
		pObj->setTEMaterialParams(idxTE, pMaterial);

		return pMaterial;
	}

	virtual void applyParams(const LLSD& sdFaceParams, LLViewerObject* pObj, S32 idxTE, LLTextureEntry* pTE, LLMaterialPtr& pMaterial) = 0;

protected:
	LLSD m_sdParams;
};

struct LLSelectedDiffuseTEFromLLSDFunctor : public LLSelectedTEFromLLSDFunctor
{
	LLSelectedDiffuseTEFromLLSDFunctor(const LLSD& sdDiffuseParams) : LLSelectedTEFromLLSDFunctor(sdDiffuseParams) {}

	/*virtual*/ void applyParams(const LLSD& sdFaceParams, LLViewerObject* pObj, S32 idxTE, LLTextureEntry* pTE, LLMaterialPtr& pMaterial)
	{
		pObj->setTETexture(idxTE, sdFaceParams["texture_id"].asUUID());
		pObj->setTETexGen(idxTE, sdFaceParams["texgen"].asInteger() & TEM_TEX_GEN_MASK);
		pObj->setTEScale(idxTE, sdFaceParams["scale_s"].asReal(), sdFaceParams["scale_t"].asReal());
		pObj->setTERotation(idxTE, sdFaceParams["rotation"].asReal());
		pObj->setTEOffset(idxTE, sdFaceParams["offset_s"].asReal(), sdFaceParams["offset_t"].asReal());

		bool fHasDiffuse = sdFaceParams["has_diffuse"].asBoolean();
		if (fHasDiffuse)
		{
			pMaterial->setDiffuseAlphaMode(sdFaceParams["alphamode"].asInteger());
			pMaterial->setAlphaMaskCutoff(sdFaceParams["alphacutoff"].asInteger());
		}
		else
		{
			pMaterial->setDiffuseAlphaMode(getDefaultDiffuseMode(pObj, idxTE));
		}
	}
};

struct LLSelectedNormalTEFromLLSDFunctor : public LLSelectedTEFromLLSDFunctor
{
	LLSelectedNormalTEFromLLSDFunctor(const LLSD& sdNormalParams) : LLSelectedTEFromLLSDFunctor(sdNormalParams) {}

	/*virtual*/ void applyParams(const LLSD& sdFaceParams, LLViewerObject* pObj, S32 idxTE, LLTextureEntry* pTE, LLMaterialPtr& pMaterial)
	{
		bool fHasNormal = sdFaceParams["has_normal"].asBoolean();
		if (fHasNormal)
		{
			pMaterial->setNormalID(sdFaceParams["texture_id"].asUUID());
			pMaterial->setNormalRepeat(sdFaceParams["scale_s"].asReal(), sdFaceParams["scale_t"].asReal());
			pMaterial->setNormalRotation(sdFaceParams["rotation"].asReal());
			pMaterial->setNormalOffset(sdFaceParams["offset_s"].asReal(), sdFaceParams["offset_t"].asReal());
		}
		else
		{
			pMaterial->setNormalID(LLUUID::null);
			pTE->setBumpmap(sdFaceParams["bumpmap"].asInteger());
		}
	}
};

struct LLSelectedSpecularTEFromLLSDFunctor : public LLSelectedTEFromLLSDFunctor
{
	LLSelectedSpecularTEFromLLSDFunctor(const LLSD& sdSpecularParams) : LLSelectedTEFromLLSDFunctor(sdSpecularParams) {}

	/*virtual*/ void applyParams(const LLSD& sdFaceParams, LLViewerObject* pObj, S32 idxTE, LLTextureEntry* pTE, LLMaterialPtr& pMaterial)
	{
		bool fHasSpecular = sdFaceParams["has_specular"].asBoolean();
		if (fHasSpecular)
		{
			pMaterial->setSpecularID(sdFaceParams["texture_id"].asUUID());
			pMaterial->setSpecularRepeat(sdFaceParams["scale_s"].asReal(), sdFaceParams["scale_t"].asReal());
			pMaterial->setSpecularRotation(sdFaceParams["rotation"].asReal());
			pMaterial->setSpecularOffset(sdFaceParams["offset_s"].asReal(), sdFaceParams["offset_t"].asReal());

			pMaterial->setSpecularLightColor(ll_color4_from_sd(sdFaceParams["color"]));
			pMaterial->setSpecularLightExponent(sdFaceParams["exponent"].asInteger());
			pMaterial->setEnvironmentIntensity(sdFaceParams["environment"].asInteger());
		}
		else
		{
			pMaterial->setSpecularID(LLUUID::null);
			pTE->setShiny(sdFaceParams["shiny"].asInteger());
		}
	}
};

void LLPanelFace::objectFromLLSD(const std::string& strParamType, const LLSD& sdParams)
{
	if (!sdParams.has(strParamType))
		return;

	LLSelectedTEFromLLSDFunctor* pFunc = NULL;
	if ("diffuse" == strParamType)
		pFunc = new LLSelectedDiffuseTEFromLLSDFunctor(sdParams[strParamType]);
	else if ("normal" == strParamType)
		pFunc = new LLSelectedNormalTEFromLLSDFunctor(sdParams[strParamType]);
	else if ("specular" == strParamType)
		pFunc = new LLSelectedSpecularTEFromLLSDFunctor(sdParams[strParamType]);

	if (pFunc)
	{
		LLSelectMgr::getInstance()->selectionSetMaterialParams(pFunc);
		delete pFunc;
	}
}

void LLPanelFace::onClickBtnPasteParams(const LLSD& sdParam)
{
	std::string strParamType = sdParam.asString();
	if ("mattype" == strParamType)
	{
		const LLComboBox* pComboMatType = findChild<LLComboBox>("combobox mattype");
		if (pComboMatType)
			strParamType = pComboMatType->getValue().asString();
	}
	objectFromLLSD(strParamType, mObjectClipboard);
}

void LLPanelFace::onClickPipette(LLUICtrl* pCtrl, LLToolPipette::EType typePipette)
{
	LLButton* pPipetteBtn = dynamic_cast<LLButton*>(pCtrl);
	if (pCtrl)
	{
		if (!pPipetteBtn->getToggleState())
		{
			LLToolMgr::getInstance()->setTransientTool(LLToolPipette::getInstance());
			LLToolPipette::getInstance()->setPippetType(typePipette);
		}
		else
		{
			LLToolMgr::getInstance()->clearTransientTool();
		}
	}
}

void LLPanelFace::onSelectPipette(LLToolPipette::EType typePipette, LLViewerObject* pObj, const LLTextureEntry& te)
{
	switch (typePipette)
	{
		case LLToolPipette::TYPE_TEXTURE:
			{
				LLTextureCtrl* pTextureCtrl = findChild<LLTextureCtrl>("texture control");
				if ( (pTextureCtrl) && (!pTextureCtrl->getPickerVisible()) )
				{
					const LLUUID& idItem = find_item_from_asset(te.getID(), true, false);
					if (idItem.notNull())
					{
						pTextureCtrl->setImageItemID(idItem);
						onSelectTexture(LLSD());
					}
				}
			}
			break;
		case LLToolPipette::TYPE_COLOR:
			{
				LLColorSwatchCtrl* pColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
				if (!pColorSwatch->getPickerVisible())
				{
					pColorSwatch->set(LLColor4(te.getColor().mV[VRED], te.getColor().mV[VGREEN], te.getColor().mV[VBLUE], 1.0));
					onSelectColor(LLSD());
				}
			}
			break;
		case LLToolPipette::TYPE_MATERIAL_TYPE:
			{
				const LLComboBox* pComboMatType = findChild<LLComboBox>("combobox mattype");
				if (pComboMatType)
				{
					const std::string strParamType = pComboMatType->getValue().asString();

					LLSelectedTEGetLLSDFunctor* pFunc = NULL;
					if ("diffuse" == strParamType)
						pFunc = new LLSelectedDiffuseTEGetLLSDFunctor(false);
					else if ("normal" == strParamType)
						pFunc = new LLSelectedNormalTEGetLLSDFunctor(false);
					else if ("specular" == strParamType)
						pFunc = new LLSelectedSpecularTEGetLLSDFunctor(false);

					if (pFunc)
					{
						for (int idxTE = 0, cntTE = llmin((int)pObj->getNumTEs(), (int)pObj->getNumFaces()); idxTE < cntTE; idxTE++)
							pFunc->apply(pObj, idxTE);

						LLSD sdParams;
						sdParams[strParamType] = pFunc->getParams();
						objectFromLLSD(strParamType, sdParams);
					}
				}
			}
			break;
		default:
			break;
	}
}
// [/SL:KB]

void LLPanelFace::onCancelSpecularTexture(const LLSD& data)
{
	U8 shiny = 0;
	bool identical_shiny = false;
	LLSelectedTE::getShiny(shiny, identical_shiny);
	LLUUID spec_map_id = getChild<LLTextureCtrl>("shinytexture control")->getImageAssetID();
	shiny = spec_map_id.isNull() ? shiny : SHINY_TEXTURE;
	sendShiny(shiny);
}

void LLPanelFace::onCancelNormalTexture(const LLSD& data)
{
	U8 bumpy = 0;
	bool identical_bumpy = false;
	LLSelectedTE::getBumpmap(bumpy, identical_bumpy);
	LLUUID spec_map_id = getChild<LLTextureCtrl>("bumpytexture control")->getImageAssetID();
	bumpy = spec_map_id.isNull() ? bumpy : BUMPY_TEXTURE;
	sendBump(bumpy);
}

void LLPanelFace::onSelectSpecularTexture(const LLSD& data)
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	sendShiny(SHINY_TEXTURE);
}

void LLPanelFace::onSelectNormalTexture(const LLSD& data)
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	LLUUID nmap_id = getCurrentNormalMap();
	sendBump(nmap_id.isNull() ? 0 : BUMPY_TEXTURE);
}

//static
void LLPanelFace::syncOffsetX(LLPanelFace* self, F32 offsetU)
{
	LLSelectedTEMaterial::setNormalOffsetX(self,offsetU);
	LLSelectedTEMaterial::setSpecularOffsetX(self,offsetU);
	self->getChild<LLSpinCtrl>("TexOffsetU")->forceSetValue(offsetU);
	self->sendTextureInfo();
}

//static
void LLPanelFace::syncOffsetY(LLPanelFace* self, F32 offsetV)
{
	LLSelectedTEMaterial::setNormalOffsetY(self,offsetV);
	LLSelectedTEMaterial::setSpecularOffsetY(self,offsetV);
	self->getChild<LLSpinCtrl>("TexOffsetV")->forceSetValue(offsetV);
	self->sendTextureInfo();
}

//static
void LLPanelFace::onCommitMaterialBumpyOffsetX(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetX(self,self->getCurrentBumpyOffsetU());
	}
	else
	{
		LLSelectedTEMaterial::setNormalOffsetX(self,self->getCurrentBumpyOffsetU());
	}

}

//static
void LLPanelFace::onCommitMaterialBumpyOffsetY(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetY(self,self->getCurrentBumpyOffsetV());
	}
	else
	{
		LLSelectedTEMaterial::setNormalOffsetY(self,self->getCurrentBumpyOffsetV());
	}
}

//static
void LLPanelFace::onCommitMaterialShinyOffsetX(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetX(self, self->getCurrentShinyOffsetU());
	}
	else
	{
		LLSelectedTEMaterial::setSpecularOffsetX(self,self->getCurrentShinyOffsetU());
	}
}

//static
void LLPanelFace::onCommitMaterialShinyOffsetY(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetY(self,self->getCurrentShinyOffsetV());
	}
	else
	{
		LLSelectedTEMaterial::setSpecularOffsetY(self,self->getCurrentShinyOffsetV());
	}
}

//static
void LLPanelFace::syncRepeatX(LLPanelFace* self, F32 scaleU)
{
	LLSelectedTEMaterial::setNormalRepeatX(self,scaleU);
	LLSelectedTEMaterial::setSpecularRepeatX(self,scaleU);
	self->sendTextureInfo();
}

//static
void LLPanelFace::syncRepeatY(LLPanelFace* self, F32 scaleV)
{
	LLSelectedTEMaterial::setNormalRepeatY(self,scaleV);
	LLSelectedTEMaterial::setSpecularRepeatY(self,scaleV);
	self->sendTextureInfo();
}

//static
void LLPanelFace::onCommitMaterialBumpyScaleX(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	F32 bumpy_scale_u = self->getCurrentBumpyScaleU();
	if (self->isIdenticalPlanarTexgen())
	{
		bumpy_scale_u *= 0.5f;
	}

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexScaleU")->forceSetValue(self->getCurrentBumpyScaleU());
		syncRepeatX(self, bumpy_scale_u);
	}
	else
	{
		LLSelectedTEMaterial::setNormalRepeatX(self,bumpy_scale_u);
	}
}

//static
void LLPanelFace::onCommitMaterialBumpyScaleY(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	F32 bumpy_scale_v = self->getCurrentBumpyScaleV();
	if (self->isIdenticalPlanarTexgen())
	{
		bumpy_scale_v *= 0.5f;
	}


	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexScaleV")->forceSetValue(self->getCurrentBumpyScaleV());
		syncRepeatY(self, bumpy_scale_v);
	}
	else
	{
		LLSelectedTEMaterial::setNormalRepeatY(self,bumpy_scale_v);
	}
}

//static
void LLPanelFace::onCommitMaterialShinyScaleX(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	F32 shiny_scale_u = self->getCurrentShinyScaleU();
	if (self->isIdenticalPlanarTexgen())
	{
		shiny_scale_u *= 0.5f;
	}

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexScaleU")->forceSetValue(self->getCurrentShinyScaleU());
		syncRepeatX(self, shiny_scale_u);
	}
	else
	{
		LLSelectedTEMaterial::setSpecularRepeatX(self,shiny_scale_u);
	}
}

//static
void LLPanelFace::onCommitMaterialShinyScaleY(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	F32 shiny_scale_v = self->getCurrentShinyScaleV();
	if (self->isIdenticalPlanarTexgen())
	{
		shiny_scale_v *= 0.5f;
	}

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexScaleV")->forceSetValue(self->getCurrentShinyScaleV());
		syncRepeatY(self, shiny_scale_v);
	}
	else
	{
		LLSelectedTEMaterial::setSpecularRepeatY(self,shiny_scale_v);
	}
}

//static
void LLPanelFace::syncMaterialRot(LLPanelFace* self, F32 rot, int te)
{
	LLSelectedTEMaterial::setNormalRotation(self,rot * DEG_TO_RAD, te);
	LLSelectedTEMaterial::setSpecularRotation(self,rot * DEG_TO_RAD, te);
	self->sendTextureInfo();
}

//static
void LLPanelFace::onCommitMaterialBumpyRot(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexRot")->forceSetValue(self->getCurrentBumpyRot());
		syncMaterialRot(self, self->getCurrentBumpyRot());
	}
	else
	{
        if ((bool)self->childGetValue("checkbox planar align").asBoolean())
        {
            LLFace* last_face = NULL;
            bool identical_face = false;
            LLSelectedTE::getFace(last_face, identical_face);
            LLPanelFaceSetAlignedTEFunctor setfunc(self, last_face);
            LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
        }
        else
        {
            LLSelectedTEMaterial::setNormalRotation(self, self->getCurrentBumpyRot() * DEG_TO_RAD);
        }
	}
}

//static
void LLPanelFace::onCommitMaterialShinyRot(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexRot")->forceSetValue(self->getCurrentShinyRot());
		syncMaterialRot(self, self->getCurrentShinyRot());
	}
	else
	{
        if ((bool)self->childGetValue("checkbox planar align").asBoolean())
        {
            LLFace* last_face = NULL;
            bool identical_face = false;
            LLSelectedTE::getFace(last_face, identical_face);
            LLPanelFaceSetAlignedTEFunctor setfunc(self, last_face);
            LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
        }
        else
        {
            LLSelectedTEMaterial::setSpecularRotation(self, self->getCurrentShinyRot() * DEG_TO_RAD);
        }
	}
}

//static
void LLPanelFace::onCommitMaterialGloss(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	LLSelectedTEMaterial::setSpecularLightExponent(self,self->getCurrentGlossiness());
}

//static
void LLPanelFace::onCommitMaterialEnv(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	LLSelectedTEMaterial::setEnvironmentIntensity(self,self->getCurrentEnvIntensity());
}

//static
void LLPanelFace::onCommitMaterialMaskCutoff(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	LLSelectedTEMaterial::setAlphaMaskCutoff(self,self->getCurrentAlphaMaskCutoff());
}

// static
//void LLPanelFace::onCommitTextureInfo( LLUICtrl* ctrl, void* userdata )
//{
//	LLPanelFace* self = (LLPanelFace*) userdata;
//	self->sendTextureInfo();
//	// vertical scale and repeats per meter depends on each other, so force set on changes
//	self->updateUI(true);
//}

// static
void LLPanelFace::onCommitTextureScaleX( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		F32 bumpy_scale_u = self->getChild<LLUICtrl>("TexScaleU")->getValue().asReal();
		if (self->isIdenticalPlanarTexgen())
		{
			bumpy_scale_u *= 0.5f;
		}
		syncRepeatX(self, bumpy_scale_u);
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// static
void LLPanelFace::onCommitTextureScaleY( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		F32 bumpy_scale_v = self->getChild<LLUICtrl>("TexScaleV")->getValue().asReal();
		if (self->isIdenticalPlanarTexgen())
		{
			bumpy_scale_v *= 0.5f;
		}
		syncRepeatY(self, bumpy_scale_v);
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// static
void LLPanelFace::onCommitTextureRot( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncMaterialRot(self, self->getChild<LLUICtrl>("TexRot")->getValue().asReal());
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// static
void LLPanelFace::onCommitTextureOffsetX( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetX(self, self->getChild<LLUICtrl>("TexOffsetU")->getValue().asReal());
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// static
void LLPanelFace::onCommitTextureOffsetY( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetY(self, self->getChild<LLUICtrl>("TexOffsetV")->getValue().asReal());
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// Commit the number of repeats per meter
// static
void LLPanelFace::onCommitRepeatsPerMeter(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	
	LLUICtrl*	repeats_ctrl	= self->getChild<LLUICtrl>("rptctrl");
	LLComboBox* combo_matmedia = self->getChild<LLComboBox>("combobox matmedia");
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	LLComboBox* combo_mattype	= self->getChild<LLComboBox>("combobox mattype");
// [/SL:KB]
//	LLRadioGroup* radio_mat_type = self->getChild<LLRadioGroup>("radio_material_type");
	
	U32 materials_media = combo_matmedia->getCurrentIndex();

// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	U32 material_type			= (materials_media == MATMEDIA_MATERIAL) ? combo_mattype->getCurrentIndex() : 0;
// [/SL:KB]
//	U32 material_type           = (materials_media == MATMEDIA_MATERIAL) ? radio_mat_type->getSelectedIndex() : 0;
	F32 repeats_per_meter	= repeats_ctrl->getValue().asReal();
	
   F32 obj_scale_s = 1.0f;
   F32 obj_scale_t = 1.0f;

	bool identical_scale_s = false;
	bool identical_scale_t = false;

	LLSelectedTE::getObjectScaleS(obj_scale_s, identical_scale_s);
	LLSelectedTE::getObjectScaleS(obj_scale_t, identical_scale_t);

	LLUICtrl* bumpy_scale_u = self->getChild<LLUICtrl>("bumpyScaleU");
	LLUICtrl* bumpy_scale_v = self->getChild<LLUICtrl>("bumpyScaleV");
	LLUICtrl* shiny_scale_u = self->getChild<LLUICtrl>("shinyScaleU");
	LLUICtrl* shiny_scale_v = self->getChild<LLUICtrl>("shinyScaleV");
 
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		LLSelectMgr::getInstance()->selectionTexScaleAutofit( repeats_per_meter );

		bumpy_scale_u->setValue(obj_scale_s * repeats_per_meter);
		bumpy_scale_v->setValue(obj_scale_t * repeats_per_meter);

		LLSelectedTEMaterial::setNormalRepeatX(self,obj_scale_s * repeats_per_meter);
		LLSelectedTEMaterial::setNormalRepeatY(self,obj_scale_t * repeats_per_meter);

		shiny_scale_u->setValue(obj_scale_s * repeats_per_meter);
		shiny_scale_v->setValue(obj_scale_t * repeats_per_meter);

		LLSelectedTEMaterial::setSpecularRepeatX(self,obj_scale_s * repeats_per_meter);
		LLSelectedTEMaterial::setSpecularRepeatY(self,obj_scale_t * repeats_per_meter);
	}
	else
	{
		switch (material_type)
		{
			case MATTYPE_DIFFUSE:
			{
				LLSelectMgr::getInstance()->selectionTexScaleAutofit( repeats_per_meter );
			}
			break;

			case MATTYPE_NORMAL:
			{
				bumpy_scale_u->setValue(obj_scale_s * repeats_per_meter);
				bumpy_scale_v->setValue(obj_scale_t * repeats_per_meter);

				LLSelectedTEMaterial::setNormalRepeatX(self,obj_scale_s * repeats_per_meter);
				LLSelectedTEMaterial::setNormalRepeatY(self,obj_scale_t * repeats_per_meter);
			}
			break;

			case MATTYPE_SPECULAR:
			{
				shiny_scale_u->setValue(obj_scale_s * repeats_per_meter);
				shiny_scale_v->setValue(obj_scale_t * repeats_per_meter);

				LLSelectedTEMaterial::setSpecularRepeatX(self,obj_scale_s * repeats_per_meter);
				LLSelectedTEMaterial::setSpecularRepeatY(self,obj_scale_t * repeats_per_meter);
			}
			break;

			default:
				llassert(false);
				break;
		}
	}
	// vertical scale and repeats per meter depends on each other, so force set on changes
	self->updateUI(true);
}

struct LLPanelFaceSetMediaFunctor : public LLSelectedTEFunctor
{
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		viewer_media_t pMediaImpl;
				
		const LLTextureEntry* tep = object->getTE(te);
		const LLMediaEntry* mep = tep->hasMedia() ? tep->getMediaData() : NULL;
		if ( mep )
		{
			pMediaImpl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());
		}
		
		if ( pMediaImpl.isNull())
		{
			// If we didn't find face media for this face, check whether this face is showing parcel media.
			pMediaImpl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(tep->getID());
		}
		
		if ( pMediaImpl.notNull())
		{
			LLPluginClassMedia *media = pMediaImpl->getMediaPlugin();
			if(media)
			{
				S32 media_width = media->getWidth();
				S32 media_height = media->getHeight();
				S32 texture_width = media->getTextureWidth();
				S32 texture_height = media->getTextureHeight();
				F32 scale_s = (F32)media_width / (F32)texture_width;
				F32 scale_t = (F32)media_height / (F32)texture_height;

				// set scale and adjust offset
				object->setTEScaleS( te, scale_s );
				object->setTEScaleT( te, scale_t );	// don't need to flip Y anymore since QT does this for us now.
				object->setTEOffsetS( te, -( 1.0f - scale_s ) / 2.0f );
				object->setTEOffsetT( te, -( 1.0f - scale_t ) / 2.0f );
			}
		}
		return true;
	};
};

void LLPanelFace::onClickAutoFix(void* userdata)
{
	LLPanelFaceSetMediaFunctor setfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

void LLPanelFace::onAlignTexture(void* userdata)
{
    LLPanelFace* self = (LLPanelFace*)userdata;
    self->alignTestureLayer();
}


// TODO: I don't know who put these in or what these are for???
void LLPanelFace::setMediaURL(const std::string& url)
{
}
void LLPanelFace::setMediaType(const std::string& mime_type)
{
}

// static
void LLPanelFace::onCommitPlanarAlign(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->getState();
	self->sendTextureInfo();
}

void LLPanelFace::onTextureSelectionChanged(LLInventoryItem* itemp)
{
	LL_DEBUGS("Materials") << "item asset " << itemp->getAssetUUID() << LL_ENDL;
//	LLRadioGroup* radio_mat_type = findChild<LLRadioGroup>("radio_material_type");
//	if(!radio_mat_type)
//	{
//	    return;
//	}
//	U32 mattype = radio_mat_type->getSelectedIndex();
// [SL:KB] - Patch: Build-Misc | Checked: Catznip-4.2
	LLComboBox* combo_mattype = getChild<LLComboBox>("combobox mattype");
	if (!combo_mattype)
	{
	    return;
	}
	U32 mattype = combo_mattype->getCurrentIndex();
// [/SL:KB]
	std::string which_control="texture control";
	switch (mattype)
	{
		case MATTYPE_SPECULAR:
			which_control = "shinytexture control";
			break;
		case MATTYPE_NORMAL:
			which_control = "bumpytexture control";
			break;
		// no default needed
	}
	LL_DEBUGS("Materials") << "control " << which_control << LL_ENDL;
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>(which_control);
	if (texture_ctrl)
	{
		LLUUID obj_owner_id;
		std::string obj_owner_name;
		LLSelectMgr::instance().selectGetOwner(obj_owner_id, obj_owner_name);

		LLSaleInfo sale_info;
		LLSelectMgr::instance().selectGetSaleInfo(sale_info);

		bool can_copy = itemp->getPermissions().allowCopyBy(gAgentID); // do we have perm to copy this texture?
		bool can_transfer = itemp->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID); // do we have perm to transfer this texture?
		bool is_object_owner = gAgentID == obj_owner_id; // does object for which we are going to apply texture belong to the agent?
		bool not_for_sale = !sale_info.isForSale(); // is object for which we are going to apply texture not for sale?

		if (can_copy && can_transfer)
		{
			texture_ctrl->setCanApply(true, true);
			return;
		}

		// if texture has (no-transfer) attribute it can be applied only for object which we own and is not for sale
		texture_ctrl->setCanApply(false, can_transfer ? true : is_object_owner && not_for_sale);

		if (gSavedSettings.getBOOL("TextureLivePreview"))
		{
			LLNotificationsUtil::add("LivePreviewUnavailable");
		}
	}
}

bool LLPanelFace::isIdenticalPlanarTexgen()
{
	LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
	bool identical_texgen = false;
	LLSelectedTE::getTexGen(selected_texgen, identical_texgen);
	return (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));
}

void LLPanelFace::LLSelectedTE::getFace(LLFace*& face_to_return, bool& identical_face)
{		
	struct LLSelectedTEGetFace : public LLSelectedTEGetFunctor<LLFace *>
	{
		LLFace* get(LLViewerObject* object, S32 te)
		{
			return (object->mDrawable) ? object->mDrawable->getFace(te): NULL;
		}
	} get_te_face_func;
	identical_face = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_te_face_func, face_to_return, false, (LLFace*)nullptr);
}

void LLPanelFace::LLSelectedTE::getImageFormat(LLGLenum& image_format_to_return, bool& identical_face)
{
	LLGLenum image_format;
	struct LLSelectedTEGetImageFormat : public LLSelectedTEGetFunctor<LLGLenum>
	{
		LLGLenum get(LLViewerObject* object, S32 te_index)
		{
			LLViewerTexture* image = object->getTEImage(te_index);
			return image ? image->getPrimaryFormat() : GL_RGB;
		}
	} get_glenum;
	identical_face = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_glenum, image_format);
	image_format_to_return = image_format;
}

void LLPanelFace::LLSelectedTE::getTexId(LLUUID& id, bool& identical)
{		
	struct LLSelectedTEGetTexId : public LLSelectedTEGetFunctor<LLUUID>
	{
		LLUUID get(LLViewerObject* object, S32 te_index)
		{
			LLTextureEntry *te = object->getTE(te_index);
			if (te)
			{
				if ((te->getID() == IMG_USE_BAKED_EYES) || (te->getID() == IMG_USE_BAKED_HAIR) || (te->getID() == IMG_USE_BAKED_HEAD) || (te->getID() == IMG_USE_BAKED_LOWER) || (te->getID() == IMG_USE_BAKED_SKIRT) || (te->getID() == IMG_USE_BAKED_UPPER)
					|| (te->getID() == IMG_USE_BAKED_LEFTARM) || (te->getID() == IMG_USE_BAKED_LEFTLEG) || (te->getID() == IMG_USE_BAKED_AUX1) || (te->getID() == IMG_USE_BAKED_AUX2) || (te->getID() == IMG_USE_BAKED_AUX3))
				{
					return te->getID();
				}
			}

			LLUUID id;
			LLViewerTexture* image = object->getTEImage(te_index);
			if (image)
			{
				id = image->getID();
			}

			if (!id.isNull() && LLViewerMedia::getInstance()->textureHasMedia(id))
			{
				if (te)
				{
					LLViewerTexture* tex = te->getID().notNull() ? gTextureList.findImage(te->getID(), TEX_LIST_STANDARD) : NULL;
					if(!tex)
					{
						tex = LLViewerFetchedTexture::sDefaultImagep;
					}
					if (tex)
					{
						id = tex->getID();
					}
				}
			}
			return id;
		}
	} func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, id );
}

void LLPanelFace::LLSelectedTEMaterial::getCurrent(LLMaterialPtr& material_ptr, bool& identical_material)
{
	struct MaterialFunctor : public LLSelectedTEGetFunctor<LLMaterialPtr>
	{
		LLMaterialPtr get(LLViewerObject* object, S32 te_index)
		{
			return object->getTE(te_index)->getMaterialParams();
		}
	} func;
	identical_material = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, material_ptr);
}

void LLPanelFace::LLSelectedTEMaterial::getMaxSpecularRepeats(F32& repeats, bool& identical)
{
	struct LLSelectedTEGetMaxSpecRepeats : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
			U32 s_axis = VX;
			U32 t_axis = VY;
			F32 repeats_s = 1.0f;
			F32 repeats_t = 1.0f;
			if (mat)
			{
				mat->getSpecularRepeat(repeats_s, repeats_t);
				repeats_s /= object->getScale().mV[s_axis];
				repeats_t /= object->getScale().mV[t_axis];
			}					
			return llmax(repeats_s, repeats_t);
		}

	} max_spec_repeats_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &max_spec_repeats_func, repeats);
}

void LLPanelFace::LLSelectedTEMaterial::getMaxNormalRepeats(F32& repeats, bool& identical)
{
	struct LLSelectedTEGetMaxNormRepeats : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
			U32 s_axis = VX;
			U32 t_axis = VY;
			F32 repeats_s = 1.0f;
			F32 repeats_t = 1.0f;
			if (mat)
			{
				mat->getNormalRepeat(repeats_s, repeats_t);
				repeats_s /= object->getScale().mV[s_axis];
				repeats_t /= object->getScale().mV[t_axis];
			}					
			return llmax(repeats_s, repeats_t);
		}

	} max_norm_repeats_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &max_norm_repeats_func, repeats);
}

void LLPanelFace::LLSelectedTEMaterial::getCurrentDiffuseAlphaMode(U8& diffuse_alpha_mode, bool& identical, bool diffuse_texture_has_alpha)
{
	struct LLSelectedTEGetDiffuseAlphaMode : public LLSelectedTEGetFunctor<U8>
	{
		LLSelectedTEGetDiffuseAlphaMode() : _isAlpha(false) {}
		LLSelectedTEGetDiffuseAlphaMode(bool diffuse_texture_has_alpha) : _isAlpha(diffuse_texture_has_alpha) {}
		virtual ~LLSelectedTEGetDiffuseAlphaMode() {}

		U8 get(LLViewerObject* object, S32 face)
		{
			U8 diffuse_mode = _isAlpha ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

			LLTextureEntry* tep = object->getTE(face);
			if (tep)
			{
				LLMaterial* mat = tep->getMaterialParams().get();
				if (mat)
				{
					diffuse_mode = mat->getDiffuseAlphaMode();
				}
			}
			
			return diffuse_mode;
		}
		bool _isAlpha; // whether or not the diffuse texture selected contains alpha information
	} get_diff_mode(diffuse_texture_has_alpha);
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &get_diff_mode, diffuse_alpha_mode);
}

void LLPanelFace::LLSelectedTE::getObjectScaleS(F32& scale_s, bool& identical)
{	
	struct LLSelectedTEGetObjectScaleS : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
			return object->getScale().mV[s_axis];
		}

	} scale_s_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &scale_s_func, scale_s );
}

void LLPanelFace::LLSelectedTE::getObjectScaleT(F32& scale_t, bool& identical)
{	
	struct LLSelectedTEGetObjectScaleS : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
			return object->getScale().mV[t_axis];
		}

	} scale_t_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &scale_t_func, scale_t );
}

void LLPanelFace::LLSelectedTE::getMaxDiffuseRepeats(F32& repeats, bool& identical)
{
	struct LLSelectedTEGetMaxDiffuseRepeats : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
			F32 repeats_s = object->getTE(face)->mScaleS / object->getScale().mV[s_axis];
			F32 repeats_t = object->getTE(face)->mScaleT / object->getScale().mV[t_axis];
			return llmax(repeats_s, repeats_t);
		}

	} max_diff_repeats_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &max_diff_repeats_func, repeats );
}

// [SL:KB] - Patch: Build-ScaleParamFlip | Checked: Catznip-5.2
void LLPanelFace::onCommitScaleFlip(LLUICtrl* ctrl)
{
	LLSpinCtrl* pSpinCtrl = dynamic_cast<LLSpinCtrl*>(ctrl);
	if (pSpinCtrl)
	{
		F32 nValue = pSpinCtrl->get();
		pSpinCtrl->set(-nValue);
		pSpinCtrl->onCommit();
	}
}
// [/SL:KB]
