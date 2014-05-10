/**
 * @file llpanelwearing.cpp
 * @brief List of agent's worn items.
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

#include "llpanelwearing.h"

#include "lltoggleablemenu.h"

#include "llappearancemgr.h"
#include "llfloatersidepanelcontainer.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llmenubutton.h"
#include "llviewermenu.h"
#include "llwearableitemslist.h"
#include "llsdserialize.h"
#include "llclipboard.h"

// Context menu and Gear menu helper.
static void edit_outfit()
{
	LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "edit_outfit"));
}

//////////////////////////////////////////////////////////////////////////

class LLWearingGearMenu
{
public:
	LLWearingGearMenu(LLPanelWearing* panel_wearing)
	:	mMenu(NULL), mPanelWearing(panel_wearing)
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

// [SL:KB] - Patch: Inventory-AttachmentActions - Checked: 2012-05-15 (Catznip-3.3)
		registrar.add("Gear.TouchAttach", boost::bind(&LLWearingGearMenu::onTouchAttach, this));
		registrar.add("Gear.EditItem", boost::bind(&LLWearingGearMenu::onEditItem, this));
		registrar.add("Gear.EditOutfit", boost::bind(&edit_outfit));
// [/SL:KB]
//		registrar.add("Gear.Edit", boost::bind(&edit_outfit));
		registrar.add("Gear.TakeOff", boost::bind(&LLWearingGearMenu::onTakeOff, this));
		registrar.add("Gear.Copy", boost::bind(&LLPanelWearing::copyToClipboard, mPanelWearing));

		enable_registrar.add("Gear.OnEnable", boost::bind(&LLPanelWearing::isActionEnabled, mPanelWearing, _2));

		mMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
			"menu_wearing_gear.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		llassert(mMenu);
	}

	LLToggleableMenu* getMenu() { return mMenu; }

private:

// [SL:KB] - Patch: Inventory-AttachmentActions - Checked: 2012-05-15 (Catznip-3.3)
	void onTouchAttach()
	{
		uuid_vec_t selected_uuids;
		mPanelWearing->getSelectedItemsUUIDs(selected_uuids);

		if (selected_uuids.size() > 0)
			handle_attachment_touch(selected_uuids.front());
	}

	void onEditItem()
	{
		uuid_vec_t selected_uuids;
		mPanelWearing->getSelectedItemsUUIDs(selected_uuids);

		if (selected_uuids.size() > 0)
			handle_item_edit(selected_uuids.front());
	}
// [/SL:KB]

	void onTakeOff()
	{
		uuid_vec_t selected_uuids;
		mPanelWearing->getSelectedItemsUUIDs(selected_uuids);
		LLAppearanceMgr::instance().removeItemsFromAvatar(selected_uuids);
	}

	LLToggleableMenu*		mMenu;
	LLPanelWearing* 		mPanelWearing;
};

//////////////////////////////////////////////////////////////////////////

class LLWearingContextMenu : public LLListContextMenu
{
protected:
	/* virtual */ LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

//		registrar.add("Wearing.Edit", boost::bind(&edit_outfit));
		registrar.add("Wearing.TakeOff",
					  boost::bind(&LLAppearanceMgr::removeItemsFromAvatar, LLAppearanceMgr::getInstance(), mUUIDs));
		registrar.add("Wearing.Detach", 
					  boost::bind(&LLAppearanceMgr::removeItemsFromAvatar, LLAppearanceMgr::getInstance(), mUUIDs));
// [SL:KB] - Patch: Inventory-AttachmentActions - Checked: 2010-09-04 (Catznip-3.3)
		registrar.add("Wearing.TouchAttach", boost::bind(handleMultiple, handle_attachment_touch, mUUIDs));
		registrar.add("Wearing.EditItem", boost::bind(handleMultiple, handle_item_edit, mUUIDs));
		registrar.add("Wearing.EditOutfit", boost::bind(&edit_outfit));
		registrar.add("Wearing.TakeOffDetach", boost::bind(&LLAppearanceMgr::removeItemsFromAvatar, LLAppearanceMgr::getInstance(), mUUIDs));
