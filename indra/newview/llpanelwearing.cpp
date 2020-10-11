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

// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-3.4
#include "llagent.h"
// [/SL:KB]
#include "llagent.h"
#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llappearancemgr.h"
#include "llfloatersidepanelcontainer.h"
#include "llinventoryfunctions.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llmenubutton.h"
#include "llscrolllistctrl.h"
#include "llviewermenu.h"
#include "llviewerregion.h"
#include "llwearableitemslist.h"
#include "llsdserialize.h"
#include "llclipboard.h"
// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-3.3
#include "llavatarrendernotifier.h"
#include "llinventorypanel.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerwearable.h"
#include "llvoavatarself.h"
// [/SL:KB]
// [RLVa:KB] - Checked: 2012-07-28 (RLVa-1.4.7)
#include "rlvcommon.h"
#include "rlvhandler.h"
// [/RLVa:KB]

// Context menu and Gear menu helper.
static void edit_outfit()
{
	LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "edit_outfit"));
}

// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-4.1
//////////////////////////////////////////////////////////////////////////

class LLWornListItem : public LLPanelWearableListItem
{
	LOG_CLASS(LLWornListItem);
public:
	struct Params : public LLInitParam::Block<Params, LLPanelWearableListItem::Params>
	{
		Optional<LLButton::Params>   up_btn;
		Optional<LLButton::Params>   down_btn;
		Optional<LLButton::Params>   delete_btn;
		Optional<LLTextBox::Params>  item_complexity;

		Params();
	};

protected:
	LLWornListItem(LLViewerInventoryItem* pItem, const Params& params);
public:
	~LLWornListItem() override;
	BOOL postBuild() override;
	S32  notify(const LLSD& sdInfo) override;
	void updateItem(const std::string& strName, EItemState itemState = IS_DEFAULT) override;

	static LLWornListItem* create(LLViewerInventoryItem* pItem);

public:
	bool getShowOrdering() const { return mShowOrdering; }
	void setShowOrdering(bool fShowOrdering);
protected:
	void onMoveWearable(bool fDown);
	void setShowMoveUpButton(bool fShow)   { setShowWidget("btn_move_up", fShow); }
	void setShowMoveDownButton(bool fShow) { setShowWidget("btn_move_down", fShow); }
	void setShowDeleteButton(bool fShow)   { setShowWidget("btn_delete", fShow); }
	void setShowComplexity(bool fShow)     { setShowWidget(mComplexityCtrl, fShow); }

protected:
	bool                  mShowOrdering;
	LLTextBox*            mComplexityCtrl;
	LLAssetType::EType    mAssetType;
	LLWearableType::EType mWearableType;
};

static LLWidgetNameRegistry::StaticRegistrar sRegisterPanelWornWearableListItem(&typeid(LLWornListItem::Params), "worn_wearable_list_item");

LLWornListItem::Params::Params()
	: up_btn("up_btn")
	, down_btn("down_btn")
	, delete_btn("delete_btn")
	, item_complexity("item_complexity")
{
}

LLWornListItem::LLWornListItem(LLViewerInventoryItem* pItem, const LLWornListItem::Params& params)
	: LLPanelWearableListItem(pItem, params)
	, mShowOrdering(true)
	, mComplexityCtrl(nullptr)
	, mAssetType(pItem->getType())
	, mWearableType((LLAssetType::AT_CLOTHING == pItem->getType()) ? pItem->getWearableType() : LLWearableType::WT_INVALID)
{
	LLTextBox::Params paramsText = params.item_complexity;
	applyXUILayout(paramsText, this);
	mComplexityCtrl = LLUICtrlFactory::create<LLTextBox>(paramsText);
	addChild(mComplexityCtrl);

	LLButton::Params paramsButton = params.up_btn;
	applyXUILayout(paramsButton, this);
	addChild(LLUICtrlFactory::create<LLButton>(paramsButton));

	paramsButton = params.down_btn;
	applyXUILayout(paramsButton, this);
	addChild(LLUICtrlFactory::create<LLButton>(paramsButton));

	paramsButton = params.delete_btn;
	applyXUILayout(paramsButton, this);
	addChild(LLUICtrlFactory::create<LLButton>(paramsButton));

	setSeparatorVisible(false);
}

LLWornListItem::~LLWornListItem()
{
}

// static
LLWornListItem* LLWornListItem::create(LLViewerInventoryItem* pItem)
{
	if (pItem)
	{
		const LLWornListItem::Params& params = LLUICtrlFactory::getDefaultParams<LLWornListItem>();
		LLWornListItem* pListItem = new LLWornListItem(pItem, params);
		pListItem->initFromParams(params);
		pListItem->postBuild();
		return pListItem;
	}
	return nullptr;
}

// virtual
BOOL LLWornListItem::postBuild()
{
	LLPanelWearableListItem::postBuild();

	addWidgetToRightSide(mComplexityCtrl);
	addWidgetToRightSide("btn_move_up");
	addWidgetToRightSide("btn_move_down");
	addWidgetToRightSide("btn_delete");

	setShowMoveUpButton(false);
	getChild<LLUICtrl>("btn_move_up")->setCommitCallback(boost::bind(&LLWornListItem::onMoveWearable, this, false));
	setShowMoveDownButton(false);
	getChild<LLUICtrl>("btn_move_down")->setCommitCallback(boost::bind(&LLWornListItem::onMoveWearable, this, true));
	setShowDeleteButton(false);

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}

