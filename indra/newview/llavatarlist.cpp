/** 
 * @file llavatarlist.h
 * @brief Generic avatar list
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

#include "llavatarlist.h"

// common
#include "lltrans.h"
#include "llcommonutils.h"

// llui
#include "lltextutil.h"

// newview
#include "llagentdata.h" // for comparator
#include "llavatariconctrl.h"
#include "llavatarnamecache.h"
#include "llcallingcard.h" // for LLAvatarTracker
#include "llcachename.h"
#include "lllistcontextmenu.h"
#include "llrecentpeople.h"
// [SL:KB] - Patch: Chat-GroupModerators | Checked: Catznip-3.3
#include "llspeakers.h"
// [/SL:KB]
#include "lluuid.h"
#include "llvoiceclient.h"
#include "llviewercontrol.h"	// for gSavedSettings
#include "lltooldraganddrop.h"
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
#include <boost/algorithm/string.hpp>
// [/SL:KB]

static LLDefaultChildRegistry::Register<LLAvatarList> r("avatar_list");

// Last interaction time update period.
//static const F32 LIT_UPDATE_PERIOD = 5;
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
static const F32 TEXT_UPDATE_PERIOD = 5;
// [/SL:KB]

// Maximum number of avatars that can be added to a list in one pass.
// Used to limit time spent for avatar list update per frame.
static const unsigned ADD_LIMIT = 50;

// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2013-06-03 (Catznip-3.4)
void LLAvatarList::ShowPermissionTypeNames::declareValues()
{
	declare("never", SP_NEVER);
	declare("hover", SP_HOVER);
	declare("nondefault", SP_NONDEFAULT);
}
// [/SL:KB]

bool LLAvatarList::contains(const LLUUID& id)
{
	const uuid_vec_t& ids = getIDs();
	return std::find(ids.begin(), ids.end(), id) != ids.end();
}

void LLAvatarList::toggleIcons()
{
	// Save the new value for new items to use.
	mShowIcons = !mShowIcons;
	gSavedSettings.setBOOL(mIconParamName, mShowIcons);
	
	// Show/hide icons for all existing items.
	std::vector<LLPanel*> items;
	getItems(items);
	for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		static_cast<LLAvatarListItem*>(*it)->setAvatarIconVisible(mShowIcons);
	}
}

void LLAvatarList::setSpeakingIndicatorsVisible(bool visible)
{
// [SL:KB] - Control-AvatarListSpeakingIndicator | Checked: 2012-06-03 (Catznip-3.3.0)
	if (visible)
		LLVoiceClient::getInstance()->addObserver(this);
	else
		LLVoiceClient::getInstance()->removeObserver(this);
	refreshSpeakingIndicatorsVisibility(LLVoiceClient::getInstance()->voiceEnabled());
}

void LLAvatarList::onChange(EStatusType status, const std::string& channelURI, bool proximal)
{
	switch (status)
	{
		case STATUS_VOICE_ENABLED:
			refreshSpeakingIndicatorsVisibility(true);
			break;
		case STATUS_VOICE_DISABLED:
			refreshSpeakingIndicatorsVisibility(false);
			break;
	}
}

void LLAvatarList::refreshSpeakingIndicatorsVisibility(bool visible)
{
// [/SL:KB]
	// Save the new value for new items to use.
	mShowSpeakingIndicator = visible;
	
	// Show/hide icons for all existing items.
	std::vector<LLPanel*> items;
	getItems(items);
	for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		static_cast<LLAvatarListItem*>(*it)->showSpeakingIndicator(mShowSpeakingIndicator);
	}
}

// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2013-06-03 (Catznip-3.4)
void LLAvatarList::showPermissions(EShowPermissionType spType)
{
	// Save the value for new items to use.
	mShowPermissions = spType;

	// Enable or disable showing permissions icons for all existing items.
	std::vector<LLPanel*> items;
	getItems(items);
	for(std::vector<LLPanel*>::const_iterator it = items.begin(), end_it = items.end(); it != end_it; ++it)
	{
		static_cast<LLAvatarListItem*>(*it)->setShowPermissions(mShowPermissions);
	}
}
// [/SL:KB]
//void LLAvatarList::showPermissions(bool visible)
//{
//	// Save the value for new items to use.
//	mShowPermissions = visible;
//
//	// Enable or disable showing permissions icons for all existing items.
//	std::vector<LLPanel*> items;
//	getItems(items);
//	for(std::vector<LLPanel*>::const_iterator it = items.begin(), end_it = items.end(); it != end_it; ++it)
//	{
//		static_cast<LLAvatarListItem*>(*it)->setShowPermissions(mShowPermissions);
//	}
//}

//static bool findInsensitive(std::string haystack, const std::string& needle_upper)
//{
//    LLStringUtil::toUpper(haystack);
//    return haystack.find(needle_upper) != std::string::npos;
//}


//comparators
static const LLAvatarItemNameComparator NAME_COMPARATOR;
static const LLFlatListView::ItemReverseComparator REVERSE_NAME_COMPARATOR(NAME_COMPARATOR);

// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
LLAvatarList::TextCallbackParam::TextCallbackParam()
	: refresh_time("refresh_time", TEXT_UPDATE_PERIOD)
{
}
// [/SL:KB]

LLAvatarList::Params::Params()
: ignore_online_status("ignore_online_status", false)
//, show_last_interaction_time("show_last_interaction_time", false)
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
, show_text_field("show_text_field", false)
// [/SL:KB]
, show_info_btn("show_info_btn", true)
, show_profile_btn("show_profile_btn", true)
, show_speaking_indicator("show_speaking_indicator", true)
// [SL:KB] - Patch: UI-AvatarListVolumeSlider | Checked: 2012-06-03 (Catznip-3.3)
, show_volume_slider("show_volume_slider", false)
// [/SL:KB]
// [SL:KB] - Patch: UI-PeopleFriendPermissions | Checked: 2013-06-03 (Catznip-3.4)
, show_permissions_granted("show_permissions_granted", SP_NEVER)
// [/SL:KB]
//, show_permissions_granted("show_permissions_granted", false)
{
}

LLAvatarList::LLAvatarList(const Params& p)
:	LLFlatListViewEx(p)
, mIgnoreOnlineStatus(p.ignore_online_status)
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
, mShowTextField(p.show_text_field)
// [/SL:KB]
//, mShowLastInteractionTime(p.show_last_interaction_time)
, mContextMenu(NULL)
, mDirty(true) // to force initial update
, mNeedUpdateNames(false)
// [SL:KB] - Patch: Control-AvatarListSort | Checked: 2012-05-30 (Catznip-3.3)
, mNeedSort(false)
// [/SL:KB]
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
, mTextFieldUpdateTimer(NULL)
, mTextFieldUpdateExpiration(p.text_callback.refresh_time)
, mTextFieldUpdateSignal(NULL)
// [/SL:KB]
//, mLITUpdateTimer(NULL)
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2012-07-04 (Catnzip-3.3)
, mNameFormat(NF_DISPLAYNAME)
// [/SL:KB]
, mShowIcons(true)
, mShowInfoBtn(p.show_info_btn)
, mShowProfileBtn(p.show_profile_btn)
, mShowSpeakingIndicator(p.show_speaking_indicator)
// [SL:KB] - Patch: UI-AvatarListVolumeSlider | Checked: 2012-06-03 (Catznip-3.3)
, mShowVolumeSlider(p.show_volume_slider)
// [/SL:KB]
, mShowPermissions(p.show_permissions_granted)
//, mShowCompleteName(false)
// [RLVa:KB] - Checked: RLVa-1.2.0
, mRlvCheckShowNames(false)
// [/RLVa:KB]
{
	setCommitOnSelectionChange(true);

	// Set default sort order.
	setComparator(&NAME_COMPARATOR);

// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	if (p.text_callback.isProvided())
	{
		mShowTextField = true;
		setTextFieldCallback(initCommitCallback(p.text_callback));
	}
// [/SL:KB]
//	if (mShowLastInteractionTime)
//	{
//		mLITUpdateTimer = new LLTimer();
//		mLITUpdateTimer->setTimerExpirySec(0); // zero to force initial update
//		mLITUpdateTimer->start();
//	}

// [SL:KB] - Control-AvatarListSpeakingIndicator | Checked: 2012-06-03 (Catznip-3.3.0)
	if (mShowSpeakingIndicator)
	{
		mShowSpeakingIndicator = false;
		LLVoiceClient::getInstance()->addObserver(this);
		refreshSpeakingIndicatorsVisibility(true);
	}
// [/SL:KB]

	LLAvatarNameCache::getInstance()->addUseDisplayNamesCallback(boost::bind(&LLAvatarList::handleDisplayNamesOptionChanged, this));
}


void LLAvatarList::handleDisplayNamesOptionChanged()
{
	mNeedUpdateNames = true;
}


LLAvatarList::~LLAvatarList()
{
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	delete mTextFieldUpdateTimer;
	delete mTextFieldUpdateSignal;
// [/SL:KB]
//	delete mLITUpdateTimer;
// [SL:KB] - Control-AvatarListSpeakingIndicator | Checked: 2012-06-03 (Catznip-3.3.0)
	LLVoiceClient::getInstance()->removeObserver(this);
// [/SL:KB]
}

void LLAvatarList::setShowIcons(std::string param_name)
{
	mIconParamName= param_name;
	mShowIcons = gSavedSettings.getBOOL(mIconParamName);
}

//std::string LLAvatarList::getAvatarName(LLAvatarName av_name)
//{
//	return mShowCompleteName? av_name.getCompleteName(false) : av_name.getDisplayName();
//}

// virtual
void LLAvatarList::draw()
{
	// *NOTE dzaporozhan
	// Call refresh() after draw() to avoid flickering of avatar list items.

	LLFlatListViewEx::draw();

	if (mNeedUpdateNames)
	{
		updateAvatarNames();
	}

	if (mDirty)
		refresh();

// [SL:KB] - Patch: Control-AvatarListSort | Checked: 2012-05-30 (Catznip-3.3)
	if (mNeedSort)
		sort();
// [/SL:KB]

// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	if ( (mShowTextField) && (mTextFieldUpdateTimer) && (mTextFieldUpdateTimer->hasExpired()) )
	{
		if (mTextFieldUpdateSignal)
			(*mTextFieldUpdateSignal)(this, LLSD());
		mTextFieldUpdateTimer->setTimerExpirySec(mTextFieldUpdateExpiration); // Restart the timer
	}
// [/SL:KB]
//	if (mShowLastInteractionTime && mLITUpdateTimer->hasExpired())
//	{
//		updateLastInteractionTimes();
//		mLITUpdateTimer->setTimerExpirySec(LIT_UPDATE_PERIOD); // restart the timer
//	}
}

//virtual
void LLAvatarList::clear()
{
	getIDs().clear();
// [RLVa:KB] - Checked: RLVa-2.0.3
	// We need to be able to call this *somehow* and it actually makes moderate sense to call this in here
	updateNoItemsMessage(mNameFilter);
// [/RLVa:KB]
	setDirty(true);
	LLFlatListViewEx::clear();
}

void LLAvatarList::setNameFilter(const std::string& filter)
{
	std::string filter_upper = filter;
	LLStringUtil::toUpper(filter_upper);
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
	mNeedUpdateNames = (mNameFilter.empty() != filter.empty());	// We always want to switch to NF_COMPLETENAME when a filter is active
// [/SL:KB]
	if (mNameFilter != filter_upper)
	{
		mNameFilter = filter_upper;

		// update message for empty state here instead of refresh() to avoid blinking when switch
		// between tabs.
		updateNoItemsMessage(filter);
		setDirty();
	}
}

void LLAvatarList::sortByName()
{
	setComparator(&NAME_COMPARATOR);
	sort();
}

void LLAvatarList::setDirty(bool val /*= true*/, bool force_refresh /*= false*/)
{
	mDirty = val;
	if(mDirty && force_refresh)
	{
		refresh();
	}
}

