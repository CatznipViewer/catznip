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
#include "llderenderlist.h"
#include "llsdserialize.h"
#include "llselectmgr.h"
#include "lltrans.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "pipeline.h"

// ============================================================================
// LLDerenderEntry
//

LLDerenderEntry::LLDerenderEntry(const LLSelectNode* pNode, bool fPersist)
	: fPersists(fPersist), idRegion(0), idRootLocal(0)
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

	idRootLocal = pObj->getLocalID();
	for (LLViewerObject::const_child_list_t::const_iterator itChild = pObj->getChildren().begin(), endChild = pObj->getChildren().end(); itChild != endChild; ++itChild)
		idsChildLocal.push_back((*itChild)->getLocalID());

	//
	// Fill in all region related information
	//
	const LLViewerRegion* pRegion = pObj->getRegion();
	if (!pRegion)
		return;

	idRegion = pRegion->getHandle();
	posRegion = pObj->getPositionRegion();
	if (!pRegion->getName().empty())
		strRegionName = pRegion->getName();
	else
		strRegionName = LLTrans::getString("Unknown");
}

LLDerenderEntry::LLDerenderEntry(const LLSD& sdData)
	: fPersists(true), idRegion(0), idRootLocal(0)
{
	strObjectName = sdData["object_name"];
	if (strObjectName.empty())
		strObjectName = LLTrans::getString("Unknown");
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

LLDerenderList::change_signal_t LLDerenderList::s_ChangeSignal;
std::string LLDerenderList::s_PersistFilename = "derender_list.xml";

LLDerenderList::LLDerenderList()
{
	load();
}

LLDerenderList::~LLDerenderList()
{
}

bool LLDerenderList::addSelection(bool fPersist, std::vector<LLUUID>* pIdList)
{
	if (pIdList)
	{
		pIdList->clear();
	}

	LLObjectSelectionHandle hSel = LLSelectMgr::getInstance()->getSelection();

	LLObjectSelection::valid_root_iterator itObj = hSel->valid_root_begin();
	while (hSel->valid_root_end() != itObj)
	{
		const LLSelectNode* pNode = *itObj++;
		if (!canAdd(pNode->getObject()))
			continue;

		LLDerenderEntry entry(pNode, fPersist);
		if ( (isDerendered(entry.idObject)) || (gAgentID == entry.idObject) )
			continue;
		m_Entries.push_back(entry);
		
		if (pIdList)
		{
			pIdList->push_back(entry.idObject);
		}

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

	if (fPersist)
		save();
	s_ChangeSignal();
	return (!pIdList) || (!pIdList->empty());
}

bool LLDerenderList::canAdd(const LLViewerObject* pObj)
{
	// Allow derendering if:
	//   - the object isn't a child prim
	//   - the object isn't currently sat on by the user
	//   - the object isn't an attachment
	return 
		(pObj) && (pObj->getRootEdit() == pObj) &&
		( (isAgentAvatarValid()) && (!pObj->isChild(gAgentAvatarp)) ) &&
		(!pObj->isAttachment());
}

bool LLDerenderList::canAddSelection() 
{
	struct CanDerender : public LLSelectedObjectFunctor
	{
		/*virtual*/ bool apply(LLViewerObject* pObj) { return LLDerenderList::canAdd(pObj); }
	} f;
	LLObjectSelectionHandle hSel = LLSelectMgr::getInstance()->getSelection();
	return (hSel.notNull()) && (0 != hSel->getRootObjectCount()) && (hSel->applyToRootObjects(&f, false));
}

LLDerenderList::entry_list_t::iterator LLDerenderList::findEntry(const LLUUID& idObject)
{
	return std::find_if(m_Entries.begin(), m_Entries.end(), [&idObject](const LLDerenderEntry& e) { return idObject == e.idObject; });
}

LLDerenderList::entry_list_t::const_iterator LLDerenderList::findEntry(const LLUUID& idObject) const
{
	return std::find_if(m_Entries.begin(), m_Entries.end(), [&idObject](const LLDerenderEntry& e) { return idObject == e.idObject; });
}

LLDerenderList::entry_list_t::iterator LLDerenderList::findEntry(U64 idRegion, const LLUUID& idObject, U32 idRootLocal)
{
	// NOTE: 'idRootLocal' will be 0 for the root prim itself and is the only time we need to compare against 'idObject'
	return std::find_if(m_Entries.begin(), m_Entries.end(), 
						[&idRegion, &idObject, &idRootLocal](const LLDerenderEntry& e)
						{ return ((idRootLocal) && (idRegion == e.idRegion) && (idRootLocal == e.idRootLocal)) || (idObject == e.idObject); });
}

LLDerenderList::entry_list_t::const_iterator LLDerenderList::findEntry(U64 idRegion, const LLUUID& idObject, U32 idRootLocal) const
{
	// NOTE: 'idRootLocal' will be 0 for the root prim itself and is the only time we need to compare against 'idObject'
	return std::find_if(m_Entries.begin(), m_Entries.end(), 
						[&idRegion, &idObject, &idRootLocal](const LLDerenderEntry& e)
						{ return ((idRootLocal) && (idRegion == e.idRegion) && (idRootLocal == e.idRootLocal)) || (idObject == e.idObject); });
}

void LLDerenderList::removeObject(const LLUUID& idObject)
{
	uuid_vec_t idsObject;
	idsObject.push_back(idObject);
	removeObjects(idsObject);
}

void LLDerenderList::removeObjects(const uuid_vec_t& idsObject)
{
	std::map<LLViewerRegion*, std::list<U32>> idRegionObjectMap; bool fSave = false;
	for (uuid_vec_t::const_iterator itObject = idsObject.begin(); itObject != idsObject.end(); ++itObject)
	{
		entry_list_t::iterator itEntry = findEntry(*itObject);
		if (m_Entries.end() == itEntry)
			continue;

		LLViewerRegion* pRegion = (0 != itEntry->idRegion) ? LLWorld::getInstance()->getRegionFromHandle(itEntry->idRegion) : NULL;
		if (pRegion)
		{
			std::list<U32>& idsLocal = idRegionObjectMap[pRegion];
			if (itEntry->idRootLocal)
				idsLocal.push_back(itEntry->idRootLocal);
			idsLocal.splice(idsLocal.end(), itEntry->idsChildLocal);
		}

		fSave |= itEntry->fPersists;
		m_Entries.erase(itEntry);
	}

	bool fNewMsg = true; int nBlockCount = 0;
	for (std::map<LLViewerRegion*, std::list<U32> >::const_iterator itRegionMap = idRegionObjectMap.begin(); itRegionMap != idRegionObjectMap.end(); ++itRegionMap)
	{
		LLViewerRegion* pRegion = itRegionMap->first;
		for (std::list<U32>::const_iterator itObject = itRegionMap->second.begin(); itObject != itRegionMap->second.end(); ++itObject)
		{
			if (fNewMsg)
			{
				fNewMsg = false;
				nBlockCount = 0;

				gMessageSystem->newMessageFast(_PREHASH_RequestMultipleObjects);
				gMessageSystem->nextBlockFast(_PREHASH_AgentData);
				gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			}

			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU8Fast(_PREHASH_CacheMissType, LLViewerRegion::CACHE_MISS_TYPE_FULL);
			gMessageSystem->addU32Fast(_PREHASH_ID, *itObject);
			nBlockCount++;

			if (nBlockCount >= 255)
			{
				gMessageSystem->sendReliable(pRegion->getHost());
				fNewMsg = true;
			}
		}

		if (!fNewMsg)
			gMessageSystem->sendReliable(pRegion->getHost());
	}

	if (fSave)
		save();
	s_ChangeSignal();
}

void LLDerenderList::updateObject(U64 idRegion, U32 idRootLocal, const LLUUID& idObject, U32 idObjectLocal)
{
	entry_list_t::iterator itEntry = findEntry(idRegion, idObject, idRootLocal);
	if (m_Entries.end() != itEntry)
	{
		if (0 != idRootLocal)
		{
			// We're updating a child prim
			if (itEntry->idsChildLocal.end() == std::find(itEntry->idsChildLocal.begin(), itEntry->idsChildLocal.end(), idRootLocal))
				itEntry->idsChildLocal.push_back(idObjectLocal);
		}
		else
		{
			// We're updating the root prim
			itEntry->idRegion = idRegion;
			itEntry->idRootLocal = idObjectLocal;
		}
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

	for (entry_list_t::const_iterator itEntry = m_Entries.begin(); itEntry != m_Entries.end(); ++itEntry)
	{
		if (itEntry->fPersists)
			fileDerender << LLSDOStreamer<LLSDNotationFormatter>(itEntry->toLLSD()) << std::endl;
	}

	fileDerender.close();
}

// ============================================================================
