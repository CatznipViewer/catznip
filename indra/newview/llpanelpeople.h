/** 
 * @file llpanelpeople.h
 * @brief Side tray "People" panel
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

#ifndef LL_LLPANELPEOPLE_H
#define LL_LLPANELPEOPLE_H

#include <llpanel.h>

// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2012-07-04 (Catznip-3.3)
#include "llavatarlistitem.h"
// [/SL:KB]
#include "llcallingcard.h" // for avatar tracker
#include "llfloaterwebcontent.h"
#include "llvoiceclient.h"

class LLAvatarList;
class LLAvatarName;
class LLFilterEditor;
class LLGroupList;
class LLMenuButton;
class LLTabContainer;
class LLNetMap;

class LLPanelPeople 
	: public LLPanel
	, public LLVoiceClientStatusObserver
{
	LOG_CLASS(LLPanelPeople);
public:
	LLPanelPeople();
	virtual ~LLPanelPeople();

	/*virtual*/ BOOL 	postBuild();
	/*virtual*/ void	onOpen(const LLSD& key);
	/*virtual*/ bool	notifyChildren(const LLSD& info);
	// Implements LLVoiceClientStatusObserver::onChange() to enable call buttons
	// when voice is available
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

// [RLVa:KB] - Checked: RLVa-1.2.0
	LLAvatarList* getNearbyList() { return mNearbyList; }
	void          updateNearbyList();
// [/RLVa:KB]

	// internals
	class Updater;

	bool updateNearbyArrivalTime();

private:

	typedef enum e_sort_oder {
		E_SORT_BY_NAME = 0,
		E_SORT_BY_STATUS = 1,
		E_SORT_BY_MOST_RECENT = 2,
		E_SORT_BY_DISTANCE = 3,
		E_SORT_BY_RECENT_SPEAKERS = 4,
		E_SORT_BY_RECENT_ARRIVAL = 5,
// [SL:KB] - Patch: UI-SidepanelPeopleSort | Checked: Catnzip-6.5
		E_SORT_BY_USERNAME = 100,
// [/SL:KB]
	} ESortOrder;

    void				    removePicker();

	// methods indirectly called by the updaters
	void					updateFriendListHelpText();
	void					updateFriendList();
//	void					updateNearbyList();
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
	void					refreshRecentList();
	void					updateRecentList(bool fForceUpdate);
// [/SL:KB]
//	void					updateRecentList();
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2010-10-24 (Catznip-2.3)
	void					updateDistances();
	void					updateLastInteractionTimes();
// [/SL:KB]

	bool					isItemsFreeOfFriends(const uuid_vec_t& uuids);

	void					updateButtons();
	std::string				getActiveTabName() const;
	LLUUID					getCurrentItemID() const;
	void					getCurrentItemIDs(uuid_vec_t& selected_uuids) const;
	void					showGroupMenu(LLMenuGL* menu);
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2012-07-04 (Catznip-3.3)
	EAvatarListNameFormat	getNameFormat(LLAvatarList* list);
	void					setNameFormat(LLAvatarList* list, EAvatarListNameFormat name_format, bool save = true);
// [/SL:KB]
	void					setSortOrder(LLAvatarList* list, ESortOrder order, bool save = true);

	// UI callbacks
	void					onFilterEdit(const std::string& search_string);
	void					onGroupLimitInfo();
	void					onTabSelected(const LLSD& param);
	void					onAddFriendButtonClicked();
	void					onAddFriendWizButtonClicked();
	void					onDeleteFriendButtonClicked();
	void					onChatButtonClicked();
	void					onGearButtonClicked(LLUICtrl* btn);
	void					onImButtonClicked();
	void					onMoreButtonClicked();
	void					onAvatarListDoubleClicked(LLUICtrl* ctrl);
	void					onAvatarListCommitted(LLAvatarList* list);
	bool					onGroupPlusButtonValidate();
	void					onGroupMinusButtonClicked();
	void					onGroupPlusMenuItemClicked(const LLSD& userdata);
// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2014-01-19 (Catznip-3.6)
	void					onGroupActivateButtonClicked();
	void					onShowBlockedList();
// [/SL:KB]

	void					onFriendsViewSortMenuItemClicked(const LLSD& userdata);
	void					onNearbyViewSortMenuItemClicked(const LLSD& userdata);
	void					onGroupsViewSortMenuItemClicked(const LLSD& userdata);
	void					onRecentViewSortMenuItemClicked(const LLSD& userdata);
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-12-09 (Catznip-3.2)
	void					onRecentSetExpiration(const LLSD& sdParam);
	void					onRecentClearHistory();
// [/SL:KB]

	bool					onFriendsViewSortMenuItemCheck(const LLSD& userdata);
	bool					onRecentViewSortMenuItemCheck(const LLSD& userdata);
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-12-09 (Catznip-3.2)
	bool					onRecentCheckExpiration(const LLSD& sdParam);
// [/SL:KB]
	bool					onNearbyViewSortMenuItemCheck(const LLSD& userdata);

	// misc callbacks
	static void				onAvatarPicked(const uuid_vec_t& ids, const std::vector<LLAvatarName> names);

	void					onFriendsAccordionExpandedCollapsed(LLUICtrl* ctrl, const LLSD& param, LLAvatarList* avatar_list);

	void					showAccordion(const std::string name, bool show);

	void					showFriendsAccordionsIfNeeded();

	void					onFriendListRefreshComplete(LLUICtrl*ctrl, const LLSD& param);

	void					setAccordionCollapsedByUser(LLUICtrl* acc_tab, bool collapsed);
	void					setAccordionCollapsedByUser(const std::string& name, bool collapsed);
	bool					isAccordionCollapsedByUser(LLUICtrl* acc_tab);
	bool					isAccordionCollapsedByUser(const std::string& name);

// [SL:KB] - Patch: UI-SidepanelPeople | Checked: 2014-01-19 (Catznip-3.6)
	LLFilterEditor*			mFilterEditor;
// [/SL:KB]
	LLTabContainer*			mTabContainer;
	LLAvatarList*			mOnlineFriendList;
	LLAvatarList*			mAllFriendList;
	LLAvatarList*			mNearbyList;
	LLAvatarList*			mRecentList;
	LLGroupList*			mGroupList;
	LLNetMap*				mMiniMap;

	std::vector<std::string> mSavedOriginalFilters;
	std::vector<std::string> mSavedFilters;

	Updater*				mFriendListUpdater;
	Updater*				mNearbyListUpdater;
	Updater*				mRecentListUpdater;
//	Updater*				mButtonsUpdater;
    LLHandle< LLFloater >	mPicker;
};

#endif //LL_LLPANELPEOPLE_H