// [/SL:KB]
		LLContextMenu* menu = createFromFile("menu_wearing_tab.xml");

		updateMenuItemsVisibility(menu);

		return menu;
	}

	void updateMenuItemsVisibility(LLContextMenu* menu)
	{
		bool bp_selected			= false;	// true if body parts selected
		bool clothes_selected		= false;
		bool attachments_selected	= false;

		// See what types of wearables are selected.
		for (uuid_vec_t::const_iterator it = mUUIDs.begin(); it != mUUIDs.end(); ++it)
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);

			if (!item)
			{
				llwarns << "Invalid item" << llendl;
				continue;
			}

			LLAssetType::EType type = item->getType();
			if (type == LLAssetType::AT_CLOTHING)
			{
				clothes_selected = true;
			}
			else if (type == LLAssetType::AT_BODYPART)
			{
				bp_selected = true;
			}
			else if (type == LLAssetType::AT_OBJECT)
			{
				attachments_selected = true;
			}
		}

		// Enable/disable some menu items depending on the selection.
// [SL:KB] - Patch: Inventory-AttachmentActions - Checked: 2012-05-05 (Catznip-3.3)
		bool show_touch = !bp_selected && !clothes_selected && attachments_selected;
		bool show_edit = bp_selected || clothes_selected || attachments_selected;
		bool show_detach = !clothes_selected && attachments_selected;
		bool show_take_off = clothes_selected && !attachments_selected;
		bool show_take_off_or_detach = clothes_selected && attachments_selected;

		menu->setItemVisible("touch_attach",       show_touch);
		menu->setItemEnabled("touch_attach",       1 == mUUIDs.size() && enable_attachment_touch(mUUIDs.front()));
		menu->setItemVisible("edit_item",          show_edit);
		menu->setItemEnabled("edit_item",          1 == mUUIDs.size() && enable_item_edit(mUUIDs.front()));
		menu->setItemVisible("detach",             show_detach);
		menu->setItemEnabled("detach",             !bp_selected);
		menu->setItemVisible("take_off",           show_take_off);
		menu->setItemEnabled("take_off",           !bp_selected);
		menu->setItemVisible("take_off_or_detach", show_take_off_or_detach);
		menu->setItemEnabled("take_off_or_detach", !bp_selected);

		menu->setItemVisible("edit_outfit_separator", show_edit || show_detach || show_take_off || show_take_off_or_detach);
// [/SL:KB]
//		bool allow_detach = !bp_selected && !clothes_selected && attachments_selected;
//		bool allow_take_off = !bp_selected && clothes_selected && !attachments_selected;
//
//		menu->setItemVisible("take_off",	allow_take_off);
//		menu->setItemVisible("detach",		allow_detach);
//		menu->setItemVisible("edit_outfit_separator", allow_take_off || allow_detach);
	}
};

//////////////////////////////////////////////////////////////////////////

std::string LLPanelAppearanceTab::sFilterSubString = LLStringUtil::null;

static LLPanelInjector<LLPanelWearing> t_panel_wearing("panel_wearing");

LLPanelWearing::LLPanelWearing()
	:	LLPanelAppearanceTab()
	,	mCOFItemsList(NULL)
	,	mIsInitialized(false)
{
	mCategoriesObserver = new LLInventoryCategoriesObserver();

	mGearMenu = new LLWearingGearMenu(this);
	mContextMenu = new LLWearingContextMenu();
}

LLPanelWearing::~LLPanelWearing()
{
	delete mGearMenu;
	delete mContextMenu;

	if (gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
	}
	delete mCategoriesObserver;
}