void LLAvatarList::addAvalineItem(const LLUUID& item_id, const LLUUID& session_id, const std::string& item_name)
{
	LL_DEBUGS("Avaline") << "Adding avaline item into the list: " << item_name << "|" << item_id << ", session: " << session_id << LL_ENDL;
	LLAvalineListItem* item = new LLAvalineListItem(/*hide_number=*/false);
//	item->setAvatarId(item_id, session_id, true, false);
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
	// It doesn't really matter what we add for name format for avaline
	item->setAvatarId(item_id, session_id, NF_DISPLAYNAME, true, false);
// [/SL:KB]
	item->setName(item_name);
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2011-03-24 (Catznip-2.6)
	item->showTextField(mShowTextField);
// [/SL:KB]
//	item->showLastInteractionTime(mShowLastInteractionTime);
	item->showSpeakingIndicator(mShowSpeakingIndicator);
	item->setOnline(false);

	addItem(item, item_id);
	mIDs.push_back(item_id);
	sort();
}

//////////////////////////////////////////////////////////////////////////
// PROTECTED SECTION
//////////////////////////////////////////////////////////////////////////
void LLAvatarList::refresh()
{
	bool have_names			= TRUE;
	bool add_limit_exceeded	= false;
	bool modified			= false;
	bool have_filter		= !mNameFilter.empty();

	// Save selection.	
	uuid_vec_t selected_ids;
	getSelectedUUIDs(selected_ids);
	LLUUID current_id = getSelectedUUID();

	// Determine what to add and what to remove.
	uuid_vec_t added, removed;
	LLAvatarList::computeDifference(getIDs(), added, removed);

	// Handle added items.
	unsigned nadded = 0;
	const std::string waiting_str = LLTrans::getString("AvatarNameWaiting");

	for (uuid_vec_t::const_iterator it=added.begin(); it != added.end(); it++)
	{
		const LLUUID& buddy_id = *it;
		LLAvatarName av_name;
		have_names &= LLAvatarNameCache::get(buddy_id, &av_name);

//		if (!have_filter || findInsensitive(getAvatarName(av_name), mNameFilter))
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
		if ( (!have_filter) || (boost::icontains(LLAvatarListItem::formatAvatarName(av_name, getAvatarNameFormat()), mNameFilter)) )
// [/SL:KB]
		{
			if (nadded >= ADD_LIMIT)
			{
				add_limit_exceeded = true;
				break;
			}
			else
			{
				// *NOTE: If you change the UI to show a different string,
				// be sure to change the filter code below.
				if (LLRecentPeople::instance().isAvalineCaller(buddy_id))
				{
					const LLSD& call_data = LLRecentPeople::instance().getData(buddy_id);
					addAvalineItem(buddy_id, call_data["session_id"].asUUID(), call_data["call_number"].asString());
				}
				else
				{
//					std::string display_name = getAvatarName(av_name);
//					addNewItem(buddy_id, 
//						display_name.empty() ? waiting_str : display_name, 
//						LLAvatarTracker::instance().isBuddyOnline(buddy_id));
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
					// NOTE-Catznip: the name parameter isn't currently used, but it might be in the future
					std::string display_name = av_name.getDisplayName();
					addNewItem(buddy_id,
						display_name.empty() ? waiting_str : LLAvatarListItem::formatAvatarName(av_name, getAvatarNameFormat()), 
						LLAvatarTracker::instance().isBuddyOnline(buddy_id));
// [/SL:KB]
				}
				
				modified = true;
				nadded++;
			}
		}
	}

	// Handle removed items.
	for (uuid_vec_t::const_iterator it=removed.begin(); it != removed.end(); it++)
	{
		removeItemByUUID(*it);
		modified = true;
	}

	// Handle filter.
	if (have_filter)
	{
		std::vector<LLSD> cur_values;
		getValues(cur_values);

		for (std::vector<LLSD>::const_iterator it=cur_values.begin(); it != cur_values.end(); it++)
		{
			const LLUUID& buddy_id = it->asUUID();
			LLAvatarName av_name;
			have_names &= LLAvatarNameCache::get(buddy_id, &av_name);
//			if (!findInsensitive(getAvatarName(av_name), mNameFilter))
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
			if (!boost::icontains(LLAvatarListItem::formatAvatarName(av_name, getAvatarNameFormat()), mNameFilter))
// [/SL:KB]
			{
				removeItemByUUID(buddy_id);
				modified = true;
			}
		}
	}

	// Changed item in place, need to request sort and update columns
	// because we might have changed data in a column on which the user
	// has already sorted. JC
	sort();

	// re-select items
	//	selectMultiple(selected_ids); // TODO: implement in LLFlatListView if need
	selectItemByUUID(current_id);

	// If the name filter is specified and the names are incomplete,
	// we need to re-update when the names are complete so that
	// the filter can be applied correctly.
	//
	// Otherwise, if we have no filter then no need to update again
	// because the items will update their names.
	bool dirty = add_limit_exceeded || (have_filter && !have_names);
	setDirty(dirty);

	// Refreshed all items.
	if(!dirty)
	{
		// Highlight items matching the filter.
		std::vector<LLPanel*> items;
		getItems(items);
		for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
		{
			static_cast<LLAvatarListItem*>(*it)->setHighlight(mNameFilter);
		}

		// Send refresh_complete signal.
		mRefreshCompleteSignal(this, LLSD((S32)size(false)));
	}

	// Commit if we've added/removed items.
	if (modified)
		onCommit();
}

