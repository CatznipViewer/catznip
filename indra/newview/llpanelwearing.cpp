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
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
#include "llinventorypanel.h"
// [/SL:KB]
#include "llmenubutton.h"
#include "llviewermenu.h"
#include "llwearableitemslist.h"
#include "llsdserialize.h"
#include "llclipboard.h"
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
#include "llviewercontrol.h"
// [/SL:KB]

// Context menu and Gear menu helper.
static void edit_outfit()
{
	LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "edit_outfit"));
}

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-14 (Catznip-3.3)
//////////////////////////////////////////////////////////////////////////

class LLWornListItem : public LLPanelClothingListItem
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanelClothingListItem::Params>
	{
		Params() {}
	};
protected:
	LLWornListItem(LLViewerInventoryItem* pItem, const Params& p);
public:
	/*virtual*/ ~LLWornListItem();
	/*virtual*/ S32  notify(const LLSD& sdInfo);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void updateItem(const std::string& strName, EItemState itemState = IS_DEFAULT);

public:
	bool getShowOrdering() { return mShowOrdering; }
	void setShowOrdering(bool fShowOrdering);
	static LLWornListItem* create(LLViewerInventoryItem* pItem);
protected:
	void onMoveWearable(bool fDown);

protected:
	bool                  mShowOrdering;
	LLAssetType::EType    mAssetType;
	LLWearableType::EType mWearableType;
};

LLWornListItem::LLWornListItem(LLViewerInventoryItem* pItem, const Params& p)
	: LLPanelClothingListItem(pItem, p)
	, mShowOrdering(true)
	, mAssetType(pItem->getType())
	, mWearableType( (LLAssetType::AT_CLOTHING == pItem->getType()) ? pItem->getWearableType() : LLWearableType::WT_INVALID)
{
	setReshapeWidgetMask(SIDE_LEFT | getReshapeWidgetMask());
}

LLWornListItem::~LLWornListItem()
{
}

// static
LLWornListItem* LLWornListItem::create(LLViewerInventoryItem* pItem)
{
	LLWornListItem* pListItem = NULL;
	if (pItem)
	{
		const LLWornListItem::Params& params = LLUICtrlFactory::getDefaultParams<LLWornListItem>();
		pListItem = new LLWornListItem(pItem, params);
		pListItem->initFromParams(params);
		pListItem->postBuild();
	}
	return pListItem;
}

void LLWornListItem::onMoveWearable(bool fDown)
{
	LLAppearanceMgr::getInstance()->moveWearable(getItem(), fDown, true);
}

S32 LLWornListItem::notify(const LLSD& sdInfo)
{
	if(sdInfo.has("show_ordering"))
	{
		setShowOrdering(sdInfo["show_ordering"].asBoolean());
		return 0;
	}
	return LLPanelClothingListItem::notify(sdInfo);
}

BOOL LLWornListItem::postBuild()
{
	LLPanelClothingListItem::postBuild();

	setShowDeleteButton(false);
	setShowLockButton(false);
	setShowEditButton(false);

	setShowMoveUpButton(false);
	getChild<LLUICtrl>("btn_move_up")->setCommitCallback(boost::bind(&LLWornListItem::onMoveWearable, this, false));
	setShowMoveDownButton(false);
	getChild<LLUICtrl>("btn_move_down")->setCommitCallback(boost::bind(&LLWornListItem::onMoveWearable, this, true));

	return TRUE;
}

void LLWornListItem::setShowOrdering(bool fShowOrdering)
{
	if (mShowOrdering != fShowOrdering)
	{
		mShowOrdering = fShowOrdering;
		setNeedsRefresh(true);
	}
}

void LLWornListItem::updateItem(const std::string& strName, EItemState itemState)
{
	LLPanelClothingListItem::updateItem(strName, itemState);

	bool fShowUp = false, fShowDown = false;
	if ( (mShowOrdering) && (LLAssetType::AT_CLOTHING == mAssetType) )
	{
		U32 cntWearable = gAgentWearables.getWearableCount(mWearableType);
		if (cntWearable > 1)
		{
			U32 idxWearable = gAgentWearables.getWearableIndex(gAgentWearables.getWearableFromItemID(mInventoryItemUUID));
			fShowDown = (idxWearable > 0);
			fShowUp = (idxWearable < cntWearable - 1);
		}
	}
	setShowMoveUpButton(fShowUp);
	setShowMoveDownButton(fShowDown);
}

//////////////////////////////////////////////////////////////////////////

