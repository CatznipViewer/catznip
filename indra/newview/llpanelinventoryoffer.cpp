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

// Message
#include "message.h"
// UI
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloater.h"
// Viewer
#include "llagent.h"
#include "llappviewer.h"
#include "llfloaterofferinvfolderbrowse.h"
#include "llfloaterofferinvfolderconfig.h"
#include "llfloaterreg.h"
#include "llinventorymodel.h"
#include "lltrans.h"
#include "llpanelinventoryoffer.h"
#include "llviewercontrol.h"
#include "llviewerfoldertype.h"
#include "llviewerregion.h"
// Boost
#include <boost/algorithm/string.hpp>
// STL
#include <chrono>

// ============================================================================
// Constants
//

#define MAX_SUBFOLDERS_DEPTH 3

// ============================================================================
// LLPanelInventoryOfferFolder class
//

static LLPanelInjector<LLPanelInventoryOfferFolder> t_places("panel_offer_invfolder");

LLPanelInventoryOfferFolder::LLPanelInventoryOfferFolder()
	: LLPanel()
{
	buildFromFile("panel_offer_invfolder.xml");
}

// virtual
LLPanelInventoryOfferFolder::~LLPanelInventoryOfferFolder()
{
	if (LLFloater* pBrowseFloater = m_BrowseFloaterHandle.get())
		pBrowseFloater->closeFloater();
	m_BrowseFloaterHandle.markDead();

	if (LLFloater* pConfigureFloater = m_ConfigureFloaterHandle.get())
		pConfigureFloater->closeFloater();
	m_ConfigureFloaterHandle.markDead();
}

//virtual
BOOL LLPanelInventoryOfferFolder::postBuild()
{
	m_pAcceptInCheck = findChild<LLCheckBoxCtrl>("chk_acceptin");
	m_pAcceptInCheck->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::refreshControls, this));

	m_pAcceptInList = findChild<LLComboBox>("list_folders");

	m_pBrowseBtn = findChild<LLButton>("btn_folder_browse");
	m_pBrowseBtn->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::onBrowseFolder, this));
	findChild<LLButton>("btn_folder_configure")->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::onConfigureFolders, this));

	refreshFolders();
	// Select the item (LLComboBox::postBuild has already been called at this point)
	m_pAcceptInList->setValue(m_pAcceptInList->getControlVariable()->getValue());

	refreshControls();
	return TRUE;
}

void LLPanelInventoryOfferFolder::refreshControls()
{
	bool fAcceptIn = m_pAcceptInCheck->get();
	m_pAcceptInList->setEnabled(fAcceptIn);
	m_pBrowseBtn->setEnabled(fAcceptIn);
}

void LLPanelInventoryOfferFolder::refreshFolders()
{
	LLSD sdSelValue = m_pAcceptInList->getSelectedValue();

	m_pAcceptInList->getListControl()->setCommitOnSelectionChange(false);
	m_pAcceptInList->clearRows();

	// Add the user list options
	const LLSD sdOptionsList = gSavedPerAccountSettings.getLLSD("InventoryOfferAcceptInOptions");
	if (sdOptionsList.isArray())
	{
		for (LLSD::array_const_iterator itFolder = sdOptionsList.beginArray(), endFolder = sdOptionsList.endArray(); itFolder != endFolder; ++itFolder)
		{
			const LLAcceptInFolder folderInfo(*itFolder);
			if (const LLInventoryCategory* pFolder = gInventory.getCategory(folderInfo.getId()))
				m_pAcceptInList->add(folderInfo.getName(), pFolder->getUUID());
		}
	}
	m_pAcceptInList->sortByName();
	m_pAcceptInList->addSeparator(ADD_TOP);

	// Add the 'Received Items' option
	m_pAcceptInList->add(LLViewerFolderType::lookupNewCategoryName(LLFolderType::FT_INBOX), gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false), ADD_TOP);

	// Add the default option
	m_pAcceptInList->add(getString("default_text"), LLUUID::null, ADD_TOP);

	// Restore selection
	if (!sdSelValue.isUndefined())
		m_pAcceptInList->selectByValue(sdSelValue);
	m_pAcceptInList->getListControl()->setCommitOnSelectionChange(true);
}