// virtual
S32 LLWornListItem::notify(const LLSD& sdInfo)
{
	if (sdInfo.has("show_ordering"))
	{
		setShowOrdering(sdInfo["show_ordering"].asBoolean());
		return 0;
	}
	return LLPanelWearableListItem::notify(sdInfo);
}

void LLWornListItem::onMoveWearable(bool fDown)
{
	bool fCentralBaking = (gAgent.getRegion()) && (gAgent.getRegion()->getCentralBakeVersion());
	if ( (fCentralBaking) && (isAgentAvatarValid()) )
	{
		gAgentAvatarp->setUsingLocalAppearance();
	}

	LLAppearanceMgr::getInstance()->moveWearable(getItem(), fDown, true);
	LLAppearanceMgr::getInstance()->updateIsDirty();

	if (fCentralBaking)
	{
		LLAppearanceMgr::getInstance()->requestServerAppearanceUpdate();
	}
}

void LLWornListItem::setShowOrdering(bool fShowOrdering)
{
	if (mShowOrdering != fShowOrdering)
	{
		mShowOrdering = fShowOrdering;
		setNeedsRefresh(true);
	}
}

// virtual
void LLWornListItem::updateItem(const std::string& strName, EItemState itemState)
{
	std::string strItemName = strName; bool fShowUp = false, fShowDown = false;

	switch (mAssetType)
	{
		case LLAssetType::AT_OBJECT:
			if (isAgentAvatarValid())
			{
				std::string strAttachPt;
				if (gAgentAvatarp->getAttachedPointName(mInventoryItemUUID, strAttachPt))
				{
					LLStringUtil::format_map_t args;
					args["[ATTACHMENT_POINT]"] = LLTrans::getString(strAttachPt);
					strItemName += LLTrans::getString("WornOnAttachmentPoint", args);
				}
				else
				{
					LLStringUtil::format_map_t args;
					args["[ATTACHMENT_ERROR]"] = LLTrans::getString(strAttachPt);
					strItemName += LLTrans::getString("AttachmentErrorMessage", args);
				}

				if (LLViewerObject* pAttachObj = gAgentAvatarp->getWornAttachment(mInventoryItemUUID))
				{
					mComplexityCtrl->setText(llformat("(%d) ", pAttachObj->getAttachmentComplexity()));
				}
			}
			else
			{
				strItemName += LLTrans::getString("worn");
			}
			break;
		case LLAssetType::AT_CLOTHING:
			if (mShowOrdering)
			{
				U32 cntWearable = gAgentWearables.getWearableCount(mWearableType), idxWearable = 0;
				if ((cntWearable > 1) && (gAgentWearables.getWearableIndex(gAgentWearables.getWearableFromItemID(mInventoryItemUUID), idxWearable)))
				{
					fShowDown = (idxWearable > 0);
					fShowUp = (idxWearable < cntWearable - 1);
				}
			}
			break;
	}

	LLPanelWearableListItem::updateItem(strItemName, itemState);

	setShowMoveUpButton(fShowUp);
	setShowMoveDownButton(fShowDown);
	setShowComplexity(LLAssetType::AT_OBJECT == mAssetType);
}

//////////////////////////////////////////////////////////////////////////

static const LLDefaultChildRegistry::Register<LLWornItemsList> r("wearing_items_list");

LLWornItemsList::LLWornItemsList(const LLWornItemsList::Params& p)
	: LLWearableItemsList(p)
{
}

// virtual
LLPanel* LLWornItemsList::createNewItem(LLViewerInventoryItem* pItem)
{
	if (!pItem)
	{
		LL_WARNS() << "No inventory item. Couldn't create new item." << LL_ENDL;
		llassert(pItem != nullptr);
		return nullptr;
	}
	return LLWornListItem::create(pItem);
}

