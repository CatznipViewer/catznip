/** 
 *
 * Copyright (c) 2011-2013, Kitty Barnett
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

#include "llimstorage.h"
#include "llimview.h"
#include "llmutelist.h"
#include "llsdserialize.h"

#define FILE_UNREAD_IMS "unread_ims.xml"

// ============================================================================
// LLPersistentUnreadIMStorage class
//

LLPersistentUnreadIMStorage::LLPersistentUnreadIMStorage()
{
	m_NewMsgConn = LLIMModel::instance().addNewMsgCallback(boost::bind(&LLPersistentUnreadIMStorage::onMessageCountChanged, this, _1));
	m_NoUnreadMsgConn = LLIMModel::instance().addNoUnreadMsgsCallback(boost::bind(&LLPersistentUnreadIMStorage::onMessageCountChanged, this, _1));
}

LLPersistentUnreadIMStorage::~LLPersistentUnreadIMStorage()
{
	m_NewMsgConn.disconnect();
	m_NoUnreadMsgConn.disconnect();
}

void LLPersistentUnreadIMStorage::addPersistedUnreadIMs(const LLUUID& idSession, const std::list<LLSD>& sdMessages)
{
	// Bit of a hack: this function is called from LLIMModel::LLIMSession::loadHistory() since we can't add new IMs at that point but we 
	// still have to store them to add them at a later point (which loadUnreadIMs() will take care off after the session is created and usuable)
	const std::string strSessionId = idSession.asString();
	if ( (m_PersistedData.isMap()) && (m_PersistedData.has(strSessionId)) )
	{
		LLSD& sdUnreadIMs = m_PersistedData[strSessionId]["unread_ims"];
		for (std::list<LLSD>::const_iterator itMsg = sdMessages.begin(); itMsg != sdMessages.end(); ++itMsg)
			sdUnreadIMs.append(*itMsg);
	}
}

bool LLPersistentUnreadIMStorage::hasPersistedUnreadIM(const LLUUID& idSession) const
{
	return (m_PersistedData.isMap()) && (m_PersistedData.has(idSession.asString()));
}

int LLPersistentUnreadIMStorage::getPersistedUnreadCount(const LLUUID& idSession) const
{
	const std::string strSessionId = idSession.asString();
	if ( (m_PersistedData.isMap()) && (m_PersistedData.has(strSessionId)) )
		return m_PersistedData[strSessionId]["unread_count"].asInteger();
	return 0;
}

const std::string LLPersistentUnreadIMStorage::getPersistedUnreadMessage(const LLUUID& idSession) const
{
	const std::string strSessionId = idSession.asString();
	if ( (m_PersistedData.isMap()) && (m_PersistedData.has(strSessionId)) )
		return m_PersistedData[strSessionId]["unread_msg"].asString();
	return std::string();
}

void LLPersistentUnreadIMStorage::loadUnreadIMs()
{
	llifstream fileUnread(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILE_UNREAD_IMS));
	if (!fileUnread.is_open())
	{
		llwarns << "Failed to open the unread IM log" << llendl;
		return;
	}

	LLPointer<LLSDParser> sdParser = new LLSDXMLParser();
	if ( (LLSDParser::PARSE_FAILURE == sdParser->parse(fileUnread, m_PersistedData, LLSDSerialize::SIZE_UNLIMITED)) || (!m_PersistedData.isMap()) )
	{
		llwarns << "Failed to parse unread IM log" << llendl;
		fileUnread.close();
		return;
	}
	fileUnread.close();

	for (LLSD::map_const_iterator itSession = m_PersistedData.beginMap(); itSession != m_PersistedData.endMap(); ++itSession)
	{
		const LLSD& sdSession = itSession->second;
		if (LLIMModel::LLIMSession::P2P_SESSION != sdSession["session_type"].asInteger())
			continue; // We only support P2P sessions

		const LLUUID idAgent = sdSession["from_id"].asUUID();
		if (LLMuteList::getInstance()->isMuted(idAgent, LLMute::flagTextChat))
			continue; // Don't restore IMs of people who are muted

		const LLUUID idSession = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, idAgent);

		LLIMModel::LLIMSession* pSession = LLIMModel::getInstance()->findIMSession(idSession);
		if (!pSession)
		{
			LLIMModel::getInstance()->newSession(idSession, sdSession["session_name"].asString(), IM_NOTHING_SPECIAL, idAgent);
			pSession = LLIMModel::getInstance()->findIMSession(idSession);
		}
		if (!pSession)
		{
			continue;
		}

		if (sdSession.has("unread_ims"))
		{
			const LLSD& sdUnreadIMs = sdSession["unread_ims"];
			for (LLSD::array_const_iterator itIM = sdUnreadIMs.beginArray(); itIM != sdUnreadIMs.endArray(); ++itIM)
			{
				const LLSD& sdIM = *itIM;
				LLIMModel::getInstance()->addMessage(idSession, sdIM[LL_IM_FROM].asString(), (sdIM.has(LL_IM_FROM_ID)) ? sdIM[LL_IM_FROM_ID].asUUID() : idAgent,
				                                     sdIM[LL_IM_TEXT].asString(), sdIM[LL_IM_TIME].asString(), false);
			}
		}
	}
	m_PersistedData.clear();

	saveUnreadIMs();
}

void LLPersistentUnreadIMStorage::saveUnreadIMs()
{
	llofstream fileUnread(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, FILE_UNREAD_IMS));
	if (!fileUnread.is_open())
	{
		llwarns << "Failed to open the unread IM log" << llendl;
		return;
	}

	LLSD sdData = LLSD::emptyMap();
	for (session_map_t::const_iterator itSession = m_SessionLookup.begin(); itSession != m_SessionLookup.end(); ++itSession)
	{
		sdData[itSession->first.asString()] = itSession->second;
	}

	LLPointer<LLSDFormatter> sdFormatter = new LLSDXMLFormatter();
	sdFormatter->format(sdData, fileUnread, LLSDFormatter::OPTIONS_PRETTY);
	fileUnread.close();
}

void LLPersistentUnreadIMStorage::onMessageCountChanged(const LLSD& sdSessionData)
{
	const LLUUID idSession = sdSessionData["session_id"].asUUID();
	if (idSession.isNull())
		return;

	S32 cntOtherUnread = sdSessionData["participant_unread"].asInteger();
	
	session_map_t::iterator itSession = m_SessionLookup.find(idSession);
	if (m_SessionLookup.end() == itSession)
	{
		// We're not tracking this session yet, add it as long as there unread IMs in it
		if (0 != cntOtherUnread)
		{
			const LLIMModel::LLIMSession* pSession = LLIMModel::instance().findIMSession(idSession);
			if ( (pSession) && (pSession->isP2PSessionType()) )
			{
				LLSD sdSession = LLSD::emptyMap();
				sdSession["session_name"] = pSession->mName;			// We should always have a session name for incoming IMs but assume it can be empty anyway
				sdSession["session_type"] = pSession->mSessionType;		// Anticipate supporting other IM session types
				sdSession["from_id"] = pSession->mOtherParticipantID;
				sdSession["unread_time"] = LLLogChat::timestamp(true);	// Not currently used, but keep track of it anyway
				sdSession["unread_msg"] = sdSessionData["message"];
				sdSession["unread_count"] = cntOtherUnread;				// Not currently used, but keep track of it anyway

				m_SessionLookup.insert(session_map_t::value_type(idSession, sdSession));
			}
		}
	}
	else
	{
		// We are currently tracking this session, remove if there are no longer any unread IMs; otherwise, track unread count
		if (0 == cntOtherUnread)
		{
			m_SessionLookup.erase(itSession);
		}
		else
		{
			itSession->second["unread_count"] = cntOtherUnread;
		}
	}

	saveUnreadIMs();
}

// ============================================================================
