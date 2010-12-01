/** 
 *
 * Copyright (c) 2010, Kitty Barnett
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

#include "llappearancemgr.h"
#include "llfoldervieweventlistener.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "llmenubutton.h"
#include "llnotificationsutil.h"
#include "lloutfitsview.h"
#include "lltoggleablemenu.h"
#include "llviewermenu.h"

// ============================================================================

// Returns the UUID of the items' common parent (or a null UUID if the items don't all belong to the same parent)
LLUUID get_items_parent(const LLInventoryModel::item_array_t& items)
{
	LLUUID idParent;
	for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
	{
		const LLViewerInventoryItem* pItem = itItem->get();
		if (!pItem)
			continue;

		if (idParent.isNull())
			idParent = pItem->getParentUUID();
		else if (idParent != pItem->getParentUUID())
			return LLUUID::null;
	}
	return idParent;
}

// Returns TRUE if the item is something that can be worn (wearables, attachments and gestures)
bool get_item_wearable(const LLInventoryItem* pItem)
{
	if (pItem)
	{
		switch (pItem->getType())
		{
			case LLAssetType::AT_OBJECT:
			case LLAssetType::AT_BODYPART:
			case LLAssetType::AT_CLOTHING:
			case LLAssetType::AT_GESTURE:
				return true;
			default:
				return false;
		}
	}
	return false;
}

bool get_item_wearable(const LLUUID& idItem)
{
	return get_item_wearable(gInventory.getItem(idItem));
}

// Returns TRUE if every item is something that can be worn (wearables, attachments and gestures)
bool get_items_wearable(const LLInventoryModel::item_array_t& items)
{
	bool fWearable = true;
	for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); (itItem != items.end()) && (fWearable); ++itItem)
	{
		const LLViewerInventoryItem* pItem = itItem->get();
		if (!pItem)
			continue;
		fWearable = get_item_wearable(pItem);
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

bool get_selected_items(LLInventoryPanel* pInvPanel, LLInventoryModel::item_array_t& items)
{
	items.clear();

	if ( (!pInvPanel) || (!pInvPanel->getRootFolder()) )
		return false;
	std::set<LLUUID> selItems = pInvPanel->getRootFolder()->getSelectionList();
	if (selItems.empty())
		return false;

	for (std::set<LLUUID>::const_iterator itItem = selItems.begin(); itItem != selItems.end(); itItem++)
	{
		LLViewerInventoryItem* pItem = pInvPanel->getModel()->getItem(*itItem);
		if (!pItem)
		{
			// Bit of a hack but if there are categories selected then we don't want to show any actions so we return an empty selection
			if (pInvPanel->getModel()->getCategory(*itItem))
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

// ============================================================================
// LLOutfitListGearMenu helper class

class LLOutfitsViewGearMenu
{
public:
	LLOutfitsViewGearMenu(LLOutfitsView* pOutfitsView)
		: mOutfitsView(pOutfitsView),
		  mMenu(NULL)
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

		registrar.add("OutfitsView.Gear.CloseFolders", boost::bind(&LLOutfitsViewGearMenu::onCloseFolders, this));
		registrar.add("OutfitsView.Gear.OnAction", boost::bind(&LLOutfitsViewGearMenu::onAction, this, _2));

		enable_registrar.add("OutfitsView.Gear.OnEnable", boost::bind(&LLOutfitsViewGearMenu::onEnable, this, _2));
		enable_registrar.add("OutfitsView.Gear.OnVisible", boost::bind(&LLOutfitsViewGearMenu::onVisible, this, _2));

		mMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
			"menu_outfitview_gear.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	}

	LLToggleableMenu* getMenu() { return mMenu; }

protected:
	void onCloseFolders()
	{
		LLInventoryPanel* pInvPanel = mOutfitsView->getPanel();
		if (!pInvPanel)
			return;

		LLFolderView* pRootFolder = pInvPanel->getRootFolder();
		if (!pRootFolder)
			return;

		LLFolderViewFolder* pStartFolder = pRootFolder->getFolderByID(pInvPanel->getStartFolderID());
		if (!pStartFolder)
			return;

		// Close all the folders except the start folder
		pStartFolder->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_DOWN);
		pStartFolder->setOpen(TRUE);
		pRootFolder->arrangeAll();
	}

	void onAction(LLSD::String sdParam)
	{
		mOutfitsView->performAction(sdParam);
	}

	bool onEnable(LLSD::String sdParam)
	{
		return mOutfitsView->isActionEnabled(sdParam);
	}

	bool onVisible(LLSD::String sdParam)
	{
		return true;
	}

protected:
	LLOutfitsView*			mOutfitsView;
	LLToggleableMenu*		mMenu;
};

// ============================================================================

static LLRegisterPanelClassWrapper<LLOutfitsView> t_outfits_view("outfits_view");

LLOutfitsView::LLOutfitsView()
	: LLPanelOutfitsTab()
	, mInvPanel(NULL)
	, mGearMenu(NULL)
	, mSavedFolderState(NULL)
	, mFetchStarted(false)
	, mItemSelection(false)
	, mOutfitSelection(false)
{
	mGearMenu = new LLOutfitsViewGearMenu(this);

	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
}

LLOutfitsView::~LLOutfitsView()
{
	delete mGearMenu;
	delete mSavedFolderState;
}

// virtual - Checked: 2010-11-09 (Catznip-2.4.0a) | Added: Catznip-2.4.0a
BOOL LLOutfitsView::postBuild()
{
	mInvPanel = getChild<LLInventoryPanel>("outfits_invpanel");
	mInvPanel->setSelectCallback(boost::bind(&LLOutfitsView::onSelectionChange, this, _1, _2));

	LLMenuButton* pGearBtn = getChild<LLMenuButton>("options_gear_btn");
	pGearBtn->setMenu(mGearMenu->getMenu());

	return TRUE;
}

//virtual
void LLOutfitsView::onOpen(const LLSD& /*info*/)
{
	if ( (!mFetchStarted) && (gInventory.isInventoryUsable()) )
	{
		const LLUUID idOutfits = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

		// Grab all the folders under "My Outfits"
		LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
		gInventory.collectDescendents(idOutfits, folders, items, FALSE);

		// Add them to the "to fetch" list
		uuid_vec_t idFolders;
		idFolders.push_back(idOutfits);
		for (S32 idxFolder = 0, cntFolder = folders.count(); idxFolder < cntFolder; idxFolder++)
			idFolders.push_back(folders.get(idxFolder)->getUUID());

		llinfos << "Starting fetch of " << idFolders.size() << " outfit folders" << llendl;

		// Now fetch them all in one go
		LLInventoryFetchDescendentsObserver fetcher = LLInventoryFetchDescendentsObserver(idFolders);
		fetcher.startFetch();

		mFetchStarted = true;
	}
}

