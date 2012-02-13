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
#ifndef LL_DERENDERLIST_H
#define LL_DERENDERLIST_H

#include "llsingleton.h"
#include "lluuid.h"

class LLSelectNode;

// ============================================================================
// LLDerenderEntry
//

struct LLDerenderEntry
{
	LLDerenderEntry(const LLSelectNode* pNode);

	std::string	strObjectName;		// Object name (at the time of adding)
	LLUUID		idObject;			// Object UUID (at the time of adding)

	std::string	strRegionName;		// Region name
	U64			idRegion;			// Region handle
	U32			idObjectLocal;		// Local object ID (region-specific)
	LLVector3	posRegion;			// Position of the object on the region
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

public:
	void addCurrentSelection();
	bool isDerendered(const LLUUID& idObject) const;
	bool isDerendered(U64 idRegion, U32 idObjectLocal) const;
	void updateObject(const LLUUID& idObject, U64 idRegion, U32 idObjectLocal);

protected:
	typedef std::list<LLDerenderEntry> entry_list_t;
	entry_list_t m_Entries;
};

// ============================================================================

#endif // LL_DERENDERLIST_H