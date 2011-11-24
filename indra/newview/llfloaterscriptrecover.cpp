/** 
 *
 * Copyright (c) 2011, Kitty Barnett
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
#include "lldiriterator.h"
#include "llinventorymodel.h"
#include "llviewerassettype.h"
#include "llviewerinventory.h"
#include "llviewerregion.h"

#include "llfloaterscriptrecover.h"

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
	std::string strFilename, strPath = LLFile::tmpdir(); std::list<std::string> strFiles;

	LLDirIterator itFiles(strPath, "*.lslbackup");
	while (itFiles.next(strFilename))
		strFiles.push_back(strPath + strFilename);

	if (!strFiles.empty())
		new LLScriptRecoverQueue(strFiles);
}

LLScriptRecoverQueue::LLScriptRecoverQueue(const std::list<std::string>& strFiles)
{
	for (std::list<std::string>::const_iterator itFilename = strFiles.begin(); itFilename != strFiles.end(); ++itFilename)
		m_FileQueue.insert(std::pair<std::string, LLUUID>(*itFilename, LLUUID::null ));
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

	// Sanity check - if the associated UUID is non-null then this file is already being processed
	filename_queue_t::const_iterator itFile = m_FileQueue.begin();
	while ( (itFile != m_FileQueue.end()) && (itFile->second.notNull()) )
		++itFile;

	if (m_FileQueue.end() == itFile) 
	{
		delete this;
		return false;
	}

	const LLUUID idParent = gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
	std::string strItemName = gDirUtilp->getBaseFileName(itFile->first, true), strItemDescr;
	LLViewerAssetType::generateDescriptionFor(LLAssetType::AT_LSL_TEXT, strItemDescr);

	create_inventory_item(gAgent.getID(), gAgent.getSessionID(), idParent, LLTransactionID::tnull, 
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
		if (pItem->getName() != gDirUtilp->getBaseFileName(itFile->first, true))
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

void LLScriptRecoverQueue::onSavedScript(const LLUUID& idItem, const LLSD& sdContent, bool fSuccess)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
	if (pItem)
	{
		filename_queue_t::const_iterator itFile = m_FileQueue.begin();
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