void LLWornItemsList::refreshList(const std::vector<LLPointer<LLViewerInventoryItem>> item_array)
{
	LLWearableItemsList::refreshList(item_array);

	if (LLAccordionCtrlTab* pAccordionTab = getParentByType<LLAccordionCtrlTab>())
	{
		int nWearableCount = 0, nAttachCount = 0;
		for (const LLPointer<LLViewerInventoryItem>& pItem : item_array)
		{
			switch (pItem->getType())
			{
				case LLAssetType::AT_BODYPART:
				case LLAssetType::AT_CLOTHING:
					nWearableCount++;
					break;
				case LLAssetType::AT_OBJECT:
					nAttachCount++;
					break;
			}
		}

		LLStringUtil::format_map_t args;
		args["[WEARABLE_COUNT]"] = std::to_string(nWearableCount);
		args["[ATTACH_COUNT]"] = std::to_string(nAttachCount);
		args["[ATTACH_LIMIT]"] = std::to_string(gAgentAvatarp->getMaxAttachments());
		pAccordionTab->setTitle(LLTrans::getString("WornItemsTabTitle", args));
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

// [SL:KB] - Patch: Inventory-AttachmentActions - Checked: 2012-05-15 (Catznip-3.3)
		registrar.add("Gear.TouchAttach", boost::bind(&LLWearingGearMenu::onTouchAttach, this));
		registrar.add("Gear.EditItem", boost::bind(&LLWearingGearMenu::onEditItem, this));
		registrar.add("Gear.EditOutfit", boost::bind(&edit_outfit));
// [/SL:KB]
//		registrar.add("Gear.Edit", boost::bind(&edit_outfit));
//		registrar.add("Gear.TakeOff", boost::bind(&LLPanelWearing::onRemoveItem, mPanelWearing));
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-08-09 (Catznip-3.3)
		registrar.add("Gear.TakeOff", boost::bind(&LLPanelWearing::onTakeOffClicked, mPanelWearing));
		registrar.add("Gear.TakeOffFolder", boost::bind(&LLPanelWearing::onTakeOffFolderClicked, mPanelWearing));
// [/SL:KB]
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

	LLToggleableMenu*		mMenu;
	LLPanelWearing* 		mPanelWearing;
};

// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-3.3
class LLWearingSortMenu
{
public:
	LLWearingSortMenu(LLPanelWearing* pWearingPanel)
		: mFolderMenu(NULL)
		, mListMenu(NULL)
		, mWearingPanel(pWearingPanel)
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		registrar.add("Sort.Folder", boost::bind(&LLWearingSortMenu::onChangeFolderSortOrder, this, _2));
		registrar.add("Sort.List", boost::bind(&LLWearingSortMenu::onChangeListSortOrder, this, _2));

		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
		enable_registrar.add("Sort.CheckFolder", boost::bind(&LLWearingSortMenu::onCheckFolderSortOrder, this, _2));
		enable_registrar.add("Sort.CheckList", boost::bind(&LLWearingSortMenu::onCheckListSortOrder, this, _2));

		mFolderMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_wearing_sort_folder.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		llassert(mFolderMenu);
		mListMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_wearing_sort_list.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		llassert(mListMenu);
	}

	LLToggleableMenu* getFolderMenu() const { return mFolderMenu; }
	LLToggleableMenu* getListMenu() const   { return mListMenu; }

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

	static LLWearableItemsList::ESortOrder getListSortOrder(const std::string& strParam)
	{
		if ("appearance" == strParam)
			return LLWearableItemsList::E_SORT_BY_APPEARANCE;
		else if ("complexity" == strParam)
			return LLWearableItemsList::E_SORT_BY_COMPLEXITY;
		else if ("name" == strParam)
			return LLWearableItemsList::E_SORT_BY_NAME;
		else if ("type_name" == strParam)
			return LLWearableItemsList::E_SORT_BY_TYPE_NAME;
		return (LLWearableItemsList::ESortOrder)- 1;
	}

	void onChangeListSortOrder(const LLSD& sdParam)
	{
		mWearingPanel->getItemsList()->setSortOrder(getListSortOrder(sdParam.asString()));
		gSavedSettings.setU32("WearingListSortOrder", mWearingPanel->getItemsList()->getSortOrder());
	}

	bool onCheckListSortOrder(const LLSD& sdParam)
	{
		return getListSortOrder(sdParam.asString()) == mWearingPanel->getItemsList()->getSortOrder();
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

//		registrar.add("Wearing.Edit", boost::bind(&edit_outfit));
// [SL:KB] - Patch: Inventory-ActivePanel | Checked: Catznip-5.4
		registrar.add("Wearing.ShowOriginal", boost::bind(show_item_original, mUUIDs.front(), EShowItemOptions::TAKE_FOCUS_YES, nullptr));
// [/SL:KB]
//		registrar.add("Wearing.ShowOriginal", boost::bind(show_item_original, mUUIDs.front()));
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
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-12 (Catznip-3.3)
		functor_t take_off_folder = boost::bind(&LLAppearanceMgr::removeFolderFromAvatar, LLAppearanceMgr::getInstance(), _1);
		registrar.add("Wearing.TakeOffFolder", boost::bind(handlePerFolder, take_off_folder, mUUIDs));
		registrar.add("Wearing.FindOriginal", boost::bind(&LLWearingContextMenu::onFindOriginal, this));
		registrar.add("Wearing.Properties", boost::bind(&LLWearingContextMenu::onProperties, this));
// [/SL:KB]

		LLContextMenu* menu = createFromFile("menu_wearing_tab.xml");

		updateMenuItemsVisibility(menu);

		return menu;
	}

//	void updateMenuItemsVisibility(LLContextMenu* menu)
// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.3
	virtual void updateMenuItemsVisibility(LLContextMenu* menu)
// [/SL:KB]
	{
		bool bp_selected			= false;	// true if body parts selected
		bool clothes_selected		= false;
		bool attachments_selected	= false;
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-08-10 (Catznip-3.3)
		bool can_remove_folder      = false;
// [/SL:KB]
// [RLVa:KB] - Checked: 2012-07-28 (RLVa-1.4.7)
		S32 rlv_locked_count = 0;
// [/RLVa:KB]

		// See what types of wearables are selected.
		for (uuid_vec_t::const_iterator it = mUUIDs.begin(); it != mUUIDs.end(); ++it)
		{
//			LLViewerInventoryItem* item = gInventory.getItem(*it);
//
//			if (!item)
//			{
//				LL_WARNS() << "Invalid item" << LL_ENDL;
//				continue;
//			}
//
//			LLAssetType::EType type = item->getType();
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-08-10 (Catznip-3.3)
			LLAssetType::EType type;
//			if (LLViewerInventoryItem* item = gInventory.getItem(*it))
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-08-10 (Catznip-3.3)
			if (LLViewerInventoryItem* item = gInventory.getLinkedItem(*it))
// [/SL:KB]
			{
				type = item->getType();
				if (!can_remove_folder)
				{
					can_remove_folder |= LLAppearanceMgr::instance().getCanRemoveFolderFromAvatar(item->getParentUUID());
				}
			}
			else
			{
				// Might be a temporary attachment
				const LLViewerObject* pAttachObj = gAgentAvatarp->getWornAttachment(*it);
				if ( (!pAttachObj) || (!pAttachObj->isTempAttachment()) )
				{
					LL_WARNS() << "Invalid item" << LL_ENDL;
					continue;
				}
				type = LLAssetType::AT_OBJECT;
			}
// [/SL:KB]

			if (type == LLAssetType::AT_CLOTHING)
			{
				clothes_selected = true;
			}
			else if (type == LLAssetType::AT_BODYPART)
			{
				bp_selected = true;
			}
			else if (type == LLAssetType::AT_OBJECT || type == LLAssetType::AT_GESTURE)
			{
				attachments_selected = true;
			}
// [RLVa:KB] - Checked: 2012-07-28 (RLVa-1.4.7)
			if ( (rlv_handler_t::isEnabled()) && (!rlvPredCanRemoveItem(*it)) )
			{
				rlv_locked_count++;
			}
// [/RLVa:KB]
		}

		// Enable/disable some menu items depending on the selection.
// [RLVa:KB] - Checked: 2012-07-28 (RLVa-1.4.7)
		bool rlv_blocked = (mUUIDs.size() == rlv_locked_count);
// [/RLVa:KB]
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
//		menu->setItemEnabled("detach",             !bp_selected);
		menu->setItemVisible("take_off",           show_take_off);
//		menu->setItemEnabled("take_off",           !bp_selected);
		menu->setItemVisible("take_off_or_detach", show_take_off_or_detach);
//		menu->setItemEnabled("take_off_or_detach", !bp_selected);
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-12 (Catznip-3.3)
		menu->setItemVisible("take_off_folder",    show_take_off || show_take_off_or_detach);
		menu->setItemEnabled("take_off_folder",    can_remove_folder);
		menu->setItemVisible("detach_folder",      show_detach);
		menu->setItemEnabled("detach_folder",      can_remove_folder);

		menu->setItemEnabled("find_original", 1 == mUUIDs.size());
		menu->setItemEnabled("properties", 1 == mUUIDs.size());
// [/SL:KB]
		menu->setItemVisible("edit_item", FALSE);
// [RLVa:KB] - Checked: 2012-07-28 (RLVa-1.4.7)
		menu->setItemEnabled("take_off",           !rlv_blocked);
		menu->setItemEnabled("detach",             !rlv_blocked);
		menu->setItemEnabled("take_off_or_detach", !bp_selected && !rlv_blocked);
// [/RLVa:KB]
		menu->setItemVisible("edit_outfit_separator", show_edit || show_detach || show_take_off || show_take_off_or_detach);
// [/SL:KB]
//		bool allow_detach = !bp_selected && !clothes_selected && attachments_selected;
//		bool allow_take_off = !bp_selected && clothes_selected && !attachments_selected;
//
//		menu->setItemVisible("take_off",	allow_take_off);
//		menu->setItemVisible("detach",		allow_detach);
//		menu->setItemVisible("edit_outfit_separator", allow_take_off || allow_detach);
//		menu->setItemVisible("show_original", mUUIDs.size() == 1);
	}

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-12 (Catznip-3.3)
	void onFindOriginal()
	{
		if (!mUUIDs.empty())
		{
			show_item_original(mUUIDs.front(), EShowItemOptions::TAKE_FOCUS_YES);
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

//class LLTempAttachmentsContextMenu : public LLListContextMenu
// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.3
class LLTempAttachmentsContextMenu : public LLWearingContextMenu
// [/SL:KB]
{
public:
	LLTempAttachmentsContextMenu(LLPanelWearing* panel_wearing)
		: mPanelWearing(panel_wearing)
	{}
protected:
//	/* virtual */ LLContextMenu* createMenu()
//	{
//		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
//
//		registrar.add("Wearing.EditItem", boost::bind(&LLPanelWearing::onEditAttachment, mPanelWearing));
//		registrar.add("Wearing.Detach", boost::bind(&LLPanelWearing::onRemoveAttachment, mPanelWearing));
//		LLContextMenu* menu = createFromFile("menu_wearing_tab.xml");
//
//		updateMenuItemsVisibility(menu);
//
//		return menu;
//	}

// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.3
	void updateMenuItemsVisibility(LLContextMenu* menu) override
	{
		LLWearingContextMenu::updateMenuItemsVisibility(menu);
		menu->setItemVisible("find_original", false);
		menu->setItemVisible("detach_folder", false);
		menu->setItemVisible("find_original", false);
		menu->setItemVisible("properties", false);
		menu->setItemVisible("edit_outfit_separator", false);
		menu->setItemVisible("edit", false);
	}
// [/SL:KB]
//	void updateMenuItemsVisibility(LLContextMenu* menu)
//	{
//		menu->setItemVisible("take_off", FALSE);
//		menu->setItemVisible("detach", TRUE);
//		menu->setItemVisible("edit_outfit_separator", TRUE);
//		menu->setItemVisible("show_original", FALSE);
//		menu->setItemVisible("edit_item", TRUE);
//		menu->setItemVisible("edit", FALSE);
//	}

	LLPanelWearing* 		mPanelWearing;
};

//////////////////////////////////////////////////////////////////////////

std::string LLPanelAppearanceTab::sFilterSubString = LLStringUtil::null;

static LLPanelInjector<LLPanelWearing> t_panel_wearing("panel_wearing");

LLPanelWearing::LLPanelWearing()
	:	LLPanelAppearanceTab()
	,	mCOFItemsList(NULL)
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	,	mInvPanel(NULL)
	,	mSavedFolderState(NULL)
	,	mSortMenuButton(NULL)
// [/SL:KB]
	,	mIsInitialized(false)
	,	mAttachmentsChangedConnection()
{
	mCategoriesObserver = new LLInventoryCategoriesObserver();

	mGearMenu = new LLWearingGearMenu(this);
// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-3.3
	mSortMenu = new LLWearingSortMenu(this);
// [/SL:KB]
	mContextMenu = new LLWearingContextMenu();
	mAttachmentsMenu = new LLTempAttachmentsContextMenu(this);
}

LLPanelWearing::~LLPanelWearing()
{
	delete mGearMenu;
// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-3.3
	delete mSortMenu;
// [/SL:KB]
	delete mContextMenu;
	delete mAttachmentsMenu;
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	delete mSavedFolderState;

	if (mComplexityChangedSlot.connected())
	{
		mComplexityChangedSlot.disconnect();
	}
// [/SL:KB]

	if (gInventory.containsObserver(mCategoriesObserver))
	{
		gInventory.removeObserver(mCategoriesObserver);
	}
	delete mCategoriesObserver;

//	if (mAttachmentsChangedConnection.connected())
//	{
//		mAttachmentsChangedConnection.disconnect();
//	}
}

BOOL LLPanelWearing::postBuild()
{
	mAccordionCtrl = getChild<LLAccordionCtrl>("wearables_accordion");
	mWearablesTab = getChild<LLAccordionCtrlTab>("tab_wearables");
	mWearablesTab->setIgnoreResizeNotification(true);
// [SL:KB] - Patch: Appearance-InvPanel | Checked: Catznip-5.0
	mWearablesTab->setDropDownStateChangedCallback(boost::bind(&LLPanelWearing::onToggleWearingView, this, EWearingView::LIST_VIEW));
	mWearablesInvTab = getChild<LLAccordionCtrlTab>("tab_wearables_invpanel");
	mWearablesInvTab->setDropDownStateChangedCallback(boost::bind(&LLPanelWearing::onToggleWearingView, this, EWearingView::FOLDER_VIEW));
// [/SL:KB]
	mAttachmentsTab = getChild<LLAccordionCtrlTab>("tab_temp_attachments");
// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.3
	mAttachmentsChangedConnection = LLAppearanceMgr::instance().setAttachmentsChangedCallback(boost::bind(&LLPanelWearing::onAttachmentsChanged, this));
// [/SL:KB]
//	mAttachmentsTab->setDropDownStateChangedCallback(boost::bind(&LLPanelWearing::onAccordionTabStateChanged, this));
//	mCOFItemsList = getChild<LLWearableItemsList>("cof_items_list");
//	mCOFItemsList->setRightMouseDownCallback(boost::bind(&LLPanelWearing::onWearableItemsListRightClick, this, _1, _2, _3));
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	mCOFItemsList = getChild<LLWornItemsList>("cof_items_list");
	mCOFItemsList->setRightMouseDownCallback(boost::bind(&LLPanelWearing::onWearableItemsListRightClick, this, _1, _2, _3));
	mCOFItemsList->setSortOrder((LLWearableItemsList::ESortOrder)gSavedSettings.getU32("WearingListSortOrder"));
	mCOFItemsList->setCommitCallback(boost::bind(&LLPanelWearing::onSelectionChange, this));
	mComplexityChangedSlot = LLAvatarRenderNotifier::instance().addComplexityChangedCallback(boost::bind(&LLWornItemsList::setNeedsRefresh, mCOFItemsList, true));
// [/SL:KB]

	mTempItemsList = getChild<LLScrollListCtrl>("temp_attachments_list");
	mTempItemsList->setFgUnselectedColor(LLColor4::white);
	mTempItemsList->setRightMouseDownCallback(boost::bind(&LLPanelWearing::onTempAttachmentsListRightClick, this, _1, _2, _3));
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	getChild<LLMenuButton>("options_gear_btn")->setMenu(mGearMenu->getMenu());
	mSortMenuButton = getChild<LLMenuButton>("options_sort_btn");

	getChild<LLUICtrl>("take_off_btn")->setCommitCallback(boost::bind(&LLPanelWearing::onTakeOffClicked, this));
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
// [SL:KB] - Patch: Appearance-InvPanel | Checked: Catznip-3.3
		// Delay creating the inventory view until the user actually opens this panel
		LLAccordionCtrlTab* pDefaultTab = nullptr;
		switch ((EWearingView)gSavedSettings.getU32("WearingViewType"))
		{
			case EWearingView::LIST_VIEW:
				pDefaultTab = mWearablesTab;
				break;
			case EWearingView::FOLDER_VIEW:
				pDefaultTab = mWearablesInvTab;
				break;
		}

		if (pDefaultTab)
		{
			pDefaultTab->changeOpenClose(false);
			pDefaultTab->showAndFocusHeader();
			mAccordionCtrl->notifyParent(LLSD().with("action", "select_current"));
		}
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
// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.3
		onAttachmentsChanged();
// [/SL:KB]

		mIsInitialized = true;
	}
}

//void LLPanelWearing::draw()
//{
//	if (mUpdateTimer.getStarted() && (mUpdateTimer.getElapsedTimeF32() > 0.1))
//	{
//		mUpdateTimer.stop();
//		updateAttachmentsList();
//	}
//	LLPanel::draw();
//}

//void LLPanelWearing::onAccordionTabStateChanged()
//{
//	if(mAttachmentsTab->isExpanded())
//	{
//		startUpdateTimer();
//		mAttachmentsChangedConnection = LLAppearanceMgr::instance().setAttachmentsChangedCallback(boost::bind(&LLPanelWearing::startUpdateTimer, this));
//	}
//	else
//	{
//		if (mAttachmentsChangedConnection.connected())
//		{
//			mAttachmentsChangedConnection.disconnect();
//		}
//	}
//}

// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.3
void LLPanelWearing::onAttachmentsChanged()
{
	doAfterInterval(boost::bind(&LLPanelWearing::updateAttachmentsList, getDerivedHandle<LLPanelWearing>()), 0.1);
}
// [/SL:KB]
//void LLPanelWearing::startUpdateTimer()
//{
//	if (!mUpdateTimer.getStarted())
//	{
//		mUpdateTimer.start();
//	}
//	else
//	{
//		mUpdateTimer.reset();
//	}
//}

// virtual
void LLPanelWearing::setFilterSubString(const std::string& string)
{
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-23 (Catznip-3.3)
	// Selective copy/paste of LLPanelOutfitEdit::onSearchEdit()
	if (sFilterSubString != string)
	{
		sFilterSubString = string;
	
		// Searches are case-insensitive
		LLStringUtil::toUpper(sFilterSubString);
		LLStringUtil::trimHead(sFilterSubString);
	}
	
	if (sFilterSubString.empty())
	{
		mCOFItemsList->setFilterSubString(LLStringUtil::null);
		if (mInvPanel)
		{
			mInvPanel->setFilterSubString(LLStringUtil::null);
			// Re-open folders that were initially open
			mSavedFolderState->setApply(TRUE);
			mInvPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
			LLOpenFoldersWithSelection opener;
			mInvPanel->getRootFolder()->applyFunctorRecursively(opener);
			mInvPanel->getRootFolder()->scrollToShowSelection();
		}
		return;
	}
	else if ( (mInvPanel) && (!mInvPanel->hasFilterSubString()) )
	{
		// Save current folder open state if no filter currently applied
		mSavedFolderState->setApply(FALSE);
		mInvPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}
	
	// Set new filter string
	if (mInvPanel)
		mInvPanel->setFilterSubString(sFilterSubString);
	mCOFItemsList->setFilterSubString(sFilterSubString);
// [/SL:KB]
//	sFilterSubString = string;
//	mCOFItemsList->setFilterSubString(sFilterSubString);
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

	if (command_name == "take_off")
	{
		if (mWearablesTab->isExpanded())
		{
			return hasItemSelected() && canTakeOffSelected();
		}
		else
		{
			LLScrollListItem* item = mTempItemsList->getFirstSelected();
			if (item && item->getUUID().notNull())
			{
				return true;
			}
		}
	}

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-08-15 (Catznip-3.3)
	if (command_name == "take_off_folder")
	{
		uuid_vec_t selected_uuids;
		getSelectedItemsUUIDs(selected_uuids);

		for (uuid_vec_t::const_iterator itItem = selected_uuids.begin(); itItem != selected_uuids.end(); ++itItem)
		{
			const LLViewerInventoryItem* pItem = gInventory.getItem(*itItem);
			if ( (!pItem) || (!LLAppearanceMgr::instance().getCanRemoveFolderFromAvatar(pItem->getParentUUID())) )
			{
				return false;
			}
		}
		return true;
	}
// [/SL:KB]

	return false;
}

// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.3
void LLPanelWearing::updateAttachmentsList(LLHandle<LLPanelWearing> hWearingPanel)
{
	LLPanelWearing* pSelf = hWearingPanel.get();
	if (!pSelf)
		return;

	std::vector<LLViewerObject*> attachs = LLAgentWearables::getTempAttachments();
	pSelf->mTempItemsList->deleteAllItems();
	pSelf->mAttachmentsMap.clear();
	if(!attachs.empty())
	{
		if(!pSelf->populateAttachmentsList())
		{
			pSelf->requestAttachmentDetails();
		}
	}
	else
	{
		std::string no_attachments = pSelf->getString("no_attachments");
		LLSD row;
		row["columns"][0]["column"] = "text";
		row["columns"][0]["value"] = no_attachments;
		row["columns"][0]["font"] = "SansSerifBold";
		pSelf->mTempItemsList->addElement(row);
	}

	if (LLAccordionCtrlTab* pAccordionTab = pSelf->findChild<LLAccordionCtrlTab>("tab_temp_attachments"))
	{
		LLStringUtil::format_map_t args;
		args["[ATTACH_COUNT]"] = std::to_string(attachs.size());
		pAccordionTab->setTitle(LLTrans::getString("TempItemsTabTitle", args));
	}
}
// [/SL:KB]
//void LLPanelWearing::updateAttachmentsList()
//{
//	std::vector<LLViewerObject*> attachs = LLAgentWearables::getTempAttachments();
//	mTempItemsList->deleteAllItems();
//	mAttachmentsMap.clear();
//	if(!attachs.empty())
//	{
//		if(!populateAttachmentsList())
//		{
//			requestAttachmentDetails();
//		}
//	}
//	else
//	{
//		std::string no_attachments = getString("no_attachments");
//		LLSD row;
//		row["columns"][0]["column"] = "text";
//		row["columns"][0]["value"] = no_attachments;
//		row["columns"][0]["font"] = "SansSerifBold";
//		mTempItemsList->addElement(row);
//	}
//}

bool LLPanelWearing::populateAttachmentsList(bool update)
{
	bool populated = true;
	if(mTempItemsList)
	{
		mTempItemsList->deleteAllItems();
		mAttachmentsMap.clear();
		std::vector<LLViewerObject*> attachs = LLAgentWearables::getTempAttachments();

		std::string icon_name = LLInventoryIcon::getIconName(LLAssetType::AT_OBJECT, LLInventoryType::IT_OBJECT);
		for (std::vector<LLViewerObject*>::iterator iter = attachs.begin();
				iter != attachs.end(); ++iter)
		{
			LLViewerObject *attachment = *iter;
			LLSD row;
			row["id"] = attachment->getID();
			row["columns"][0]["column"] = "icon";
			row["columns"][0]["type"] = "icon";
			row["columns"][0]["value"] = icon_name;
			row["columns"][1]["column"] = "text";
			if(mObjectNames.count(attachment->getID()) && !mObjectNames[attachment->getID()].empty())
			{
				row["columns"][1]["value"] = mObjectNames[attachment->getID()];
			}
			else if(update)
			{
				row["columns"][1]["value"] = attachment->getID();
				populated = false;
			}
			else
			{
				row["columns"][1]["value"] = "Loading...";
				populated = false;
			}
			mTempItemsList->addElement(row);
			mAttachmentsMap[attachment->getID()] = attachment;
		}
	}
	return populated;
}

void LLPanelWearing::requestAttachmentDetails()
{
	LLSD body;
	std::string url = gAgent.getRegionCapability("AttachmentResources");
	if (!url.empty())
	{
		LLCoros::instance().launch("LLPanelWearing::getAttachmentLimitsCoro",
		boost::bind(&LLPanelWearing::getAttachmentLimitsCoro, this, url));
	}
}

void LLPanelWearing::getAttachmentLimitsCoro(std::string url)
{
	LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
	LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
	httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getAttachmentLimitsCoro", httpPolicy));
	LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

	LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

	LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
	LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

	if (!status)
	{
		LL_WARNS() << "Unable to retrieve attachment limits." << LL_ENDL;
		return;
	}

	result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
	setAttachmentDetails(result);
}


void LLPanelWearing::setAttachmentDetails(LLSD content)
{
	mObjectNames.clear();
	S32 number_attachments = content["attachments"].size();
	for(int i = 0; i < number_attachments; i++)
	{
		S32 number_objects = content["attachments"][i]["objects"].size();
		for(int j = 0; j < number_objects; j++)
		{
			LLUUID task_id = content["attachments"][i]["objects"][j]["id"].asUUID();
			std::string name = content["attachments"][i]["objects"][j]["name"].asString();
			mObjectNames[task_id] = name;
		}
	}
	if(!mObjectNames.empty())
	{
		populateAttachmentsList(true);
	}
}

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-23 (Catznip-3.3)
boost::signals2::connection LLPanelWearing::setSelectionChangeCallback(selection_change_signal_t::slot_type cb)
{
	return mSelectionSignal.connect(cb);
}
// [/SL:KB]
//boost::signals2::connection LLPanelWearing::setSelectionChangeCallback(commit_callback_t cb)
//{
//	if (!mCOFItemsList) return boost::signals2::connection();
//
//	return mCOFItemsList->setCommitCallback(cb);
//}

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-23 (Catznip-3.3)
void LLPanelWearing::onSelectionChange()
{
	mSelectionSignal();
}
// [/SL:KB]

void LLPanelWearing::onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
	LLWearableItemsList* list = dynamic_cast<LLWearableItemsList*>(ctrl);
	if (!list) return;

	uuid_vec_t selected_uuids;

	list->getSelectedUUIDs(selected_uuids);

	mContextMenu->show(ctrl, selected_uuids, x, y);
}

void LLPanelWearing::onTempAttachmentsListRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
	LLScrollListCtrl* list = dynamic_cast<LLScrollListCtrl*>(ctrl);
	if (!list) return;
	list->selectItemAt(x, y, MASK_NONE);
	uuid_vec_t selected_uuids;

	if(list->getCurrentID().notNull())
	{
		selected_uuids.push_back(list->getCurrentID());
		mAttachmentsMenu->show(ctrl, selected_uuids, x, y);
	}
}

