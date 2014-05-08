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

#include "llviewerprecompiledheaders.h"

#include "llavatarname.h"
#include "llderenderlist.h"
#include "llfloateravatarpicker.h"
#include "llfloaterblocked.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llpanelblockedlist.h"
#include "llscrolllistctrl.h"
#include "lltabcontainer.h"
#include "llviewercontrol.h"

// ============================================================================
// Constants
//

const std::string BLOCKED_PARAM_NAME  = "blocked_to_select";
const std::string DERENDER_PARAM_NAME = "derender_to_select";
const std::string BLOCKED_TAB_NAME  = "mute_tab";
const std::string DERENDER_TAB_NAME = "derender_tab";

// ============================================================================
// LLPanelBlockList
//

static LLPanelInjector<LLPanelBlockList> t_panel_blocked_list("panel_block_list");

LLPanelBlockList::LLPanelBlockList()
	: LLPanel()
	, m_fRefreshOnChange(true)
	, m_pBlockList(NULL)
	, m_pTrashBtn(NULL)
{
	mCommitCallbackRegistrar.add("Block.AddAvatar", boost::bind(&LLPanelBlockList::onClickAddAvatar, this, _1));
	mCommitCallbackRegistrar.add("Block.AddByName",	boost::bind(&LLPanelBlockList::onClickAddByName));
	mCommitCallbackRegistrar.add("Block.Remove", boost::bind(&LLPanelBlockList::onClickRemoveSelection, this));
}

LLPanelBlockList::~LLPanelBlockList()
{
	LLMuteList::getInstance()->removeObserver(this);
}

BOOL LLPanelBlockList::postBuild()
{
    setVisibleCallback(boost::bind(&LLPanelBlockList::removePicker, this));

	m_pBlockList = findChild<LLScrollListCtrl>("block_list");
	m_pBlockList->setCommitOnDelete(true);
	m_pBlockList->setCommitOnSelectionChange(true);
	m_pBlockList->setCommitCallback(boost::bind(&LLPanelBlockList::onSelectionChange, this));

	// Restore last sort order
	U32 nSortValue = gSavedSettings.getU32("BlockMuteSortOrder");
	if (nSortValue)
	{
		m_pBlockList->sortByColumnIndex(nSortValue >> 4, nSortValue & 0xF);
	}
	m_pBlockList->setSortChangedCallback(boost::bind(&LLPanelBlockList::onColumnSortChange, this));

	m_pTrashBtn = findChild<LLButton>("block_trash_btn");

	LLMuteList::getInstance()->addObserver(this);

	return TRUE;
}

void LLPanelBlockList::onOpen(const LLSD& sdParam)
{
	refresh();

	if ( (sdParam.has(BLOCKED_PARAM_NAME)) && (sdParam[BLOCKED_PARAM_NAME].asUUID().notNull()) )
	{
		m_pBlockList->selectByID(sdParam[BLOCKED_PARAM_NAME].asUUID());
	}
}

void LLPanelBlockList::refresh()
{
	const LLSD& sdSel = m_pBlockList->getSelectedValue();
	m_pBlockList->deleteAllItems();

	LLSD sdRow;	LLSD& sdColumns = sdRow["columns"];
	sdColumns[0]["column"] = "item_name"; sdColumns[0]["type"] = "text";
	sdColumns[1]["column"] = "item_type"; sdColumns[1]["type"] = "text";

	std::vector<LLMute> lMutes = LLMuteList::getInstance()->getMutes();
	for (std::vector<LLMute>::const_iterator itMute = lMutes.begin(); itMute != lMutes.end(); ++itMute)
	{
		sdRow["value"] = LLSD().with("id", itMute->mID).with("name", itMute->mName);
		sdColumns[0]["value"] = itMute->mName;
		sdColumns[1]["value"] = itMute->getDisplayType();

		m_pBlockList->addElement(sdRow, ADD_BOTTOM);
	}
	m_pBlockList->setSelectedByValue(sdSel, TRUE);
}

void LLPanelBlockList::removePicker()
{
    if(m_hPicker.get())
    {
        m_hPicker.get()->closeFloater();
    }
}

void LLPanelBlockList::updateButtons()
{
	bool fHasSelection = (NULL != m_pBlockList->getFirstSelected());
	m_pTrashBtn->setEnabled(fHasSelection);
}

void LLPanelBlockList::onChange()
{
	if (m_fRefreshOnChange)
	{
		refresh();
	}
}