// virtual - Checked: 2010-11-09 (Catznip-2.4.0a) | Added: Catznip-2.4.0a
void LLOutfitsView::setFilterSubString(const std::string& strFilter)
{
	if (mFilterSubString == strFilter)
		return;

	if (!strFilter.empty())
	{
		// Save folder open state if no filter currently applied
		if (!mInvPanel->getRootFolder()->isFilterModified())
		{
			mSavedFolderState->setApply(FALSE);
			mInvPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		}
	}
	else
	{
		// Restore saved open folder state
		mSavedFolderState->setApply(TRUE);
		mInvPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);

		LLOpenFoldersWithSelection opener;
		mInvPanel->getRootFolder()->applyFunctorRecursively(opener);
		mInvPanel->getRootFolder()->scrollToShowSelection();
	}

	mFilterSubString = strFilter;
	mInvPanel->setFilterSubString(strFilter);
}

// virtual - Checked: 2010-11-09 (Catznip-2.4.0a) | Added: Catznip-2.4.0a
bool LLOutfitsView::canWearSelected()
{
	LLInventoryModel::item_array_t items;
	if (get_selected_items(mInvPanel, items))
		return get_items_wearable(items);
	return false;
}

// virtual
bool LLOutfitsView::isActionEnabled(const LLSD& sdParam)
{
	if (mSelectedCategory.isNull())
		return false;

	const std::string strAction = sdParam.asString();
	if ( ("delete" == strAction) || ("delete_outfit" == strAction) )
	{
		return (!mItemSelection) && (mOutfitSelection) && (LLAppearanceMgr::instance().getCanRemoveOutfit(mSelectedCategory));
	}
	else if ( ("rename" == strAction) || ("rename_outfit" == strAction) )
	{
		return (mOutfitSelection) && (get_is_category_renameable(&gInventory, mSelectedCategory));
	}
	else if ("save_outfit" == strAction)
	{
		bool outfit_locked = LLAppearanceMgr::getInstance()->isOutfitLocked();
		bool outfit_dirty = LLAppearanceMgr::getInstance()->isOutfitDirty();
		// allow save only if outfit isn't locked and is dirty
		return !outfit_locked && outfit_dirty;
	}
	else if ("wear" == strAction)
	{
		if (gAgentWearables.isCOFChangeInProgress())
			return false;
		if (hasItemSelected())
			return canWearSelected();
		if (mOutfitSelection)
			return LLAppearanceMgr::instance().getCanReplaceCOF(mSelectedCategory);
		return false;
	}
	else if ("take_off" == strAction)
	{
		// Enable "Take Off" if any of selected items can be taken off
		// or the selected outfit contains items that can be taken off.
		return ( (hasItemSelected()) && (canTakeOffSelected()) ) || 
			( (!hasItemSelected()) && (LLAppearanceMgr::getCanRemoveFromCOF(mSelectedCategory)) );
	}
	else if ("wear_add" == strAction)
	{
		// *TODO: do we ever get here?
		return LLAppearanceMgr::getCanAddToCOF(mSelectedCategory);
	}
	return false;
}