class LLWornItemsList : public LLWearableItemsList
{
public:
	struct Params : public LLInitParam::Block<Params, LLWearableItemsList::Params>
	{
		Params() {}
	};
protected:
	friend class LLUICtrlFactory;
	LLWornItemsList(const LLWornItemsList::Params& p);

public:
	/*virtual*/ void setSortOrder(ESortOrder sortOrder, bool sortNow = true);
protected:
	/*virtual*/ void addNewItem(LLViewerInventoryItem* item, bool rearrange /*= true*/);
};

static const LLDefaultChildRegistry::Register<LLWornItemsList> r("wearing_items_list");

LLWornItemsList::LLWornItemsList(const LLWornItemsList::Params& p)
	: LLWearableItemsList(p)
{
}

void LLWornItemsList::addNewItem(LLViewerInventoryItem* pItem, bool rearrange)
{
	if (!pItem)
	{
		llwarns << "No inventory item. Couldn't create new item." << llendl;
		llassert(pItem != NULL);
	}

	LLPanelClothingListItem* pListItem = LLWornListItem::create(pItem);
	if (!pListItem)
		return;

	bool fAdded = addItem(pListItem, pItem->getUUID(), ADD_BOTTOM, rearrange);
	if (!fAdded)
	{
		llwarns << "Couldn't add new item." << llendl;
		llassert(fAdded);
	}
}

void LLWornItemsList::setSortOrder(ESortOrder sortOrder, bool sortNow)
{
	LLWearableItemsList::setSortOrder(sortOrder, sortNow);

	const LLWearableListItemComparator* pComparator = dynamic_cast<const LLWearableListItemComparator*>(mItemComparator);
	bool fOrdered = (pComparator) && (pComparator->areWearablesOrdered());
	notifyItems(LLSD().with("show_ordering", fOrdered));
}
// [/SL:KB]

//////////////////////////////////////////////////////////////////////////

class LLWearingGearMenu
{
public:
	LLWearingGearMenu(LLPanelWearing* panel_wearing)
	:	mMenu(NULL), mPanelWearing(panel_wearing)
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

		registrar.add("Gear.Edit", boost::bind(&edit_outfit));
		registrar.add("Gear.TakeOff", boost::bind(&LLWearingGearMenu::onTakeOff, this));
		registrar.add("Gear.Copy", boost::bind(&LLPanelWearing::copyToClipboard, mPanelWearing));

		enable_registrar.add("Gear.OnEnable", boost::bind(&LLPanelWearing::isActionEnabled, mPanelWearing, _2));

		mMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
			"menu_wearing_gear.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		llassert(mMenu);
	}

	LLToggleableMenu* getMenu() { return mMenu; }

private:
	void onTakeOff()
	{
		uuid_vec_t selected_uuids;
		mPanelWearing->getSelectedItemsUUIDs(selected_uuids);

		for (uuid_vec_t::const_iterator it=selected_uuids.begin(); it != selected_uuids.end(); ++it)
		{
				LLAppearanceMgr::instance().removeItemFromAvatar(*it);
		}
	}

	LLToggleableMenu*		mMenu;
	LLPanelWearing* 		mPanelWearing;
};

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
class LLWearingSortMenu
{
public:
	LLWearingSortMenu(LLPanelWearing* pWearingPanel)
		: mFolderMenu(NULL), mListMenu(NULL), mWearingPanel(pWearingPanel)
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		registrar.add("Sort.Folder", boost::bind(&LLWearingSortMenu::onChangeFolderSortOrder, this, _2));
		registrar.add("Sort.List", boost::bind(&LLWearingSortMenu::onChangeListSortOrder, this, _2));

		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
		enable_registrar.add("Sort.CheckFolder", boost::bind(&LLWearingSortMenu::onCheckFolderSortOrder, this, _2));
		enable_registrar.add("Sort.CheckList", boost::bind(&LLWearingSortMenu::onCheckListSortOrder, this, _2));

		mFolderMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
			"menu_wearing_sort_folder.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		llassert(mFolderMenu);
		mListMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
			"menu_wearing_sort_list.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		llassert(mListMenu);
	}

	LLToggleableMenu* getFolderMenu() { return mFolderMenu; }
	LLToggleableMenu* getListMenu()    { return mListMenu; }

