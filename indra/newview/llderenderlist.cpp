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

#include "llagentdata.h"
#include "llappviewer.h"
#include "llderenderlist.h"
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
		strObjectName = idObject.asString();

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

// ============================================================================
// LLDerenderList
//

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

bool LLDerenderList::isDerendered(const LLUUID& idObject) const
{
	for (entry_list_t::const_iterator itEntry = m_Entries.begin(); itEntry != m_Entries.end(); ++itEntry)
		if (idObject == itEntry->idObject)
			return true;
	return false;
}

bool LLDerenderList::isDerendered(U64 idRegion, U32 idObjectLocal) const
{
	for (entry_list_t::const_iterator itEntry = m_Entries.begin(); itEntry != m_Entries.end(); ++itEntry)
	{
		const LLDerenderEntry& entry = *itEntry;
		if ( (idRegion == entry.idRegion) && (idObjectLocal == entry.idObjectLocal) )
			return true;
	}
	return false;
}

void LLDerenderList::updateObject(const LLUUID& idObject, U64 idRegion, U32 idObjectLocal)
{
	for (entry_list_t::iterator itEntry = m_Entries.begin(); itEntry != m_Entries.end(); ++itEntry)
	{
		LLDerenderEntry& entry = *itEntry;
		if (idObject == entry.idObject)
		{
			entry.idRegion = idRegion;
			entry.idObjectLocal = idObjectLocal;
		}
	}
}

// ============================================================================
