/** 
 *
 * Copyright (c) 2011-2014, Kitty Barnett
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
#include "llvocache.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "pipeline.h"

// ============================================================================
// LLDerenderEntry
//

LLDerenderEntry::LLDerenderEntry(EEntryType eType, bool fPersists)
	: m_eType(eType), m_nFlags(FLAG_NONE)
{
	if (fPersists)
		m_nFlags |= FLAG_PERSIST;
}

LLDerenderEntry::LLDerenderEntry(EEntryType eType, const LLSD& sdData)
	: m_eType(TYPE_NONE)
{
	// Sanity check
	if ( (sdData.has("entry_type")) && (eType != sdData["entry_type"].asInteger()) )
	{
		LL_WARNS("Derender") << "Attempting to parse derender entry with reported type of " << sdData["entry_type"].asInteger() << " as type " << eType << LL_ENDL;
		return;
	}
	m_eType = eType;

	// Pre-flags legacy support
	m_nFlags = (sdData.has("entry_flags")) ? sdData["entry_flags"].asInteger() : FLAG_PERSIST;

	// Legacy support for object_id
	m_idEntry = (sdData.has("entry_id")) ? sdData["entry_id"].asUUID() : sdData["object_id"].asUUID();

	// Legacy support for object_name
	m_strEntryName = (sdData.has("entry_name")) ? sdData["entry_name"] : sdData["object_name"];
	if (m_strEntryName.empty())
		m_strEntryName = LLTrans::getString("Unknown");
}

LLDerenderEntry* LLDerenderEntry::fromLLSD(const LLSD& sdData)
{
	EEntryType eType = (sdData.has("entry_type")) ? (EEntryType)sdData["entry_type"].asInteger() : TYPE_OBJECT;
	switch (eType)
	{
		case TYPE_OBJECT:
			return new LLDerenderObject(sdData);
		case TYPE_AVATAR:
			return new LLDerenderAvatar(sdData);
		default:
			return NULL;
	}
}

LLSD LLDerenderEntry::toLLSD() const
{
	LLSD sdData;

	sdData["entry_type"] = m_eType;
	sdData["entry_flags"] = m_nFlags;
	sdData["entry_name"] = m_strEntryName;
	sdData["entry_id"] = m_idEntry;

	return sdData;
}

// ============================================================================
// LLDerenderObject
//

LLDerenderObject::LLDerenderObject(const LLSelectNode* pNode, bool fPersist)
	: LLDerenderEntry(TYPE_OBJECT, fPersist), idRegion(0), idRootLocal(0)
{
	//
	// Fill in all object related information
	//
	const LLViewerObject* pObj = (pNode) ? pNode->getObject() : NULL;
	if (!pObj)
		return;

	m_idEntry = pObj->getID();
	m_strEntryName = ((pNode->mValid) && (!pNode->mName.empty())) ? pNode->mName : LLTrans::getString("Unknown");

	idRootLocal = pObj->getLocalID();
	for (LLViewerObject::const_child_list_t::const_iterator itChild = pObj->getChildren().begin(), endChild = pObj->getChildren().end(); itChild != endChild; ++itChild)
		idsChildLocal.push_back((*itChild)->getLocalID());

	//
	// Fill in all region related information
	//
	if (const LLViewerRegion* pRegion = pObj->getRegion())
	{
		idRegion = pRegion->getHandle();
		posRegion = pObj->getPositionRegion();
		strRegionName = pRegion->getName();
	}
	if (strRegionName.empty())
	{
		strRegionName = LLTrans::getString("Unknown");
	}
}

LLDerenderObject::LLDerenderObject(const LLSD& sdData)
	: LLDerenderEntry(TYPE_OBJECT, sdData), idRegion(0), idRootLocal(0)
{
	if (sdData.has("region_pos"))
		posRegion.setValue(sdData["region_pos"]);
	strRegionName = (sdData.has("region_name")) ? sdData["region_name"] : LLTrans::getString("Unknown");
}

LLSD LLDerenderObject::toLLSD() const
{
	LLSD sdData = LLDerenderEntry::toLLSD();

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
	std::for_each(m_Entries.begin(), m_Entries.end(), DeletePointer());
	m_Entries.clear();
}

void LLDerenderList::load()
{
	llifstream fileDerender(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, s_PersistFilename));
	if (!fileDerender.is_open())
	{
		LL_WARNS() << "Can't open derender list file \"" << s_PersistFilename << "\" for reading" << LL_ENDL;
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
			LL_INFOS() << "Failed to parse derender list entry" << LL_ENDL;
			break;
		}

		LLDerenderEntry* pEntry = LLDerenderEntry::fromLLSD(sdEntry);
		if (pEntry->isValid())
			m_Entries.push_back(pEntry);
		else
			delete pEntry;
	}

	fileDerender.close();
}

void LLDerenderList::save() const
{
	llofstream fileDerender(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, s_PersistFilename));
	if (!fileDerender.is_open())
	{
		LL_WARNS() << "Can't open derender list file \"" << s_PersistFilename << "\" for writing" << LL_ENDL;
		return;
	}

	for (entry_list_t::const_iterator itEntry = m_Entries.begin(); itEntry != m_Entries.end(); ++itEntry)
	{
		LLDerenderEntry* pEntry = *itEntry;
		if (pEntry->isPersistent())
			fileDerender << LLSDOStreamer<LLSDNotationFormatter>(pEntry->toLLSD()) << std::endl;
	}

	fileDerender.close();
}

// ============================================================================
// Entry helper functions
//

LLDerenderList::entry_list_t::iterator LLDerenderList::findEntry(LLDerenderEntry::EEntryType eType, const LLUUID& idEntry)
{
	return std::find_if(m_Entries.begin(), m_Entries.end(), [eType, &idEntry](const LLDerenderEntry* e) { return (eType == e->getType()) && (idEntry == e->getID()); });
}

LLDerenderList::entry_list_t::iterator LLDerenderList::findObjectEntry(U64 idRegion, const LLUUID& idObject, U32 idRootLocal)
{
	// NOTE: 'idRootLocal' will be 0 for the root prim itself and is the only time we need to compare against 'idObject'
	return std::find_if(m_Entries.begin(), m_Entries.end(), 
						[&idRegion, &idObject, &idRootLocal](const LLDerenderEntry* e)
						{ return (LLDerenderEntry::TYPE_OBJECT == e->getType()) && 
						         ( ((idRootLocal) && (idRegion == ((LLDerenderObject&)e).idRegion) && (idRootLocal == ((LLDerenderObject*)e)->idRootLocal)) || (idObject == e->getID()) ); });
}

void LLDerenderList::removeObject(LLDerenderEntry::EEntryType eType, const LLUUID& idObject)
{
	uuid_vec_t idsObject;
	idsObject.push_back(idObject);
	removeObjects(eType, idsObject);
}

void LLDerenderList::removeObjects(LLDerenderEntry::EEntryType eType, const uuid_vec_t& idsObject)
{
	bool fSave = false;
	std::map<LLViewerRegion*, std::list<U32>> idRegionObjectMap; // TYPE_OBJECT

	for (uuid_vec_t::const_iterator itObject = idsObject.begin(); itObject != idsObject.end(); ++itObject)
	{
		entry_list_t::iterator itEntry = findEntry(eType, *itObject);
		if (m_Entries.end() == itEntry)
			continue;

		switch (eType)
		{
			case LLDerenderEntry::TYPE_OBJECT:
				{
					LLDerenderObject* pObjEntry = (LLDerenderObject*)&*itEntry;

					LLViewerRegion* pRegion = (0 != pObjEntry->idRegion) ? LLWorld::getInstance()->getRegionFromHandle(pObjEntry->idRegion) : NULL;
					if (pRegion)
					{
						std::list<U32>& idsLocal = idRegionObjectMap[pRegion];
						if (pObjEntry->idRootLocal)
							idsLocal.push_back(pObjEntry->idRootLocal);
						idsLocal.splice(idsLocal.end(), pObjEntry->idsChildLocal);
					}
				}
				break;
		}

		const LLDerenderEntry* pEntry = *itEntry;
		m_Entries.erase(itEntry);
		fSave |= pEntry->isPersistent();
		delete pEntry;
	}

	// TYPE_OBJECT
	for (std::map<LLViewerRegion*, std::list<U32> >::const_iterator itRegionMap = idRegionObjectMap.begin(); itRegionMap != idRegionObjectMap.end(); ++itRegionMap)
	{
		LLViewerRegion* pRegion = itRegionMap->first;
		for (std::list<U32>::const_iterator itObject = itRegionMap->second.begin(); itObject != itRegionMap->second.end(); ++itObject)
			pRegion->addCacheMissFull(*itObject);
		pRegion->requestCacheMisses();
	}

	if (fSave)
		save();
	s_ChangeSignal();
}

// ============================================================================
// Object handling functions
//

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

		LLDerenderObject* pEntry = new LLDerenderObject(pNode, fPersist);
		if ( (isDerendered(pEntry->getID())) || (gAgentID == pEntry->getID()) )
			continue;
		m_Entries.push_back(pEntry);
		
		if (pIdList)
		{
			pIdList->push_back(pEntry->getID());
		}

		LLViewerObject* pObj = pNode->getObject();
		if (pObj)
		{
			// Display green bubble on kill [see process_kill_object()]
			if (gShowObjectUpdates)
				gPipeline.addDebugBlip(pObj->getPositionAgent(), LLColor4(0.f, 1.f, 0.f, 1.f));
			// Keep the order of these the same as process_kill_object()
			LLViewerRegion* pObjRegion = pObj->getRegion(); U32 idObjLocal = pObj->getLocalID();
			gObjectList.killObject(pObj);
			if ( (LLViewerRegion::sVOCacheCullingEnabled) && (pObjRegion) )
				pObjRegion->killCacheEntry(idObjLocal);
			LLSelectMgr::getInstance()->removeObjectFromSelections(pEntry->getID());
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
	return 
		//   - the object isn't a child prim
		(pObj) && (pObj->getRootEdit() == pObj) &&
		//   - the object isn't currently sat on by the user
		( (isAgentAvatarValid()) && (!pObj->isChild(gAgentAvatarp)) ) &&
		//   - the object isn't an attachment
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

LLDerenderObject* LLDerenderList::getObjectEntry(const LLUUID& idObject) /*const*/
{
	LLDerenderList::entry_list_t::iterator itEntry = findEntry(LLDerenderEntry::TYPE_OBJECT, idObject);
	return (m_Entries.end() != itEntry) ? (LLDerenderObject*)*itEntry : NULL;
}

