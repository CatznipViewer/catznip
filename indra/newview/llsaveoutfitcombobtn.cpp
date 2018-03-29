/** 
 * @file llsaveoutfitcombobtn.cpp
 * @brief Represents outfit save/save as combo button.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llappearancemgr.h"
#include "llpaneloutfitsinventory.h"
#include "llsidepanelappearance.h"
#include "llsaveoutfitcombobtn.h"
#include "llviewermenu.h"
// [SL:KB] - Patch: Control-SaveOutfitComboBtn | Checked: 2012-08-09 (Catznip-3.3)
#include "llflyoutbutton.h"
#include "lltrans.h"
// [/SL:KB]

static const std::string SAVE_BTN("save_btn");
//static const std::string SAVE_FLYOUT_BTN("save_flyout_btn");

LLSaveOutfitComboBtn::LLSaveOutfitComboBtn(LLPanel* parent, bool saveAsDefaultAction):
	mParent(parent), mSaveAsDefaultAction(saveAsDefaultAction)
{
// [SL:KB] - Patch: Control-SaveOutfitComboBtn | Checked: 2012-08-09 (Catznip-3.3)
	mSaveBtn = mParent->getChild<LLFlyoutButton>(SAVE_BTN);
	mSaveBtn->setCommitCallback(boost::bind(&LLSaveOutfitComboBtn::saveOutfit, this, _1));

	setMenuItemLabel("save_outfit", LLTrans::getString("SaveOutfit"));
	setMenuItemLabel("save_as_new_outfit", LLTrans::getString("SaveOutfitAs"));
// [/SL:KB]
//	// register action mapping before creating menu
//	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar save_registar;
//	save_registar.add("Outfit.Save.Action", boost::bind(
//			&LLSaveOutfitComboBtn::saveOutfit, this, false));
//	save_registar.add("Outfit.SaveAs.Action", boost::bind(
//			&LLSaveOutfitComboBtn::saveOutfit, this, true));
//
//	mParent->childSetAction(SAVE_BTN, boost::bind(&LLSaveOutfitComboBtn::saveOutfit, this, mSaveAsDefaultAction));
//	mParent->childSetAction(SAVE_FLYOUT_BTN, boost::bind(&LLSaveOutfitComboBtn::showSaveMenu, this));
//
//	mSaveMenu = LLUICtrlFactory::getInstance()->createFromFile<
//			LLToggleableMenu> ("menu_save_outfit.xml", gMenuHolder,
//			LLViewerMenuHolderGL::child_registry_t::instance());
}

//void LLSaveOutfitComboBtn::showSaveMenu()
//{
//	S32 x, y;
//	LLUI::getMousePositionLocal(mParent, &x, &y);
//
//	mSaveMenu->updateParent(LLMenuGL::sMenuContainer);
//	LLMenuGL::showPopup(mParent, mSaveMenu, x, y);
//}

//void LLSaveOutfitComboBtn::saveOutfit(bool as_new)
// [SL:KB] - Patch: Control-SaveOutfitComboBtn | Checked: 2012-08-09 (Catznip-3.3)
void LLSaveOutfitComboBtn::saveOutfit(const LLUICtrl* pBtnCtrl)
// [/SL:KB]
{
// [SL:KB] - Patch: Control-SaveOutfitComboBtn | Checked: 2012-08-09 (Catznip-3.3)
	const std::string strAction = pBtnCtrl->getValue().asString();
	bool fSaveAs = mSaveAsDefaultAction;
	if ("save_outfit" == strAction)
		fSaveAs = false;
	else if ("save_as_new_outfit" == strAction)
		fSaveAs = true;

	if ( (!fSaveAs) && (LLAppearanceMgr::getInstance()->updateBaseOutfit()) )
	{
		// we don't need to ask for an outfit name, and updateBaseOutfit() successfully saved.
		// If updateBaseOutfit fails, ask for an outfit name anyways
		return;
	}

	LLPanelOutfitsInventory* panel_outfits_inventory = LLPanelOutfitsInventory::findInstance();
	if (panel_outfits_inventory)
	{
		panel_outfits_inventory->onSave();
	} 	
// [/SL:KB]
//	if (!as_new && LLAppearanceMgr::getInstance()->updateBaseOutfit())
//	{
//		// we don't need to ask for an outfit name, and updateBaseOutfit() successfully saved.
//		// If updateBaseOutfit fails, ask for an outfit name anyways
//		return;
//	}
//
//	LLPanelOutfitsInventory* panel_outfits_inventory =
//			LLPanelOutfitsInventory::findInstance();
//	if (panel_outfits_inventory)
//	{
//		panel_outfits_inventory->onSave();
//	}
//
//	//*TODO how to get to know when base outfit is updated or new outfit is created?
}
    
// [SL:KB] - Patch: Control-SaveOutfitComboBtn | Checked: 2012-08-09 (Catznip-3.3)
void LLSaveOutfitComboBtn::setMenuItemLabel(const std::string& item, const std::string& label)
{
	LLScrollListCtrl* pListCtrl = mSaveBtn->getListControl();
	if (pListCtrl)
	{
		LLScrollListItem* pItem = pListCtrl->getItem(item);
		if (pItem)
		{
			LLScrollListText* pTextCell = dynamic_cast<LLScrollListText*>(pItem->getColumn(0));
			if (pTextCell)
			{
				pTextCell->setText(label);
			}
		}
	}
}
// [/SL:KB]

void LLSaveOutfitComboBtn::setMenuItemEnabled(const std::string& item, bool enabled)
{
// [SL:KB] - Patch: Control-SaveOutfitComboBtn | Checked: 2012-08-09 (Catznip-3.3)
	LLScrollListCtrl* pListCtrl = mSaveBtn->getListControl();
	if (pListCtrl)
	{
		LLScrollListItem* pItem = pListCtrl->getItem(item);
		if (pItem)
		{
			pItem->setEnabled(enabled);
		}
	}
// [/SL:KB]
//	mSaveMenu->setItemEnabled("save_outfit", enabled);
}

void LLSaveOutfitComboBtn::setSaveBtnEnabled(bool enabled)
{
	mParent->getChildView(SAVE_BTN)->setEnabled(enabled);
}