void LLPanelBlockList::onClickAddAvatar(LLUICtrl* pCtrl)
{
    LLFloater* pRootFloater = gFloaterView->getParentFloater(pCtrl);

	LLFloaterAvatarPicker* pPicker = LLFloaterAvatarPicker::show(
		boost::bind(&LLPanelBlockList::onClickAddAvatarCallback, _1, _2),
		FALSE /*allow_multiple*/, TRUE /*close_on_select*/, FALSE /*skip_agent*/, pRootFloater->getName(), pCtrl);
    
    if (pRootFloater)
    {
        pRootFloater->addDependentFloater(pPicker);
    }

	m_hPicker = pPicker->getHandle();
}

// static
void LLPanelBlockList::onClickAddAvatarCallback(const uuid_vec_t& idAgents, const std::vector<LLAvatarName>& avAgents)
{
	if ( (idAgents.empty()) || (avAgents.empty()) )
	{
		return;
	}

	LLMute mute(idAgents[0], avAgents[0].getLegacyName(), LLMute::AGENT);
	if (LLMuteList::getInstance()->add(mute))
	{
		LLFloaterBlocked::showMuteAndSelect(mute.mID);
	}
}

// static
void LLPanelBlockList::onClickAddByName()
{
	LLFloaterGetBlockedObjectName::show(&LLPanelBlockList::onClickAddByNameCallback);
}

// static
void LLPanelBlockList::onClickAddByNameCallback(const std::string& strBlockName)
{
	if (strBlockName.empty())
	{
		return;
	}

	LLMute mute(LLUUID::null, strBlockName, LLMute::BY_NAME);
	if (!LLMuteList::getInstance()->add(mute))
	{
		LLNotificationsUtil::add("MuteByNameFailed");
	}
}

void LLPanelBlockList::onClickRemoveSelection()
{
	m_fRefreshOnChange = false;

	std::vector<LLScrollListItem*> selItems = m_pBlockList->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator itItem = selItems.begin(); itItem != selItems.end(); ++itItem)
	{
		LLScrollListItem* pSelItem = *itItem; const LLSD sdValue = pSelItem->getValue();

		LLMute selMute(sdValue["id"].asUUID(), sdValue["name"].asString());
		if (LLMuteList::getInstance()->remove(selMute))
		{
			m_pBlockList->deleteSingleItem(pSelItem);
		}
	}

	m_fRefreshOnChange = true;
}

void LLPanelBlockList::onColumnSortChange()
{
	U32 nSortValue = 0;
	
	S32 idxColumn = m_pBlockList->getSortColumnIndex();
	if (-1 != idxColumn)
	{
		nSortValue = idxColumn << 4 | ((m_pBlockList->getSortAscending()) ? 1 : 0);
	}

	gSavedSettings.setU32("BlockMuteSortOrder", nSortValue);
}

void LLPanelBlockList::onSelectionChange()
{
	updateButtons();
}

// ============================================================================
// LLPanelDerenderList
//

static LLPanelInjector<LLPanelDerenderList> t_panel_derender_list("panel_derender_list");

LLPanelDerenderList::LLPanelDerenderList()
	: LLPanel()
	, m_pDerenderList(NULL)
{
}

LLPanelDerenderList::~LLPanelDerenderList()
{
	m_DerenderChangeConn.disconnect();
}

BOOL LLPanelDerenderList::postBuild()
{
	m_pDerenderList = findChild<LLScrollListCtrl>("derender_list");
	m_pDerenderList->setCommitCallback(boost::bind(&LLPanelDerenderList::onSelectionChange, this));
	m_pDerenderList->setCommitOnDelete(true);
	m_pDerenderList->setCommitOnSelectionChange(true);

	// Restore last sort order
	U32 nSortValue = gSavedSettings.getU32("BlockDerenderSortOrder");
	if (nSortValue)
	{
		m_pDerenderList->sortByColumnIndex(nSortValue >> 4, nSortValue & 0xF);
	}
	m_pDerenderList->setSortChangedCallback(boost::bind(&LLPanelDerenderList::onColumnSortChange, this));

	m_DerenderChangeConn = LLDerenderList::setChangeCallback(boost::bind(&LLPanelDerenderList::refresh, this));
	findChild<LLUICtrl>("derender_trash_btn")->setCommitCallback(boost::bind(&LLPanelDerenderList::onSelectionRemove, this));

	return TRUE;
}

