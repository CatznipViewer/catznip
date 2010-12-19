/**
 * @file LLSidepanelInventory.cpp
 * @brief Side Bar "Inventory" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "llsidepanelinventory.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llavataractions.h"
#include "llbutton.h"
#include "llfirstuse.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "lloutfitobserver.h"
#include "llpanelmaininventory.h"
#include "llsidepaneliteminfo.h"
#include "llsidepaneltaskinfo.h"
#include "lltabcontainer.h"
#include "llselectmgr.h"
// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2010-04-15 (Catznip-2.1.2a) | Added: Catznip-2.0.0a
#include "lltrans.h"
// [/SL:KB]
#include "llweb.h"

static LLRegisterPanelClassWrapper<LLSidepanelInventory> t_inventory("sidepanel_inventory");

LLSidepanelInventory::LLSidepanelInventory()
	:	LLPanel(),
		mItemPanel(NULL),
		mPanelMainInventory(NULL)
{

	//buildFromFile( "panel_inventory.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLSidepanelInventory::~LLSidepanelInventory()
{
}

BOOL LLSidepanelInventory::postBuild()
{
	// UI elements from inventory panel
	{
		mInventoryPanel = getChild<LLPanel>("sidepanel__inventory_panel");

		mInfoBtn = mInventoryPanel->getChild<LLButton>("info_btn");
		mInfoBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onInfoButtonClicked, this));
		
		mShareBtn = mInventoryPanel->getChild<LLButton>("share_btn");
		mShareBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onShareButtonClicked, this));
		
/*
		mShopBtn = mInventoryPanel->getChild<LLButton>("shop_btn");
		mShopBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onShopButtonClicked, this));
*/

		mWearBtn = mInventoryPanel->getChild<LLButton>("wear_btn");
		mWearBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onWearButtonClicked, this));
		
/*
		mPlayBtn = mInventoryPanel->getChild<LLButton>("play_btn");
		mPlayBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onPlayButtonClicked, this));
		
		mTeleportBtn = mInventoryPanel->getChild<LLButton>("teleport_btn");
		mTeleportBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onTeleportButtonClicked, this));
*/
		
		mOverflowBtn = mInventoryPanel->getChild<LLButton>("overflow_btn");
		mOverflowBtn->setClickedCallback(boost::bind(&LLSidepanelInventory::onOverflowButtonClicked, this));
		
		mPanelMainInventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
		mPanelMainInventory->setSelectCallback(boost::bind(&LLSidepanelInventory::onSelectionChange, this, _1, _2));
// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2010-04-15 (Catznip-2.1.2a) | Added: Catznip-2.0.0a
		mPanelMainInventory->setActivePanelCallback(boost::bind(&LLSidepanelInventory::onActivePanelChanged, this, _1));
		mPanelMainInventory->setModelChangedCallback(boost::bind(&LLSidepanelInventory::onModelChanged, this, _1));
// [/SL:KB]
		LLTabContainer* tabs = mPanelMainInventory->getChild<LLTabContainer>("inventory filter tabs");
		tabs->setCommitCallback(boost::bind(&LLSidepanelInventory::updateVerbs, this));

		/* 
		   EXT-4846 : "Can we suppress the "Landmarks" and "My Favorites" folder since they have their own Task Panel?"
		   Deferring this until 2.1.
		LLInventoryPanel *my_inventory_panel = mPanelMainInventory->getChild<LLInventoryPanel>("All Items");
		my_inventory_panel->addHideFolderType(LLFolderType::FT_LANDMARK);
		my_inventory_panel->addHideFolderType(LLFolderType::FT_FAVORITE);
		*/

		LLOutfitObserver::instance().addCOFChangedCallback(boost::bind(&LLSidepanelInventory::updateVerbs, this));
	}

	// UI elements from item panel
	{
		mItemPanel = findChild<LLSidepanelItemInfo>("sidepanel__item_panel");
		
		LLButton* back_btn = mItemPanel->getChild<LLButton>("back_btn");
		back_btn->setClickedCallback(boost::bind(&LLSidepanelInventory::onBackButtonClicked, this));
	}

	// UI elements from task panel
	{
		mTaskPanel = findChild<LLSidepanelTaskInfo>("sidepanel__task_panel");
		if (mTaskPanel)
		{
			LLButton* back_btn = mTaskPanel->getChild<LLButton>("back_btn");
			back_btn->setClickedCallback(boost::bind(&LLSidepanelInventory::onBackButtonClicked, this));
		}
	}
	
	return TRUE;
}

