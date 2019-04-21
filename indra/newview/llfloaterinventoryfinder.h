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
class LLRadioGroup;
class LLSpinCtrl;

// ====================================================================================
// LLFloaterUpdate class
//

#pragma warning(push)
#pragma warning(disable : 4351)

class LLFloaterInventoryFinder : public LLFloater
{
public:
	LLFloaterInventoryFinder(LLPanelMainInventory* inventory_view);
	~LLFloaterInventoryFinder() override;

	/*
	 * Base class overrides
	 */
public:
	BOOL        postBuild() override;
	void        closeFloater(bool fAppQuitting = false) override;

	/*
	 * Member functions
	 */
public:
	void        refreshControls();
	void        refreshFilter(const LLUICtrl* pCtrl = nullptr);
	void        selectAllTypes();
	void        selectNoTypes();
	void        setFilterFromPanel();
protected:
	bool        areDateLimitsSet() const;
	void        resetDateControls();
	void        setDateSpinnerValue(LLSpinCtrl* pDateSpinCtrl, LLComboBox* pDateTypeCtrl, U32 nHoursValue);

	std::string getNameFilterValue() const;
	void        setNameFilterValue(std::string strFilter);

	void        onExpandCollapse(bool fExpand);
	void        onFilterAllTypes(bool fSelectAll);
	void        onFilterByDate(const LLUICtrl* pCheckCtrl, bool fRefreshFilter);

	/*
	 * Member variables
	 */
protected:
	LLInventoryFilter* m_pFilter = nullptr;
	boost::signals2::connection m_FilterModifiedConnection;
	bool m_FilterRefreshing = false;
	LLPanelMainInventory* m_pPanelMainInventory = nullptr;

	// Filter by name/description/creator/permissions
	LLFilterEditor* m_pFilterName = nullptr;
	LLRadioGroup* m_pFilterNameMatchType = nullptr;
	LLFilterEditor* m_pFilterDescription = nullptr;
	LLAvatarEditor* m_pFilterCreator = nullptr;
	LLCheckBoxCtrl* m_pFilterPermModify = nullptr;
	LLCheckBoxCtrl* m_pFilterPermCopy = nullptr;
	LLCheckBoxCtrl* m_pFilterPermTransfer = nullptr;

	// Filter by type
	LLCheckBoxCtrl* m_pFilterTypeCtrls[LLInventoryType::IT_COUNT] = {};

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

#pragma warning(pop)

// ====================================================================================