void LLPanelInventoryOfferFolder::onBrowseFolder()
{
	if (!m_BrowseFloaterHandle.isDead())
		return;

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
		const LLAcceptInFolder folderInfo(idFolder, strFolderName, LLStringUtil::null);
		LLSD sdOptionsList = gSavedPerAccountSettings.getLLSD("InventoryOfferAcceptInOptions");
		sdOptionsList.append(folderInfo.toLLSD());
		gSavedPerAccountSettings.setLLSD("InventoryOfferAcceptInOptions", sdOptionsList);

		refreshFolders();
		m_pAcceptInList->selectByValue(idFolder);
	}
}

void LLPanelInventoryOfferFolder::onConfigureFolders()
{
	bool fOwnInstance = !LLFloaterReg::findInstance("offer_invfolder_configure");

	LLFloater* pConfigFloater = LLFloaterReg::showInstance("offer_invfolder_configure");
	if ( (pConfigFloater) && (fOwnInstance) )
	{
		m_ConfigureFloaterHandle = pConfigFloater->getHandle();
	}
	pConfigFloater->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::onConfigureFoldersCb, this));
}

void LLPanelInventoryOfferFolder::onConfigureFoldersCb()
{
	refreshFolders();
}

// ============================================================================
// LLAcceptInFolderOfferBase class
//

bool LLAcceptInFolderOfferBase::createDestinationFolder()
{
	// NOTE: derived classes will delete the instance in their onDestinationCreated override, so don't do anything after triggering the callback

	m_DestPath.clear();

	//
	// Find the user-configured subfolder path (if there is one)
	//
	const LLSD sdOptionsList = gSavedPerAccountSettings.getLLSD("InventoryOfferAcceptInOptions");
	if (!sdOptionsList.isArray())
		return false;

	std::string strSubfolderPath;
	for (LLSD::array_const_iterator itFolder = sdOptionsList.beginArray(), endFolder = sdOptionsList.endArray(); itFolder != endFolder; ++itFolder)
	{
		const LLAcceptInFolder folderInfo(*itFolder);
		if (folderInfo.getId() == m_idBaseFolder)
		{
			strSubfolderPath = folderInfo.getSubFolder();
			break;
		}
	}

	// If there's no subfolder path then we're done
	if (strSubfolderPath.empty())
	{
		onCategoryCreateCallback(m_idBaseFolder, this);
		return true;
	}

	//
	// Perform replacements
	//
	time_t timeNow;
	time(&timeNow);
	struct tm* timeParts = std::localtime(&timeNow);

	// %yyyy and %yy
	boost::replace_all(strSubfolderPath, "%yyyy", llformat("%d", 1900 + timeParts->tm_year));
	boost::replace_all(strSubfolderPath, "%yy", llformat("%d", timeParts->tm_year % 100));
	// %mmm and %mm
	if (LLStringOps::sMonthList.empty())
		LLStringOps::setupMonthNames(LLTrans::getString("dateTimeMonthNames"));
	if (LLStringOps::sMonthList.size() == 12)
		boost::replace_all(strSubfolderPath, "%mmm", LLStringOps::sMonthList[timeParts->tm_mon]);
	boost::replace_all(strSubfolderPath, "%mm", llformat("%02d", timeParts->tm_mon));
	// %dd
	boost::replace_all(strSubfolderPath, "%dd", llformat("%02d", timeParts->tm_mday));
	// %date
	{
		char strDateBuf[32];
		strftime(strDateBuf, sizeof(strDateBuf) / sizeof(char), "%Y-%m-%d", localtime(&timeNow));
		boost::replace_all(strSubfolderPath, "%date", strDateBuf);
	}
	// %region
	{
		std::string strRegion;
		if (gAgent.getRegion())
			strRegion = gAgent.getRegion()->getName();
		boost::replace_all(strSubfolderPath, "%region", strRegion);
	}

	//
	// Split the path up in individual folders
	//
	if (std::string::npos != strSubfolderPath.find("/"))
		boost::split(m_DestPath, strSubfolderPath, boost::is_any_of(std::string("/")));

	//
	// Kick off creating the destination folder (if it doesn't already exist)
	//
	if (m_DestPath.size() <= MAX_SUBFOLDERS_DEPTH)
	{
		onCategoryCreateCallback(m_idBaseFolder, this);
		return true;
	}

	m_DestPath.clear();
	return false;
}