void LLAvatarList::updateAvatarNames()
{
	std::vector<LLPanel*> items;
	getItems(items);

	for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		LLAvatarListItem* item = static_cast<LLAvatarListItem*>(*it);
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
		item->updateAvatarName(getAvatarNameFormat());
// [/SL:KB]
//		item->setShowCompleteName(mShowCompleteName);
//		item->updateAvatarName();
	}
	mNeedUpdateNames = false;
}

// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
EAvatarListNameFormat LLAvatarList::getAvatarNameFormat() const
{
	return (mNameFilter.empty()) ? mNameFormat : NF_COMPLETENAME;
}

void LLAvatarList::setAvatarNameFormat(EAvatarListNameFormat name_format)
{
	if (mNameFormat == name_format)
		return;

	mNameFormat = name_format;
	mNeedUpdateNames = true;
}
// [/SL:KB]

bool LLAvatarList::filterHasMatches()
{
	uuid_vec_t values = getIDs();

	for (uuid_vec_t::const_iterator it=values.begin(); it != values.end(); it++)
	{
		const LLUUID& buddy_id = *it;
		LLAvatarName av_name;
		bool have_name = LLAvatarNameCache::get(buddy_id, &av_name);

		// If name has not been loaded yet we consider it as a match.
		// When the name will be loaded the filter will be applied again(in refresh()).

//		if (have_name && !findInsensitive(getAvatarName(av_name), mNameFilter))
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
		if ( (have_name) && (!boost::icontains(LLAvatarListItem::formatAvatarName(av_name, getAvatarNameFormat()), mNameFilter)) )
// [/SL:KB]
		{
			continue;
		}

		return true;
	}
	return false;
}