void LLSidepanelInventory::onOpen(const LLSD& key)
{
	LLFirstUse::newInventory(false);

	if(key.size() == 0)
		return;

	mItemPanel->reset();

	if (key.has("id"))
	{
		mItemPanel->setItemID(key["id"].asUUID());
		if (key.has("object"))
		{
			mItemPanel->setObjectID(key["object"].asUUID());
		}
		showItemInfoPanel();
	}
	if (key.has("task"))
	{
		if (mTaskPanel)
			mTaskPanel->setObjectSelection(LLSelectMgr::getInstance()->getSelection());
		showTaskInfoPanel();
	}
}

void LLSidepanelInventory::onInfoButtonClicked()
{
	LLInventoryItem *item = getSelectedItem();
	if (item)
	{
		mItemPanel->reset();
		mItemPanel->setItemID(item->getUUID());
		showItemInfoPanel();
	}
}

void LLSidepanelInventory::onShareButtonClicked()
{
	LLAvatarActions::shareWithAvatars();
}

void LLSidepanelInventory::onShopButtonClicked()
{
	LLWeb::loadURLExternal(gSavedSettings.getString("MarketplaceURL"));
}

void LLSidepanelInventory::performActionOnSelection(const std::string &action)
{
// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2010-04-15 (Catznip-2.1.2a) | Added: Catznip-2.0.0a
	/*const*/ LLInventoryPanel* pPanel = getActivePanel();
	if ( (!pPanel) || (!pPanel->getRootFolder()) )
		return;

	if (!action.empty())
		pPanel->getRootFolder()->doToSelected(pPanel->getModel(), action);
// [/SL:KB]
/*
	LLPanelMainInventory *panel_main_inventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
	LLFolderViewItem* current_item = panel_main_inventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return;
	}
	current_item->getListener()->performAction(panel_main_inventory->getActivePanel()->getModel(), action);
*/
}

void LLSidepanelInventory::onWearButtonClicked()
{
// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2010-04-15 (Catznip-2.1.2a) | Added: Catznip-2.0.0a
	performActionOnSelection(getSelectionAction());
// [/SL:KB]
/*
	LLPanelMainInventory *panel_main_inventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
	if (!panel_main_inventory)
	{
		llassert(panel_main_inventory != NULL);
		return;
	}

	// Get selected items set.
	const std::set<LLUUID> selected_uuids_set = panel_main_inventory->getActivePanel()->getRootFolder()->getSelectionList();
	if (selected_uuids_set.empty()) return; // nothing selected

	// Convert the set to a vector.
	uuid_vec_t selected_uuids_vec;
	for (std::set<LLUUID>::const_iterator it = selected_uuids_set.begin(); it != selected_uuids_set.end(); ++it)
	{
		selected_uuids_vec.push_back(*it);
	}

	// Wear all selected items.
	wear_multiple(selected_uuids_vec, true);
*/
}

/*
void LLSidepanelInventory::onPlayButtonClicked()
{
	const LLInventoryItem *item = getSelectedItem();
	if (!item)
	{
		return;
	}

	switch(item->getInventoryType())
	{
	case LLInventoryType::IT_GESTURE:
		performActionOnSelection("play");
		break;
	default:
		performActionOnSelection("open");
		break;
	}
}
*/

/*
void LLSidepanelInventory::onTeleportButtonClicked()
{
	performActionOnSelection("teleport");
}
*/

void LLSidepanelInventory::onOverflowButtonClicked()
{
}

void LLSidepanelInventory::onBackButtonClicked()
{
	showInventoryPanel();
}

void LLSidepanelInventory::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	updateVerbs();
}

void LLSidepanelInventory::showItemInfoPanel()
{
	mItemPanel->setVisible(TRUE);
	if (mTaskPanel)
		mTaskPanel->setVisible(FALSE);
	mInventoryPanel->setVisible(FALSE);

	mItemPanel->dirty();
	mItemPanel->setIsEditing(FALSE);
}

void LLSidepanelInventory::showTaskInfoPanel()
{
	mItemPanel->setVisible(FALSE);
	mInventoryPanel->setVisible(FALSE);

	if (mTaskPanel)
	{
		mTaskPanel->setVisible(TRUE);
		mTaskPanel->dirty();
		mTaskPanel->setIsEditing(FALSE);
	}
}

void LLSidepanelInventory::showInventoryPanel()
{
	mItemPanel->setVisible(FALSE);
	if (mTaskPanel)
		mTaskPanel->setVisible(FALSE);
	mInventoryPanel->setVisible(TRUE);
	updateVerbs();
}