LLDerenderObject* LLDerenderList::getObjectEntry(U64 idRegion, const LLUUID& idObject, U32 idRootLocal) /*const*/
{
	LLDerenderList::entry_list_t::const_iterator itEntry = findObjectEntry(idRegion, idObject, idRootLocal);
	return (m_Entries.end() != itEntry) ? (LLDerenderObject*)*itEntry : NULL;
}

bool LLDerenderList::processObjectUpdate(U64 idRegion, const LLUUID& idObject, const LLVOCacheEntry* pCacheEntry)
{
	// Used by OUT_FULL_CACHED
	if (!pCacheEntry->isChild())
	{
		// We know it's a root prim so we can skip looking up its parent
		LLDerenderObject* pObjEntry = getObjectEntry(idObject);
		if (!pObjEntry)
		{
			return false;	// Not a derendered object
		}

		// Update root prim information
		pObjEntry->idRegion = idRegion;
		pObjEntry->idRootLocal = pCacheEntry->getLocalID();

		// We can use the object cache entry to give us all child prims IDs
		pObjEntry->idsChildLocal.clear();
		for (LLVOCacheEntry::vocache_entry_set_t::const_iterator itChild = pCacheEntry->getChildrenBegin(), endChild = pCacheEntry->getChildrenEnd(); itChild != endChild; ++itChild)
			pObjEntry->idsChildLocal.push_back((*itChild)->getLocalID());

		return true;
	}
	
	// Child prim so perform normal processing
	return processObjectUpdate(idRegion, idObject, pCacheEntry->getLocalID(), pCacheEntry->getDPBuffer());
}

