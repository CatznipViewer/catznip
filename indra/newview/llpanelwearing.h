/**
 * @file llpanelwearing.h
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

#ifndef LL_LLPANELWEARING_H
#define LL_LLPANELWEARING_H

#include "llpanel.h"

// newview
#include "llpanelappearancetab.h"
#include "llselectmgr.h"
#include "lltimer.h"
// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-4.2
#include "llwearableitemslist.h"
// [/SL:KB]

class LLAccordionCtrl;
class LLAccordionCtrlTab;
class LLInventoryCategoriesObserver;
class LLListContextMenu;
class LLScrollListCtrl;
//class LLWearableItemsList;
class LLWearingGearMenu;
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
class LLWornItemsList;
class LLInventoryPanel;
class LLMenuButton;
class LLWearingSortMenu;
class LLSaveFolderState;

//////////////////////////////////////////////////////////////////////////

class LLWornItemsList : public LLWearableItemsList
{
	friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLWearableItemsList::Params>
	{
		Params() {}
	};
protected:
	LLWornItemsList(const LLWornItemsList::Params& p);

public:
	void refreshList(const std::vector<LLPointer<LLViewerInventoryItem>> item_array) override;
	void setSortOrder(ESortOrder sortOrder, bool sortNow = true) override;
protected:
	LLPanel* createNewItem(LLViewerInventoryItem* pItem) override;
};

//////////////////////////////////////////////////////////////////////////
// [/SL:KB]

/**
 * @class LLPanelWearing
 *
 * A list of agents's currently worn items represented by
 * a flat list view.
 * Starts fetching necessary inventory content on first opening.
 */
class LLPanelWearing : public LLPanelAppearanceTab
{
public:
	LLPanelWearing();
	virtual ~LLPanelWearing();

	/*virtual*/ BOOL postBuild();

//	/*virtual*/ void draw();

	/*virtual*/ void onOpen(const LLSD& info);

	/*virtual*/ void setFilterSubString(const std::string& string);

	/*virtual*/ bool isActionEnabled(const LLSD& userdata);

	/*virtual*/ void getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const;

	/*virtual*/ void copyToClipboard();

// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.3
	void onAttachmentsChanged();
	static void updateAttachmentsList(LLHandle<LLPanelWearing> hWearingPanel);
// [/SL:KB]
//	void startUpdateTimer();
//	void updateAttachmentsList();

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-23 (Catznip-3.3)
	typedef boost::signals2::signal<void()> selection_change_signal_t;
	boost::signals2::connection setSelectionChangeCallback(selection_change_signal_t::slot_type cb);
// [/SL:KB]
//	boost::signals2::connection setSelectionChangeCallback(commit_callback_t cb);

	bool hasItemSelected();

	bool populateAttachmentsList(bool update = false);
//	void onAccordionTabStateChanged();
	void setAttachmentDetails(LLSD content);
	void requestAttachmentDetails();
//	void onRemoveItem();
//	void onEditAttachment();
//	void onRemoveAttachment();

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	void onTakeOffClicked();
	void onTakeOffFolderClicked();
// [/SL:KB]
// [SL:KB] - Patch: Appearance-InvPanel | Checked: Catznip-3.3
	LLInventoryPanel* getInvPanel() const  { return mInvPanel; }
	LLWornItemsList*  getItemsList() const { return mCOFItemsList; }
protected:
	enum class EWearingView { FOLDER_VIEW = 0, LIST_VIEW = 1 };
	void createInventoryPanel();
	void onToggleWearingView(EWearingView eView);
// [/SL:KB]

private:
	void onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y);
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-23 (Catznip-3.3)
	void onSelectionChange();
// [/SL:KB]
	void onTempAttachmentsListRightClick(LLUICtrl* ctrl, S32 x, S32 y);

	void getAttachmentLimitsCoro(std::string url);

	LLInventoryCategoriesObserver* 	mCategoriesObserver;
//	LLWearableItemsList* 			mCOFItemsList;
	LLScrollListCtrl*				mTempItemsList;
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	boost::signals2::connection		mComplexityChangedSlot;
	selection_change_signal_t		mSelectionSignal;
	LLWornItemsList*				mCOFItemsList;
	LLInventoryPanel*				mInvPanel;
	LLSaveFolderState*				mSavedFolderState;
	LLMenuButton*					mSortMenuButton;
// [/SL:KB]
	LLWearingGearMenu*				mGearMenu;
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	LLWearingSortMenu*				mSortMenu;
// [/SL:KB]
	LLListContextMenu*				mContextMenu;
	LLListContextMenu*				mAttachmentsMenu;

	LLAccordionCtrlTab* 			mWearablesTab;
// [SL:KB] - Patch: Appearance-InvPanel | Checked: Catznip-5.0
	LLAccordionCtrlTab* 			mWearablesInvTab = nullptr;
// [/SL:KB]
	LLAccordionCtrlTab* 			mAttachmentsTab;
	LLAccordionCtrl*				mAccordionCtrl;

	std::map<LLUUID, LLViewerObject*> mAttachmentsMap;

	std::map<LLUUID, std::string> 	mObjectNames;

// [SL:KB] - Patch: Appearance-Wearing | Checked: Catznip-5.3
	boost::signals2::scoped_connection mAttachmentsChangedConnection;
// [/SL:KB]
//	boost::signals2::connection 	mAttachmentsChangedConnection;
//	LLFrameTimer					mUpdateTimer;

	bool							mIsInitialized;
};

#endif //LL_LLPANELWEARING_H