BOOL LLPanelWearing::postBuild()
{
	mCOFItemsList = getChild<LLWearableItemsList>("cof_items_list");
	mCOFItemsList->setRightMouseDownCallback(boost::bind(&LLPanelWearing::onWearableItemsListRightClick, this, _1, _2, _3));

	LLMenuButton* menu_gear_btn = getChild<LLMenuButton>("options_gear_btn");

	menu_gear_btn->setMenu(mGearMenu->getMenu());

	return TRUE;
}

//virtual
void LLPanelWearing::onOpen(const LLSD& /*info*/)
{
	if (!mIsInitialized)
	{
		// *TODO: I'm not sure is this check necessary but it never match while developing.
		if (!gInventory.isInventoryUsable())
			return;

		const LLUUID cof = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);

		// *TODO: I'm not sure is this check necessary but it never match while developing.
		LLViewerInventoryCategory* category = gInventory.getCategory(cof);
		if (!category)
			return;

		gInventory.addObserver(mCategoriesObserver);

		// Start observing changes in Current Outfit category.
		mCategoriesObserver->addCategory(cof, boost::bind(&LLWearableItemsList::updateList, mCOFItemsList, cof));

		// Fetch Current Outfit contents and refresh the list to display
		// initially fetched items. If not all items are fetched now
		// the observer will refresh the list as soon as the new items
		// arrive.
		category->fetch();

		mCOFItemsList->updateList(cof);

		mIsInitialized = true;
	}
}

// virtual
void LLPanelWearing::setFilterSubString(const std::string& string)
{
	sFilterSubString = string;
	mCOFItemsList->setFilterSubString(sFilterSubString);
}

// virtual
bool LLPanelWearing::isActionEnabled(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();

	if (command_name == "save_outfit")
	{
		bool outfit_locked = LLAppearanceMgr::getInstance()->isOutfitLocked();
		bool outfit_dirty = LLAppearanceMgr::getInstance()->isOutfitDirty();
		// allow save only if outfit isn't locked and is dirty
		return !outfit_locked && outfit_dirty;
	}

	if (command_name == "take_off")
	{
		return hasItemSelected() && canTakeOffSelected();
	}

// [SL:KB] - Patch: Inventory-AttachmentActions - Checked: 2012-05-15 (Catznip-3.3)
	uuid_vec_t selected_uuids;
	getSelectedItemsUUIDs(selected_uuids);

	if (command_name == "touch_attach")
	{
		return (1 == selected_uuids.size()) && (enable_attachment_touch(selected_uuids.front()));
	}

	if (command_name == "edit_item")
	{
		return (1 == selected_uuids.size()) && (enable_item_edit(selected_uuids.front()));
	}
// [/SL:KB]

	return false;
}

boost::signals2::connection LLPanelWearing::setSelectionChangeCallback(commit_callback_t cb)
{
	if (!mCOFItemsList) return boost::signals2::connection();

	return mCOFItemsList->setCommitCallback(cb);
}

void LLPanelWearing::onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
	LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(ctrl);
	if (!list) return;

	uuid_vec_t selected_uuids;

	list->getSelectedUUIDs(selected_uuids);

	mContextMenu->show(ctrl, selected_uuids, x, y);
}

bool LLPanelWearing::hasItemSelected()
{
	return mCOFItemsList->getSelectedItem() != NULL;
}

void LLPanelWearing::getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const
{
	mCOFItemsList->getSelectedUUIDs(selected_uuids);
}

void LLPanelWearing::copyToClipboard()
{
	std::string text;
	std::vector<LLSD> data;
	mCOFItemsList->getValues(data);

	for(std::vector<LLSD>::const_iterator iter = data.begin(); iter != data.end();)
	{
		LLSD uuid = (*iter);
		LLViewerInventoryItem* item = gInventory.getItem(uuid);

		iter++;
		if (item != NULL)
		{
			// Append a newline to all but the last line
			text += iter != data.end() ? item->getName() + "\n" : item->getName();
		}
	}

	LLClipboard::instance().copyToClipboard(utf8str_to_wstring(text),0,text.size());
}
// EOF
