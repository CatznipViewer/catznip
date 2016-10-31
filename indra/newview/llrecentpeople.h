/** 
 * @file llrecentpeople.h
 * @brief List of people with which the user has recently interacted.
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

#ifndef LL_LLRECENTPEOPLE_H
#define LL_LLRECENTPEOPLE_H

#include "llevent.h"
#include "llsingleton.h"
#include "lluuid.h"

#include <vector>
#include <set>
#include <boost/signals2.hpp>

class LLDate;
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
class LLRecentPeoplePersistentItem;
// [/SL:KB]

/**
 * List of people the agent recently interacted with.
 * 
 * Includes: anyone with whom the user IM'd or called
 * (1:1 and ad-hoc but not SL Group chat),
 * anyone with whom the user has had a transaction
 * (inventory offer, friend request, etc),
 * and anyone that has chatted within chat range of the user in-world. 
 * 
 *TODO: purge least recently added items? 
 */
//class LLRecentPeople: public LLSingleton<LLRecentPeople>, public LLOldEvents::LLSimpleListener
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
class LLRecentPeople : public LLSingleton<LLRecentPeople>
// [/SL:KB]
{
	LOG_CLASS(LLRecentPeople);

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-01-21 (Catznip-2.5)
	friend class LLSingleton<LLRecentPeople>;
protected:
	LLRecentPeople();
// [/SL:KB]
public:
	typedef boost::signals2::signal<void ()> signal_t;
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
public:
	enum EInteractionType
	{
		IT_GENERAL = 0,	// 
		IT_CHAT,		// Public/main chat (text or voice)
		IT_IM,			// Instant messages (text or voice)
		IT_INVENTORY,	// Offered or accepted inventory
		IT_AVALINE,
		IT_COUNT,
		IT_INVALID = IT_COUNT
	};
protected:
	static const std::string s_itTypeNames[IT_COUNT + 1];	// +1 to store "invalid"
public:
	static const std::string& getTypeNameFromType(EInteractionType eInteraction);
	static EInteractionType   getTypeFromTypeName(const std::string& strInteraction);
// [/SL:KB]

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-01-21 (Catznip-2.5)
	void load();
	void loadLegacy();
	void save() const;
	void saveLegacy() const;

	void purgeItems();
	void reloadItems();
// [/SL:KB]
	
	/**
	 * Add specified avatar to the list if it's not there already.
	 *
	 * @param id avatar to add.
	 *
	 * @param userdata additional information about last interaction party.
	 *				   For example when last interaction party is not an avatar
	 *				   but an avaline caller, additional info (such as phone
	 *				   number, session id and etc.) should be added.
	 *
	 * @return false if the avatar is in the list already, true otherwise
	 */
//	bool add(const LLUUID& id, const LLSD& userdata = LLSD().with("date", LLDate::now()));
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
	bool add(const LLUUID& id, EInteractionType interaction, const LLSD& userdata = LLSD().with("date", LLDate::now()));
// [/SL:KB]

	/**
	 * @param id avatar to search.
	 * @return true if the avatar is in the list, false otherwise.
	 */
	bool contains(const LLUUID& id) const;

	/**
	 * Get the whole list.
	 * 
	 * @param result where to put the result.
	 */
//	void get(uuid_vec_t& result) const;
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
	void get(uuid_vec_t& result, EInteractionType interaction) const;
// [/SL:KB]

	/**
	 * Returns last interaction time with specified participant
	 *
	 */
//	const LLDate getDate(const LLUUID& id) const;
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
	const LLDate getDate(const LLUUID& id, EInteractionType interaction) const;
// [/SL:KB]

	/**
	 * Returns data about specified participant
	 *
	 * @param id identifier of specific participant
	 */
	const LLSD& getData(const LLUUID& id) const;

	/**
	 * Checks whether specific participant is an avaline caller
	 *
	 * @param id identifier of specific participant
	 */
	bool isAvalineCaller(const LLUUID& id) const;

	/**
	 * Set callback to be called when the list changed.
	 * 
	 * Multiple callbacks can be set.
	 * 
	 * @return no connection; use boost::bind + boost::signals2::trackable to disconnect slots.
	 */
	void setChangedCallback(const signal_t::slot_type& cb) { mChangedSignal.connect(cb); }

	/**
	 * LLSimpleListener interface.
	 */
//	/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

private:

	const LLUUID& getIDByPhoneNumber(const LLSD& userdata);

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-01-21 (Catznip-2.5)
	typedef std::map<LLUUID, LLRecentPeoplePersistentItem> recent_people_t;
	recent_people_t     mPeople;
// [/SL:KB]
//	typedef std::map<LLUUID, LLDate> recent_people_t;
//	recent_people_t		mPeople;
	signal_t			mChangedSignal;
};

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
class LLRecentPeoplePersistentItem
{
public:
	LLRecentPeoplePersistentItem(const LLUUID& idAgent, LLRecentPeople::EInteractionType itType, const LLSD& sdUserdata = LLSD());
	LLRecentPeoplePersistentItem(const LLSD& sdItem);

	const LLUUID& getAgentId() const                     { return m_idAgent; }
	LLDate        getLastInteraction(LLRecentPeople::EInteractionType eInteraction) const;
	const LLSD&   getUserdata() const                    { return m_sdUserdata; }
	bool          isDefault() const;
	void          setLastInteraction(LLRecentPeople::EInteractionType eInteraction, LLDate timestamp = LLDate::now());
	void          setUserdata(const LLSD& sdUserdate)    { m_sdUserdata = sdUserdate; }
	LLSD          toLLSD() const;

protected:
	LLUUID	m_idAgent;
	LLDate	m_InteractionTimes[LLRecentPeople::IT_COUNT];
	LLSD	m_sdUserdata;
};
// [/SL:KB]

#endif // LL_LLRECENTPEOPLE_H