bool LLPanelWearing::hasItemSelected()
{
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-23 (Catznip-3.3)
	if (mCOFItemsList->getVisible())
	{
		return mCOFItemsList->getSelectedItem() != NULL;
	}
	else if ( (mInvPanel) && (mInvPanel->getVisible()) )
	{
		return mInvPanel->getRootFolder()->getCurSelectedItem() != NULL;
	}
	return false;
// [/SL:KB]
//	return mCOFItemsList->getSelectedItem() != NULL;
}

//void LLPanelWearing::onEditAttachment()
//{
//	LLScrollListItem* item = mTempItemsList->getFirstSelected();
//	if (item)
//	{
//		LLSelectMgr::getInstance()->deselectAll();
//		LLSelectMgr::getInstance()->selectObjectAndFamily(mAttachmentsMap[item->getUUID()]);
//		handle_object_edit();
//	}
//}

//void LLPanelWearing::onRemoveAttachment()
//{
//	LLScrollListItem* item = mTempItemsList->getFirstSelected();
//	if (item && item->getUUID().notNull())
//	{
//		LLSelectMgr::getInstance()->deselectAll();
//		LLSelectMgr::getInstance()->selectObjectAndFamily(mAttachmentsMap[item->getUUID()]);
//		LLSelectMgr::getInstance()->sendDropAttachment();
//	}
//}

