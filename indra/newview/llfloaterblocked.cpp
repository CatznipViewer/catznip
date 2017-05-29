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

	LLSD sdGenericRow; LLSD& sdGenericColumns = sdGenericRow["columns"];
	sdGenericColumns[0]["column"] = "item_name"; sdGenericColumns[0]["type"] = "text";
	sdGenericColumns[1]["column"] = "item_type"; sdGenericColumns[1]["type"] = "text";

	LLSD sdAgentRow(sdGenericRow); LLSD& sdAgentColumns = sdAgentRow["columns"];
	sdAgentColumns[2]["column"] = "item_text"; sdAgentColumns[2]["type"] = "checkbox";
	sdAgentColumns[3]["column"] = "item_voice"; sdAgentColumns[3]["type"] = "checkbox";
	sdAgentColumns[4]["column"] = "item_particles"; sdAgentColumns[4]["type"] = "checkbox";
	sdAgentColumns[5]["column"] = "item_sounds"; sdAgentColumns[5]["type"] = "checkbox";

	const std::vector<LLMute> lMutes = LLMuteList::getInstance()->getMutes();
	for (const LLMute& entryMute : lMutes)
	{
		switch (entryMute.mType)
		{
			case LLMute::AGENT:
				{
					sdAgentRow["value"] = LLSD().with("id", entryMute.mID).with("name", entryMute.mName);
					sdAgentColumns[0]["value"] = entryMute.mName;
					sdAgentColumns[1]["value"] = entryMute.getDisplayType();
					sdAgentColumns[2]["value"] = (entryMute.mFlags & LLMute::flagTextChat) == 0;
					sdAgentColumns[3]["value"] = (entryMute.mFlags & LLMute::flagVoiceChat) == 0;
					sdAgentColumns[4]["value"] = (entryMute.mFlags & LLMute::flagParticles) == 0;
					sdAgentColumns[5]["value"] = (entryMute.mFlags & LLMute::flagObjectSounds) == 0;
					m_pBlockList->addElement(sdAgentRow, boost::bind(&LLPanelBlockList::onToggleMuteFlag, this, _1, _2), ADD_BOTTOM);
				}
				break;
			default:
				{
					sdGenericRow["value"] = LLSD().with("id", entryMute.mID).with("name", entryMute.mName);
					sdGenericColumns[0]["value"] = entryMute.mName;
					sdGenericColumns[1]["value"] = entryMute.getDisplayType();
					m_pBlockList->addElement(sdGenericRow, ADD_BOTTOM);
				}
				break;
		}
	}
	m_pBlockList->setSelectedByValue(sdSel, true);
}

void LLPanelBlockList::selectEntry(const LLMute& muteEntry)
{
	m_pBlockList->deselectAllItems();

	const std::vector<LLScrollListItem*> muteEntries = m_pBlockList->getAllData();
	for (auto itEntry = muteEntries.begin(); itEntry != muteEntries.end(); ++itEntry)
	{
		const LLSD& sdValue = (*itEntry)->getValue();
		if ( (muteEntry.mID == sdValue["id"].asUUID()) && (muteEntry.mName == sdValue["name"].asString()) )
		{
			m_pBlockList->selectNthItem(itEntry - muteEntries.begin());
			break;
		}
	}
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

void LLPanelBlockList::onToggleMuteFlag(const LLSD& sdValue, const LLScrollListCell* pCell)
{
	if (!pCell)
		return;

	LLMute muteEntry(sdValue["id"].asUUID(), sdValue["name"].asString(), LLMute::AGENT); U32 muteFlag = 0;

	const std::string& strColumnName = pCell->getColumnName();
	if ("item_text" == strColumnName)
		muteFlag = LLMute::flagTextChat;
	else if ("item_voice" == strColumnName)
		muteFlag = LLMute::flagVoiceChat;
	else if ("item_particles" == strColumnName)
		muteFlag = LLMute::flagParticles;
	else if ("item_sounds" == strColumnName)
		muteFlag = LLMute::flagObjectSounds;

	if (muteFlag)
	{
		if (pCell->getValue().asBoolean())
			LLMuteList::getInstance()->add(muteEntry, muteFlag);
		else
			LLMuteList::getInstance()->remove(muteEntry, muteFlag);

		refresh();
		selectEntry(muteEntry);
	}
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

	LLDerenderList::instance().removeObjects(LLDerenderEntry::TYPE_OBJECT, idsObject);
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
			const LLDerenderEntry* pEntry = *itEntry;

			sdRow["value"] = pEntry->getID();
			sdColumns[0]["value"] = pEntry->getName();
			if (LLDerenderEntry::TYPE_OBJECT == pEntry->getType())
				sdColumns[1]["value"] = ((const LLDerenderObject*)pEntry)->strRegionName;
			sdColumns[2]["value"] = (pEntry->isPersistent()) ? "Permanent" : "Temporary";

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