// virtual - Checked: 2010-11-09 (Catznip-2.4.0a) | Added: Catznip-2.4.0a
void LLOutfitsView::getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const
{
	std::set<LLUUID> selItems = mInvPanel->getRootFolder()->getSelectionList();
	selected_uuids.resize(selItems.size());
	std::copy(selItems.begin(), selItems.end(), selected_uuids.begin());
}

// virtual - Checked: 
void LLOutfitsView::performAction(std::string strAction)
{
	if (!isActionEnabled(strAction))
		return;

	LLViewerInventoryCategory* pSelectedCat = gInventory.getCategory(mSelectedCategory);
	if ( (!pSelectedCat) || (LLFolderType::FT_OUTFIT != pSelectedCat->getPreferredType()) )
		return;

	if ("replaceoutfit" == strAction)
	{
		LLAppearanceMgr::instance().wearInventoryCategory(pSelectedCat, FALSE, FALSE);
	}
	else if ("addtooutfit" == strAction)
	{
		LLAppearanceMgr::instance().wearInventoryCategory(pSelectedCat, FALSE, TRUE);
	}
	else if ( ("rename" == strAction) || ("rename_outfit" == strAction) )
	{
		LLAppearanceMgr::instance().renameOutfit(mSelectedCategory);
	}
	else if ( ("delete" == strAction) || ("delete_outfit" == strAction) )
	{
		removeSelected();
	}
}

// virtual - Checked: 2010-11-30 (Catznip-2.4.0g) | Added: Catznip-2.4.0g
void LLOutfitsView::removeSelected()
{
	LLNotificationsUtil::add("DeleteOutfits", LLSD(), LLSD(), boost::bind(&LLOutfitsView::onOutfitsRemovalConfirmation, this, _1, _2));
}

