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

#ifndef LLFLOATERBLOCKED_H
#define LLFLOATERBLOCKED_H

#include "llfloater.h"

class LLScrollListCtrl;
class LLTabContainer;

// ============================================================================

class LLFloaterBlocked : public LLFloater
{
public:
	LLFloaterBlocked(const LLSD& sdKey);
	/*virtual*/ ~LLFloaterBlocked();

public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& sdParam);
protected:
	void refreshDerender();

	/*
	 * Event callbacks
	 */
protected:
	void onDerenderEntrySelChange();
	void onDerenderEntryRemove();
	void onTabSelect(const LLSD& sdParam);

	/*
	 * Member variables
	 */
protected:
	LLTabContainer*             m_pBlockedTabs;
	LLScrollListCtrl*           m_pDerenderList;
	boost::signals2::connection m_DerenderChangeConn;
};

// ============================================================================

#endif // LLFLOATERBLOCKED_H