boost::signals2::connection LLAvatarList::setRefreshCompleteCallback(const commit_signal_t::slot_type& cb)
{
	return mRefreshCompleteSignal.connect(cb);
}

boost::signals2::connection LLAvatarList::setItemDoubleClickCallback(const mouse_signal_t::slot_type& cb)
{
	return mItemDoubleClickSignal.connect(cb);
}

//virtual
S32 LLAvatarList::notifyParent(const LLSD& info)
{
// [SL:KB] - Patch: Control-AvatarListSort | Checked: 2012-05-30 (Catznip-3.3)
	if (info.has("sort"))
	{
		mNeedSort = true;
		return 1;
	}
// [/SL:KB]
// [SL:KB] - Patch: UI-AvatarListDndShare | Checked: 2011-06-19 (Catznip-2.6)
	else if ( (info.has("select")) && (info["select"].isUUID()) )
	{
		const LLSD& sdValue = getSelectedValue();
		const LLUUID idItem = info["select"].asUUID();
		if ( (!sdValue.isDefined()) || ((sdValue.isUUID()) && (sdValue.asUUID() != idItem)) )
		{
			resetSelection();
			selectItemByUUID(idItem);
		}
	}
// [/SL:KB]
//	if (info.has("sort") && &NAME_COMPARATOR == mItemComparator)
//	{
//		sort();
//		return 1;
//	}
	return LLFlatListViewEx::notifyParent(info);
}

