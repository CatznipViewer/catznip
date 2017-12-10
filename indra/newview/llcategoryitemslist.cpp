/**
 *
 * Copyright (c) 2017, Kitty Barnett
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
#include "llcategoryitemslist.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "lltransutil.h"
#include "llviewercontrol.h"
#include "llwearableitemslist.h"

// ============================================================================
// LLFindCategoryItems - Inventory functor to populate LLCategoryItemsList
//

class LLFindCategoryItems : public LLInventoryCollectFunctor
{
public:
	LLFindCategoryItems() {}
	~LLFindCategoryItems() override {}

	bool operator() (LLInventoryCategory* cat, LLInventoryItem* item) override
	{
		if (item)
		{
			switch (item->getType())
			{
				case LLAssetType::AT_BODYPART:
				case LLAssetType::AT_CLOTHING:
				case LLAssetType::AT_GESTURE:
				case LLAssetType::AT_OBJECT:
					return true;
				case LLAssetType::AT_CATEGORY:
					return item->getIsLinkType();
				case LLAssetType::AT_LINK:
					// Indicates a broken link
					return true;
				default:
					return false;
			}
		}
		return false;
	}
};

// ============================================================================
// LLPanelCategoryInventoryItem - Panel representing an item in the inventory list
//

class LLPanelCategoryInventoryItem : public LLPanelWearableOutfitItem
{
	LOG_CLASS(LLPanelCategoryInventoryItem);
protected:
	LLPanelCategoryInventoryItem(LLViewerInventoryItem* pItem, unsigned int nIndicatorMask, const Params& params);

public:
	void updateItem(const std::string& strName, EItemState eItemState = IS_DEFAULT) override;

	static LLPanelCategoryInventoryItem* create(LLViewerInventoryItem* pItem, unsigned int nIndicatorMask);

	unsigned int m_nIndicatorMask;
};

LLPanelCategoryInventoryItem::LLPanelCategoryInventoryItem(LLViewerInventoryItem* pItem, unsigned int nIndicatorMask, const LLPanelCategoryInventoryItem::Params& params)
	: LLPanelWearableOutfitItem(pItem, nIndicatorMask & (int)LLCategoryItemsList::EIndicatorMask::SHOW_WORN, params)
	, m_nIndicatorMask(nIndicatorMask)
{
}

// static
LLPanelCategoryInventoryItem* LLPanelCategoryInventoryItem::create(LLViewerInventoryItem* pItem, unsigned int nIndicatorMask)
{
	LLPanelCategoryInventoryItem* pListItem = nullptr;
	if (pItem)
	{
		const LLPanelInventoryListItemBase::Params& params = LLUICtrlFactory::getDefaultParams<LLPanelInventoryListItemBase>();
		pListItem = new LLPanelCategoryInventoryItem(pItem, nIndicatorMask, params);
		pListItem->initFromParams(params);
		pListItem->postBuild();
	}
	return pListItem;
}

// virtual
void LLPanelCategoryInventoryItem::updateItem(const std::string& strName, EItemState eItemState)
{
	std::string strLabel = strName;

	if (m_nIndicatorMask & (int)LLCategoryItemsList::EIndicatorMask::SHOW_LINK)
	{
		if (LLViewerInventoryItem* pItem = gInventory.getItem(mInventoryItemUUID))
		{
			if (LLAssetType::lookupIsLinkType(pItem->getType()))
				strLabel += LLTrans::getString("broken_link");
			else if (pItem->getIsLinkType())
				strLabel += LLTrans::getString("link");
		}
	}

	LLPanelWearableOutfitItem::updateItem(strLabel, eItemState);
}

// ============================================================================
// LLCategoryItemsList::ContextMenu
//

LLCategoryItemsList::ContextMenu::ContextMenu()
{
}

void LLCategoryItemsList::ContextMenu::show(LLView* pSpawningView, const uuid_vec_t& idSelItems, S32 x, S32 y)
{
	LLListContextMenu::show(pSpawningView, idSelItems, x, y);
}

// ============================================================================
// LLCategoryItemsList - Shows the contents of a folder as a inventory item list
//

static const LLWearableItemNameComparator WEARABLE_NAME_COMPARATOR;
static const LLWearableItemCreationDateComparator WEARABLE_CREATION_DATE_COMPARATOR;

static const LLDefaultChildRegistry::Register<LLCategoryItemsList> r("category_items_list");

LLCategoryItemsList::Params::Params()
	: worn_indication_enabled("worn_indication_enabled", true)
	, link_indication_enabled("link_indication_enabled", true)
{
}

LLCategoryItemsList::LLCategoryItemsList(const LLCategoryItemsList::Params& p)
	: LLInventoryItemsList(p)
{
	m_pFolderObserver = new LLInventoryCategoriesObserver(false);

	setSortOrder(ESortOrder::BY_NAME, false);

	setRightMouseDownCallback(boost::bind(&LLCategoryItemsList::onRightClick, this, _2, _3));

	m_nIndicatorMask |= (p.link_indication_enabled) ? (int)EIndicatorMask::SHOW_LINK : 0;
	m_nIndicatorMask |= (p.worn_indication_enabled) ? (int)EIndicatorMask::SHOW_WORN : 0;
	setNoItemsCommentText(LLTrans::getString("LoadingData"));
}

// virtual
LLCategoryItemsList::~LLCategoryItemsList()
{
	if (gInventory.containsObserver(m_pFolderObserver))
		gInventory.removeObserver(m_pFolderObserver);
	delete m_pFolderObserver;
	m_pFolderObserver = nullptr;
}

BOOL LLCategoryItemsList::postBuild()
{
	LLInventoryItemsList::postBuild();

	gInventory.addObserver(m_pFolderObserver);

	return TRUE;
}

// virtual
LLPanel* LLCategoryItemsList::createNewItem(LLViewerInventoryItem* pItem)
{
	if (!pItem)
	{
		LL_WARNS() << "No inventory item. Couldn't create flat list item." << LL_ENDL;
		llassert(pItem != nullptr);
		return nullptr;
	}
	return LLPanelCategoryInventoryItem::create(pItem, m_nIndicatorMask);
}

// virtual
void LLCategoryItemsList::onItemMouseDoubleClick(item_pair_t* pItemPair, MASK mask)
{
	LLInventoryItemsList::onItemMouseDoubleClick(pItemPair, mask);

	bool fCtrlDown = (MASK_CONTROL == mask), fOpenAdd = gSavedSettings.getBOOL("DoubleClickAttachmentAdd");
	const LLPanelCategoryInventoryItem* pPanelItem = dynamic_cast<const LLPanelCategoryInventoryItem*>(pItemPair->first);

	if (LLViewerInventoryItem* pItem = pPanelItem->getItem())
	{
		switch (pItem->getType())
		{
			case LLAssetType::AT_LINK:
			case LLAssetType::AT_LINK_FOLDER:
				// Broken links
				break;
			case LLAssetType::AT_CATEGORY:
				{
					const LLViewerInventoryCategory* pFolder = pItem->getLinkedCategory(); bool isOutfit = pFolder->getPreferredType() == LLFolderType::FT_OUTFIT;

					const std::string pstrAction = LLAppearanceMgr::getCanRemoveFromCOF(pFolder->getUUID()) ? "detach" : ((fCtrlDown ^ fOpenAdd) ? "wear_add" : "wear_replace");
					if (pstrAction == "detach")
						LLAppearanceMgr::instance().takeOffOutfit(pFolder->getUUID());
					else if ( (pstrAction == "wear_add") && (LLAppearanceMgr::instance().getCanAddToCOF(pFolder->getUUID())) )
						LLAppearanceMgr::instance().addCategoryToCurrentOutfit(pFolder->getUUID());
					else if ( (pstrAction == "wear_replace") && ((isOutfit && LLAppearanceMgr::instance().getCanReplaceCOF(pFolder->getUUID())) || (!isOutfit && !gAgentWearables.isCOFChangeInProgress())) )
						LLAppearanceMgr::instance().replaceCurrentOutfit(pFolder->getUUID());
				}
				break;
			default:
				{
					const std::string pstrAction = get_is_item_worn(pItem->getUUID()) ? "detach" : ((fCtrlDown ^ fOpenAdd) ? "wear_add" : "wear_replace");
					if (pstrAction == "detach")
						LLAppearanceMgr::instance().removeItemFromAvatar(pItem->getUUID());
					else if (pstrAction == "wear_add")
						LLAppearanceMgr::instance().wearItemOnAvatar(pItem->getUUID(), true, false);
					else if (pstrAction == "wear_replace")
						LLAppearanceMgr::instance().wearItemOnAvatar(pItem->getUUID(), true, true);
				}
			break;
		}
	}
}

void LLCategoryItemsList::setFolderId(const LLUUID& idFolder)
{
	if (m_idFolder == idFolder)
		return;

	if (m_idFolder.notNull())
	{
		m_pFolderObserver->removeCategory(m_idFolder);
	}

	m_idFolder = idFolder;
	if (m_idFolder.notNull())
	{
		if (LLViewerInventoryCategory* pFolder = gInventory.getCategory(m_idFolder))
			pFolder->fetch();
		m_pFolderObserver->addCategory(m_idFolder, boost::bind(&LLCategoryItemsList::updateList, this));
	}

//	setForceShowingUnmatchedItems(true);
	setForceRefresh(true);

	updateList();
	onCommit();
}

void LLCategoryItemsList::setCollectFromChildFolders(bool fCollectAll)
{
	if (m_fCollectAll != fCollectAll)
	{
		m_fCollectAll = fCollectAll;
		setForceRefresh(true);
		updateList();
	}
}

void LLCategoryItemsList::setSortOrder(ESortOrder eSortOrder, bool fSortImmediately)
{
	switch (eSortOrder)
	{
		case ESortOrder::BY_NAME:
			setComparator(&WEARABLE_NAME_COMPARATOR);
			break;
		case ESortOrder::BY_MOST_RECENT:
			setComparator(&WEARABLE_CREATION_DATE_COMPARATOR);
			break;
	}

	m_eSortOrder = eSortOrder;

	if (fSortImmediately)
	{
		sort();
	}
}

void LLCategoryItemsList::updateList()
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;

	if (m_idFolder.notNull())
	{
		if (gInventory.getCategory(m_idFolder))
		{
			LLFindCategoryItems f;
			if (m_fCollectAll)
				gInventory.collectDescendentsIf(m_idFolder, cats, items, LLInventoryModel::EXCLUDE_TRASH, f);
			else
				gInventory.getDirectDescendentsOf(m_idFolder, cats, items, f);
		}
		else
		{
			m_idFolder.setNull();
			onCommit();
		}
	}

	if ( (items.empty()) && (gInventory.isCategoryComplete(m_idFolder)) )
	{
		setNoItemsCommentText(LLTrans::getString("EmptyCategoryText"));
	}

	refreshList(items);
}

void LLCategoryItemsList::onRightClick(S32 x, S32 y)
{
	uuid_vec_t selItemIds;

	getSelectedUUIDs(selItemIds);
	if (selItemIds.empty())
		return;

	ContextMenu::instance().show(this, selItemIds, x, y);
}

// ============================================================================