void LLSidepanelInventory::updateVerbs()
{
	mInfoBtn->setEnabled(FALSE);
	mShareBtn->setEnabled(FALSE);

	mWearBtn->setVisible(FALSE);
	mWearBtn->setEnabled(FALSE);
/*
	mPlayBtn->setVisible(FALSE);
	mPlayBtn->setEnabled(FALSE);
 	mTeleportBtn->setVisible(FALSE);
 	mTeleportBtn->setEnabled(FALSE);
 	mShopBtn->setVisible(TRUE);
*/

	mShareBtn->setEnabled(canShare());

// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2010-04-15 (Catznip-2.1.2a) | Added: Catznip-2.0.0a
	// We usurp the "Wear" button and just make it handle everything
	std::string strAction = getSelectionAction();
	if (!strAction.empty())
	{
		mWearBtn->setLabel(LLTrans::getString("InvAction " + strAction));
		mWearBtn->setVisible(TRUE);
		mWearBtn->setEnabled(TRUE);

		bool is_single_selection = getSelectedCount() == 1;
		mInfoBtn->setEnabled(is_single_selection);
		mShareBtn->setEnabled(is_single_selection);
	}
// [/SL:KB]

/*
	const LLInventoryItem *item = getSelectedItem();
	if (!item)
		return;

	bool is_single_selection = getSelectedCount() == 1;

	mInfoBtn->setEnabled(is_single_selection);

	switch(item->getInventoryType())
	{
		case LLInventoryType::IT_WEARABLE:
		case LLInventoryType::IT_OBJECT:
		case LLInventoryType::IT_ATTACHMENT:
			mWearBtn->setVisible(TRUE);
			mWearBtn->setEnabled(canWearSelected());
		 	mShopBtn->setVisible(FALSE);
			break;
		case LLInventoryType::IT_SOUND:
		case LLInventoryType::IT_GESTURE:
		case LLInventoryType::IT_ANIMATION:
			mPlayBtn->setVisible(TRUE);
			mPlayBtn->setEnabled(TRUE);
		 	mShopBtn->setVisible(FALSE);
			break;
		case LLInventoryType::IT_LANDMARK:
			mTeleportBtn->setVisible(TRUE);
			mTeleportBtn->setEnabled(TRUE);
		 	mShopBtn->setVisible(FALSE);
			break;
		default:
			break;
	}
*/
}

bool LLSidepanelInventory::canShare()
{
	LLPanelMainInventory* panel_main_inventory =
		mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");

	if (!panel_main_inventory)
	{
		llwarns << "Failed to get the main inventory panel" << llendl;
		return false;
	}

	LLInventoryPanel* active_panel = panel_main_inventory->getActivePanel();
	// Avoid flicker in the Recent tab while inventory is being loaded.
	if (!active_panel->getRootFolder()->hasVisibleChildren()) return false;

	return LLAvatarActions::canShareSelectedItems(active_panel);
}

bool LLSidepanelInventory::canWearSelected()
{
	LLPanelMainInventory* panel_main_inventory =
		mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");

	if (!panel_main_inventory)
	{
		llassert(panel_main_inventory != NULL);
		return false;
	}

	std::set<LLUUID> selected_uuids = panel_main_inventory->getActivePanel()->getRootFolder()->getSelectionList();
	for (std::set<LLUUID>::const_iterator it = selected_uuids.begin();
		it != selected_uuids.end();
		++it)
	{
		if (!get_can_item_be_worn(*it)) return false;
	}

	return true;
}

LLInventoryItem *LLSidepanelInventory::getSelectedItem()
{
	LLPanelMainInventory *panel_main_inventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
	LLFolderViewItem* current_item = panel_main_inventory->getActivePanel()->getRootFolder()->getCurSelectedItem();
	if (!current_item)
	{
		return NULL;
	}
	const LLUUID &item_id = current_item->getListener()->getUUID();
	LLInventoryItem *item = gInventory.getItem(item_id);
	return item;
}

U32 LLSidepanelInventory::getSelectedCount()
{
	LLPanelMainInventory *panel_main_inventory = mInventoryPanel->findChild<LLPanelMainInventory>("panel_main_inventory");
	std::set<LLUUID> selection_list = panel_main_inventory->getActivePanel()->getRootFolder()->getSelectionList();
	return selection_list.size();
}

LLInventoryPanel *LLSidepanelInventory::getActivePanel()
{
	if (!getVisible())
	{
		return NULL;
	}
	if (mInventoryPanel->getVisible())
	{
		return mPanelMainInventory->getActivePanel();
	}
	return NULL;
}

BOOL LLSidepanelInventory::isMainInventoryPanelActive() const
{
	return mInventoryPanel->getVisible();
}

