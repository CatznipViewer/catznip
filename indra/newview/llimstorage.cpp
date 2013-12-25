/** 
 *
 * Copyright (c) 2011, Kitty Barnett
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

#include "llavatarnamecache.h"
#include "llimstorage.h"
#include "llimview.h"
#include "llmutelist.h"
#include "llsdserialize.h"

// ============================================================================

LLPersistentUnreadIMStorage::LLPersistentUnreadIMStorage()
{
	LLIMModel::instance().addNewMsgCallback(boost::bind(&LLPersistentUnreadIMStorage::onMessageCountChanged, this, _1));
	LLIMModel::instance().addNoUnreadMsgsCallback(boost::bind(&LLPersistentUnreadIMStorage::onMessageCountChanged, this, _1));

	m_strFilePath = gDirUtilp->getExpandedFilename (LL_PATH_PER_SL_ACCOUNT, "unread_ims.xml" );
}

void LLPersistentUnreadIMStorage::loadUnreadIMs()
{
	llifstream fileUnread(m_strFilePath.c_str());
	if (!fileUnread.is_open())
	{
		llwarns << "Failed to open " << m_strFilePath << llendl;
		return;
	}

	LLSD sdData;
	LLPointer<LLSDParser> sdParser = new LLSDXMLParser();
	if (sdParser->parse(fileUnread, sdData, LLSDSerialize::SIZE_UNLIMITED) < 0)
	{
		llwarns << "Failed to parse unread IMs" << llendl;
		return;
	}

	for (LLSD::array_const_iterator itSession = sdData.beginArray(); itSession != sdData.endArray(); ++itSession)
	{
		const LLSD& sdSession = *itSession;

		LLUUID idAgent = sdSession["participant_id"].asUUID();
		S32 typeSession = sdSession["session_type"].asInteger();
		if ( (idAgent.isNull()) || (LLIMModel::LLIMSession::P2P_SESSION != typeSession) )
			continue;

		LLAvatarNameCache::get(idAgent, boost::bind(&LLPersistentUnreadIMStorage::onNameLookup, this, _1, _2, sdSession["unread_ims"]));
	}
}

void LLPersistentUnreadIMStorage::onNameLookup(const LLUUID& idAgent, const LLAvatarName& avName, const LLSD& sdUnreadIMs)
{
	if (LLMuteList::getInstance()->isMuted(idAgent, LLMute::flagTextChat))
		return;

	LLUUID idSession = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, idAgent);
	if (!LLIMMgr::getInstance()->hasSession(idSession))
		LLIMModel::getInstance()->newSession(idSession, avName.getCompleteName(), IM_NOTHING_SPECIAL, idAgent);

	LLIMModel::LLIMSession* pSession = LLIMModel::getInstance()->findIMSession(idSession);
	if ( (!pSession) || (!pSession->isP2PSessionType()) )
		return;

	for (LLSD::array_const_iterator itIM = sdUnreadIMs.beginArray(); itIM != sdUnreadIMs.endArray(); ++itIM)
	{
		const LLSD& sdIM = *itIM;
		LLIMModel::getInstance()->addMessage(idSession, sdIM["from"].asString(), sdIM["from_id"].asUUID(),  sdIM["message"].asString(), false, sdIM["time"]);
	}
}
										  
void LLPersistentUnreadIMStorage::saveUnreadIMs()
{
	llofstream fileUnread(m_strFilePath.c_str());
	if (!fileUnread.is_open())
	{
		llwarns << "Failed to open " << m_strFilePath << llendl;
		return;
	}

	LLSD sdData;
	for (uuid_vec_t::const_iterator itSession = m_idUnreadSessions.begin(); itSession != m_idUnreadSessions.end(); ++itSession)
	{
		LLIMModel::LLIMSession* pSession = LLIMModel::instance().findIMSession(*itSession);
		if ( (!pSession) || (!pSession->isP2PSessionType()) )
			continue;

		LLSD sdSession;
		sdSession["participant_id"] = pSession->mOtherParticipantID;
		sdSession["session_type"] = pSession->mSessionType;
		LLSD& sdIMs = sdSession["unread_ims"];

		std::list<LLSD>::const_reverse_iterator itMsg = pSession->mMsgs.rend();
		std::advance(itMsg, -1 * pSession->mNumUnread);
		for (; itMsg != pSession->mMsgs.rend(); ++itMsg )
			sdIMs.append(*itMsg);

		sdData.append(sdSession);
	}

	LLPointer<LLSDFormatter> sdFormatter = new LLSDXMLFormatter();
	sdFormatter->format(sdData, fileUnread, LLSDFormatter::OPTIONS_PRETTY);
}

void LLPersistentUnreadIMStorage::onMessageCountChanged(const LLSD& sdSessionData)
{
	const LLUUID idSession = sdSessionData["session_id"].asUUID();
	if (idSession.isNull())
		return;

	U32 cntOtherUnread = sdSessionData["participant_unread"].asInteger();
	uuid_vec_t::iterator itSession = std::find(m_idUnreadSessions.begin(), m_idUnreadSessions.end(), idSession);
	if (m_idUnreadSessions.end() == itSession)
	{
		// We're not tracking this session yet; add it as long as there unread IMs in it
		if (0 != cntOtherUnread)
			m_idUnreadSessions.push_back(idSession);
	}
	else
	{
		// We are currently tracking this session; remove if there are no longer any unread IMs
		if (0 == cntOtherUnread)
			m_idUnreadSessions.erase(itSession);
	}

	saveUnreadIMs();
}

// ============================================================================
