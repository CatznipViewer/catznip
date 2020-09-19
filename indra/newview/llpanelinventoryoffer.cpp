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
#include "llnotifications.h"
// Viewer
#include "llagent.h"
#include "llappviewer.h"
#include "llfloaterofferinvfolderbrowse.h"
#include "llfloaterofferinvfolderconfig.h"
#include "llfloaterreg.h"
#include "llinventorymodel.h"
#include "lltoastnotifypanel.h"
#include "lltrans.h"
#include "llpanelinventoryoffer.h"
#include "llviewercontrol.h"
#include "llviewerfoldertype.h"
#include "llviewerobjectlist.h"
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
static std::string s_strUnknownFolder = "unknown";

LLPanelInventoryOfferFolder::LLPanelInventoryOfferFolder()
	: LLPanel()
{
	setXMLFilename("panel_offer_invfolder.xml");
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

	if (m_SelectionUpdateConnection.connected())
		m_SelectionUpdateConnection.disconnect();

	m_ObjectSelectionHandle.clear();
}

//virtual
BOOL LLPanelInventoryOfferFolder::postBuild()
{
	m_pAcceptInCheck = findChild<LLCheckBoxCtrl>("chk_acceptin");
	m_pAcceptInCheck->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::refreshControls, this));

	m_pAcceptInList = findChild<LLComboBox>("list_folders");
	m_pAcceptInList->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::onSelectedFolderChanged, this));

	m_pBrowseBtn = findChild<LLButton>("btn_folder_browse");
	m_pBrowseBtn->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::onBrowseFolder, this));
	findChild<LLButton>("btn_folder_configure")->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::onConfigureFolders, this));

	refreshFolders();
	// Select the item (LLComboBox::postBuild has already been called at this point)
	if (m_pAcceptInList->getControlVariable())
		m_pAcceptInList->setValue(m_pAcceptInList->getControlVariable()->getValue());

	refreshControls();
	return TRUE;
}

// virtual
void LLPanelInventoryOfferFolder::onVisibilityChange(BOOL new_visibility)
{
	if ( (new_visibility) && (!m_fHasBeenVisible) )
	{
		onOpen(LLSD());
		m_fHasBeenVisible = true;
	}
	LLPanel::onVisibilityChange(new_visibility);
}

// virtual
void LLPanelInventoryOfferFolder::onOpen(const LLSD& sdKey)
{
	// If we're part of a notification then default behaviour can be overriden
	if (LLToastNotifyPanel* pToastPanel = getParentByType<LLToastNotifyPanel>())
	{
		const LLNotificationPtr notification = pToastPanel->getNotification();
		if (notification)
		{
			const LLSD& sdPayload = notification->getPayload();

			if ( (sdPayload.has("accept_in")) && (sdPayload["accept_in"].isBoolean()) )
			{
				m_pAcceptInCheck->clearControlName();
				m_pAcceptInCheck->set(sdPayload["accept_in"].asBoolean());
			}

			if ( (sdPayload.has("accept_in_folder")) && (sdPayload["accept_in_folder"].isUUID()) )
			{
				m_pAcceptInList->clearControlName();
				m_pAcceptInList->setValue(sdPayload["accept_in_folder"]);
			}

			if ( (sdPayload.has("from_object_id")) && (sdPayload["from_object_id"].isUUID()) )
			{
				m_idObject = sdPayload["from_object_id"].asUUID();
				if (sdPayload["from_id"].asUUID() == gAgentID)
					showObjectFolder(sdPayload["from_object_folder_id"].asUUID());
			}

			refreshControls();
		}
	}
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
			if (gInventory.isInTrash(folderInfo.getId()))
				continue;
			if (const LLInventoryCategory* pFolder = gInventory.getCategory(folderInfo.getId()))
				m_pAcceptInList->add(folderInfo.getName(), pFolder->getUUID());
		}
	}
	m_pAcceptInList->sortByName();
	m_pAcceptInList->addSeparator(ADD_TOP);

	// Add the 'Received Items' option
	m_pAcceptInList->add(LLViewerFolderType::lookupNewCategoryName(LLFolderType::FT_INBOX), gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false), ADD_TOP);

	// Add the originating folder (if it exists)
	if (m_fShowObjectFolder)
	{
		if (m_idObjectFolder.notNull())
		{
			if (!gInventory.isInTrash(m_idObjectFolder))
			{
				if (LLViewerInventoryCategory* pFolder = gInventory.getCategory(m_idObjectFolder))
					m_pAcceptInList->add(llformat("[%s: %s]", getString("originating_text").c_str(), pFolder->getName().c_str()), pFolder->getUUID(), ADD_TOP);
			}
			else
			{
				sdSelValue = LLUUID(gSavedPerAccountSettings.getString("InventoryOfferAcceptInFolder"));
			}
		}
		else if (LLViewerObject* pObj = gObjectList.findObject(m_idObject))
		{
			if (pObj->permYouOwner())
				m_pAcceptInList->add(llformat("[%s: %s]", getString("originating_text").c_str(), getString("originating_unknown_text").c_str()), s_strUnknownFolder, ADD_TOP);
		}
	}

	// Add the default option
	m_pAcceptInList->add(getString("default_text"), LLUUID::null, ADD_TOP);

	// Restore selection
	if (!sdSelValue.isUndefined())
	{
		if (sdSelValue.isUUID())
			m_pAcceptInList->selectByValue(sdSelValue);
		else if (s_strUnknownFolder == sdSelValue.asString())
			m_pAcceptInList->selectNthItem(1);
	}
	m_pAcceptInList->getListControl()->setCommitOnSelectionChange(true);
}