protected:
	void onChangeFolderSortOrder(const LLSD& sdParam)
	{
		if (mWearingPanel->getInvPanel())
		{
			mWearingPanel->getInvPanel()->setSortBy(sdParam);
			gSavedSettings.setU32("WearingFolderSortOrder", mWearingPanel->getInvPanel()->getSortOrder());
		}
	}

	bool onCheckFolderSortOrder(const LLSD& sdParam)
	{
		const std::string strParam = sdParam.asString();
		if (mWearingPanel->getInvPanel())
		{
			U32 nSortOderMask = mWearingPanel->getInvPanel()->getSortOrder();
			if ("name" == strParam)
				return ~nSortOderMask & LLInventoryFilter::SO_DATE;
			else if ("date" == strParam)
				return nSortOderMask & LLInventoryFilter::SO_DATE;
			else if ("foldersalwaysbyname" == strParam)
				return nSortOderMask & LLInventoryFilter::SO_FOLDERS_BY_NAME;
			else if ("systemfolderstotop" == strParam)
				return nSortOderMask & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP;
		}
		return false;
	}

	void onChangeListSortOrder(const LLSD& sdParam)
	{
		const std::string strParam = sdParam.asString();
		if ("appearance" == strParam)
			mWearingPanel->getItemsList()->setSortOrder(LLWearableItemsList::E_SORT_BY_APPEARANCE);
		else if ("name" == strParam)
			mWearingPanel->getItemsList()->setSortOrder(LLWearableItemsList::E_SORT_BY_NAME);
		else if ("type_name" == strParam)
			mWearingPanel->getItemsList()->setSortOrder(LLWearableItemsList::E_SORT_BY_TYPE_NAME);
		gSavedSettings.setU32("WearingListSortOrder", mWearingPanel->getItemsList()->getSortOrder());
	}

	bool onCheckListSortOrder(const LLSD& sdParam)
	{
		const std::string strParam = sdParam.asString();
		if ("appearance" == strParam)
			return LLWearableItemsList::E_SORT_BY_APPEARANCE == mWearingPanel->getItemsList()->getSortOrder();
		else if ("name" == strParam)
			return LLWearableItemsList::E_SORT_BY_NAME == mWearingPanel->getItemsList()->getSortOrder();
		else if ("type_name" == strParam)
			return LLWearableItemsList::E_SORT_BY_TYPE_NAME == mWearingPanel->getItemsList()->getSortOrder();
		return false;
	}

protected:
	LLToggleableMenu* mFolderMenu;
	LLToggleableMenu* mListMenu;
	LLPanelWearing*   mWearingPanel;
};
// [/SL:KB]

//////////////////////////////////////////////////////////////////////////

class LLWearingContextMenu : public LLListContextMenu
{
protected:
	/* virtual */ LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

		functor_t take_off = boost::bind(&LLAppearanceMgr::removeItemFromAvatar, LLAppearanceMgr::getInstance(), _1);

		registrar.add("Wearing.Edit", boost::bind(&edit_outfit));
		registrar.add("Wearing.TakeOff", boost::bind(handleMultiple, take_off, mUUIDs));
		registrar.add("Wearing.Detach", boost::bind(handleMultiple, take_off, mUUIDs));
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-12 (Catznip-3.3)
		registrar.add("Wearing.FindOriginal", boost::bind(&LLWearingContextMenu::onFindOriginal, this));
		registrar.add("Wearing.Properties", boost::bind(&LLWearingContextMenu::onProperties, this));
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
		bool allow_detach = !bp_selected && !clothes_selected && attachments_selected;
		bool allow_take_off = !bp_selected && clothes_selected && !attachments_selected;

		menu->setItemVisible("take_off",	allow_take_off);
		menu->setItemVisible("detach",		allow_detach);
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-12 (Catznip-3.3)
		menu->setItemEnabled("find_original", 1 == mUUIDs.size());
		menu->setItemEnabled("properties", 1 == mUUIDs.size());
// [/SL:KB]
		menu->setItemVisible("edit_outfit_separator", allow_take_off || allow_detach);
	}

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-12 (Catznip-3.3)
	void onFindOriginal()
	{
		const LLUUID& idItem = (!mUUIDs.empty()) ? mUUIDs.front() : LLUUID::null;
		if (idItem.notNull())
		{
			LLInventoryPanel* pInvPanel = LLInventoryPanel::getActiveInventoryPanel();
			if (pInvPanel)
				pInvPanel->setSelection(gInventory.getLinkedItemID(idItem), TAKE_FOCUS_YES);
		}
	}

	void onProperties()
	{
		const LLUUID& idItem = (!mUUIDs.empty()) ? mUUIDs.front() : LLUUID::null;
		if (idItem.notNull())
			show_item_profile(gInventory.getLinkedItemID(idItem));
	}
// [/SL:KB]
};

//////////////////////////////////////////////////////////////////////////

