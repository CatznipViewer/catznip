/**
 *
 * Copyright (c) 2017, Kitty Barnett
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

// UI
#include "llfiltereditor.h"
// Viewer
#include "llfloaterofferinvfolderbrowse.h"
#include "llinventorybridge.h"
#include "llinventorypanel.h"

// ============================================================================
// LLFloaterInventoryOfferFolderBrowse class
//

LLFloaterInventoryOfferFolderBrowse::LLFloaterInventoryOfferFolderBrowse()
	: LLFloater(LLSD())
{
	buildFromFile("floater_offer_invfolder_browse.xml");
}

// virtual
LLFloaterInventoryOfferFolderBrowse::~LLFloaterInventoryOfferFolderBrowse()
{
	delete m_pSavedFolderState;
	m_pSavedFolderState = nullptr;
}

// virtual
BOOL LLFloaterInventoryOfferFolderBrowse::postBuild()
{
	m_pFilterEditor = findChild<LLFilterEditor>("inv_filter_editor");
	m_pFilterEditor->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderBrowse::onFilterEdit, this, _2));

	m_pInvPanel = findChild<LLInventoryPanel>("inv_panel");

	// Set up the inventory panel
	U32 maskFilterTypes = 0x1 << LLInventoryType::IT_CATEGORY;
	m_pInvPanel->setFilterTypes(maskFilterTypes);
	m_pInvPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	m_pInvPanel->getFilter().setFilterCategoryTypes(m_pInvPanel->getFilter().getFilterCategoryTypes() | (1ULL << LLFolderType::FT_INBOX));
	m_pInvPanel->getFilter().markDefault();

	m_pSavedFolderState = new LLSaveFolderState();
	m_pSavedFolderState->setApply(false);

	findChild<LLButton>("btn_select")->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderBrowse::onSelect, this));
	findChild<LLButton>("btn_cancel")->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderBrowse::onCancel, this));
	findChild<LLButton>("btn_closeall")->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderBrowse::onCloseAllFolders, this));

	return TRUE;
}

// virtual
void LLFloaterInventoryOfferFolderBrowse::onCommit()
{
	if (mCommitSignal)
	{
		LLFolderView::selected_items_t& selItems = m_pInvPanel->getRootFolder()->getSelectedItems();

		const LLInvFVBridge* pFVItem = (!selItems.empty()) ? dynamic_cast<const LLInvFVBridge*>(selItems.front()->getViewModelItem()) : nullptr;
		(*mCommitSignal)(this, LLSD().with("name", (pFVItem) ? pFVItem->getName() : LLStringUtil::null).with("uuid", (pFVItem) ? pFVItem->getUUID() : LLUUID::null));
	}
}

// virtual
void LLFloaterInventoryOfferFolderBrowse::onOpen(const LLSD& sdKey)
{
	const LLUUID idInvObject = sdKey["folder_id"].asUUID();
	if (idInvObject.notNull())
	{
		m_pInvPanel->setSelection(idInvObject, TAKE_FOCUS_NO);
	}
}

void LLFloaterInventoryOfferFolderBrowse::onCancel()
{
	closeFloater();
}

void LLFloaterInventoryOfferFolderBrowse::onCloseAllFolders()
{
#ifndef CATZNIP
	m_pInvPanel->getRootFolder()->closeAllFolders();
#else
	m_pInvPanel->getRootFolder()->collapseAllFolders();
#endif // CATZNIP
}

void LLFloaterInventoryOfferFolderBrowse::onFilterEdit(const std::string& strFilter)
{
	if (!strFilter.empty())
	{
		// Save the current open folder state if we don't have a filter applied right now
		if (!m_pInvPanel->getFilter().isNotDefault())
		{
			m_pSavedFolderState->setApply(FALSE);
			m_pInvPanel->getRootFolder()->applyFunctorRecursively(*m_pSavedFolderState);
		}
	}
	else
	{
		if (!m_pInvPanel->hasFilterSubString())
		{
			// New and existing filter both empty; nothing to do
			return;
		}

		m_pSavedFolderState->setApply(true);
		m_pInvPanel->getRootFolder()->applyFunctorRecursively(*m_pSavedFolderState);

		// Make sure the current selection is still visible
		LLOpenFoldersWithSelection f;
		m_pInvPanel->getRootFolder()->applyFunctorRecursively(f);
		m_pInvPanel->getRootFolder()->scrollToShowSelection();
	}
	m_pInvPanel->setFilterSubString(strFilter);
}

void LLFloaterInventoryOfferFolderBrowse::onSelect()
{
	onCommit();
	closeFloater();
}

// ============================================================================
