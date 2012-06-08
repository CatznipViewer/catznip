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

#include "llderenderlist.h"
#include "llfloaterblocked.h"
#include "llscrolllistctrl.h"
#include "lltabcontainer.h"

// ============================================================================

//
// Constants
//
const std::string BLOCKED_PARAM_NAME  = "blocked_to_select";
const std::string DERENDER_PARAM_NAME = "derender_to_select";
const std::string BLOCKED_TAB_NAME  = "mute_tab";
const std::string DERENDER_TAB_NAME = "derender_tab";

LLFloaterBlocked::LLFloaterBlocked(const LLSD& sdKey)
	: LLFloater(sdKey), m_pBlockedTabs(NULL), m_pDerenderList(NULL)
{
}

LLFloaterBlocked::~LLFloaterBlocked()
{
	m_DerenderChangeConn.disconnect();
}

BOOL LLFloaterBlocked::postBuild()
{
	m_pBlockedTabs = findChild<LLTabContainer>("blocked_tabs");
	m_pBlockedTabs->setCommitCallback(boost::bind(&LLFloaterBlocked::onTabSelect, this, _2));

	m_pDerenderList = findChild<LLScrollListCtrl>("derender_list");
	m_pDerenderList->setCommitCallback(boost::bind(&LLFloaterBlocked::onDerenderEntrySelChange, this));
	m_pDerenderList->setCommitOnDelete(true);
	m_pDerenderList->setCommitOnSelectionChange(true);

	m_DerenderChangeConn = LLDerenderList::setChangeCallback(boost::bind(&LLFloaterBlocked::refreshDerender, this));
	findChild<LLUICtrl>("derender_trash_btn")->setCommitCallback(boost::bind(&LLFloaterBlocked::onDerenderEntryRemove, this));

	return TRUE;
}

void LLFloaterBlocked::onOpen(const LLSD& sdParam)
{
	if (sdParam.has(BLOCKED_PARAM_NAME))
	{
		m_pBlockedTabs->selectTabByName(BLOCKED_TAB_NAME);
		m_pBlockedTabs->getCurrentPanel()->onOpen(sdParam);
	}
	else if (sdParam.has(DERENDER_PARAM_NAME))
	{
		m_pBlockedTabs->selectTabByName(DERENDER_TAB_NAME);
		m_pDerenderList->selectByID(sdParam[DERENDER_PARAM_NAME].asUUID());
	}
}

void LLFloaterBlocked::onDerenderEntrySelChange()
{
	bool hasSelected = NULL != m_pDerenderList->getFirstSelected();
	getChildView("derender_trash_btn")->setEnabled(hasSelected);
}

void LLFloaterBlocked::onDerenderEntryRemove()
{
	const LLScrollListCtrl* pDerenderList = getChild<LLScrollListCtrl>("derender_list");

	std::vector<LLScrollListItem*> selItems = pDerenderList->getAllSelected(); uuid_vec_t idsObject;
	std::for_each(selItems.begin(), selItems.end(), [&idsObject](const LLScrollListItem* i) { idsObject.push_back(i->getValue().asUUID()); });

	LLDerenderList::instance().removeObjects(idsObject);
}

void LLFloaterBlocked::onTabSelect(const LLSD& sdParam)
{
	const std::string strTabName = sdParam.asString();
	if (DERENDER_TAB_NAME == strTabName)
	{
		refreshDerender();
	}
}

void LLFloaterBlocked::refreshDerender()
{
	// Sanity check - only refresh if we're the active tab
	if ((m_pBlockedTabs->getCurrentPanel()) && ("derender_tab" != m_pBlockedTabs->getCurrentPanel()->getName()))
		return;

	LLScrollListCtrl* pDerenderList = getChild<LLScrollListCtrl>("derender_list");
	pDerenderList->clearRows();

	if (LLDerenderList::instanceExists())
	{
		LLSD sdRow;	LLSD& sdColumns = sdRow["columns"];
		sdColumns[0]["column"] = "object_name";   sdColumns[0]["type"] = "text";
		sdColumns[1]["column"] = "location";      sdColumns[1]["type"] = "text";
		sdColumns[2]["column"] = "derender_type"; sdColumns[2]["type"] = "text";

		const LLDerenderList::entry_list_t& entries = LLDerenderList::instance().getEntries();
		for (auto itEntry = entries.cbegin(); itEntry != entries.cend(); ++itEntry)
		{
			sdRow["value"] = itEntry->idObject;
			sdColumns[0]["value"] = itEntry->strObjectName;
			sdColumns[1]["value"] = itEntry->strRegionName;
			sdColumns[2]["value"] = (itEntry->fPersists) ? "Permanent" : "Temporary";

			pDerenderList->addElement(sdRow, ADD_BOTTOM);
		}
	}
}

// ============================================================================
