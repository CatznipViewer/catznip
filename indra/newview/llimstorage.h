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

#ifndef LL_IMSTORAGE_H
#define LL_IMSTORAGE_H

#include "llsd.h"
#include "llsingleton.h"

// ============================================================================

class LLPersistentUnreadIMStorage : public LLSingleton<LLPersistentUnreadIMStorage>
{
	friend LLSingleton<LLPersistentUnreadIMStorage>;
protected:
	LLPersistentUnreadIMStorage();

public:
	void loadUnreadIMs();
	void saveUnreadIMs();
protected:
	void onMessageCountChanged(const LLSD& sdData);
	void onNameLookup(const LLUUID& idAgent, const LLAvatarName& avName, const LLSD& sdUnreadIMs);

protected:
	uuid_vec_t	m_idUnreadSessions;		// P2P IM sessions with unread messages
	std::string	m_strFilePath;
};

// ============================================================================

#endif // LL_IMSTORAGE_H
