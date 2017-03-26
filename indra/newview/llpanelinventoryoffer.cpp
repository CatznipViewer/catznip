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
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloater.h"
// Viewer
#include "llappviewer.h"
#include "llfloaterofferinvfolderbrowse.h"
#include "llfloaterreg.h"
#include "llinventorymodel.h"
#include "llpanelinventoryoffer.h"
#include "llviewercontrol.h"
#include "llviewerfoldertype.h"

// ============================================================================
// LLPanelInventoryOfferFolder class
//

static LLPanelInjector<LLPanelInventoryOfferFolder> t_places("panel_offer_invfolder");

LLPanelInventoryOfferFolder::LLPanelInventoryOfferFolder()
	: LLPanel()
{
	buildFromFile("panel_offer_invfolder.xml");
}

LLPanelInventoryOfferFolder::~LLPanelInventoryOfferFolder()
{
	if (LLFloater* pBrowseFloater = m_BrowseFloaterHandle.get())
		pBrowseFloater->closeFloater();
	m_BrowseFloaterHandle.markDead();
}

//virtual
BOOL LLPanelInventoryOfferFolder::postBuild()
{
	m_pAcceptInCheck = findChild<LLCheckBoxCtrl>("chk_acceptin");
	m_pAcceptInCheck->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::refreshControls, this));

	m_pAcceptInList = findChild<LLComboBox>("list_folders");
	m_pAcceptInList->getListControl()->setCommitOnSelectionChange(true);

	// Add the 'Received Items' option
	m_pAcceptInList->add(LLViewerFolderType::lookupNewCategoryName(LLFolderType::FT_INBOX), gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false));
	m_pAcceptInList->addSeparator();

	// Add the user list options
	const LLSD sdOptionsList = gSavedPerAccountSettings.getLLSD("InventoryOfferAcceptInOptions");
	if (sdOptionsList.isArray())
	{
		for (LLSD::array_const_iterator itOption = sdOptionsList.beginArray(), endOption = sdOptionsList.endArray(); itOption != endOption; ++itOption)
		{
			if (const LLInventoryCategory* pFolder = gInventory.getCategory((*itOption).asUUID()))
				m_pAcceptInList->add(pFolder->getName(), pFolder->getUUID());
		}
	}

	// Select the item (LLComboBox::postBuild has already been called at this point)
	m_pAcceptInList->setValue(m_pAcceptInList->getControlVariable()->getValue());

	m_pBrowseBtn = findChild<LLButton>("btn_folder_browse");
	m_pBrowseBtn->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::onBrowseFolder, this));

	refreshControls();
	return TRUE;
}

void LLPanelInventoryOfferFolder::refreshControls()
{
	bool fAcceptIn = m_pAcceptInCheck->get();
	m_pAcceptInList->setEnabled(fAcceptIn);
	m_pBrowseBtn->setEnabled(fAcceptIn);
}

void LLPanelInventoryOfferFolder::onBrowseFolder()
{
	if (LLFloater* pBrowseFloater = new LLFloaterInventoryOfferFolderBrowse())
	{
		pBrowseFloater->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::onBrowseFolderCb, this, _2));
		pBrowseFloater->openFloater();

		m_BrowseFloaterHandle = pBrowseFloater->getHandle();
	}
}

void LLPanelInventoryOfferFolder::onBrowseFolderCb(const LLSD& sdData)
{
	const std::string& strFolderName = sdData["name"].asString();
	const LLUUID idFolder = sdData["uuid"].asUUID();

	if (!m_pAcceptInList->selectByValue(idFolder))
	{
		m_pAcceptInList->add(strFolderName, idFolder);
		m_pAcceptInList->selectByValue(idFolder);
	}
}

// ============================================================================
// LLAcceptInFolderAgentOffer - move an agent-to-agent accepted inventory offer to the specified folder
//

LLAcceptInFolderAgentOffer::LLAcceptInFolderAgentOffer(const LLUUID& idInvObject, const LLUUID& idDestFolder)
	: LLInventoryFetchItemsObserver(idInvObject)
	, m_InvObjectId(idInvObject)
	, m_DestFolderId(idDestFolder)
{
}

void LLAcceptInFolderAgentOffer::done()
{
	LLAppViewer::instance()->addOnIdleCallback(boost::bind(&LLAcceptInFolderAgentOffer::onDone, this));
	gInventory.removeObserver(this);
}

// static
void LLAcceptInFolderAgentOffer::onDone()
{
	if (LLViewerInventoryItem* pInvItem = gInventory.getItem(m_InvObjectId))
		gInventory.changeItemParent(pInvItem, m_DestFolderId, false);
	else if (LLViewerInventoryCategory* pInvFolder = gInventory.getCategory(m_InvObjectId))
		gInventory.changeCategoryParent(pInvFolder, m_DestFolderId, false);
	delete this;
}

// ============================================================================
