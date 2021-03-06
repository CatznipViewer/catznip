/** 
 * @file llrecentpeople.cpp
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

#include "llviewerprecompiledheaders.h"

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-10-01 (Catznip-2.8)
#include "llsd.h"
#include "llsdserialize.h"
#include "llviewercontrol.h"
// [/SL:KB]

#include "llrecentpeople.h"
#include "llgroupmgr.h"

#include "llagent.h"

using namespace LLOldEvents;

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
LLRecentPeoplePersistentItem::LLRecentPeoplePersistentItem(const LLUUID& idAgent, LLRecentPeople::EInteractionType itType, const LLSD& sdUserdata)
	: m_idAgent(idAgent)
{
	setLastInteraction(itType);
	setUserdata(sdUserdata);
}

LLRecentPeoplePersistentItem::LLRecentPeoplePersistentItem(const LLSD& sdItem)
{
	static LLCachedControl<U32> CUTOFF_DAYS(gSavedSettings, "RecentPeopleCutOffDays", 0);

	m_idAgent = sdItem["agent_id"].asUUID();
	if ( (sdItem.has("interactions")) && (sdItem["interactions"].isMap()) )
	{
		const LLSD& sdInteractions = sdItem["interactions"];
		for (LLSD::map_const_iterator itInteraction = sdInteractions.beginMap(); itInteraction != sdInteractions.endMap(); ++itInteraction)
		{
			LLRecentPeople::EInteractionType eInteraction = LLRecentPeople::getTypeFromTypeName(itInteraction->first);
			const LLDate tsInteraction = itInteraction->second.asDate();
			if ( (eInteraction < LLRecentPeople::IT_COUNT) && ((0 == CUTOFF_DAYS) || (LLDate::now() - tsInteraction < CUTOFF_DAYS * 86400)) )
				setLastInteraction(eInteraction, tsInteraction);
		}
	}
	setUserdata(sdItem["userdata"]);
}

LLDate LLRecentPeoplePersistentItem::getLastInteraction(LLRecentPeople::EInteractionType eInteraction) const
{
	return (eInteraction < LLRecentPeople::IT_COUNT) ? m_InteractionTimes[eInteraction] : LLDate();
}

bool LLRecentPeoplePersistentItem::isDefault() const
{
	for (int idxInteraction = 0; idxInteraction < LLRecentPeople::IT_COUNT; idxInteraction++)
		if (getLastInteraction((LLRecentPeople::EInteractionType)idxInteraction).notNull())
			return false;
	return true;
}

void LLRecentPeoplePersistentItem::setLastInteraction(LLRecentPeople::EInteractionType eInteraction, LLDate timestamp)
{
	m_InteractionTimes[eInteraction] = timestamp;
	if ( (LLRecentPeople::IT_GENERAL != eInteraction) && (getLastInteraction(LLRecentPeople::IT_GENERAL) < timestamp) )
		m_InteractionTimes[LLRecentPeople::IT_GENERAL] = timestamp;
}

LLSD LLRecentPeoplePersistentItem::toLLSD() const
{
	LLSD sdInteractions;
	for (int idxInteraction = 0; idxInteraction < LLRecentPeople::IT_COUNT; idxInteraction++)
	{
		const LLDate timestamp = getLastInteraction((LLRecentPeople::EInteractionType)idxInteraction);
		if (timestamp.notNull())
			sdInteractions[LLRecentPeople::getTypeNameFromType((LLRecentPeople::EInteractionType)idxInteraction)] = timestamp;
	}

	LLSD sdItem;
	sdItem["agent_id"] = m_idAgent;
	sdItem["interactions"] = sdInteractions;
	sdItem["userdata"] = m_sdUserdata;

	return sdItem;
}
// [/SL:KB]

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-01-21 (Catznip-2.5)
static const char* RECENT_PEOPLE_FILENAME = "recent_people.xml";

const std::string LLRecentPeople::s_itTypeNames[] = 
{
	"general",
	"chat",
	"im",
	"inventory",
	"avaline",
	"invalid"
};

const std::string& LLRecentPeople::getTypeNameFromType(LLRecentPeople::EInteractionType eInteraction)
{
	return (eInteraction < IT_COUNT) ? s_itTypeNames[eInteraction] : s_itTypeNames[IT_INVALID];
}

LLRecentPeople::EInteractionType LLRecentPeople::getTypeFromTypeName(const std::string& strInteraction)
{
	for (int idxName = 0; idxName < IT_COUNT; idxName++)
	{
		if (s_itTypeNames[idxName] == strInteraction)
			return (EInteractionType)idxName;
	}
	return IT_INVALID;
}

LLRecentPeople::LLRecentPeople()
{
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-01-21 (Catznip-2.5)
	mSaveTimer = new LLRecentPeopleSaveTimer();
// [/SL:KB]

	load();
}

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-01-21 (Catznip-2.5)
LLRecentPeople::~LLRecentPeople()
{
	delete mSaveTimer;
	mSaveTimer = nullptr;

	save();
}
// [/SL:KB]

void LLRecentPeople::load()
{
	// Try to load the new file format if it exists, otherwise attempt to load the legacy file
	const std::string strPath = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, RECENT_PEOPLE_FILENAME);
	if (gDirUtilp->fileExists(strPath))
	{
		LLSD sdRecentPeople;

		llifstream fileRecentPeople(strPath);
		if (!fileRecentPeople.is_open())
		{
			LL_WARNS() << "Can't open recent people persistent file \"" << RECENT_PEOPLE_FILENAME << "\" for reading" << LL_ENDL;
			return;
		}
		LLSDSerialize::fromXMLDocument(sdRecentPeople, fileRecentPeople);
		fileRecentPeople.close();

		mPeople.clear();
		for (LLSD::array_const_iterator itPerson = sdRecentPeople.beginArray(), endPerson = sdRecentPeople.endArray();
				itPerson != endPerson; ++itPerson)
		{
			LLRecentPeoplePersistentItem persistentItem(*itPerson);
			if (!persistentItem.isDefault())
			{
				mPeople.insert(std::pair<LLUUID, LLRecentPeoplePersistentItem>(persistentItem.getAgentId(), persistentItem));
			}
		}
	}
}

void LLRecentPeople::save() const
{
	if (!mNeedsSave)
		return;
	mNeedsSave = false;

	llofstream fileRecentPeople(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, RECENT_PEOPLE_FILENAME));
	if (!fileRecentPeople.is_open())
	{
		LL_WARNS() << "Can't open people history file \"" << RECENT_PEOPLE_FILENAME << "\" for writing" << LL_ENDL;
		return;
	}

	LLSD sdRecentPeople;
	for (recent_people_t::const_iterator itPerson = mPeople.begin(); itPerson != mPeople.end(); ++itPerson)
	{
		sdRecentPeople.append(itPerson->second.toLLSD());
	}
	LLSDSerialize::toXML(sdRecentPeople, fileRecentPeople);

	fileRecentPeople.close();
}

void LLRecentPeople::purgeItems()
{
	mPeople.clear();
	mChangedSignal();
}

void LLRecentPeople::reloadItems()
{
	save();
	load();
}

// [/SL:KB]

//bool LLRecentPeople::add(const LLUUID& id, const LLSD& userdata)
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
bool LLRecentPeople::add(const LLUUID& id, EInteractionType interaction, const LLSD& userdata)
// [/SL:KB]
{
	if (id == gAgent.getID())
		return false;

	bool is_not_group_id = LLGroupMgr::getInstance()->getGroupData(id) == NULL;

	if (is_not_group_id)
	{
		// For each avaline call the id of caller is different even if
		// the phone number is the same.
		// To avoid duplication of avaline list items in the recent list
		// of panel People, deleting id's with similar phone number.
		const LLUUID& caller_id = getIDByPhoneNumber(userdata);
		if (caller_id.notNull())
			mPeople.erase(caller_id);

//		//[] instead of insert to replace existing id->llsd["date"] with new date value
//		mPeople[id] = userdata;
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
		// Update the timestamp and userdata if the person already exists, otherwise insert a new item
		recent_people_t::iterator itItem = mPeople.find(id);
		if (mPeople.end() != itItem)
		{
			itItem->second.setLastInteraction(interaction);
			itItem->second.setUserdata(userdata);
		}
		else
		{
			mPeople.insert(std::pair<LLUUID, LLRecentPeoplePersistentItem>(id, LLRecentPeoplePersistentItem(id, interaction, userdata)));
		}

		mNeedsSave = true;
// [/SL:KB]

		mChangedSignal();
	}

	return is_not_group_id;
}

bool LLRecentPeople::contains(const LLUUID& id) const
{
	return mPeople.find(id) != mPeople.end();
}

//void LLRecentPeople::get(uuid_vec_t& result) const
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
void LLRecentPeople::get(uuid_vec_t& result, EInteractionType interaction) const
// [/SL:KB]
{
	result.clear();
	for (recent_people_t::const_iterator pos = mPeople.begin(); pos != mPeople.end(); ++pos)
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
	{
		if ((*pos).second.getLastInteraction(interaction).notNull())
			result.push_back((*pos).first);
	}
// [/SL:KB]
//		result.push_back((*pos).first);
}

//const LLDate LLRecentPeople::getDate(const LLUUID& id) const
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
const LLDate LLRecentPeople::getDate(const LLUUID& id, EInteractionType interaction) const
// [/SL:KB]
{
	recent_people_t::const_iterator it = mPeople.find(id);
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-08-22 (Catznip-2.8)
	if (it!= mPeople.end()) return it->second.getLastInteraction(interaction);
// [/SL:KB]
//	if (it!= mPeople.end()) return it->second["date"].asDate();

	static LLDate no_date = LLDate();
	return no_date;
}

const LLSD& LLRecentPeople::getData(const LLUUID& id) const
{
	recent_people_t::const_iterator it = mPeople.find(id);

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-04-12 (Catznip-2.6)
	if (it != mPeople.end()) 
		return it->second.getUserdata();
// [/SL:KB]
//	if (it != mPeople.end())
//		return it->second;

	static LLSD no_data = LLSD();
	return no_data;
}

bool LLRecentPeople::isAvalineCaller(const LLUUID& id) const
{
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-04-12 (Catznip-2.6)
	const LLSD& sdData = getData(id);
	return (sdData.has("avaline_call")) && (sdData["avaline_call"].asBoolean());
// [/SL:KB]
//	recent_people_t::const_iterator it = mPeople.find(id);
//
//	if (it != mPeople.end())
//	{
//		const LLSD& user = it->second;		
//		return user["avaline_call"].asBoolean();
//	}
//
//	return false;
}

const LLUUID& LLRecentPeople::getIDByPhoneNumber(const LLSD& userdata)
{
	if (!userdata["avaline_call"].asBoolean())
		return LLUUID::null;

	for (recent_people_t::const_iterator it = mPeople.begin(); it != mPeople.end(); ++it)
	{
//		const LLSD& user_info = it->second;
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2011-04-12 (Catznip-2.6)
		const LLSD& user_info = it->second.getUserdata();
// [/SL:KB]
		
		if (user_info["call_number"].asString() == userdata["call_number"].asString())
			return it->first;
	}
	
	return LLUUID::null;
}

// [SL:KB] - Dead code
// virtual
//bool LLRecentPeople::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
//{
//	(void) userdata;
//	add(event->getValue().asUUID());
//	return true;
//}

void LLRecentPeople::updateAvatarsArrivalTime(uuid_vec_t& uuids)
{
	id_to_time_map_t buf = mAvatarsArrivalTime;
	mAvatarsArrivalTime.clear();

	for (uuid_vec_t::const_iterator id_it = uuids.begin(); id_it != uuids.end(); ++id_it)
	{
		if (buf.find(*id_it) != buf.end())
		{
			mAvatarsArrivalTime[*id_it] = buf[*id_it];
		}
		else
		{
			mAvatarsArrivalTime[*id_it] = LLDate::now().secondsSinceEpoch();
		}
	}
}

F32 LLRecentPeople::getArrivalTimeByID(const LLUUID& id)
{
	id_to_time_map_t::const_iterator it = mAvatarsArrivalTime.find(id);

	if (it != mAvatarsArrivalTime.end())
	{
		return it->second;
	}
	return LLDate::now().secondsSinceEpoch();
}

