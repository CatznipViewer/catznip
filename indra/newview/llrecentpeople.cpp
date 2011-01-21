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

// [SL:KB] - Patch: Sidepanel-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.5.0a) | Added: Catznip-2.5.0a
#include "llsd.h"
#include "llsdserialize.h"
// [/SL:KB]

#include "llrecentpeople.h"
#include "llgroupmgr.h"

#include "llagent.h"

using namespace LLOldEvents;

// [SL:KB] - Patch: Sidepanel-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.5.0a) | Added: Catznip-2.5.0a
LLRecentPeoplePersistentItem::LLRecentPeoplePersistentItem(const LLSD& sdItem)
{
	m_idAgent = sdItem["agent_id"].asUUID();
	m_Date = sdItem["date"].asDate();
}

LLSD LLRecentPeoplePersistentItem::toLLSD() const
{
	LLSD sdItem;
	sdItem["agent_id"] = m_idAgent;
	sdItem["date"] = m_Date;
	return sdItem;
}
// [/SL:KB]

// [SL:KB] - Patch: Sidepanel-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.5.0a) | Added: Catznip-2.5.0a
LLRecentPeople::LLRecentPeople()
	: mHistoryFilename("recent_people.txt")
{
	load();
}

void LLRecentPeople::load()
{
	llifstream fileHistory(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mHistoryFilename));
	if (!fileHistory.is_open())
	{
		llwarns << "Can't open people history file \"" << mHistoryFilename << "\" for reading" << llendl;
		return;
	}

	mPeople.clear();

	// The parser's destructor is protected so we cannot create in the stack.
	LLPointer<LLSDParser> sdParser = new LLSDNotationParser();

	std::string strHistoryLine; LLSD sdItem;
	while (std::getline(fileHistory, strHistoryLine))
	{
		std::istringstream iss(strHistoryLine);
		if (sdParser->parse(iss, sdItem, strHistoryLine.length()) == LLSDParser::PARSE_FAILURE)
		{
			llinfos << "Parsing saved teleport history failed" << llendl;
			break;
		}

		LLRecentPeoplePersistentItem persistentItem(sdItem);
		mPeople.insert(std::pair<LLUUID, LLRecentPeoplePersistentItem>(persistentItem.m_idAgent, persistentItem));
	}

	fileHistory.close();

	mChangedSignal();
}

void LLRecentPeople::save() const
{
	llofstream fileHistory(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mHistoryFilename));
	if (!fileHistory.is_open())
	{
		llwarns << "Can't open people history file \"" << mHistoryFilename << "\" for writing" << llendl;
		return;
	}

	for (recent_people_t::const_iterator itItem = mPeople.begin(); itItem != mPeople.end(); ++itItem)
		fileHistory << LLSDOStreamer<LLSDNotationFormatter>(itItem->second.toLLSD()) << std::endl;
	fileHistory.close();
}

void LLRecentPeople::purgeItems()
{
	mPeople.clear();
	mChangedSignal();
}

// [/SL:KB]

bool LLRecentPeople::add(const LLUUID& id)
{
	if (id == gAgent.getID())
		return false;

	bool is_not_group_id = LLGroupMgr::getInstance()->getGroupData(id) == NULL;

	if (is_not_group_id)
	{
//		LLDate date_added = LLDate::now();
//
//		//[] instead of insert to replace existing id->date with new date value
//		mPeople[id] = date_added;
// [SL:KB] - Patch: Sidepanel-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.5.0a) | Added: Catznip-2.5.0a
		// Update the timestamp if it already exists, otherwise insert a new item
		recent_people_t::iterator itItem = mPeople.find(id);
		if (mPeople.end() != itItem)
			itItem->second.m_Date = LLDate::now();
		else
			mPeople.insert(std::pair<LLUUID, LLRecentPeoplePersistentItem>(id, LLRecentPeoplePersistentItem(id)));

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

const LLDate& LLRecentPeople::getDate(const LLUUID& id) const
{
	recent_people_t::const_iterator it = mPeople.find(id);
//	if (it!= mPeople.end()) return (*it).second;
// [SL:KB] - Patch: Sidepanel-RecentPeopleStorage | Checked: 2010-01-21 (Catznip-2.5.0a) | Added: Catznip-2.5.0a
	if (it!= mPeople.end()) return (*it).second.m_Date;
// [/SL:KB]

	static LLDate no_date = LLDate();
	return no_date;
}

// virtual
bool LLRecentPeople::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	(void) userdata;
	add(event->getValue().asUUID());
	return true;
}