// virtual - Checked: 2010-11-30 (Catznip-2.4.0g) | Added: Catznip-2.4.0g
void LLOutfitsView::onOutfitsRemovalConfirmation(const LLSD& notification, const LLSD& response)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(notification, response);
	if (idxOption != 0)
		return; // canceled

	// Copy/paste of the "delete" check in LLOutfitsView::isActionEnabled
	if ( (!mItemSelection) && (mOutfitSelection) && (LLAppearanceMgr::instance().getCanRemoveOutfit(mSelectedCategory)) )
		remove_category(&gInventory, mSelectedCategory);
}

// virtual - Checked: 2010-11-09 (Catznip-2.4.0a) | Added: Catznip-2.4.0a
void LLOutfitsView::setSelectedOutfitByUUID(const LLUUID& idOutfit)
{
	LLFolderView* pRootFolder = mInvPanel->getRootFolder();
	LLFolderViewItem *pOutfitFolder = pRootFolder->getItemByID(idOutfit);
	if (pOutfitFolder)
	{
		pOutfitFolder->setOpen(!pOutfitFolder->isOpen());
		pRootFolder->setSelectionFromRoot(pOutfitFolder, TRUE);
		pRootFolder->scrollToShowSelection();
	}
}

// virtual - Checked: 2010-11-09 (Catznip-2.4.0a) | Added: Catznip-2.4.0a
void LLOutfitsView::wearSelectedItems()
{
	// NOTE-Catznip: this will only work correctly if the Inventory-MultiXXX patch branches are merged in as well
	if (hasItemSelected())
		mInvPanel->doToSelected(LLSD("wear"));
}

// virtual - Checked: 2010-11-09 (Catznip-2.4.0a) | Added: Catznip-2.4.0a
bool LLOutfitsView::hasItemSelected()
{
	return mItemSelection;
}

// Checked: 2010-11-09 (Catznip-2.4.0a) | Added: Catznip-2.4.0a
void LLOutfitsView::onSelectionChange(const std::deque<LLFolderViewItem*> &selItems, BOOL fUserAction)
{
	mItemSelection = false;
	mOutfitSelection = false;
	mSelectedCategory.setNull();

	if (1 == selItems.size())
	{
		// One selected inventory object, check if it's a folder
		const LLFolderViewFolder* pFolder = dynamic_cast<const LLFolderViewFolder*>(*selItems.begin());
		if (pFolder)
		{
			mSelectedCategory = pFolder->getListener()->getUUID();
		}
		else
		{
			mItemSelection = true;
			const LLFolderViewFolder* pFolder = (*selItems.begin())->getParentFolder();
			if (pFolder)
				mSelectedCategory = pFolder->getListener()->getUUID();
		}
	}
	else
	{
		// If more than one inventory object is selected we need to make sure it isn't a mixed selection of items and categories
		LLInventoryModel::item_array_t items;
		if (get_selected_items(mInvPanel, items))
		{
			// Selection consists solely of inventory items
			mItemSelection = true;
			mSelectedCategory = get_items_parent(items);
		}
	}

	if (mSelectedCategory.notNull())
	{
		const LLViewerInventoryCategory* pSelectedCat = gInventory.getCategory(mSelectedCategory);
		mOutfitSelection = (pSelectedCat) && (LLFolderType::FT_OUTFIT == pSelectedCat->getPreferredType());
	}

	// URGENT-Catznip: LLPanelOutfitsInventory doesn't currently use the param but we should still try and pass something meaningful
	mSelectionChangeSignal(LLUUID::null);
}

// virtual - Checked: 2010-11-09 (Catznip-2.4.0a) | Added: Catznip-2.4.0a
boost::signals2::connection LLOutfitsView::setSelectionChangeCallback(selection_change_callback_t cb)
{
	return mSelectionChangeSignal.connect(cb);
}

// ============================================================================
// EOF
