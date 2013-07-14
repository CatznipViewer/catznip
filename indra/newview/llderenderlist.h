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
#ifndef LL_DERENDERLIST_H
#define LL_DERENDERLIST_H

#include "llsingleton.h"
#include "lluuid.h"

class LLSelectNode;
class LLViewerObject;

// ============================================================================
// LLDerenderEntry
//

struct LLDerenderEntry
{
	LLDerenderEntry(const LLSelectNode* pNode, bool fPersist = false);
	LLDerenderEntry(const LLSD& sdData);
	bool isValid() const { return idObject.notNull(); }
	LLSD toLLSD() const;

	bool			fPersists;			// TRUE if this entry persist across sessions

	std::string		strObjectName;		// Object name (at the time of adding)
	LLUUID			idObject;			// Object UUID (at the time of adding)

	std::string		strRegionName;		// Region name
	LLVector3		posRegion;			// Position of the object on the region
	U64				idRegion;			// Region handle
	U32				idRootLocal;		// Local object ID of the root (region-specific)
	std::list<U32>	idsChildLocal;		// Local object ID of all child prims
};

// ============================================================================
// LLDerenderList
//

class LLDerenderList : public LLSingleton<LLDerenderList>
{
	friend class LLSingleton<LLDerenderList>;
protected:
	LLDerenderList();
	/*virtual*/ ~LLDerenderList();

	/*
	 * Member functions
	 */
public:
	bool addSelection(bool fPersist, std::vector<LLUUID>* pIdList = NULL);
	bool isDerendered(const LLUUID& idObject) const									{ return m_Entries.end() != findEntry(idObject); }
	bool isDerendered(U64 idRegion, const LLUUID& idObject, U32 idRootLocal) const	{ return m_Entries.end() != findEntry(idRegion, idObject, idRootLocal); }
	void removeObject(const LLUUID& idObject);
	void removeObjects(const uuid_vec_t& idsObject);
	void updateObject(U64 idRegion, U32 idRootLocal, const LLUUID& idObject, U32 idObjectLocal);

	static bool canAdd(const LLViewerObject* pObj);
	static bool canAddSelection();
protected:
	void load();
	void save() const;

	/*
	 * Entry helper functions
	 */
public:
	typedef std::list<LLDerenderEntry> entry_list_t;
	const entry_list_t&			 getEntries() const { return m_Entries; }
protected:
	entry_list_t::iterator		 findEntry(const LLUUID& idObject);
	entry_list_t::const_iterator findEntry(const LLUUID& idObject) const;
	entry_list_t::iterator		 findEntry(U64 idRegion, const LLUUID& idObject, U32 idRootLocal);
	entry_list_t::const_iterator findEntry(U64 idRegion, const LLUUID& idObject, U32 idRootLocal) const;

	/*
	 * Static member functions
	 */
public:
	typedef boost::signals2::signal<void()> change_signal_t;
	static boost::signals2::connection setChangeCallback(const change_signal_t::slot_type& cb) { return s_ChangeSignal.connect(cb); }

protected:
	entry_list_t m_Entries;

	static change_signal_t	s_ChangeSignal;
	static std::string		s_PersistFilename;
};

// ============================================================================

#endif // LL_DERENDERLIST_H
