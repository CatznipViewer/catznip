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

class LLInventoryCategoriesObserver;
class LLListContextMenu;
class LLWearableItemsList;
class LLWearingGearMenu;
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
class LLWornItemsList;
class LLInventoryPanel;
class LLMenuButton;
class LLWearingSortMenu;
class LLSaveFolderState;
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

	/*virtual*/ void onOpen(const LLSD& info);

	/*virtual*/ void setFilterSubString(const std::string& string);

	/*virtual*/ bool isActionEnabled(const LLSD& userdata);

	/*virtual*/ void getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const;

	/*virtual*/ void copyToClipboard();

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-23 (Catznip-3.3)
	typedef boost::signals2::signal<void()> selection_change_signal_t;
	boost::signals2::connection setSelectionChangeCallback(selection_change_signal_t::slot_type cb);
// [/SL:KB]
//	boost::signals2::connection setSelectionChangeCallback(commit_callback_t cb);

	bool hasItemSelected();

// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	LLInventoryPanel* getInvPanel() const  { return mInvPanel; }
	LLWornItemsList*  getItemsList() const { return mCOFItemsList; }

	void onTakeOffClicked();
	void onTakeOffFolderClicked();
protected:
	enum EWearingView { FOLDER_VIEW = 0, LIST_VIEW = 1 };
	void onToggleWearingView(EWearingView eView);

	bool createInventoryPanel();
// [/SL:KB]

private:
	void onWearableItemsListRightClick(LLUICtrl* ctrl, S32 x, S32 y);
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-23 (Catznip-3.3)
	void onSelectionChange();
// [/SL:KB]

	LLInventoryCategoriesObserver* 	mCategoriesObserver;
//	LLWearableItemsList* 			mCOFItemsList;
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	boost::signals2::connection		mComplexityChangedSlot;
	selection_change_signal_t		mSelectionSignal;
	LLWornItemsList*				mCOFItemsList;
	LLInventoryPanel*				mInvPanel;
	LLSaveFolderState*				mSavedFolderState;
	LLMenuButton*					mSortMenuButton;
	LLButton*						mToggleFolderView;
	LLButton*						mToggleListView;
// [/SL:KB]
	LLWearingGearMenu*				mGearMenu;
// [SL:KB] - Patch: Appearance-Wearing | Checked: 2012-07-11 (Catznip-3.3)
	LLWearingSortMenu*				mSortMenu;
// [/SL:KB]
	LLListContextMenu*				mContextMenu;

	bool							mIsInitialized;
};

#endif //LL_LLPANELWEARING_H
