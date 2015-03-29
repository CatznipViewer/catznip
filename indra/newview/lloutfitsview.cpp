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
#include "llfolderview.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "llmenubutton.h"
#include "llnotificationsutil.h"
#include "lloutfitobserver.h"
#include "lloutfitsview.h"
#include "lltoggleablemenu.h"
#include "llviewermenu.h"

// ============================================================================
// LLOutfitListGearMenu helper class
//

class LLOutfitsViewGearMenu
{
public:
	LLOutfitsViewGearMenu(LLOutfitsView* pOutfitsView)
		: mOutfitsView(pOutfitsView),
		  mMenu(NULL)
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

		registrar.add("OutfitsView.Gear.CloseFolders", boost::bind(&LLOutfitsView::closeAllFolders, mOutfitsView));
		registrar.add("OutfitsView.Gear.OnAction", boost::bind(&LLOutfitsViewGearMenu::onAction, this, _2));

		enable_registrar.add("OutfitsView.Gear.OnEnable", boost::bind(&LLOutfitsViewGearMenu::onEnable, this, _2));
		enable_registrar.add("OutfitsView.Gear.OnVisible", boost::bind(&LLOutfitsViewGearMenu::onVisible, this, _2));

		mMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
			"menu_outfitview_gear.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	}

	LLToggleableMenu* getMenu() { return mMenu; }

protected:
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

//static LLRegisterPanelClassWrapper<LLOutfitsView> t_outfits_view("outfits_view");

LLOutfitsView::LLOutfitsView()
	: LLPanelOutfitsTab()
	, mInvPanel(NULL)
	, mGearMenu(NULL)
	, mSavedFolderState(NULL)
	, mInitialized(false)
	, mItemSelection(false)
	, mOutfitSelection(false)
{
	setXMLFilename("panel_outfits_view.xml");

	mGearMenu = new LLOutfitsViewGearMenu(this);

	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
}

LLOutfitsView::~LLOutfitsView()
{
	delete mGearMenu;
	delete mSavedFolderState;
}

// virtual
BOOL LLOutfitsView::postBuild()
{
	// EXP-915 (Revision aa1ada3f8878) broke being able to initialize a start folder so we're working around it by dynamically creating
	// the inventory panel after the inventory has reached a usuable state
	doOnIdleRepeating(boost::bind(&LLOutfitsView::onIdle, this));

	getChild<LLMenuButton>("options_gear_btn")->setMenu(mGearMenu->getMenu());
	getChild<LLUICtrl>("collapse_btn")->setCommitCallback(boost::bind(&LLOutfitsView::closeAllFolders, this));

	return TRUE;
}

bool LLOutfitsView::onIdle()
{
	if ( (gInventory.isInventoryUsable()) && (!mInvPanel) )
	{
		LLView *pInvPanelPlaceholder = findChild<LLView>("outfits_invpanel_placeholder");
	
		mInvPanel = LLUICtrlFactory::createFromFile<LLInventoryPanel>("panel_outfits_invpanel.xml", pInvPanelPlaceholder->getParent(), LLInventoryPanel::child_registry_t::instance());
		mInvPanel->setShape(pInvPanelPlaceholder->getRect());
		mInvPanel->setSelectCallback(boost::bind(&LLOutfitsView::onSelectionChange, this, _1, _2));

		mInvPanel->getFilter().markDefault();
		mInvPanel->getRootFolder()->setAutoSelectOverride(true);
		mInvPanel->getRootFolder()->closeAllFolders();
		highlightBaseOutfit();

		pInvPanelPlaceholder->setVisible(FALSE);
	}
	return (NULL != mInvPanel);
}

//virtual
void LLOutfitsView::onOpen(const LLSD& /*info*/)
{
	if ( (!mInitialized) && (gInventory.isInventoryUsable()) )
	{
		const LLUUID idOutfits = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

		// Grab all the folders under "My Outfits"
		LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
		gInventory.collectDescendents(idOutfits, folders, items, FALSE);

		// Add them to the "to fetch" list
		uuid_vec_t idFolders;
		idFolders.push_back(idOutfits);
		for (LLInventoryModel::cat_array_t::const_iterator itFolder = folders.begin(); itFolder != folders.end(); ++itFolder)
			idFolders.push_back((*itFolder)->getUUID());

		LL_INFOS() << "Starting fetch of " << idFolders.size() << " outfit folders" << LL_ENDL;

		// Now fetch them all in one go
		LLInventoryFetchDescendentsObserver fetcher = LLInventoryFetchDescendentsObserver(idFolders);
		fetcher.startFetch();

		// Select and open the current base outfit
		if (mInvPanel)
		{
			mInvPanel->getRootFolder()->closeAllFolders();
			highlightBaseOutfit();
		}
		LLOutfitObserver::instance().addBOFChangedCallback(boost::bind(&LLOutfitsView::highlightBaseOutfit, this));
		LLOutfitObserver::instance().addBOFReplacedCallback(boost::bind(&LLOutfitsView::highlightBaseOutfit, this));

		mInitialized = true;
	}
}

// virtual
void LLOutfitsView::setFilterSubString(const std::string& strFilter)
{
	if (mFilterSubString == strFilter)
		return;

	if (!strFilter.empty())
	{
		// Save folder open state if no filter currently applied
		if (!mInvPanel->getFilter().isNotDefault())
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

// virtual
bool LLOutfitsView::canWearSelected()
{
	LLInventoryModel::item_array_t items;
	if (mInvPanel->getSelectedItems(items))
	{
		for (LLInventoryModel::item_array_t::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
		{
			const LLViewerInventoryItem* pItem = *itItem;
			if ( (pItem) && (!get_can_item_be_worn(pItem->getUUID())) )
				return false;
		}
		return true;
	}
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
	else if ("replaceoutfit" == strAction)
	{
		return (mOutfitSelection) ? LLAppearanceMgr::instance().getCanReplaceCOF(mSelectedCategory) : false;
	}
	else if ("addtooutfit" == strAction)
	{
		return (mOutfitSelection) ? LLAppearanceMgr::getCanAddToCOF(mSelectedCategory) : false;
	}
	else if ("take_off" == strAction)
	{
		// Enable "Take Off" if any of selected items can be taken off
		// or the selected outfit contains items that can be taken off.
		return ( (hasItemSelected()) && (canTakeOffSelected()) ) || 
			( (!hasItemSelected()) && (LLAppearanceMgr::getCanRemoveFromCOF(mSelectedCategory)) );
	}
	else if ("takeoffoutfit" == strAction)
	{
		return (mOutfitSelection) ? LLAppearanceMgr::getCanRemoveFromCOF(mSelectedCategory) : false;
	}
	else if ("wear_add" == strAction)
	{
		// *TODO: do we ever get here?
		return LLAppearanceMgr::getCanAddToCOF(mSelectedCategory);
	}
	return false;
}

// virtual
void LLOutfitsView::getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const
{
	std::set<LLFolderViewItem*> selected_items = mInvPanel->getRootFolder()->getSelectionList();
	for (std::set<LLFolderViewItem*>::const_iterator itItem = selected_items.begin(); itItem != selected_items.end(); ++itItem)
	{
		const LLFolderViewItem* pItem = *itItem;
		if ( (pItem) && (pItem->getViewModelItem()) )
		{
			selected_uuids.push_back(static_cast<const LLFolderViewModelItemInventory*>(pItem->getViewModelItem())->getUUID());
		}
	}
}

void LLOutfitsView::highlightBaseOutfit()
{
	if ( (!mInvPanel) || (mInvPanel->getFilter().hasFilterString()) )
		return;

	const LLUUID idBOF = LLAppearanceMgr::getInstance()->getBaseOutfitUUID();
	if ( (idBOF.isNull()) || (mHighlightedFolder == idBOF) )
		return;

	if (mHighlightedFolder.notNull())
	{
		LLFolderViewFolder* pFolder = mInvPanel->getFolderByID(mHighlightedFolder);
		if ( (pFolder) && (pFolder->isOpen()) )
			pFolder->setOpenArrangeRecursively(FALSE);
		mHighlightedFolder.setNull();
	}

	LLFolderViewFolder* pBOFFolder = mInvPanel->getFolderByID(idBOF);
	if (pBOFFolder)
	{
	 	pBOFFolder->setOpen(TRUE);
		mInvPanel->getRootFolder()->setSelection(pBOFFolder, TRUE, TRUE);
		mInvPanel->getRootFolder()->scrollToShowSelection();
		mHighlightedFolder = idBOF;
	}
}

// virtual
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
	else if ("takeoffoutfit" == strAction)
	{
		LLAppearanceMgr::instance().takeOffOutfit(mSelectedCategory);
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

// virtual
void LLOutfitsView::removeSelected()
{
	LLNotificationsUtil::add("DeleteOutfits", LLSD(), LLSD(), boost::bind(&LLOutfitsView::onOutfitsRemovalConfirmation, this, _1, _2));
}

// virtual
void LLOutfitsView::onOutfitsRemovalConfirmation(const LLSD& notification, const LLSD& response)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(notification, response);
	if (idxOption != 0)
		return; // canceled

	// Copy/paste of the "delete" check in LLOutfitsView::isActionEnabled
	if ( (!mItemSelection) && (mOutfitSelection) && (LLAppearanceMgr::instance().getCanRemoveOutfit(mSelectedCategory)) )
		gInventory.removeCategory(mSelectedCategory);
}

// virtual
void LLOutfitsView::setSelectedOutfitByUUID(const LLUUID& idOutfit)
{
	// If mInvPanel hasn't been initialized yet (for instance when makeNewOutfitLinks() is called during the logon of a new accout) then the
	// base outfit will be highlighted when they open the floater later on so we don't have to do anything in that case
	if (!mInvPanel)
		return;

	LLFolderView* pRootFolder = mInvPanel->getRootFolder();
	LLFolderViewItem *pOutfitFolder = mInvPanel->getItemByID(idOutfit);
	if (pOutfitFolder)
	{
		pOutfitFolder->setOpen(!pOutfitFolder->isOpen());
		pRootFolder->setSelection(pOutfitFolder, TRUE);
		pRootFolder->scrollToShowSelection();
	}
}

// virtual
void LLOutfitsView::wearSelectedItems()
{
	// NOTE-Catznip: this will only work correctly if the Inventory-MultiXXX patch branches are merged in as well
	if (hasItemSelected())
		mInvPanel->doToSelected(LLSD("wear"));
}

// virtual
bool LLOutfitsView::hasItemSelected()
{
	return mItemSelection;
}

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
			mSelectedCategory = static_cast<const LLFolderViewModelItemInventory*>(pFolder->getViewModelItem())->getUUID();
		}
		else
		{
			mItemSelection = true;
			const LLFolderViewFolder* pFolder = (*selItems.begin())->getParentFolder();
			if (pFolder)
			{
				mSelectedCategory = static_cast<const LLFolderViewModelItemInventory*>(pFolder->getViewModelItem())->getUUID();
			}
		}
	}
	else
	{
		// If more than one inventory object is selected we need to make sure it isn't a mixed selection of items and categories
		LLInventoryModel::item_array_t items;
		if (mInvPanel->getSelectedItems(items))
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

	// TODO-Catznip: LLPanelOutfitsInventory doesn't currently use the param but we should still try and pass something meaningful
	mSelectionChangeSignal(LLUUID::null);
}

// virtual
boost::signals2::connection LLOutfitsView::setSelectionChangeCallback(selection_change_callback_t cb)
{
	return mSelectionChangeSignal.connect(cb);
}

void LLOutfitsView::closeAllFolders()
{
	LLInventoryPanel* pInvPanel = getInventoryPanel();
	if (!pInvPanel)
		return;

	LLFolderView* pRootFolder = pInvPanel->getRootFolder();
	if (!pRootFolder)
		return;

	LLFolderViewFolder* pStartFolder = pInvPanel->getRootFolder();
	if (!pStartFolder)
		return;

	// Close all the folders except the start folder
	pStartFolder->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_DOWN);
	pStartFolder->setOpen(TRUE);
	pRootFolder->arrangeAll();
}

// ============================================================================
// EOF
