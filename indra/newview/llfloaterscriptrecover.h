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

#ifndef LL_FLOATERSCRIPTRECOVER_H
#define LL_FLOATERSCRIPTRECOVER_H

// ============================================================================

class LLScriptRecoverQueue
{
protected:
	LLScriptRecoverQueue(const std::list<std::string>& strFiles);

public:
	static void recoverIfNeeded();

protected:
	bool recoverNext();

	void onCreateScript(const LLUUID& idItem);
	void onSavedScript(const LLUUID& idItem, const LLSD& sdContent, bool fSuccess);

protected:
	typedef std::map<std::string, LLUUID> filename_queue_t;
	filename_queue_t m_FileQueue;

	friend class LLCreateRecoverScriptCallback;
};

// ============================================================================

#endif // LL_FLOATERSCRIPTRECOVER_H
