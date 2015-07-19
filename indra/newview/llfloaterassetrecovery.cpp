/** 
 *
 * Copyright (c) 2011-2013, Kitty Barnett
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

#include "llagent.h"
#include "llassetuploadresponders.h"
#include "llcheckboxctrl.h"
#include "lldiriterator.h"
#include "llfloaterreg.h"
#include "llfolderview.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "llscrolllistctrl.h"
#include "llviewerassettype.h"
#include "llviewerinventory.h"
#include "llviewerregion.h"

#include "llfloaterassetrecovery.h"

// ============================================================================
// LLFloaterAssetRecovery
//

LLFloaterAssetRecovery::LLFloaterAssetRecovery(const LLSD& sdKey)
	: LLFloater(sdKey)
{
}

void LLFloaterAssetRecovery::onOpen(const LLSD& sdKey)
{
	LLScrollListCtrl* pListCtrl = findChild<LLScrollListCtrl>("item_list");

	LLSD sdBhvrRow; LLSD& sdBhvrColumns = sdBhvrRow["columns"];
	sdBhvrColumns[0] = LLSD().with("column", "item_check").with("type", "checkbox");
	sdBhvrColumns[1] = LLSD().with("column", "item_name").with("type", "text");
	sdBhvrColumns[2] = LLSD().with("column", "item_type").with("type", "text");

	pListCtrl->clearRows();
	for (LLSD::array_const_iterator itFile = sdKey["files"].beginArray(), endFile = sdKey["files"].endArray(); 
			itFile != endFile;  ++itFile)
	{
		const LLSD& sdFile = *itFile;

		sdBhvrRow["value"] = sdFile;
		sdBhvrColumns[0]["value"] = true;
		sdBhvrColumns[1]["value"] = sdFile["name"];
		sdBhvrColumns[2]["value"] = sdFile["type"];

		pListCtrl->addElement(sdBhvrRow, ADD_BOTTOM);
	}
}

BOOL LLFloaterAssetRecovery::postBuild()
{
	findChild<LLUICtrl>("recover_btn")->setCommitCallback(boost::bind(&LLFloaterAssetRecovery::onBtnRecover, this));
	findChild<LLUICtrl>("cancel_btn")->setCommitCallback(boost::bind(&LLFloaterAssetRecovery::onBtnCancel, this));

	return TRUE;
}

void LLFloaterAssetRecovery::onBtnCancel()
{
	LLScrollListCtrl* pListCtrl = findChild<LLScrollListCtrl>("item_list");

	// Delete all listed files
	std::vector<LLScrollListItem*> items = pListCtrl->getAllData();
	for (std::vector<LLScrollListItem*>::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
	{
		LLFile::remove((*itItem)->getValue()["path"].asString());
	}

	closeFloater();
}

void LLFloaterAssetRecovery::onBtnRecover()
{
	LLScrollListCtrl* pListCtrl = findChild<LLScrollListCtrl>("item_list");

	// Recover all selected, delete any unselected
	std::vector<LLScrollListItem*> items = pListCtrl->getAllData(); LLSD sdFiles;
	for (std::vector<LLScrollListItem*>::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
	{
		LLScrollListCheck* pCheckColumn = dynamic_cast<LLScrollListCheck*>((*itItem)->getColumn(0));
		if (!pCheckColumn)
			continue;

		const LLSD sdFile = (*itItem)->getValue();
		if (pCheckColumn->getCheckBox()->getValue().asBoolean())
			sdFiles.append(sdFile);
		else
			LLFile::remove(sdFile["path"]);
	}

	if (!sdFiles.emptyArray())
		new LLAssetRecoverQueue(sdFiles);

	closeFloater();
}

// ============================================================================
// LLCreateRecoverAssetCallback
//

class LLCreateRecoverAssetCallback : public LLInventoryCallback
{
public:
	LLCreateRecoverAssetCallback(LLAssetRecoverQueue* pRecoverQueue)
		: LLInventoryCallback(), mRecoverQueue(pRecoverQueue)
	{
	}

	void fire(const LLUUID& idItem)
	{
		mRecoverQueue->onCreateItem(idItem);
	}

protected:
	LLAssetRecoverQueue* mRecoverQueue;
};

// ============================================================================
// Helper functions
//

// static
static bool removeEmbeddedMarkers(const std::string& strFilename)
{
	std::ifstream inNotecardFile(strFilename.c_str(), std::ios::in | std::ios::binary);
	if (!inNotecardFile.is_open())
		return false;

	std::string strText((std::istreambuf_iterator<char>(inNotecardFile)), std::istreambuf_iterator<char>());
	inNotecardFile.close();

	std::string::size_type idxText = strText.find((char)'\xF4', 0), lenText = strText.length();
	while ( (std::string::npos != idxText) && (idxText + 4 <= lenText) )
	{
		// In UTF-8 we're looking for F4808080-F48FBFBF
		char chByte2 = strText[idxText + 1];
		char chByte3 = strText[idxText + 2];
		char chByte4 = strText[idxText + 3];
		if ( ((chByte2 >= '\x80') && (chByte2 <= '\x8F')) &&
		     ((chByte3 >= '\x80') && (chByte3 <= '\xBF')) &&
		     ((chByte4 >= '\x80') && (chByte4 <= '\xBF')) )
		{
			// We're being lazy and replacing embedded markers with spaces since we don't want to adjust the notecard length field
			strText.replace(idxText, 4, 4, ' ');
			continue;
		}
		idxText = strText.find('\xF4', idxText + 1);
	}

	std::ofstream outNotecardFile(strFilename.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!outNotecardFile.is_open())
		return false;

	outNotecardFile.write(strText.c_str(), strText.length());
	outNotecardFile.close();

	return true;
}

// ============================================================================
// LLAssetRecoverQueue
//

static void findRecoverFiles(LLSD& sdFiles, const std::string& strPath, const std::string& strMask, const std::string& strType)
{
	LLDirIterator itFiles(strPath, strMask); std::string strFilename;
	while (itFiles.next(strFilename))
	{
		// Build a friendly name for the file
		std::string strName = gDirUtilp->getBaseFileName(strFilename, true);
		std::string::size_type offset = strName.find_last_of("-");
		if ( (std::string::npos != offset) && (offset != 0) && (offset == strName.length() - 9))
			strName.erase(strName.length() - 9);

		LLStringUtil::trim(strName);
		if (0 == strName.length())
			strName = llformat("(Unknown %s)", strType.c_str());

		sdFiles.append(LLSD().with("path", strPath + strFilename).with("name", strName).with("type", strType));
	}
}

// static
void LLAssetRecoverQueue::recoverIfNeeded()
{
	const std::string strTempPath = LLFile::tmpdir();
	LLSD sdData, &sdFiles = sdData["files"];

	findRecoverFiles(sdFiles, strTempPath, "*.lslbackup", "script");
	findRecoverFiles(sdFiles, strTempPath, "*.ncbackup", "notecard");

	if (sdFiles.size())
	{
		LLFloaterReg::showInstance("asset_recovery", sdData);
	}
}

LLAssetRecoverQueue::LLAssetRecoverQueue(const LLSD& sdFiles)
{
	for (LLSD::array_const_iterator itFile = sdFiles.beginArray(), endFile = sdFiles.endArray(); itFile != endFile;  ++itFile)
	{
		const LLSD& sdFile = *itFile;
		if (LLFile::isfile(sdFile["path"]))
		{
			m_FileQueue.insert(std::pair<std::string, LLSD>(sdFile["path"], sdFile));
		}
	}
	recoverNext();
}

bool LLAssetRecoverQueue::recoverNext()
{
	/**
	 * Steps:
	 *  (1) create a script inventory item under Lost and Found
	 *  (2) once we have the item's UUID we can upload the script
	 *  (3) once the script is uploaded we move on to the next item
	 */
	const LLUUID idFNF = gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);

	// Sanity check - if the associated UUID is non-null then this file is already being processed
	filename_queue_t::const_iterator itFile = m_FileQueue.begin();
	while ( (itFile != m_FileQueue.end()) && (itFile->second.has("item")) && (itFile->second["item"].asUUID().notNull()) )
		++itFile;

	if (m_FileQueue.end() == itFile) 
	{
		LLInventoryPanel* pInvPanel = LLInventoryPanel::getActiveInventoryPanel(TRUE);
		if (pInvPanel)
		{
			LLFolderViewFolder* pFVF = dynamic_cast<LLFolderViewFolder*>(pInvPanel->getItemByID(idFNF));
			if (pFVF)
			{
				pFVF->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
				pInvPanel->setSelection(idFNF, TRUE);
			}
		}

		delete this;
		return false;
	}

	std::string strItemDescr;
	LLViewerAssetType::generateDescriptionFor(LLAssetType::AT_LSL_TEXT, strItemDescr);

	if ("script" == itFile->second["type"].asString())
	{
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(), idFNF, LLTransactionID::tnull, 
		                      itFile->second["name"].asString(), strItemDescr, LLAssetType::AT_LSL_TEXT, LLInventoryType::IT_LSL,
		                      NOT_WEARABLE, PERM_MOVE | PERM_TRANSFER, new LLCreateRecoverAssetCallback(this));
	}
	else if ("notecard" == itFile->second["type"].asString())
	{
		removeEmbeddedMarkers(itFile->first);
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(), idFNF, LLTransactionID::tnull, 
		                      itFile->second["name"].asString(), strItemDescr, LLAssetType::AT_NOTECARD, LLInventoryType::IT_NOTECARD,
		                      NOT_WEARABLE, PERM_MOVE | PERM_TRANSFER, new LLCreateRecoverAssetCallback(this));
	}
	return true;
}

