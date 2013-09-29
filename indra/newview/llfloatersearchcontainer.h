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
#ifndef LL_LLFLOATERSEARCHCONTAINER_H
#define LL_LLFLOATERSEARCHCONTAINER_H

#include "llmultifloater.h"

class LLFloaterSearch;
class LLTabContainer;

// ============================================================================
// LLFloaterSearchContainer class
//

class LLFloaterSearchContainer : public LLMultiFloater
{
public:
	LLFloaterSearchContainer(const LLSD& sdKey);
	/*virtual*/ ~LLFloaterSearchContainer();

	/*
	 * Base class overrides
	 */
public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool fAppQuitting);
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void addFloater(LLFloater* pFloater, BOOL fSelect, LLTabContainer::eInsertionPoint eInsert = LLTabContainer::END);

	/*
	 * Member variables
	 */
protected:
	LLFloaterSearch* m_pWebSearch;
	LLFloater*       m_pPlacesSearch;
};

// ============================================================================

#endif // LL_LLFLOATERSEARCHCONTAINER_H