void LLAvatarList::addNewItem(const LLUUID& id, const std::string& name, BOOL is_online, EAddPosition pos)
{
	LLAvatarListItem* item = new LLAvatarListItem();
//	item->setShowCompleteName(mShowCompleteName);
// [RLVa:KB] - Checked: RLVa-1.2.0
	item->setRlvCheckShowNames(mRlvCheckShowNames);
// [/RLVa:KB]
	// This sets the name as a side effect
//	item->setAvatarId(id, mSessionID, mIgnoreOnlineStatus);
// [SL:KB] - Patch: Control-AvatarListNameFormat | Checked: 2010-05-30 (Catnzip-2.6)
	item->setAvatarId(id, mSessionID, getAvatarNameFormat(), mIgnoreOnlineStatus);
// [/SL:KB]
	item->setOnline(mIgnoreOnlineStatus ? true : is_online);
//	item->showLastInteractionTime(mShowLastInteractionTime);
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
	item->showTextField(mShowTextField);
// [/SL:KB]

	item->setAvatarIconVisible(mShowIcons);
	item->setShowInfoBtn(mShowInfoBtn);
	item->setShowProfileBtn(mShowProfileBtn);
	item->showSpeakingIndicator(mShowSpeakingIndicator);
// [SL:KB] - Patch: UI-AvatarListVolumeSlider | Checked: 2012-06-03 (Catznip-3.3)
	item->showVolumeSlider(mShowVolumeSlider);
// [/SL:KB]
	item->setShowPermissions(mShowPermissions);


	item->setDoubleClickCallback(boost::bind(&LLAvatarList::onItemDoubleClicked, this, _1, _2, _3, _4));

	addItem(item, id, pos);
}