void LLAssetRecoverQueue::onCreateItem(const LLUUID& idItem)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
	if (!pItem)
	{
		// TODO: error handling
		return;
	}

	// Viewer will localize 'New Script' so we have to undo that
	std::string strItemName = pItem->getName();
	LLViewerInventoryItem::lookupSystemName(strItemName);

	filename_queue_t::iterator itFile = m_FileQueue.begin();
	while (itFile != m_FileQueue.end())
	{
		if (itFile->second["name"].asString() == strItemName)
			break;
		++itFile;
	}

	if (m_FileQueue.end() != itFile)
	{
		std::string strFileName = itFile->second["path"];
		itFile->second["item"] = idItem;

		std::string strCapsUrl; LLSD sdBody; 

		if (LLAssetType::AT_LSL_TEXT == pItem->getType())
		{
			strCapsUrl = gAgent.getRegion()->getCapability("UpdateScriptAgent");
			sdBody["item_id"] = idItem;
			sdBody["target"] = "lsl2";
		}
		else if (LLAssetType::AT_NOTECARD == pItem->getType())
		{
			strCapsUrl = gAgent.getRegion()->getCapability("UpdateNotecardAgentInventory");
			sdBody["item_id"] = idItem;
		}

		if (!strCapsUrl.empty())
		{
			LLHTTPClient::post(strCapsUrl, sdBody, 
			                   new LLUpdateAgentInventoryResponder(sdBody, strFileName, pItem->getType(), 
			                                                       boost::bind(&LLAssetRecoverQueue::onSavedAsset, this, _1, _2, _3),
			                                                       boost::bind(&LLAssetRecoverQueue::onUploadError, this, _1)));
		}
	}
}

void LLAssetRecoverQueue::onSavedAsset(const LLUUID& idItem, const LLSD&, bool fSuccess)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
	if (pItem)
	{
		filename_queue_t::iterator itFile = m_FileQueue.begin();
		while ( (itFile != m_FileQueue.end()) && ((!itFile->second.has("item")) || (itFile->second["item"].asUUID() != idItem)) )
			++itFile;
		if (itFile != m_FileQueue.end())
		{
			LLFile::remove(itFile->first);
			m_FileQueue.erase(itFile);
		}
	}
	recoverNext();
}

bool LLAssetRecoverQueue::onUploadError(const std::string& strFilename)
{
	// Skip over the file when there's an error, we can try again on the next relog
	filename_queue_t::iterator itFile = m_FileQueue.find(strFilename);
	if (itFile != m_FileQueue.end())
	{
		LLViewerInventoryItem* pItem = gInventory.getItem(itFile->second["item"]);
		if (pItem)
			gInventory.changeItemParent(pItem, gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH), FALSE);
		m_FileQueue.erase(itFile);
	}
	recoverNext();
	return false;
}

// ============================================================================