bool LLDerenderList::processObjectUpdate(U64 idRegion, const LLUUID& idObject, U32 idObjectLocal, U32 idRootLocal)
{
	// Used by OUT_FULL
	LLDerenderObject* pObjEntry = (0 == idRootLocal) ? getObjectEntry(idObject) : getObjectEntry(idRegion, idObject, idRootLocal);
	if (pObjEntry)
	{
		if (0 != idRootLocal)
		{
			// We're updating a child prim
			if (pObjEntry->idsChildLocal.end() == std::find(pObjEntry->idsChildLocal.begin(), pObjEntry->idsChildLocal.end(), idRootLocal))
				pObjEntry->idsChildLocal.push_back(idObjectLocal);
		}
		else
		{
			// We're updating the root prim
			pObjEntry->idRegion = idRegion;
			pObjEntry->idRootLocal = idObjectLocal;
		}
		return true;
	}
	return false;
}

bool LLDerenderList::processObjectUpdate(U64 idRegion, const LLUUID& idObject, U32 idObjectLocal, const U8* pBuffer)
{
	// Used by OUT_FULL_COMPRESSED
	U32 idRootLocal = 0, nMask = 0;
	htonmemcpy(&nMask, pBuffer + 64, MVT_U32, 4);			// "SpecialCode"
	if (nMask & 0x20)
		htonmemcpy(&idRootLocal, pBuffer + 84, MVT_U32, 4);	// "ParentID"
	return processObjectUpdate(idRegion, idObject, idObjectLocal, idRootLocal);
}

// ============================================================================