// virtual
BOOL LLAvatarList::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLUICtrl::handleRightMouseDown(x, y, mask);
	if ( mContextMenu && !isAvalineItemSelected())
	{
		uuid_vec_t selected_uuids;
		getSelectedUUIDs(selected_uuids);
		mContextMenu->show(this, selected_uuids, x, y);
	}
	return handled;
}

BOOL LLAvatarList::handleMouseDown(S32 x, S32 y, MASK mask)
{
	gFocusMgr.setMouseCapture(this);

	S32 screen_x;
	S32 screen_y;
	localPointToScreen(x, y, &screen_x, &screen_y);
	LLToolDragAndDrop::getInstance()->setDragStart(screen_x, screen_y);

	return LLFlatListViewEx::handleMouseDown(x, y, mask);
}

BOOL LLAvatarList::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if(hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
	}

	return LLFlatListViewEx::handleMouseUp(x, y, mask);
}

BOOL LLAvatarList::handleHover(S32 x, S32 y, MASK mask)
{
	bool handled = hasMouseCapture();
	if(handled)
	{
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y);

		if(LLToolDragAndDrop::getInstance()->isOverThreshold(screen_x, screen_y))
		{
			// First, create the global drag and drop object
			std::vector<EDragAndDropType> types;
			uuid_vec_t cargo_ids;
			getSelectedUUIDs(cargo_ids);
			types.resize(cargo_ids.size(), DAD_PERSON);
			LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_PEOPLE;
			LLToolDragAndDrop::getInstance()->beginMultiDrag(types, cargo_ids, src);
		}
	}

	if(!handled)
	{
		handled = LLFlatListViewEx::handleHover(x, y, mask);
	}

	return handled;
}

bool LLAvatarList::isAvalineItemSelected()
{
	std::vector<LLPanel*> selected_items;
	getSelectedItems(selected_items);
	std::vector<LLPanel*>::iterator it = selected_items.begin();
	
	for(; it != selected_items.end(); ++it)
	{
		if (dynamic_cast<LLAvalineListItem*>(*it))
			return true;
	}

	return false;
}

void LLAvatarList::setVisible(BOOL visible)
{
	if ( visible == FALSE && mContextMenu )
	{
		mContextMenu->hide();
	}
	LLFlatListViewEx::setVisible(visible);
}

void LLAvatarList::computeDifference(
	const uuid_vec_t& vnew_unsorted,
	uuid_vec_t& vadded,
	uuid_vec_t& vremoved)
{
	uuid_vec_t vcur;

	// Convert LLSDs to LLUUIDs.
	{
		std::vector<LLSD> vcur_values;
		getValues(vcur_values);

		for (size_t i=0; i<vcur_values.size(); i++)
			vcur.push_back(vcur_values[i].asUUID());
	}

	LLCommonUtils::computeDifference(vnew_unsorted, vcur, vadded, vremoved);
}