void LLAcceptInFolderOfferBase::onCategoryCreateCallback(LLUUID idFolder, LLAcceptInFolderOfferBase* pInstance)
{
	if (idFolder.isNull())
	{
		// Problem encountered, abort
		pInstance->onDestinationCreated(LLUUID::null);
		return;
	}

	while (pInstance->m_DestPath.size() > 1)
	{
		std::string strFolder = pInstance->m_DestPath.front();
		pInstance->m_DestPath.pop_front();

		LLInventoryModel::cat_array_t* folders;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(idFolder, folders, items);
		if (!folders)
		{
			// Problem encountered, abort
			pInstance->onDestinationCreated(LLUUID::null);
			return;
		}

		LLInventoryModel::cat_array_t::const_iterator itFolder = std::find_if(folders->begin(), folders->end(), [&strFolder](const LLViewerInventoryCategory* pFolder) { return pFolder->getName() == strFolder; });
		if (folders->cend() != itFolder)
		{
			idFolder = (*itFolder)->getUUID();
		}
		else
		{
			LLInventoryObject::correctInventoryName(strFolder);
			inventory_func_type f = boost::bind(LLAcceptInFolderOfferBase::onCategoryCreateCallback, _1, pInstance);
			const LLUUID idTemp = gInventory.createNewCategory(idFolder, LLFolderType::FT_NONE, strFolder, f);
			if (idTemp.notNull())
				onCategoryCreateCallback(idTemp, pInstance);
			return;
		}
	}

	// Destination folder should exist at this point (we'll be deallocated when the function returns)
	pInstance->onDestinationCreated(idFolder);
}

// ============================================================================
// LLAcceptInFolderOfferBase class
//

LLAcceptInFolderTaskOffer::LLAcceptInFolderTaskOffer(const std::string& strDescription, const LLUUID& idTransaction, const LLUUID& idBaseFolder)
	: LLAcceptInFolderOfferBase(idBaseFolder)
	, LLInventoryObserver()
	, m_strDescription(strDescription)
	, m_idTransaction(idTransaction)
{
}

//virtual
void LLAcceptInFolderTaskOffer::changed(U32 mask)
{
	if (mask & LLInventoryObserver::ADD)
	{
		LLMessageSystem* pMsg = gMessageSystem;
		if ( (pMsg->getMessageName()) && (0 == strcmp(pMsg->getMessageName(), _PREHASH_BulkUpdateInventory)) )
		{
			LLUUID idTransaction;

			pMsg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TransactionID, idTransaction);
			if (m_idTransaction == idTransaction)
			{
				LLUUID idInvObject;

				for (S32 idxBlock = 0, cntBlock = pMsg->getNumberOfBlocksFast(_PREHASH_FolderData); idxBlock < cntBlock; idxBlock++)
				{
					pMsg->getUUIDFast(_PREHASH_FolderData, _PREHASH_FolderID, idInvObject, idxBlock);
					if ( (idInvObject.notNull()) && (std::find(m_Folders.begin(), m_Folders.end(), idInvObject) == m_Folders.end()) )
						m_Folders.push_back(idInvObject);
				}

				for (S32 idxBlock = 0, cntBlock = pMsg->getNumberOfBlocksFast(_PREHASH_ItemData); idxBlock < cntBlock; idxBlock++)
				{
					// We don't care about items we're already tracking the parent folder of
					pMsg->getUUIDFast(_PREHASH_ItemData, _PREHASH_FolderID, idInvObject, idxBlock);
					if ( (idInvObject.notNull()) && (std::find(m_Folders.begin(), m_Folders.end(), idInvObject) != m_Folders.end()) )
						continue;

					pMsg->getUUIDFast(_PREHASH_ItemData, _PREHASH_ItemID, idInvObject, idxBlock);
					if ( (idInvObject.notNull()) && (std::find(m_Items.begin(), m_Items.end(), idInvObject) == m_Items.end()) )
						m_Items.push_back(idInvObject);
				}

				done();
			}
		}
		else if ( (pMsg->getMessageName()) && (0 == strcmp(pMsg->getMessageName(), _PREHASH_UpdateCreateInventoryItem)) )
		{
			LLUUID idInvObject;
			pMsg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, idInvObject);
			if (idInvObject.notNull())
			{
				if (LLInventoryItem* pItem = gInventory.getItem(idInvObject))
				{
					if (boost::starts_with(m_strDescription, llformat("'%s'", pItem->getName().c_str())))
					{
						m_Items.push_back(idInvObject);

						done();
					}
				}
			}

			//LLUUID idTransaction;
			//
			//pMsg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TransactionID, idTransaction);
			//if (m_idTransaction == idTransaction)
			//{
			//	LLUUID idInvObject;
			//
			//	pMsg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, idInvObject);
			//	if (idInvObject.notNull())
			//		m_Items.push_back(idInvObject);
			//
			//	done();
			//}
		}
	}
}

