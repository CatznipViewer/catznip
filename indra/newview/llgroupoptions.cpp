/** 
 *
 * Copyright (c) 2012, Kitty Barnett
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

#include "llagent.h"
#include "llgroupoptions.h"
#include "llsdserialize.h"


// ============================================================================
// LLGroupOptions
//

LLGroupOptions::LLGroupOptions(const LLUUID& idGroup)
	: mGroupId(idGroup), mReceiveGroupChat(true)
{
}

LLGroupOptions::LLGroupOptions(const LLSD& sdData)
{
	mGroupId = (sdData.has("group_id")) ? sdData["group_id"].asUUID() : LLUUID::null;
	mReceiveGroupChat = (sdData.has("receive_chat")) ? sdData["receive_chat"].asBoolean() : true;
}

bool LLGroupOptions::isValid() const
{
	return (mGroupId.notNull()) && (gAgent.isInGroup(mGroupId, true));
}

LLSD LLGroupOptions::toLLSD() const
{
	return LLSD().with("group_id", mGroupId).with("receive_chat", mReceiveGroupChat);
}

// ============================================================================
// LLGroupOptionsMgr
//

LLGroupOptionsMgr::LLGroupOptionsMgr()
{
	load();
}

LLGroupOptionsMgr::~LLGroupOptionsMgr()
{
}

void LLGroupOptionsMgr::clearOptions(const LLUUID& idGroup)
{
	options_map_t::iterator itOption = mGroupOptions.find(idGroup);
	if (mGroupOptions.end() != itOption)
	{
		delete itOption->second;
		mGroupOptions.erase(itOption);
	}
}

void LLGroupOptionsMgr::setOptionReceiveChat(const LLUUID& idGroup, bool fReceiveChat)
{
	LLGroupOptions* pOptions = getOptions(idGroup);
	if (pOptions)
	{
		pOptions->mReceiveGroupChat = fReceiveChat;
		save();
	}
}

LLGroupOptions* LLGroupOptionsMgr::getOptions(const LLUUID& idGroup)
{
	options_map_t::iterator itOption = mGroupOptions.find(idGroup);
	if (mGroupOptions.end() != itOption)
		return itOption->second;
		
	LLGroupOptions* pOptions = NULL;
	if (gAgent.isInGroup(idGroup))
	{
		pOptions = new LLGroupOptions(idGroup);
		mGroupOptions.insert(std::pair<LLUUID, LLGroupOptions*>(idGroup, pOptions));
	}
	return pOptions;
}

void LLGroupOptionsMgr::load()
{
	llifstream file(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "group_options.txt"));
	if (!file.is_open())
	{
		llwarns << "Can't open group options file" << llendl;
		return;
	}

	for (options_map_t::iterator itOption = mGroupOptions.begin(); itOption != mGroupOptions.end(); ++itOption)
		delete itOption->second;
	mGroupOptions.clear();

	LLPointer<LLSDNotationParser> sdParser = new LLSDNotationParser(); std::string strLine;
	while (std::getline(file, strLine))
	{
		std::istringstream iss(strLine); LLSD sdData;
		if (LLSDParser::PARSE_FAILURE == sdParser->parse(iss, sdData, strLine.length()))
		{
			llinfos << "Failed to parse group option entry" << llendl;
			continue;
		}

		LLGroupOptions* pOptions = new LLGroupOptions(sdData);
		if (!pOptions->isValid())
		{
			delete pOptions;
			continue;
		}
		mGroupOptions.insert(std::pair<LLUUID, LLGroupOptions*>(pOptions->mGroupId, pOptions));
	}

	file.close();
}

void LLGroupOptionsMgr::save()
{
	llofstream file(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "group_options.txt"));
	if (!file.is_open())
	{
		llwarns << "Can't open group options file" << llendl;
		return;
	}

	for (options_map_t::const_iterator itOption = mGroupOptions.begin(); itOption != mGroupOptions.end(); ++itOption)
	{
		if ( (itOption->second->isValid()) && (gAgent.isInGroup(itOption->second->mGroupId)) )
		{
			file << LLSDOStreamer<LLSDNotationFormatter>(itOption->second->toLLSD()) << std::endl;
		}
	}

	file.close();
}