// Refresh shown time of our last interaction with all listed avatars.
//void LLAvatarList::updateLastInteractionTimes()
//{
//	S32 now = (S32) LLDate::now().secondsSinceEpoch();
//	std::vector<LLPanel*> items;
//	getItems(items);
//
//// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
//	static LLCachedControl<U32> INTERACTION_FILTER(gSavedSettings, "RecentPeopleInteractionFilter", 0);
//// [/SL:KB]
//
//	for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
//	{
//		// *TODO: error handling
//		LLAvatarListItem* item = static_cast<LLAvatarListItem*>(*it);
//// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
//		S32 secs_since = now - (S32)LLRecentPeople::instance().getDate(item->getAvatarId(), (LLRecentPeople::EInteractionType)(U32)INTERACTION_FILTER).secondsSinceEpoch();
//// [/SL:KB]
////		S32 secs_since = now - (S32) LLRecentPeople::instance().getDate(item->getAvatarId()).secondsSinceEpoch();
//		if (secs_since >= 0)
//			item->setLastInteractionTime(secs_since);
//	}
//}

// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
boost::signals2::connection LLAvatarList::setTextFieldCallback(const commit_signal_t::slot_type& cb) 
{ 
	if (!mTextFieldUpdateSignal) mTextFieldUpdateSignal = new commit_signal_t();
	return mTextFieldUpdateSignal->connect(cb); 
}

void LLAvatarList::setTextFieldRefresh(F32 refresh_time)
{
	if (mTextFieldUpdateTimer)
		delete mTextFieldUpdateTimer;
	mTextFieldUpdateTimer = new LLTimer();
	mTextFieldUpdateTimer->setTimerExpirySec(0); // Zero to force initial update
	mTextFieldUpdateTimer->start();

	mTextFieldUpdateExpiration = refresh_time;
}
// [/SL:KB]

void LLAvatarList::onItemDoubleClicked(LLUICtrl* ctrl, S32 x, S32 y, MASK mask)
{
	mItemDoubleClickSignal(ctrl, x, y, mask);
}

// [SL:KB] - Patch: Control-AvatarListSort | Checked: 2012-05-30 (Catznip-3.3)
void LLAvatarList::sort()
{
	LLFlatListViewEx::sort();
	mNeedSort = false;
}
// [/SL:KB]

bool LLAvatarItemComparator::compare(const LLPanel* item1, const LLPanel* item2) const
{
	const LLAvatarListItem* avatar_item1 = dynamic_cast<const LLAvatarListItem*>(item1);
	const LLAvatarListItem* avatar_item2 = dynamic_cast<const LLAvatarListItem*>(item2);
	
	if (!avatar_item1 || !avatar_item2)
	{
		LL_ERRS() << "item1 and item2 cannot be null" << LL_ENDL;
		return true;
	}

	return doCompare(avatar_item1, avatar_item2);
}

bool LLAvatarItemNameComparator::doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
{
	std::string name1 = avatar_item1->getAvatarName();
	std::string name2 = avatar_item2->getAvatarName();

	LLStringUtil::toUpper(name1);
	LLStringUtil::toUpper(name2);

	return name1 < name2;
}

// [SL:KB] - Patch: UI-SidepanelPeopleSort | Checked: Catznip-6.5
bool LLAvatarItemUsernameComparator::doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
{
	// RLVa anonymizing might have cleared out the user names and in that case we have to sort by anonyms (=avatar name) instead

	std::string name1 = avatar_item1->getAvatarUsername();
	if (name1.empty())
	{
		name1 = avatar_item1->getAvatarName();
	}

	std::string name2 = avatar_item2->getAvatarUsername();
	if (name2.empty())
	{
		name2 = avatar_item1->getAvatarName();
	}

	LLStringUtil::toUpper(name1);
	LLStringUtil::toUpper(name2);

	return name1 < name2;
}
// [/SL:KB]

