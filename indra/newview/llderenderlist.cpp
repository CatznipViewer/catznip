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

#include "llagentdata.h"
#include "llappviewer.h"
#include "llderenderlist.h"
#include "llsdserialize.h"
#include "llselectmgr.h"
#include "lltrans.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "pipeline.h"

// ============================================================================
// LLDerenderEntry
//

LLDerenderEntry::LLDerenderEntry(const LLSelectNode* pNode)
	: fPersists(false), idRegion(0), idObjectLocal(0)
{
	//
	// Fill in all object related information
	//
	const LLViewerObject* pObj = (pNode) ? pNode->getObject() : NULL;
	if (!pObj)
		return;

	idObject = pObj->getID();
	if ( (pNode->mValid) && (!pNode->mName.empty()) )
		strObjectName = pNode->mName;
	else
		strObjectName = LLTrans::getString("Unknown");

	//
	// Fill in all region related information
	//
	const LLViewerRegion* pRegion = pObj->getRegion();
	if (!pRegion)
		return;

	idRegion = pRegion->getHandle();
	idObjectLocal = pObj->getLocalID();
	posRegion = pObj->getPositionRegion();
	if (!pRegion->getName().empty())
		strRegionName = pRegion->getName();
	else
		strRegionName = LLTrans::getString("Unknown");
}

LLDerenderEntry::LLDerenderEntry(const LLSD& sdData)
	: fPersists(true), idRegion(0), idObjectLocal(0)
{
	strObjectName = sdData["object_name"];
	if (strRegionName.empty())
		strRegionName = LLTrans::getString("Unknown");
	idObject = sdData["object_id"].asUUID();

	strRegionName = sdData["region_name"];
	if (strRegionName.empty())
		strRegionName = LLTrans::getString("Unknown");
	posRegion.setValue(sdData["region_pos"]);
}

LLSD LLDerenderEntry::toLLSD() const
{
	LLSD sdData;

	sdData["object_name"] = strObjectName;
	sdData["object_id"] = idObject;

	sdData["region_name"] = strRegionName;
	sdData["region_pos"] = posRegion.getValue();

	return sdData;
}

// ============================================================================
// LLDerenderList
//

std::string LLDerenderList::s_PersistFilename = "derender_list.xml";

LLDerenderList::LLDerenderList()
{
}

LLDerenderList::~LLDerenderList()
{
}

void LLDerenderList::addCurrentSelection()
{
	LLObjectSelectionHandle hSel = LLSelectMgr::getInstance()->getSelection();

	LLObjectSelection::valid_root_iterator itObj = hSel->valid_root_begin();
	while (hSel->valid_root_end() != itObj)
	{
		const LLSelectNode* pNode = *itObj++;

		LLDerenderEntry entry(pNode);
		if ( (isDerendered(entry.idObject)) || (gAgentID == entry.idObject) )
			continue;
		m_Entries.push_back(entry);

		LLViewerObject* pObj = pNode->getObject();
		if (pObj)
		{
			// Display green bubble on kill [see process_kill_object()]
			if (gShowObjectUpdates)
				gPipeline.addDebugBlip(pObj->getPositionAgent(), LLColor4(0.f, 1.f, 0.f, 1.f));
			LLSelectMgr::getInstance()->removeObjectFromSelections(entry.idObject);
			gObjectList.killObject(pObj);
		}
	}
}

LLDerenderList::entry_list_t::iterator LLDerenderList::findEntry(const LLUUID& idObject)
{
	return std::find_if(m_Entries.begin(), m_Entries.end(), [&idObject](const LLDerenderEntry& e) { return idObject == e.idObject; });
}

LLDerenderList::entry_list_t::const_iterator LLDerenderList::findEntry(const LLUUID& idObject) const
{
	return std::find_if(m_Entries.cbegin(), m_Entries.cend(), [&idObject](const LLDerenderEntry& e) { return idObject == e.idObject; });
}

void LLDerenderList::updateObject(const LLUUID& idObject, U64 idRegion, U32 idObjectLocal)
{
	entry_list_t::iterator itEntry = findEntry(idObject);
	if (m_Entries.end() != itEntry)
	{
		itEntry->idRegion = idRegion;
		itEntry->idObjectLocal = idObjectLocal;
	}
}

void LLDerenderList::load()
{
	llifstream fileDerender(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, s_PersistFilename));
	if (!fileDerender.is_open())
	{
		llwarns << "Can't open derender list file \"" << s_PersistFilename << "\" for reading" << llendl;
		return;
	}

	m_Entries.clear();

	// The parser's destructor is protected so we cannot create in the stack.
	LLPointer<LLSDNotationParser> sdParser = new LLSDNotationParser();

	std::string strLine; LLSD sdEntry;
	while (std::getline(fileDerender, strLine))
	{
		std::istringstream iss(strLine);
		if (sdParser->parse(iss, sdEntry, strLine.length()) == LLSDParser::PARSE_FAILURE)
		{
			llinfos << "Failed to parse derender list entry" << llendl;
			break;
		}

		LLDerenderEntry entry(sdEntry);
		if (entry.isValid())
			m_Entries.push_back(entry);
	}

	fileDerender.close();
}

void LLDerenderList::save() const
{
	llofstream fileDerender(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, s_PersistFilename));
	if (!fileDerender.is_open())
	{
		llwarns << "Can't open derender list file \"" << s_PersistFilename << "\" for writing" << llendl;
		return;
	}

	for (auto itEntry = m_Entries.cbegin(); itEntry != m_Entries.cend(); ++itEntry)
	{
		if (itEntry->fPersists)
			fileDerender << LLSDOStreamer<LLSDNotationFormatter>(itEntry->toLLSD()) << std::endl;
	}

	fileDerender.close();
}

// ============================================================================
