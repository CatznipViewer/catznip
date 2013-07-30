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

#ifndef LL_FLOATERSCRIPTRECOVER_H
#define LL_FLOATERSCRIPTRECOVER_H

#include "llfloater.h"

// ============================================================================
// LLFloaterAssetRecovery
//

class LLFloaterAssetRecovery : public LLFloater
{
	friend class LLFloaterReg;
private:
	LLFloaterAssetRecovery(const LLSD& sdKey);

	/*
	 * LLFloater overrides
	 */
public:
	/*virtual*/ void onOpen(const LLSD& sdKey);
	/*virtual*/ BOOL postBuild();

	/*
	 * Member functions
	 */
protected:
	void onBtnCancel();
	void onBtnRecover();
};

// ============================================================================
// LLAssetRecoverQueue
//

class LLAssetRecoverQueue
{
	friend class LLCreateRecoverAssetCallback;
	friend class LLFloaterAssetRecovery;
protected:
	LLAssetRecoverQueue(const LLSD& sdFiles);

public:
	static void recoverIfNeeded();

protected:
	bool recoverNext();

	void onCreateItem(const LLUUID& idItem);
	void onSavedAsset(const LLUUID& idItem, const LLSD& sdContent, bool fSuccess);
	bool onUploadError(const std::string& strFilename);

protected:
	typedef std::map<std::string, LLSD> filename_queue_t;
	filename_queue_t m_FileQueue;
};

// ============================================================================

#endif // LL_FLOATERSCRIPTRECOVER_H