bool LLPanelInventoryOfferFolder::getAcceptIn() const
{
	return m_pAcceptInCheck->get();
}

const LLUUID LLPanelInventoryOfferFolder::getSelectedFolder() const
{
	const LLUUID idFolder = (m_pAcceptInCheck->get()) ? m_pAcceptInList->getValue().asUUID() : LLUUID::null;
	return ( (idFolder.notNull()) && (!gInventory.isInTrash(idFolder)) ) ? idFolder : LLUUID::null;
}

void LLPanelInventoryOfferFolder::onBrowseFolder()
{
	if (!m_BrowseFloaterHandle.isDead())
		return;

	if (LLFloater* pBrowseFloater = new LLFloaterInventoryOfferFolderBrowse())
	{
		pBrowseFloater->setCommitCallback(boost::bind(&LLPanelInventoryOfferFolder::onBrowseFolderCb, this, _2));
		pBrowseFloater->openFloater(LLSD().with("folder_id", m_pAcceptInList->getSelectedValue().asUUID()));

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

LLUUID LLPanelInventoryOfferFolder::getFolderFromObject(const LLViewerObject* pObj, const std::string& strName, bool* pfFound)
{
	if ( (pObj) && (pObj->permYouOwner()) )
	{
		if (pObj->isAttachment())
		{
			LLViewerInventoryItem* pItem = gInventory.getItem(pObj->getAttachmentItemID());
			if ( (pItem) && ((strName.empty()) ||(pItem->getName() == strName)) )
			{
				if (pfFound)
					*pfFound = true;
				return pItem->getParentUUID();
			}
		}
		else if (pObj->isSelected())
		{
			LLObjectSelectionHandle hSel = LLSelectMgr::instance().getSelection();
			LLSelectNode* pSelNode = hSel->findNode(const_cast<LLViewerObject*>(pObj));
			if ( (pSelNode) && ((strName.empty()) || (pSelNode->mName == strName)) )
			{
				if (pfFound)
					*pfFound = true;
				return pSelNode->mFolderID;
			}
		}
	}
	if (pfFound)
		*pfFound = false;
	return LLUUID::null;
}

void LLPanelInventoryOfferFolder::setObjectId(const LLUUID& idObject)
{
	if (m_idObject != idObject)
	{
		LLViewerObject* pObj = gObjectList.findObject(idObject);
		if (pObj)
			pObj = pObj->getRootEdit();
		m_idObject = (pObj) ? pObj->getID() : idObject;

		bool fFound = false; const LLUUID idObjectFolder = getFolderFromObject(pObj, LLStringUtil::null, &fFound);
		if ( (!fFound) || (idObjectFolder.notNull()) )
			showObjectFolder(idObjectFolder);
		else
			clearObjectFolder();
	}
}

void LLPanelInventoryOfferFolder::clearObjectFolder()
{
	if (m_idObject.notNull())
	{
		m_idObjectFolder.setNull();
		m_fShowObjectFolder = false;
		refreshFolders();
		m_pAcceptInList->setControlVariable(gSavedPerAccountSettings.getControl("InventoryOfferAcceptInFolder"));
	}
}

void LLPanelInventoryOfferFolder::showObjectFolder(const LLUUID& idObjectFolder)
{
	if (m_idObject.notNull())
	{
		m_idObjectFolder = idObjectFolder;
		m_fShowObjectFolder = gSavedPerAccountSettings.getBOOL("InventoryOfferAcceptInObjectFolder");
		if (m_fShowObjectFolder)
		{
			m_pAcceptInList->clearControlName();
			refreshFolders();
			m_pAcceptInList->setValue( (m_idObjectFolder.notNull()) ? LLSD(m_idObjectFolder) : LLSD(s_strUnknownFolder) );
		}
	}
}

void LLPanelInventoryOfferFolder::onSelectedFolderChanged()
{
	if ( (m_idObject.isNull()) || (m_idObjectFolder.notNull()) )
		return;

	const LLSD sdSelValue = m_pAcceptInList->getSelectedValue();
	if ( (m_ObjectSelectionHandle.notNull()) && (s_strUnknownFolder != sdSelValue.asString()) )
	{
		m_ObjectSelectionHandle.clear();
		m_SelectionUpdateConnection.disconnect();
	}
	else if ( (m_ObjectSelectionHandle.isNull()) && (s_strUnknownFolder == sdSelValue.asString()) )
	{
		if (LLViewerObject* pObj = gObjectList.findObject(m_idObject))
		{
			LLSelectMgr::instance().deselectAll();
			m_ObjectSelectionHandle = LLSelectMgr::instance().selectObjectAndFamily(pObj, false, true);
			if (m_ObjectSelectionHandle.notNull())
				m_ObjectSelectionHandle->getFirstRootNode()->setTransient(true);
			if (m_SelectionUpdateConnection.connected())
				m_SelectionUpdateConnection.disconnect();
			m_SelectionUpdateConnection = LLSelectMgr::getInstance()->mUpdateSignal.connect(boost::bind(&LLPanelInventoryOfferFolder::onUpdateSelection, this));
		}
	}
}

void LLPanelInventoryOfferFolder::onUpdateSelection()
{
	LLSelectNode* pSelNode = m_ObjectSelectionHandle->getFirstRootNode();
	if ( (!pSelNode) || (!pSelNode->mValid) || (pSelNode->getObject()->getID() != m_idObject) )
		return;
	if (pSelNode->mFolderID.notNull())
		showObjectFolder(pSelNode->mFolderID);
	else
		clearObjectFolder();
	m_SelectionUpdateConnection.disconnect();
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

	// Trim excess slashes and append optional destination folder
	boost::trim_if(strSubfolderPath, boost::is_any_of("/"));

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
	boost::ireplace_all(strSubfolderPath, "%yyyy", llformat("%d", 1900 + timeParts->tm_year));
	boost::ireplace_all(strSubfolderPath, "%yy", llformat("%d", timeParts->tm_year % 100));
	// %mmm and %mm
	if (LLStringOps::sMonthList.empty())
		LLStringOps::setupMonthNames(LLTrans::getString("dateTimeMonthNames"));
	if (LLStringOps::sMonthList.size() == 12)
		boost::ireplace_all(strSubfolderPath, "%mmm", LLStringOps::sMonthList[timeParts->tm_mon]);
	boost::ireplace_all(strSubfolderPath, "%mm", llformat("%02d", timeParts->tm_mon + 1));
	// %dd
	boost::ireplace_all(strSubfolderPath, "%dd", llformat("%02d", timeParts->tm_mday));
	// %date
	{
		char strDateBuf[32];
		strftime(strDateBuf, sizeof(strDateBuf) / sizeof(char), "%Y-%m-%d", localtime(&timeNow));
		boost::ireplace_all(strSubfolderPath, "%date", strDateBuf);
	}
	// %region
	{
		std::string strRegion;
		if (gAgent.getRegion())
			strRegion = gAgent.getRegion()->getName();
		boost::ireplace_all(strSubfolderPath, "%region", strRegion);
	}

	//
	// Split the path up in individual folders
	//
	boost::split(m_DestPath, strSubfolderPath, boost::is_any_of(std::string("/")), boost::algorithm::token_compress_on);

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

	while (!pInstance->m_DestPath.empty())
	{
		std::string strFolder = pInstance->m_DestPath.front();
		LLInventoryObject::correctInventoryName(strFolder);
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
			inventory_func_type f = boost::bind(&LLAcceptInFolderOfferBase::onCategoryCreateCallback, _1, pInstance);
			const LLUUID idTemp = gInventory.createNewCategory(idFolder, LLFolderType::FT_NONE, strFolder, f);
			if (idTemp.notNull())
				onCategoryCreateCallback(idTemp, pInstance);
			return;
		}
	}

	// Create a brand new folder (if requested)
	if (!pInstance->m_strNewFolder.empty())
	{
		inventory_func_type f = boost::bind(&LLAcceptInFolderOfferBase::onDestinationCreated, pInstance, _1);
		const LLUUID idTemp = gInventory.createNewCategory(idFolder, LLFolderType::FT_NONE, pInstance->m_strNewFolder, f);
		if (idTemp.notNull())
			pInstance->onDestinationCreated(idTemp);
		pInstance->m_strNewFolder.clear();
		return;
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
	if ( (mask & LLInventoryObserver::ADD) && (gInventory.getTransactionId().notNull()) && (m_idTransaction == gInventory.getTransactionId()))
	{	// BulkUpdateInventory

		// Ugh, when we used the messaging system we had all the folders first and then the items... with mAddedItemIDs we have improvise
		// so we just loop twice first calling getCategory(..), then getItem(...)
		const auto& idItems = gInventory.getAddedIDs();
		for (const LLUUID& idItem : idItems)
		{
			if (LLInventoryCategory* pCategory = gInventory.getCategory(idItem))
			{
				if (std::find(m_Folders.begin(), m_Folders.end(), pCategory->getUUID()) == m_Folders.end())
					m_Folders.push_back(pCategory->getUUID());
			}
		}
		for (const LLUUID& idItem : idItems)
		{
			if (LLInventoryItem* pItem = gInventory.getItem(idItem))
			{
				// We don't care about items we're already tracking the parent folder of
				if (std::find(m_Folders.begin(), m_Folders.end(), pItem->getParentUUID()) != m_Folders.end())
					continue;

				if (std::find(m_Items.begin(), m_Items.end(), pItem->getUUID()) == m_Items.end())
					m_Items.push_back(pItem->getUUID());
			}
		}
		done();
	}
	else if (mask & (LLInventoryObserver::ADD | LLInventoryObserver::UPDATE_CREATE))
	{	// UpdateCreateInventoryItem

		const auto& idItems = gInventory.getAddedIDs();
		for (const LLUUID& idItem : idItems)
		{
			if (LLInventoryItem* pItem = gInventory.getItem(idItem))
			{
				if (boost::starts_with(m_strDescription, llformat("'%s'", pItem->getName().c_str())))
				{
					m_Items.push_back(idItem);
					done();
					break;
				}
			}
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

LLCreateAcceptInFolder::LLCreateAcceptInFolder(const LLUUID& idBaseFolder, const std::string& strNewFolder, const folder_created_signal_t::slot_type& cb)
	: LLAcceptInFolderOfferBase(idBaseFolder, strNewFolder)
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
