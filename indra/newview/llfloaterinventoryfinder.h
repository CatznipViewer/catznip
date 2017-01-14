/**
 *
 * Copyright (c) 2017, Kitty Barnett
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

#pragma once

#include "llfloater.h"
#include "llinventorytype.h"

// ====================================================================================
// Forward declarations
//

class LLAvatarEditor;
class LLComboBox;
class LLInventoryFilter;
class LLPanelMainInventory;
class LLSpinCtrl;

// ====================================================================================
// LLFloaterUpdate class
//

class LLFloaterInventoryFinder : public LLFloater
{
public:
	LLFloaterInventoryFinder(LLPanelMainInventory* inventory_view);
	~LLFloaterInventoryFinder();

	/*
	 * Base class overrides
	 */
public:
	BOOL postBuild() override;

	/*
	 * Member functions
	 */
public:
	void refreshControls();
	void refreshFilter();
	void selectAllTypes() { onFilterAllTypes(true); }
	void selectNoTypes() { onFilterAllTypes(false); }
	void setFilterFromPanel();
protected:
	bool areDateLimitsSet() const;
	void onFilterAllTypes(bool fSelectAll);
	void onFilterByDate(const LLUICtrl* pCheckCtrl, bool fRefreshFilter);
	void resetDateControls();
	void setDateSpinnerValue(LLSpinCtrl* pDateSpinCtrl, LLComboBox* pDateTypeCtrl, U32 nHoursValue);

	/*
	 * Member variables
	 */
protected:
	LLInventoryFilter* m_pFilter = nullptr;
	boost::signals2::connection m_FilterModifiedConnection;
	bool m_FilterRefreshing = false;
	LLPanelMainInventory* m_pPanelMainInventory = nullptr;

	// Filter by name/description/creator
	LLFilterEditor* m_pFilterName = nullptr;
	LLFilterEditor* m_pFilterDescription = nullptr;
	LLAvatarEditor* m_pFilterCreator = nullptr;

	// Filter by type
	LLCheckBoxCtrl* m_pFilterTypeCtrls[LLInventoryType::IT_COUNT];

	// Filter by date
	LLCheckBoxCtrl* m_pFilterSinceLogoff = nullptr;
	LLCheckBoxCtrl* m_pFilterMinAgeCheck = nullptr;
	LLSpinCtrl* m_pFilterMinAgeSpin = nullptr;
	LLComboBox* m_pFilterMinAgeType = nullptr;
	LLCheckBoxCtrl* m_pFilterMaxAgeCheck = nullptr;
	LLSpinCtrl* m_pFilterMaxAgeSpin = nullptr;
	LLComboBox* m_pFilterMaxAgeType = nullptr;
	LLCheckBoxCtrl* m_pFilterAgeRangeCheck = nullptr;
	LLSpinCtrl* m_pFilterAgeRangeStart = nullptr;
	LLSpinCtrl* m_pFilterAgeRangeEnd = nullptr;
	LLComboBox* m_pFilterAgeRangeType = nullptr;

	// General options
	LLCheckBoxCtrl* m_pFilterShowEmpty = nullptr;
	LLCheckBoxCtrl* m_pFilterShowLinks = nullptr;
};

// ====================================================================================
