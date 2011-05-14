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

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.6.0a) | Added: Catznip-2.5.0a
#include "llsd.h"
#include "llsdserialize.h"
// [/SL:KB]

#include "llrecentpeople.h"
#include "llgroupmgr.h"

#include "llagent.h"

using namespace LLOldEvents;

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.6.0a) | Added: Catznip-2.5.0a
LLRecentPeoplePersistentItem::LLRecentPeoplePersistentItem(const LLSD& sdItem)
{
	m_idAgent = sdItem["agent_id"].asUUID();
	m_Date = sdItem["date"].asDate();
	m_sdUserdata = sdItem["userdata"];
}

LLSD LLRecentPeoplePersistentItem::toLLSD() const
{
	LLSD sdItem;
	sdItem["agent_id"] = m_idAgent;
	sdItem["date"] = m_Date;
	sdItem["userdata"] = m_sdUserdata;
	return sdItem;
}
// [/SL:KB]

// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.6.0a) | Added: Catznip-2.5.0a
LLRecentPeople::LLRecentPeople()
	: mPersistentFilename("recent_people.txt")
{
	load();
}

void LLRecentPeople::load()
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	llifstream fileRecentPeople(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mPersistentFilename));
	if (!fileRecentPeople.is_open())
	{
		llwarns << "Can't open recent people persistent file \"" << mPersistentFilename << "\" for reading" << llendl;
		return;
	}

	mPeople.clear();

	// The parser's destructor is protected so we cannot create in the stack.
	LLPointer<LLSDParser> sdParser = new LLSDNotationParser();

	std::string strLine; LLSD sdItem;
	while (std::getline(fileRecentPeople, strLine))
	{
		std::istringstream iss(strLine);
		if (sdParser->parse(iss, sdItem, strLine.length()) == LLSDParser::PARSE_FAILURE)
		{
			llinfos << "Parsing recent people failed" << llendl;
			break;
		}

		LLRecentPeoplePersistentItem persistentItem(sdItem);
		mPeople.insert(std::pair<LLUUID, LLRecentPeoplePersistentItem>(persistentItem.m_idAgent, persistentItem));
	}

	fileRecentPeople.close();

	mChangedSignal();
#endif // LL_RELEASE_FOR_DOWNLOAD
}

void LLRecentPeople::save() const
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	llofstream fileRecentPeople(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mPersistentFilename));
	if (!fileRecentPeople.is_open())
	{
		llwarns << "Can't open people history file \"" << mPersistentFilename << "\" for writing" << llendl;
		return;
	}

	for (recent_people_t::const_iterator itItem = mPeople.begin(); itItem != mPeople.end(); ++itItem)
		fileRecentPeople << LLSDOStreamer<LLSDNotationFormatter>(itItem->second.toLLSD()) << std::endl;
	fileRecentPeople.close();
#endif // LL_RELEASE_FOR_DOWNLOAD
}

void LLRecentPeople::purgeItems()
{
	mPeople.clear();
	mChangedSignal();
}
// [/SL:KB]

bool LLRecentPeople::add(const LLUUID& id, const LLSD& userdata)
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
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.6.0a) | Added: Catznip-2.5.0a
		// Update the timestamp and userdata if the person already exists, otherwise insert a new item
		recent_people_t::iterator itItem = mPeople.find(id);
		if (mPeople.end() != itItem)
		{
			itItem->second.m_Date = LLDate::now();
			itItem->second.m_sdUserdata = userdata;
		}
		else
		{
			mPeople.insert(std::pair<LLUUID, LLRecentPeoplePersistentItem>(id, LLRecentPeoplePersistentItem(id)));
		}

		save();
// [/SL:KB]

		mChangedSignal();
	}

	return is_not_group_id;
}

bool LLRecentPeople::contains(const LLUUID& id) const
{
	return mPeople.find(id) != mPeople.end();
}

void LLRecentPeople::get(uuid_vec_t& result) const
{
	result.clear();
	for (recent_people_t::const_iterator pos = mPeople.begin(); pos != mPeople.end(); ++pos)
		result.push_back((*pos).first);
}

const LLDate LLRecentPeople::getDate(const LLUUID& id) const
{
	recent_people_t::const_iterator it = mPeople.find(id);
//	if (it!= mPeople.end()) return it->second["date"].asDate();
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.6.0a) | Added: Catznip-2.5.0a
	if (it!= mPeople.end()) return it->second.m_Date;
// [/SL:KB]

	static LLDate no_date = LLDate();
	return no_date;
}

const LLSD& LLRecentPeople::getData(const LLUUID& id) const
{
	recent_people_t::const_iterator it = mPeople.find(id);

//	if (it != mPeople.end())
//		return it->second;
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2010-04-12 (Catznip-2.6.0a) | Added: Catznip-2.6.0a
	if (it != mPeople.end()) 
		return it->second.m_sdUserdata;
// [/SL:KB]

	static LLSD no_data = LLSD();
	return no_data;
}

bool LLRecentPeople::isAvalineCaller(const LLUUID& id) const
{
//	recent_people_t::const_iterator it = mPeople.find(id);
//
//	if (it != mPeople.end())
//	{
//		const LLSD& user = it->second;		
//		return user["avaline_call"].asBoolean();
//	}
//
//	return false;
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2010-04-12 (Catznip-2.6.0a) | Added: Catznip-2.6.0a
	const LLSD& sdData = getData(id);
	return (sdData.has("avaline_call")) && (sdData["avaline_call"].asBoolean());
// [/SL:KB]
}

const LLUUID& LLRecentPeople::getIDByPhoneNumber(const LLSD& userdata)
{
	if (!userdata["avaline_call"].asBoolean())
		return LLUUID::null;

	for (recent_people_t::const_iterator it = mPeople.begin(); it != mPeople.end(); ++it)
	{
//		const LLSD& user_info = it->second;
// [SL:KB] - Patch: Settings-RecentPeopleStorage | Checked: 2010-04-12 (Catznip-2.6.0a) | Added: Catznip-2.6.0a
		const LLSD& user_info = it->second.m_sdUserdata;
// [/SL:KB]
		
		if (user_info["call_number"].asString() == userdata["call_number"].asString())
			return it->first;
	}
	
	return LLUUID::null;
}

// virtual
bool LLRecentPeople::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	(void) userdata;
	add(event->getValue().asUUID());
	return true;
}