// [SL:KB] - Patch: UI-SidepanelInventory | Checked: 2010-04-15 (Catznip-2.1.2a) | Added: Catznip-2.0.0a

// Returns IT_XXX if every item has the same inventory type or IT_NONE otherwise
LLInventoryType::EType get_items_invtype(const LLInventoryModel::item_array_t& items)
{
	LLInventoryType::EType invType = LLInventoryType::IT_NONE;
	for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
	{
		const LLViewerInventoryItem* pItem = itItem->get();
		if (!pItem)
			continue;

		LLInventoryType::EType invTypeItem = pItem->getInventoryType();
		if (LLInventoryType::IT_NONE == invType)
			invType = invTypeItem;
		else if (invType != invTypeItem)
			return LLInventoryType::IT_NONE;
	}
	return invType;
}

// Returns TRUE if every item is something that can be worn (wearables, attachments and gestures)
bool get_items_wearable(const LLInventoryModel::item_array_t& items)
{
	bool fWearable = true;
	for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); (itItem != items.end()) && (fWearable); ++itItem)
	{
		// TODO-Catznip: there has to be a generic function for this *somewhere*?
		const LLViewerInventoryItem* pItem = itItem->get();
		if (!pItem)
			continue;

		switch (pItem->getType())
		{
			case LLAssetType::AT_OBJECT:
			case LLAssetType::AT_BODYPART:
			case LLAssetType::AT_CLOTHING:
			case LLAssetType::AT_GESTURE:
				break;
			default:
				fWearable = false;
				break;
		}
	}
	return fWearable;
}

// Returns TRUE if every item is worn (handles wearables, attachments and gestures)
bool get_items_worn(const LLInventoryModel::item_array_t& items)
{
	bool fWorn = true;
	for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); (itItem != items.end()) && (fWorn); ++itItem)
	{
		const LLViewerInventoryItem* pItem = itItem->get();
		if (!pItem)
			continue;

		fWorn = get_is_item_worn(pItem->getUUID());
	}
	return fWorn;
}

bool LLSidepanelInventory::getSelectedItems(LLInventoryModel::item_array_t& items) /*const*/
{
	items.clear();

	/*const*/ LLInventoryPanel* pPanel = getActivePanel();
	if ( (!pPanel) || (!pPanel->getRootFolder()) )
		return false;
	std::set<LLUUID> selItems = pPanel->getRootFolder()->getSelectionList();
	if (selItems.empty())
		return false;

	for (std::set<LLUUID>::const_iterator itItem = selItems.begin(); itItem != selItems.end(); itItem++)
	{
		LLViewerInventoryItem* pItem = pPanel->getModel()->getItem(*itItem);
		if (!pItem)
		{
			// Bit of a hack but if there are categories selected then we don't want to show any actions so we return an empty selection
			if (pPanel->getModel()->getCategory(*itItem))
			{
				items.clear();
				return false;
			}
			continue;
		}
		items.push_back(pItem);
	}
	return !items.empty();
}

std::string LLSidepanelInventory::getSelectionAction() /*const*/
{
	LLInventoryModel::item_array_t items;
	if (!getSelectedItems(items))
		return LLStringUtil::null;

	LLInventoryType::EType invType = get_items_invtype(items);
	switch (invType)
	{
		case LLInventoryType::IT_WEARABLE:
			return (!get_items_worn(items)) ? "wear" : "take_off";
		case LLInventoryType::IT_OBJECT:
		case LLInventoryType::IT_ATTACHMENT:
			return (!get_items_worn(items)) ? "attach" : "detach";
		case LLInventoryType::IT_GESTURE:
			return (!get_items_worn(items)) ? "activate" : "deactivate";
		case LLInventoryType::IT_LANDMARK:
			// It doesn't make much sense to teleport to more than one landmark
			return (1 == getSelectedCount()) ? "teleport" : LLStringUtil::null;
		case LLInventoryType::IT_NONE:
			// Mixed selection type
			return (get_items_wearable(items)) ? ( (!get_items_worn(items)) ? "wear" : "take_off" ) : "open";
		default:
			return "open";
	}
//	return LLStringUtil::null;
}

void LLSidepanelInventory::onActivePanelChanged(LLInventoryPanel*)
{
	if (isMainInventoryPanelActive())
		updateVerbs();
}

void LLSidepanelInventory::onModelChanged(U32 mask)
{
	if ( (mask & LLInventoryObserver::LABEL) && (isMainInventoryPanelActive()) )
		updateVerbs();
}

// [/SL:KB]