//void LLPanelWearing::onRemoveItem()
//{
//	if (mWearablesTab->isExpanded())
//	{
//		uuid_vec_t selected_uuids;
//		getSelectedItemsUUIDs(selected_uuids);
//		LLAppearanceMgr::instance().removeItemsFromAvatar(selected_uuids);
//	}
//	else
//	{
//		onRemoveAttachment();
//	}
//}

void LLPanelWearing::getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const
{
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-23 (Catznip-3.3)
	if (mCOFItemsList->getVisible())
	{
		mCOFItemsList->getSelectedUUIDs(selected_uuids);
	}
	else if ( (mInvPanel) && (mInvPanel->getVisible()) )
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
// [/SL:KB]
//	mCOFItemsList->getSelectedUUIDs(selected_uuids);
}

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
void LLPanelWearing::onTakeOffClicked()
{
	uuid_vec_t selected_uuids;
	getSelectedItemsUUIDs(selected_uuids);
	LLAppearanceMgr::instance().removeItemsFromAvatar(selected_uuids);
}

void LLPanelWearing::onTakeOffFolderClicked()
{
	// Copy/paste of LLWearingContextMenu::handlePerFolder()
	uuid_vec_t selected_uuids, folder_ids;
	getSelectedItemsUUIDs(selected_uuids);

	for (uuid_vec_t::const_iterator itItem = selected_uuids.begin(); itItem != selected_uuids.end(); ++itItem)
	{
		LLViewerInventoryItem* pItem = gInventory.getLinkedItem(*itItem);
		if (pItem)
		{
			if (folder_ids.end() == std::find(folder_ids.begin(), folder_ids.end(), pItem->getParentUUID()))
			{
				folder_ids.push_back(pItem->getParentUUID());
			}
		}
	}

	LLAppearanceMgr::instance().removeFoldersFromAvatar(folder_ids);
}
// [/SL:KB]