void LLAcceptInFolderTaskOffer::done()
{
	gInventory.removeObserver(this);

	// We shouldn't be messing with inventory items during LLInventoryModel::notifyObservers()
	LLAppViewer::instance()->addOnIdleCallback(boost::bind(&LLAcceptInFolderTaskOffer::doneIdle, this));
}

void LLAcceptInFolderTaskOffer::doneIdle()
{
	if (!createDestinationFolder())
		delete this;
}

//virtual
void LLAcceptInFolderTaskOffer::onDestinationCreated(const LLUUID& idFolder)
{
	if (!m_Folders.empty())
		if (LLViewerInventoryCategory* pInvFolder = gInventory.getCategory(m_Folders.front()))
			gInventory.changeCategoryParent(pInvFolder, idFolder, false);

	if (!m_Items.empty())
		if (LLViewerInventoryItem* pInvItem = gInventory.getItem(m_Items.front()))
			gInventory.changeItemParent(pInvItem, idFolder, false);

	delete this;
}

// ============================================================================
// LLAcceptInFolderAgentOffer class
//

LLAcceptInFolderAgentOffer::LLAcceptInFolderAgentOffer(const LLUUID& idInvObject, const LLUUID& idBaseFolder)
	: LLAcceptInFolderOfferBase(idBaseFolder)
	, LLInventoryFetchItemsObserver(idInvObject)
	, m_InvObjectId(idInvObject)
{
}

void LLAcceptInFolderAgentOffer::done()
{
	gInventory.removeObserver(this);

	// We shouldn't be messing with inventory items during LLInventoryModel::notifyObservers()
	LLAppViewer::instance()->addOnIdleCallback(boost::bind(&LLAcceptInFolderAgentOffer::doneIdle, this));
}

// static
void LLAcceptInFolderAgentOffer::doneIdle()
{
	if (!createDestinationFolder())
		delete this;
}

//virtual
void LLAcceptInFolderAgentOffer::onDestinationCreated(const LLUUID& idFolder)
{
	if (LLViewerInventoryItem* pInvItem = gInventory.getItem(m_InvObjectId))
		gInventory.changeItemParent(pInvItem, idFolder, false);
	else if (LLViewerInventoryCategory* pInvFolder = gInventory.getCategory(m_InvObjectId))
		gInventory.changeCategoryParent(pInvFolder, idFolder, false);
	delete this;
}

// ============================================================================
// LLCreateAcceptInFolder class
//

LLCreateAcceptInFolder::LLCreateAcceptInFolder(const LLUUID& idBaseFolder, const folder_created_signal_t::slot_type& cb)
	: LLAcceptInFolderOfferBase(idBaseFolder)
{
	m_FolderCreatedSignal.connect(cb);

	if (!createDestinationFolder())
		delete this;
}

//virtual
void LLCreateAcceptInFolder::onDestinationCreated(const LLUUID& idFolder)
{
	m_FolderCreatedSignal(idFolder);
	delete this;
}

// ============================================================================