std::string LLPanelAppearanceTab::sFilterSubString = LLStringUtil::null;

static LLRegisterPanelClassWrapper<LLPanelWearing> t_panel_wearing("panel_wearing");

LLPanelWearing::LLPanelWearing()
	:	LLPanelAppearanceTab()
	,	mCOFItemsList(NULL)
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	,	mInvPanel(NULL)
	,	mSortMenuButton(NULL)
	,	mToggleFolderView(NULL)
	,	mToggleListView(NULL)
// [/SL:KB]
	,	mIsInitialized(false)
{
	mCategoriesObserver = new LLInventoryCategoriesObserver();

	mGearMenu = new LLWearingGearMenu(this);
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	mSortMenu = new LLWearingSortMenu(this);
// [/SL:KB]
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
//	mCOFItemsList = getChild<LLWearableItemsList>("cof_items_list");
//	mCOFItemsList->setRightMouseDownCallback(boost::bind(&LLPanelWearing::onWearableItemsListRightClick, this, _1, _2, _3));
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	mCOFItemsList = getChild<LLWornItemsList>("cof_items_list");
	mCOFItemsList->setRightMouseDownCallback(boost::bind(&LLPanelWearing::onWearableItemsListRightClick, this, _1, _2, _3));
	mCOFItemsList->setSortOrder((LLWearableItemsList::ESortOrder)gSavedSettings.getU32("WearingListSortOrder"));
// [/SL:KB]

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	getChild<LLMenuButton>("options_gear_btn")->setMenu(mGearMenu->getMenu());
	mSortMenuButton = getChild<LLMenuButton>("options_sort_btn");

	mToggleFolderView = getChild<LLButton>("folder_view_btn");
	mToggleFolderView->setCommitCallback(boost::bind(&LLPanelWearing::onToggleWearingView, this, FOLDER_VIEW));
	mToggleListView = getChild<LLButton>("list_view_btn");
	mToggleListView->setCommitCallback(boost::bind(&LLPanelWearing::onToggleWearingView, this, LIST_VIEW));
// [/SL:KB]
//	LLMenuButton* menu_gear_btn = getChild<LLMenuButton>("options_gear_btn");
//
//	menu_gear_btn->setMenu(mGearMenu->getMenu());

	return TRUE;
}

//virtual
void LLPanelWearing::onOpen(const LLSD& /*info*/)
{
	if (!mIsInitialized)
	{
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
		// Delay creating the inventory view until the user actually opens this panel
		onToggleWearingView((EWearingView)gSavedSettings.getU32("WearingViewType"));
// [/SL:KB]

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

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
void LLPanelWearing::onToggleWearingView(EWearingView eView)
{
	if (FOLDER_VIEW == eView)
	{
		if ( (mInvPanel) || (mInvPanel = createInventoryPanel()) )
		{
			mCOFItemsList->setVisible(false);
			mInvPanel->setVisible(true);
		}
	}
	else
	{
		mCOFItemsList->setVisible(true);
		if (mInvPanel)
			mInvPanel->setVisible(false);
	}
	mSortMenuButton->setMenu( (FOLDER_VIEW == eView) ? mSortMenu->getFolderMenu() : mSortMenu->getListMenu());
	mToggleFolderView->setToggleState(FOLDER_VIEW == eView);
	mToggleListView->setToggleState(LIST_VIEW == eView);
	gSavedSettings.setU32("WearingViewType", eView);
}

LLInventoryPanel* LLPanelWearing::createInventoryPanel() const
{
	if (mInvPanel)
		return mInvPanel;

	LLView* pInvPanelPlaceholder = findChild<LLView>("wearing_invpanel_placeholder");
	
	LLInventoryPanel* pInvPanel = LLUICtrlFactory::createFromFile<LLInventoryPanel>("panel_outfits_wearing_invpanel.xml", pInvPanelPlaceholder->getParent(), LLInventoryPanel::child_registry_t::instance());
	pInvPanel->setShape(pInvPanelPlaceholder->getRect());
	pInvPanel->setFilterLinks(LLInventoryFilter::FILTERLINK_EXCLUDE_LINKS);
	pInvPanel->setFilterWorn(true);
	pInvPanel->setSortOrder(gSavedSettings.getU32("WearingFolderSortOrder"));
//	pInvPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	pInvPanel->getFilter()->markDefault();
	pInvPanel->openAllFolders();
//	pInvPanel->setSelectCallback(boost::bind(&LLPanelMainInventory::onSelectionChange, this, recent_items_panel, _1, _2));

	return pInvPanel;
}
// [/SL:KB]

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
