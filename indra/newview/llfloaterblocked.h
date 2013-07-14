/** 
 *
 * Copyright (c) 2012-2013, Kitty Barnett
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

#include "llavatarname.h"
#include "llfloater.h"
#include "llmutelist.h"

class LLScrollListCtrl;
class LLTabContainer;

// ============================================================================
// LLPanelBlockList
//

class LLPanelBlockList : public LLPanel, public LLMuteListObserver
{
public:
	LLPanelBlockList();
	/*virtual*/ ~LLPanelBlockList();

	/*
	 * LLPanel overrides
	 */
public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& sdParam);

	/*
	 * Member functions
	 */
protected:
	void refresh();
	void removePicker();
	void updateButtons();

	/*
	 * Event handlers
	 */
public:
	/*virtual*/ void onChange();
protected:
	       void onClickAddAvatar(LLUICtrl* pCtrl);
	static void onClickAddAvatarCallback(const uuid_vec_t& idAgents, const std::vector<LLAvatarName>& avAgents);
	static void onClickAddByName();
	static void onClickAddByNameCallback(const std::string& strBlockName);
	       void onClickRemoveSelection();
		   void onColumnSortChange();
	       void onSelectionChange();

	/*
	 * Member variables
	 */
protected:
	bool                m_fRefreshOnChange;
	LLScrollListCtrl*   m_pBlockList;
	LLButton*           m_pTrashBtn;
    LLHandle<LLFloater> m_hPicker;
};

// ============================================================================
// LLPanelDerenderList
//

class LLPanelDerenderList : public LLPanel
{
public:
	LLPanelDerenderList();
	/*virtual*/ ~LLPanelDerenderList();

	/*
	 * LLPanel overrides
	 */
public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& sdParam);

	/*
	 * Member functions
	 */
protected:
	void refresh();

	/*
	 * Event handlers
	 */
protected:
	void onColumnSortChange();
	void onSelectionChange();
	void onSelectionRemove();

	/*
	 * Member variables
	 */
protected:
	LLScrollListCtrl*           m_pDerenderList;
	boost::signals2::connection m_DerenderChangeConn;
};

// ============================================================================
// LLFloaterBlocked
//

class LLFloaterBlocked : public LLFloater
{
public:
	LLFloaterBlocked(const LLSD& sdKey);
	/*virtual*/ ~LLFloaterBlocked();

public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& sdParam);

	/*
	 * Event handlers
	 */
protected:
	void onTabSelect(const LLSD& sdParam);

	/*
	 * Member functions
	 */
public:
	static void showMuteAndSelect(const LLUUID& idMute);
	static void showDerenderAndSelect(const LLUUID& idEntry);

	/*
	 * Member variables
	 */
protected:
	LLTabContainer*             m_pBlockedTabs;
};

// ============================================================================

#endif // LLFLOATERBLOCKED_H
