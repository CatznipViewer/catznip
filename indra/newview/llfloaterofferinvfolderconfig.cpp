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
#include "llbutton.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
// Viewer
#include "llfloaterofferinvfolderbrowse.h"
#include "llfloaterofferinvfolderconfig.h"
#include "llinventoryfunctions.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llviewercontrol.h"

// ============================================================================
// LLFloaterInventoryOfferFolderConfig class
//

LLFloaterInventoryOfferFolderConfig::LLFloaterInventoryOfferFolderConfig(const LLSD& sdKey)
	: LLFloater(sdKey)
{
}

// virtual
LLFloaterInventoryOfferFolderConfig::~LLFloaterInventoryOfferFolderConfig()
{
	if (LLFloater* pBrowseFloater = m_BrowseFloaterHandle.get())
		pBrowseFloater->closeFloater();
	m_BrowseFloaterHandle.markDead();
}

// virtual
BOOL LLFloaterInventoryOfferFolderConfig::postBuild()
{
	m_pFolderList = findChild<LLScrollListCtrl>("list_folders");
	//m_pFolderList->setCommitOnSelectionChange(true);
	m_pFolderList->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::onSelectFolder, this));

	m_pFolderAddBtn = findChild<LLButton>("btn_folder_add");
	m_pFolderAddBtn->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::onAddFolder, this));
	m_pFolderRemoveBtn = findChild<LLButton>("btn_folder_remove");
	m_pFolderRemoveBtn->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::onRemoveFolder, this));
	m_pFolderSaveBtn = findChild<LLButton>("btn_folder_save");
	m_pFolderSaveBtn->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::onSaveFolder, this));

	m_pEditFolderName = findChild<LLLineEditor>("input_folder_name");
	m_pEditFolderName->setKeystrokeCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::refreshControls, this), nullptr);
	m_pEditFolderPath = findChild<LLLineEditor>("input_folder_path");
	m_pEditFolderBrowse = findChild<LLButton>("btn_folder_path_browse");
	m_pEditFolderBrowse->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::onBrowseFolder, this));
	m_pEditFolderSubfolder = findChild<LLLineEditor>("input_folder_subfolder");
	m_pEditFolderSubfolder->setKeystrokeCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::refreshControls, this), nullptr);

	findChild<LLButton>("btn_save")->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::onOk, this));
	findChild<LLButton>("btn_cancel")->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::onCancel, this));

	refreshControls();
	return TRUE;
}

// virtual
void LLFloaterInventoryOfferFolderConfig::closeFloater(bool app_quitting /*=false*/)
{
	if ( ((m_fFolderItemsDirty) || (isDirty())) && (!app_quitting) )
	{
		LLNotificationsUtil::add("InventoryOfferFolderConfigSaveChanges", LLSD(), LLSD(), boost::bind(&LLFloaterInventoryOfferFolderConfig::onSaveChangesCb, this, _1, _2));
		return;
	}

	LLFloater::closeFloater(app_quitting);
}

// virtual
void LLFloaterInventoryOfferFolderConfig::onCommit()
{
	gSavedPerAccountSettings.setLLSD("InventoryOfferAcceptInOptions", m_sdFolderItems);
	m_fFolderItemsDirty = false;
	LLFloater::onCommit();
}

// virtual
void LLFloaterInventoryOfferFolderConfig::onOpen(const LLSD& sdKey)
{
	m_sdFolderItems = gSavedPerAccountSettings.getLLSD("InventoryOfferAcceptInOptions");
	refreshItems();
}

void LLFloaterInventoryOfferFolderConfig::addItem(const LLAcceptInFolder& folderInfo)
{
	LLScrollListItem::Params folderItemParams;
	folderItemParams.value = folderInfo.getId();

	LLScrollListCell::Params folderItemColumnParams;
	folderItemColumnParams.font = LLFontGL::getFontSansSerif();

	folderItemColumnParams.font_halign = LLFontGL::LEFT;
	folderItemColumnParams.column = "name";
	folderItemColumnParams.value = folderInfo.getName();
	folderItemParams.columns.add(folderItemColumnParams);

	folderItemColumnParams.font_halign = LLFontGL::LEFT;
	folderItemColumnParams.column = "path";
	folderItemColumnParams.value = folderInfo.getPath();
	folderItemParams.columns.add(folderItemColumnParams);

	folderItemColumnParams.font_halign = LLFontGL::LEFT;
	folderItemColumnParams.column = "subfolder";
	folderItemColumnParams.value = folderInfo.getSubFolder();
	folderItemParams.columns.add(folderItemColumnParams);

	m_pFolderList->addRow(folderItemParams);
	m_pFolderList->selectByValue(folderItemParams.value);
}

