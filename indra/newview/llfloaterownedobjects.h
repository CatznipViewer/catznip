/** 
 *
 * Copyright (c) 2013, Kitty Barnett
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

#ifndef LL_LLFLOATEROWNEDOBJECTS_H
#define LL_LLFLOATEROWNEDOBJECTS_H

#include "llfloaterpathfindingobjects.h"

// ============================================================================
// Forward delcarations
//

class LLCheckBoxCtrl;
class LLLineEditor;
class LLSpinCtrl;

// ============================================================================
// LLFloaterOwnedObjects class declaration
//

class LLFloaterOwnedObjects : public LLFloaterPathfindingObjects
{
protected:
	friend class LLFloaterReg;
public:
	LLFloaterOwnedObjects(const LLSD& sdSeed);
	/*virtual*/ ~LLFloaterOwnedObjects();

	//
	// LLView overrides
	//
protected:
	/*virtual*/ BOOL postBuild();

	//
	// LLFloaterPathfindingObjects overrides
	//
public:
	/*virtual*/ void requestGetObjects();
protected:
	/*virtual*/ void                       buildObjectsScrollList(const LLPathfindingObjectListPtr pObjectList);
	/*virtual*/ LLPathfindingObjectListPtr getEmptyObjectList() const;
	/*virtual*/ S32                        getNameColumnIndex() const;
	/*virtual*/ S32                        getOwnerNameColumnIndex() const;

	//
	// Event handlers
	//
protected:
	void onApplyFilter();
	void onClearFilter();
	void onMinHeightFilterChanged();
	void onToggleFilter();
	void refreshFilterButtons();

	//
	// Member variables
	//
protected:
	LLCheckBoxCtrl* m_pLimitAgentParcel;
	LLCheckBoxCtrl* m_pFilterObjects;
	LLLineEditor*   m_pNameFilter;
	LLLineEditor*   m_pDescrFilter;
	LLCheckBoxCtrl* m_pFilterHeight;
	LLSpinCtrl*     m_pMinHeightFilter;
	LLSpinCtrl*     m_pMaxHeightFilter;
};

#endif // LL_LLFLOATEROWNEDOBJECTS_H