// [SL:KB] - Patch: Appearance-InvPanel | Checked: Catznip-3.3
void LLPanelWearing::onToggleWearingView(EWearingView eView)
{
	if ( (EWearingView::FOLDER_VIEW == eView) && (!mInvPanel) )
	{
		createInventoryPanel();
	}

	mSortMenuButton->setMenu( (EWearingView::FOLDER_VIEW == eView) ? mSortMenu->getFolderMenu() : mSortMenu->getListMenu());
	gSavedSettings.setU32("WearingViewType", (U32)eView);
}

void LLPanelWearing::createInventoryPanel()
{
	if (mInvPanel)
		return;

	LLView* pInvPanelPlaceholder = findChild<LLView>("wearing_invpanel_placeholder");

	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);

	mInvPanel = LLUICtrlFactory::createFromFile<LLInventoryPanel>("panel_outfits_wearing_invpanel.xml", pInvPanelPlaceholder->getParent(), LLInventoryPanel::child_registry_t::instance());
	mInvPanel->setShape(pInvPanelPlaceholder->getRect());
	mInvPanel->setFilterLinks(LLInventoryFilter::FILTERLINK_EXCLUDE_LINKS, false);
	mInvPanel->setFilterWorn(true);
	mInvPanel->setSortOrder(gSavedSettings.getU32("WearingFolderSortOrder"));
