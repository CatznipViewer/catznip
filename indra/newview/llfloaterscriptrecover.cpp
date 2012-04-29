/** 
 *
 * Copyright (c) 2011-2012, Kitty Barnett
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
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "llscrolllistctrl.h"
#include "llviewerassettype.h"
#include "llviewerinventory.h"
#include "llviewerregion.h"

#include "llfloaterscriptrecover.h"

// ============================================================================
// LLFloaterScriptRecover
//

LLFloaterScriptRecover::LLFloaterScriptRecover(const LLSD& sdKey)
	: LLFloater(sdKey)
{
}

void LLFloaterScriptRecover::onOpen(const LLSD& sdKey)
{
	LLScrollListCtrl* pListCtrl = findChild<LLScrollListCtrl>("script_list");

	LLSD sdBhvrRow; LLSD& sdBhvrColumns = sdBhvrRow["columns"];
	sdBhvrColumns[0] = LLSD().with("column", "script_check").with("type", "checkbox");
	sdBhvrColumns[1] = LLSD().with("column", "script_name").with("type", "text");

	pListCtrl->clearRows();
	for (LLSD::array_const_iterator itFile = sdKey["files"].beginArray(), endFile = sdKey["files"].endArray(); itFile != endFile;  ++itFile)
	{
		std::string strFile = itFile->asString();

		sdBhvrRow["value"] = strFile;
		sdBhvrColumns[0]["value"] = true;

		std::string strName = gDirUtilp->getBaseFileName(strFile, true);
		if (strName.find_last_of("-") == strName.length() - 9)
			strName.erase(strName.length() - 9);
		sdBhvrColumns[1]["value"] = strName;

		pListCtrl->addElement(sdBhvrRow, ADD_BOTTOM);
	}
}

BOOL LLFloaterScriptRecover::postBuild()
{
	findChild<LLUICtrl>("recover_btn")->setCommitCallback(boost::bind(&LLFloaterScriptRecover::onBtnRecover, this));
	findChild<LLUICtrl>("cancel_btn")->setCommitCallback(boost::bind(&LLFloaterScriptRecover::onBtnCancel, this));

	return TRUE;
}

void LLFloaterScriptRecover::onBtnCancel()
{
	LLScrollListCtrl* pListCtrl = findChild<LLScrollListCtrl>("script_list");

	// Delete all listed files
	std::vector<LLScrollListItem*> items = pListCtrl->getAllData();
	for (std::vector<LLScrollListItem*>::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
	{
		LLFile::remove((*itItem)->getValue().asString());
	}

	closeFloater();
}

void LLFloaterScriptRecover::onBtnRecover()
{
	LLScrollListCtrl* pListCtrl = findChild<LLScrollListCtrl>("script_list");

	// Recover all selected, delete any unselected
	std::vector<LLScrollListItem*> items = pListCtrl->getAllData(); std::list<std::string> strFiles;
	for (std::vector<LLScrollListItem*>::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem)
	{
		LLScrollListCheck* pCheckColumn = dynamic_cast<LLScrollListCheck*>((*itItem)->getColumn(0));
		if (!pCheckColumn)
			continue;

		std::string strFile = (*itItem)->getValue().asString();
		if (!strFile.empty())
		{
			if (pCheckColumn->getCheckBox()->getValue().asBoolean())
				strFiles.push_back(strFile);
			else
				LLFile::remove(strFile);
		}
	}

	if (!strFiles.empty())
		new LLScriptRecoverQueue(strFiles);

	closeFloater();
}

// ============================================================================
// LLCreateRecoverScriptCallback
//

class LLCreateRecoverScriptCallback : public LLInventoryCallback
{
public:
	LLCreateRecoverScriptCallback(LLScriptRecoverQueue* pRecoverQueue)
		: LLInventoryCallback(), mRecoverQueue(pRecoverQueue)
	{
	}

	void fire(const LLUUID& idItem)
	{
		mRecoverQueue->onCreateScript(idItem);
	}

protected:
	LLScriptRecoverQueue* mRecoverQueue;
};

// ============================================================================
// LLScriptRecoverQueue
//

// static
void LLScriptRecoverQueue::recoverIfNeeded()
{
	std::string strFilename, strPath = LLFile::tmpdir(); LLSD sdData, &sdFiles = sdData["files"];

	LLDirIterator itFiles(strPath, "*.lslbackup");
	while (itFiles.next(strFilename))
		sdFiles.append(strPath + strFilename);

	if (sdFiles.size())
		LLFloaterReg::showInstance("script_recover", sdData);
}

LLScriptRecoverQueue::LLScriptRecoverQueue(const std::list<std::string>& strFiles)
{
	for (std::list<std::string>::const_iterator itFilename = strFiles.begin(); itFilename != strFiles.end(); ++itFilename)
	{
		if (LLFile::isfile(*itFilename))
			m_FileQueue.insert(std::pair<std::string, LLUUID>(*itFilename, LLUUID::null ));
	}
	recoverNext();
}

bool LLScriptRecoverQueue::recoverNext()
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
	while ( (itFile != m_FileQueue.end()) && (itFile->second.notNull()) )
		++itFile;

	if (m_FileQueue.end() == itFile) 
	{
		LLInventoryPanel* pInvPanel = LLInventoryPanel::getActiveInventoryPanel(TRUE);
		if (pInvPanel)
		{
			LLFolderViewFolder* pFVF = dynamic_cast<LLFolderViewFolder*>(pInvPanel->getRootFolder()->getItemByID(idFNF));
			if (pFVF)
			{
				pFVF->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
				pInvPanel->setSelection(idFNF, TRUE);
			}
		}

		delete this;
		return false;
	}

	std::string strItemName = gDirUtilp->getBaseFileName(itFile->first, true);
	if (strItemName.find_last_of("-") == strItemName.length() - 9)
		strItemName.erase(strItemName.length() - 9);
	std::string strItemDescr;
	LLViewerAssetType::generateDescriptionFor(LLAssetType::AT_LSL_TEXT, strItemDescr);

	create_inventory_item(gAgent.getID(), gAgent.getSessionID(), idFNF, LLTransactionID::tnull, 
	                      strItemName, strItemDescr, LLAssetType::AT_LSL_TEXT, LLInventoryType::IT_LSL,
	                      NOT_WEARABLE, PERM_MOVE | PERM_TRANSFER, new LLCreateRecoverScriptCallback(this));
	return true;
}

void LLScriptRecoverQueue::onCreateScript(const LLUUID& idItem)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
	if (!pItem)
	{
		// TODO: error handling
		return;
	}

	std::string strFileName;
	for (filename_queue_t::iterator itFile = m_FileQueue.begin(); itFile != m_FileQueue.end(); ++itFile)
	{
		if (0 != gDirUtilp->getBaseFileName(itFile->first, true).find(pItem->getName()))
			continue;
		strFileName = itFile->first;
		itFile->second = idItem;
	}

	if (strFileName.empty())
	{
		// TODO: error handling
		return;
	}

	LLSD sdBody;
	sdBody["item_id"] = idItem;
	sdBody["target"] = "lsl2";

	std::string strCapsUrl = gAgent.getRegion()->getCapability("UpdateScriptAgent");
	LLHTTPClient::post(strCapsUrl, sdBody, new LLUpdateAgentInventoryResponder(sdBody, strFileName, LLAssetType::AT_LSL_TEXT, boost::bind(&LLScriptRecoverQueue::onSavedScript, this, _1, _2, _3)));
}

void LLScriptRecoverQueue::onSavedScript(const LLUUID& idItem, const LLSD&, bool fSuccess)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
	if (pItem)
	{
		filename_queue_t::iterator itFile = m_FileQueue.begin();
		while ( (itFile != m_FileQueue.end()) && (itFile->second != idItem) )
			++itFile;
		if (itFile != m_FileQueue.end())
		{
			LLFile::remove(itFile->first);
			m_FileQueue.erase(itFile);
		}
	}
	recoverNext();
}

// ============================================================================
