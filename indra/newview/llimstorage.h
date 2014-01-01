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
#ifndef LL_IMSTORAGE_H
#define LL_IMSTORAGE_H

#include "llsd.h"
#include "llsingleton.h"

// ============================================================================
// LLPersistentUnreadIMStorage class
//

class LLPersistentUnreadIMStorage : public LLSingleton<LLPersistentUnreadIMStorage>
{
	friend class LLSingleton<LLPersistentUnreadIMStorage>;
protected:
	LLPersistentUnreadIMStorage();
	/*virtual */ ~LLPersistentUnreadIMStorage();

	/*
	 * Member functions
	 */
public:
	// The following functions act on persisted data rather than the current data
	void              addPersistedUnreadIMs(const LLUUID& idSession, const std::list<LLSD>& sdMessages);
	int               getPersistedUnreadCount(const LLUUID& idSession) const;
	const std::string getPersistedUnreadMessage(const LLUUID& idSession) const;
	bool              hasPersistedUnreadIM(const LLUUID& idSession) const;
	// General purpose functions
	void              loadUnreadIMs();
	void              saveUnreadIMs();
protected:
	void onMessageCountChanged(const LLSD& sdData);

	/*
	 * Member variables
	 */
protected:
	LLSD          m_PersistedData; // Persisted P2P IM sessions with unread messages
	typedef std::map<LLUUID, LLSD> session_map_t;
	session_map_t m_SessionLookup; // Active P2P IM sessions with unread messages

	boost::signals2::connection m_NewMsgConn;
	boost::signals2::connection m_NoUnreadMsgConn;
};

// ============================================================================

#endif // LL_IMSTORAGE_H