void LLFloaterInventoryOfferFolderConfig::refreshItems()
{
	m_pFolderList->operateOnSelection(LLScrollListCtrl::OP_DESELECT);
	m_pFolderList->clearRows();
	for (LLSD::array_const_iterator itFolder = m_sdFolderItems.beginArray(), endFolder = m_sdFolderItems.endArray(); itFolder != endFolder; ++itFolder)
	{
		const LLAcceptInFolder folderInfo(*itFolder);
		addItem(folderInfo);
	}
	m_pFolderList->updateSort();

	if (m_idEditFolder.notNull())
	{
		m_pFolderList->selectByID(m_idEditFolder);
	}
}

void LLFloaterInventoryOfferFolderConfig::clearControls()
{
	m_pFolderList->deselectAllItems();
	m_pEditFolderItem = nullptr;
	m_idEditFolder.setNull();;
	m_pEditFolderName->clear();
	m_pEditFolderPath->clear();
	m_pEditFolderSubfolder->clear();
}

// virtual
BOOL LLFloaterInventoryOfferFolderConfig::isDirty() const
{
	return (m_pEditFolderItem != nullptr) && ((m_pEditFolderName->isDirty()) || (m_pEditFolderPath->isDirty()) || (m_pEditFolderSubfolder->isDirty()));
}

void LLFloaterInventoryOfferFolderConfig::refreshControls()
{
	bool fFolderSelected = m_pFolderList->getNumSelected() > 0;
	m_pFolderRemoveBtn->setEnabled(fFolderSelected);

	m_pEditFolderName->setEnabled(m_pEditFolderItem != nullptr);
	m_pEditFolderPath->setEnabled(false);
	m_pEditFolderBrowse->setEnabled(m_pEditFolderItem != nullptr);
	m_pEditFolderSubfolder->setEnabled(m_pEditFolderItem != nullptr);

	m_pFolderSaveBtn->setEnabled(isDirty());
}

void LLFloaterInventoryOfferFolderConfig::onAddFolder()
{
	clearControls();
	m_pEditFolderItem = new LLSD();
	refreshControls();
}

void LLFloaterInventoryOfferFolderConfig::onRemoveFolder()
{
	const LLSD sdSelValue = m_pFolderList->getSelectedValue();

	for (int idxFolder = 0, cntFolder = m_sdFolderItems.size(); idxFolder < cntFolder; idxFolder++)
	{
		if (m_sdFolderItems[idxFolder]["uuid"].asUUID() == sdSelValue.asUUID())
		{
			m_sdFolderItems.erase(idxFolder);
			m_fFolderItemsDirty = true;
			break;
		}
	}
	m_pFolderList->deleteSingleItem(m_pFolderList->getFirstSelectedIndex());

	clearControls();
	refreshControls();
}

void LLFloaterInventoryOfferFolderConfig::onSelectFolder()
{
	LLNotification::Params::Functor f;
	f.function = boost::bind(&LLFloaterInventoryOfferFolderConfig::onSelectFolderCb, this, _1, _2);

	if (isDirty())
	{
		LLNotifications::instance().add(LLNotification::Params().name("InventoryOfferFolderConfigSaveChangesFolder")
		                                                        .substitutions(LLSD().with("NAME", m_pEditFolderName->getText()))
																.payload(LLSD())
																.functor(f));
	}
	else
	{
		LLNotifications::instance().forceResponse(LLNotification::Params("InventoryOfferFolderConfigSaveChangesFolder").payload(LLSD())
			                                                                                                           .functor(f), 1 /*NO*/);
	}
}

void LLFloaterInventoryOfferFolderConfig::onSelectFolderCb(const LLSD& sdNotification, const LLSD& sdResponse)
{
	const LLSD sdSelValue = m_pFolderList->getSelectedValue();

	//
	// Handle the notification response
	//
	S32 idxOption = LLNotificationsUtil::getSelectedOption(sdNotification, sdResponse);
	switch (idxOption)
	{
		case 0: // Yes (Save)
			if (!onSaveFolder())
				return;
			m_pFolderList->selectByValue(sdSelValue);
			break;
		case 1: // No (Don't Save)
			break;
		case 2: // Cancel
			return;
	}

	//
	// Handle the selection
	//
	m_pEditFolderItem = nullptr;
	for (LLSD::array_iterator itFolder = m_sdFolderItems.beginArray(), endFolder = m_sdFolderItems.endArray(); itFolder != endFolder; ++itFolder)
	{
		if (itFolder->get("uuid").asUUID() == sdSelValue.asUUID())
		{
			m_pEditFolderItem = &*itFolder;
			break;
		}
	}

	if (m_pEditFolderItem)
	{
		const LLAcceptInFolder folderInfo(*m_pEditFolderItem);
		m_idEditFolder = folderInfo.getId();
		m_pEditFolderName->setText(folderInfo.getName(), true);
		m_pEditFolderPath->setText(folderInfo.getPath(), true);
		m_pEditFolderSubfolder->setText(folderInfo.getSubFolder(), true);
	}
	else
	{
		clearControls();
	}

	refreshControls();
}