void LLPanelDerenderList::onOpen(const LLSD& sdParam)
{
	refresh();

	if ( (sdParam.has(DERENDER_PARAM_NAME)) && (sdParam[DERENDER_PARAM_NAME].asUUID().notNull()) )
	{
		m_pDerenderList->selectByID(sdParam[DERENDER_PARAM_NAME].asUUID());
	}
}

void LLPanelDerenderList::onColumnSortChange()
{
	U32 nSortValue = 0;
	
	S32 idxColumn = m_pDerenderList->getSortColumnIndex();
	if (-1 != idxColumn)
	{
		nSortValue = idxColumn << 4 | ((m_pDerenderList->getSortAscending()) ? 1 : 0);
	}

	gSavedSettings.setU32("BlockDerenderSortOrder", nSortValue);
}

void LLPanelDerenderList::onSelectionChange()
{
	bool hasSelected = (NULL != m_pDerenderList->getFirstSelected());
	getChildView("derender_trash_btn")->setEnabled(hasSelected);
}

void LLPanelDerenderList::onSelectionRemove()
{
	std::vector<LLScrollListItem*> selItems = m_pDerenderList->getAllSelected(); uuid_vec_t idsObject;
//	std::for_each(selItems.begin(), selItems.end(), [&idsObject](const LLScrollListItem* i) { idsObject.push_back(i->getValue().asUUID()); });
	for (std::vector<LLScrollListItem*>::iterator itItem = selItems.begin(); itItem != selItems.end(); ++itItem)
	{
		idsObject.push_back((*itItem)->getValue().asUUID());
	}

	LLDerenderList::instance().removeObjects(idsObject);
}

void LLPanelDerenderList::refresh()
{
	m_pDerenderList->clearRows();
	if (LLDerenderList::instanceExists())
	{
		LLSD sdRow;	LLSD& sdColumns = sdRow["columns"];
		sdColumns[0]["column"] = "object_name";   sdColumns[0]["type"] = "text";
		sdColumns[1]["column"] = "location";      sdColumns[1]["type"] = "text";
		sdColumns[2]["column"] = "derender_type"; sdColumns[2]["type"] = "text";

		const LLDerenderList::entry_list_t& entries = LLDerenderList::instance().getEntries();
		for (LLDerenderList::entry_list_t::const_iterator itEntry = entries.begin(); itEntry != entries.end(); ++itEntry)
		{
			sdRow["value"] = itEntry->idObject;
			sdColumns[0]["value"] = itEntry->strObjectName;
			sdColumns[1]["value"] = itEntry->strRegionName;
			sdColumns[2]["value"] = (itEntry->fPersists) ? "Permanent" : "Temporary";

			m_pDerenderList->addElement(sdRow, ADD_BOTTOM);
		}
	}
}

// ============================================================================
// LLFloaterBlocked
//

LLFloaterBlocked::LLFloaterBlocked(const LLSD& sdKey)
	: LLFloater(sdKey)
	, m_pBlockedTabs(NULL)
{
}

LLFloaterBlocked::~LLFloaterBlocked()
{
}

BOOL LLFloaterBlocked::postBuild()
{
	m_pBlockedTabs = findChild<LLTabContainer>("blocked_tabs");
	m_pBlockedTabs->setCommitCallback(boost::bind(&LLFloaterBlocked::onTabSelect, this, _2));

	return TRUE;
}

void LLFloaterBlocked::onOpen(const LLSD& sdParam)
{
	if (sdParam.has(BLOCKED_PARAM_NAME))
		m_pBlockedTabs->selectTabByName(BLOCKED_TAB_NAME);
	else if (sdParam.has(DERENDER_PARAM_NAME))
		m_pBlockedTabs->selectTabByName(DERENDER_TAB_NAME);
	else
		m_pBlockedTabs->getCurrentPanel()->onOpen(sdParam);
	mKey.clear();
}

void LLFloaterBlocked::onTabSelect(const LLSD& sdParam)
{
	LLPanel* pActivePanel = m_pBlockedTabs->getPanelByName(sdParam.asString());
	if (pActivePanel)
	{
		pActivePanel->onOpen(mKey);
	}
}

void LLFloaterBlocked::showMuteAndSelect(const LLUUID& idMute)
{
	LLFloaterReg::showInstance("blocked", LLSD().with(BLOCKED_PARAM_NAME, idMute));
}

void LLFloaterBlocked::showDerenderAndSelect(const LLUUID& idEntry)
{
	LLFloaterReg::showInstance("blocked", LLSD().with(DERENDER_PARAM_NAME, idEntry));
}

// ============================================================================