//	mInvPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mInvPanel->getFilter().markDefault();
	mInvPanel->openAllFolders();
	mInvPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	mInvPanel->setSelectCallback(boost::bind(&LLPanelWearing::onSelectionChange, this));

	if (!sFilterSubString.empty())
		mInvPanel->setFilterSubString(sFilterSubString);
}
// [/SL:KB]

void LLPanelWearing::copyToClipboard()
{
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2013-12-08 (Catznip-3.6)
	copy_folder_to_clipboard(LLAppearanceMgr::instance().getCOF());
// [/SL:KB]
//	std::string text;
//	std::vector<LLSD> data;
//	mCOFItemsList->getValues(data);
//
//	for(std::vector<LLSD>::const_iterator iter = data.begin(); iter != data.end();)
//	{
//		LLSD uuid = (*iter);
//		LLViewerInventoryItem* item = gInventory.getItem(uuid);
//
//		iter++;
//		if (item != NULL)
//		{
//			// Append a newline to all but the last line
//			text += iter != data.end() ? item->getName() + "\n" : item->getName();
//		}
//	}
//
//	LLClipboard::instance().copyToClipboard(utf8str_to_wstring(text),0,text.size());
}

// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.5
namespace LLListContextMenuUtil
{
	LLListContextMenu* createWearingContextMenu()
	{
		return new LLWearingContextMenu();
	}
}
// [/SL:KB]

// EOF