void LLFloaterInventoryOfferFolderConfig::onBrowseFolder()
{
	if (!m_BrowseFloaterHandle.isDead())
		return;

	if (LLFloater* pBrowseFloater = new LLFloaterInventoryOfferFolderBrowse())
	{
		pBrowseFloater->setCommitCallback(boost::bind(&LLFloaterInventoryOfferFolderConfig::onBrowseFolderCb, this, _2));
		pBrowseFloater->openFloater(LLSD().with("folder_id", m_idEditFolder));

		m_BrowseFloaterHandle = pBrowseFloater->getHandle();
	}
}

void LLFloaterInventoryOfferFolderConfig::onBrowseFolderCb(const LLSD& sdData)
{
	m_idEditFolder = sdData["uuid"].asUUID();
	if (m_pEditFolderName->getText().empty())
	{
		if (LLViewerInventoryCategory* pFolder = gInventory.getCategory(m_idEditFolder))
			m_pEditFolderName->setText(pFolder->getName(), false);
	}
	m_pEditFolderPath->setText(LLAcceptInFolder::getPath(m_idEditFolder), false);
	refreshControls();
}

bool LLFloaterInventoryOfferFolderConfig::onSaveFolder()
{
	if (m_idEditFolder.isNull())
	{
		LLSD args;
		args["MESSAGE"] = getString("MissingFolder");
		LLNotificationsUtil::add("GenericAlert", args);
		return false;
	}

	const LLAcceptInFolder folderInfo(m_idEditFolder, m_pEditFolderName->getText(), m_pEditFolderSubfolder->getText());
	if ( (m_pEditFolderItem) && (m_pEditFolderItem->isUndefined()) )
	{
		m_sdFolderItems.append(folderInfo.toLLSD());
		m_fFolderItemsDirty = true;
		m_pEditFolderItem = &m_sdFolderItems[m_sdFolderItems.size() - 1];
	}
	else if (m_pEditFolderItem)
	{
		m_pEditFolderItem->assign(folderInfo.toLLSD());
		m_fFolderItemsDirty = true;
	}

	m_pEditFolderName->resetDirty();
	m_pEditFolderPath->resetDirty();
	m_pEditFolderSubfolder->resetDirty();

	refreshItems();
	refreshControls();
	return true;
}

void LLFloaterInventoryOfferFolderConfig::onOk()
{
	LLNotification::Params::Functor f;
	f.function = boost::bind(&LLFloaterInventoryOfferFolderConfig::onOkCb, this, _1, _2);

	if (isDirty())
	{
		LLNotifications::instance().add(LLNotification::Params().name("InventoryOfferFolderConfigSaveChangesFolder")
		                                                        .substitutions(LLSD().with("NAME", m_pEditFolderName->getText()))
		                                                        .payload(LLSD())
		                                                        .functor(f));
	}
	else
	{
		LLNotifications::instance().forceResponse(LLNotification::Params("InventoryOfferFolderConfigSaveChangesFolder").payload(LLSD())
		                                                                                                               .functor(f), 1 /*NO*/);
	}
}

void LLFloaterInventoryOfferFolderConfig::onOkCb(const LLSD& sdNotification, const LLSD& sdResponse)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(sdNotification, sdResponse);
	switch (idxOption)
	{
		case 0: // Save
			if (!onSaveFolder())
				return;
			break;
		case 1: // Don't save
			break;
		case 2: // Cancel
			return;
	}

	onCommit();
	closeFloater();
}

void LLFloaterInventoryOfferFolderConfig::onCancel()
{
	m_fFolderItemsDirty = false;
	clearControls();
	closeFloater();
}

void LLFloaterInventoryOfferFolderConfig::onSaveChangesCb(const LLSD& sdNotification, const LLSD& sdResponse)
{
	S32 idxOption = LLNotificationsUtil::getSelectedOption(sdNotification, sdResponse);
	switch (idxOption)
	{
		case 0: // Persist the current entry changes and then update the setting
			if (!onSaveFolder())
				return;
			onCommit();
			break;
		case 1: // Don't save anything
			m_fFolderItemsDirty = false;
			clearControls();
			break;
		case 2: // Cancel
			return;
	}

	closeFloater();
}


// ============================================================================
// LLAcceptInFolder - helper data structure
//

std::string LLAcceptInFolder::getPath(const LLUUID& idFolder)
{
	std::string strFolderPath;
	append_path(idFolder, strFolderPath, false, false);
	return strFolderPath;
}

// ============================================================================