bool LLAvatarItemAgentOnTopComparator::doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
{
	//keep agent on top, if first is agent, 
	//then we need to return true to elevate this id, otherwise false.
	if(avatar_item1->getAvatarId() == gAgentID)
	{
		return true;
	}
	else if (avatar_item2->getAvatarId() == gAgentID)
	{
		return false;
	}
	return LLAvatarItemNameComparator::doCompare(avatar_item1,avatar_item2);
}

// [SL:KB] - Patch: Chat-GroupModerators | Checked: Catznip-3.3
bool LLAvatarItemRecentSpeakerComparator::doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
{
	if (LLSpeakerMgr* pSpeakerMgr = (m_pSpeakerMgr) ? m_pSpeakerMgr : LLActiveSpeakerMgr::getInstance())
	{
		LLPointer<LLSpeaker> lhs = (avatar_item1) ? pSpeakerMgr->findSpeaker(avatar_item1->getAvatarId()) : nullptr;
		LLPointer<LLSpeaker> rhs = (avatar_item2) ? pSpeakerMgr->findSpeaker(avatar_item2->getAvatarId()) : nullptr;
		if ( (lhs.notNull()) && (rhs.notNull()) )
		{
			// Compare by last speaking time
			if (lhs->mLastSpokeTime != rhs->mLastSpokeTime)
				return (lhs->mLastSpokeTime > rhs->mLastSpokeTime);
			else if (lhs->mSortIndex != rhs->mSortIndex)
				return (lhs->mSortIndex < rhs->mSortIndex);
		}

		// True if only avatar_item1 speaker info available
		return lhs.notNull();
	}

	// By default compare by name.
	return LLAvatarItemNameComparator::doCompare(avatar_item1, avatar_item2);
}
// [/SL:KB]

/************************************************************************/
/*             class LLAvalineListItem                                  */
/************************************************************************/
LLAvalineListItem::LLAvalineListItem(bool hide_number/* = true*/) : LLAvatarListItem(false)
, mIsHideNumber(hide_number)
{
	// should not use buildPanel from the base class to ensure LLAvalineListItem::postBuild is called.
	buildFromFile( "panel_avatar_list_item.xml");
}

BOOL LLAvalineListItem::postBuild()
{
	BOOL rv = LLAvatarListItem::postBuild();

	if (rv)
	{
		setOnline(true);
// [SL:KB] - Patch: UI-AvatarListTextField | Checked: 2010-10-24 (Catznip-2.3)
		showTextField(false);
// [/SL:KB]
//		showLastInteractionTime(false);
		setShowProfileBtn(false);
		setShowInfoBtn(false);
		mAvatarIcon->setValue("Avaline_Icon");
		mAvatarIcon->setToolTip(std::string(""));
	}
	return rv;
}

// to work correctly this method should be called AFTER setAvatarId for avaline callers with hidden phone number
void LLAvalineListItem::setName(const std::string& name)
{
	if (mIsHideNumber)
	{
		static U32 order = 0;
		typedef std::map<LLUUID, U32> avaline_callers_nums_t;
		static avaline_callers_nums_t mAvalineCallersNums;

		llassert(getAvatarId() != LLUUID::null);

		const LLUUID &uuid = getAvatarId();

		if (mAvalineCallersNums.find(uuid) == mAvalineCallersNums.end())
		{
			mAvalineCallersNums[uuid] = ++order;
			LL_DEBUGS("Avaline") << "Set name for new avaline caller: " << uuid << ", order: " << order << LL_ENDL;
		}
		LLStringUtil::format_map_t args;
		args["[ORDER]"] = llformat("%u", mAvalineCallersNums[uuid]);
		std::string hidden_name = LLTrans::getString("AvalineCaller", args);

		LL_DEBUGS("Avaline") << "Avaline caller: " << uuid << ", name: " << hidden_name << LL_ENDL;
		LLAvatarListItem::setAvatarName(hidden_name);
		LLAvatarListItem::setAvatarToolTip(hidden_name);
	}
	else
	{
		const std::string& formatted_phone = LLTextUtil::formatPhoneNumber(name);
		LLAvatarListItem::setAvatarName(formatted_phone);
		LLAvatarListItem::setAvatarToolTip(formatted_phone);
	}
}
