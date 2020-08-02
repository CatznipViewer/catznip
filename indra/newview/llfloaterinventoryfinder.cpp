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

#include "llviewerprecompiledheaders.h"

#include "llavatareditor.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfiltereditor.h"
#include "llfloaterinventoryfinder.h"
#include "llinventoryfilter.h"
#include "llinventorypanel.h"
#include "llpanelmaininventory.h"
#include "llradiogroup.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"

#include <boost/algorithm/string.hpp>

#pragma warning(disable : 4351)

// ====================================================================================
// Constants
//

#define PREFIX_MATCH_EXACT "exact"
#define PREFIX_MATCH_ANY   "any"
#define PREFIX_MATCH_ALL   "all"

// ====================================================================================
// LLFloaterInventoryFinder class
//

LLFloaterInventoryFinder::LLFloaterInventoryFinder(LLPanelMainInventory* inventory_view)
	: LLFloater(LLSD())
	, m_pPanelMainInventory(inventory_view)
{
	mCommitCallbackRegistrar.add("Filter.Refresh", boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	mCommitCallbackRegistrar.add("Filter.Reset", boost::bind(&LLPanelMainInventory::resetFilters, inventory_view));
	mCommitCallbackRegistrar.add("Filter.SelectNoTypes", boost::bind(&LLFloaterInventoryFinder::onFilterAllTypes, this, false));
	mCommitCallbackRegistrar.add("Filter.SelectAllTypes", boost::bind(&LLFloaterInventoryFinder::onFilterAllTypes, this, true));
	mCommitCallbackRegistrar.add("Floater.Collapse", boost::bind(&LLFloaterInventoryFinder::onExpandCollapse, this, false));
	mCommitCallbackRegistrar.add("Floater.Expand", boost::bind(&LLFloaterInventoryFinder::onExpandCollapse, this, true));

	if (!gSavedSettings.getBOOL("InventoryFinderExpanded"))
		buildFromFile("floater_inventory_view_finder_tabbed.xml");
	else
		buildFromFile("floater_inventory_view_finder_expanded.xml");

	setFilterFromPanel();
}

LLFloaterInventoryFinder::~LLFloaterInventoryFinder()
{
	m_FilterModifiedConnection.disconnect();
}

// virtual
BOOL LLFloaterInventoryFinder::postBuild()
{
	const LLRect& rct = m_pPanelMainInventory->getRect();
	setRect(LLRect(rct.mLeft - getRect().getWidth(), rct.mTop, rct.mLeft, rct.mTop - getRect().getHeight()));

	//
	// Filter by name/description/creator/permissions
	//
	m_pFilterName = findChild<LLFilterEditor>("filter_name");
	m_pFilterName->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterNameMatchType = findChild<LLRadioGroup>("radio_name_match_type");
	m_pFilterNameMatchType->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterDescription = findChild<LLFilterEditor>("filter_description");
	m_pFilterDescription->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterCreator = findChild<LLAvatarEditor>("filter_creator");
	m_pFilterCreator->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterPermModify = findChild<LLCheckBoxCtrl>("check_perm_modify");
	m_pFilterPermModify->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterPermCopy = findChild<LLCheckBoxCtrl>("check_perm_copy");
	m_pFilterPermCopy->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterPermTransfer = findChild<LLCheckBoxCtrl>("check_perm_transfer");
	m_pFilterPermTransfer->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));

	//
	// Filter by (inventory) type
	//
	m_pFilterTypeCtrls[LLInventoryType::IT_ANIMATION] = findChild<LLCheckBoxCtrl>("check_animation");
	m_pFilterTypeCtrls[LLInventoryType::IT_CALLINGCARD] = findChild<LLCheckBoxCtrl>("check_calling_card");
	m_pFilterTypeCtrls[LLInventoryType::IT_WEARABLE] = findChild<LLCheckBoxCtrl>("check_clothing");
	m_pFilterTypeCtrls[LLInventoryType::IT_GESTURE] = findChild<LLCheckBoxCtrl>("check_gesture");
	m_pFilterTypeCtrls[LLInventoryType::IT_LANDMARK] = findChild<LLCheckBoxCtrl>("check_landmark");
	m_pFilterTypeCtrls[LLInventoryType::IT_NOTECARD] = findChild<LLCheckBoxCtrl>("check_notecard");
	m_pFilterTypeCtrls[LLInventoryType::IT_OBJECT] = findChild<LLCheckBoxCtrl>("check_object");
	m_pFilterTypeCtrls[LLInventoryType::IT_ATTACHMENT] = findChild<LLCheckBoxCtrl>("check_object");
	m_pFilterTypeCtrls[LLInventoryType::IT_LSL] = findChild<LLCheckBoxCtrl>("check_script");
	m_pFilterTypeCtrls[LLInventoryType::IT_SETTINGS] = findChild<LLCheckBoxCtrl>("check_settings");
	m_pFilterTypeCtrls[LLInventoryType::IT_SNAPSHOT] = findChild<LLCheckBoxCtrl>("check_snapshot");
	m_pFilterTypeCtrls[LLInventoryType::IT_SOUND] = findChild<LLCheckBoxCtrl>("check_sound");
	m_pFilterTypeCtrls[LLInventoryType::IT_TEXTURE] = findChild<LLCheckBoxCtrl>("check_texture");
	for (int idxType = 0; idxType < LLInventoryType::IT_COUNT; idxType++)
	{
		if (m_pFilterTypeCtrls[idxType])
			m_pFilterTypeCtrls[idxType]->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	}

	//
	// Filter by date
	//
	m_pFilterSinceLogoff = findChild<LLCheckBoxCtrl>("check_since_logoff");
	m_pFilterSinceLogoff->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::onFilterByDate, this, _1, true));
	m_pFilterMinAgeCheck = findChild<LLCheckBoxCtrl>("check_min_age");
	m_pFilterMinAgeCheck->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::onFilterByDate, this, _1, true));
	m_pFilterMinAgeSpin = findChild<LLSpinCtrl>("spin_min_age");
	m_pFilterMinAgeSpin->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterMinAgeType = findChild<LLComboBox>("combo_min_age");
	m_pFilterMinAgeType->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterMaxAgeCheck = findChild<LLCheckBoxCtrl>("check_max_age");
	m_pFilterMaxAgeCheck->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::onFilterByDate, this, _1, true));
	m_pFilterMaxAgeSpin = findChild<LLSpinCtrl>("spin_max_age");
	m_pFilterMaxAgeSpin->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterMaxAgeType = findChild<LLComboBox>("combo_max_age");
	m_pFilterMaxAgeType->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterAgeRangeCheck = findChild<LLCheckBoxCtrl>("check_range");
	m_pFilterAgeRangeCheck->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::onFilterByDate, this, _1, true));
	m_pFilterAgeRangeStart = findChild<LLSpinCtrl>("spin_range_start");
	m_pFilterAgeRangeStart->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterAgeRangeEnd = findChild<LLSpinCtrl>("spin_range_end");
	m_pFilterAgeRangeEnd->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterAgeRangeType = findChild<LLComboBox>("combo_range");
	m_pFilterAgeRangeType->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));

	//
	// General options
	//
	m_pFilterShowEmpty = findChild<LLCheckBoxCtrl>("check_show_empty");
	m_pFilterShowEmpty->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));
	m_pFilterShowLinks = findChild<LLCheckBoxCtrl>("check_show_links");
	m_pFilterShowLinks->setCommitCallback(boost::bind(&LLFloaterInventoryFinder::refreshFilter, this, _1));

	refreshControls();

	return TRUE;
}

// virtual
void LLFloaterInventoryFinder::closeFloater(bool fAppQuitting)
{
	if ( (!fAppQuitting) && (gSavedSettings.getBOOL("InventoryResetFilterOnFinderClose")) )
	{
		m_pPanelMainInventory->resetFilters();
	}

	LLFloater::closeFloater(fAppQuitting);
}

// ====================================================================================
// LLFloaterInventoryFinder member functions
//

void LLFloaterInventoryFinder::refreshControls()
 {
	if ( (!m_pFilter) || (m_FilterRefreshing) )
		return;

	setTitle(m_pFilter->getName());

	//
	// Filter by name/description/creator
	//
	setNameFilterValue( (m_pFilter->hasFilterString()) ? m_pFilter->getFilterSubStringOrig() : LLStringUtil::null );
	m_pFilterDescription->setValue( (m_pFilter->hasFilterDescriptionString()) ? m_pFilter->getFilterDescriptionSubString() : LLStringUtil::null );
	m_pFilterCreator->setValue( (m_pFilter->isFilterCreatorUUID()) ? m_pFilter->getFilterCreatorUUID() : LLUUID::null );
	m_pFilterPermModify->set(m_pFilter->getFilterPermissionsDeny() & PERM_MODIFY);
	m_pFilterPermCopy->set(m_pFilter->getFilterPermissionsDeny() & PERM_COPY);
	m_pFilterPermTransfer->set(m_pFilter->getFilterPermissionsDeny() & PERM_TRANSFER);

	//
	// Filter by (inventory) type
	//
	for (int idxType = 0; idxType < LLInventoryType::IT_COUNT; idxType++)
	{
		if (m_pFilterTypeCtrls[idxType])
			m_pFilterTypeCtrls[idxType]->set(m_pFilter->getFilterObjectTypes() & ((U64)0x1 << idxType));
	}

	//
	// Filter by date
	//
	LLCheckBoxCtrl* pActiveDateCtrl = nullptr;
	if (m_pFilter->isSinceLogoff())
	{
		m_pFilterSinceLogoff->set(true);
		pActiveDateCtrl = m_pFilterSinceLogoff;
	}
	else if (m_pFilter->isHoursAgo())
	{
		bool fOlderThan = (LLInventoryFilter::FILTERDATEDIRECTION_OLDER == m_pFilter->getDateSearchDirection());
		pActiveDateCtrl = (fOlderThan) ? m_pFilterMinAgeCheck : m_pFilterMaxAgeCheck;
		pActiveDateCtrl->set(true);
		setDateSpinnerValue( (fOlderThan) ? m_pFilterMinAgeSpin : m_pFilterMaxAgeSpin, (fOlderThan) ? m_pFilterMinAgeType : m_pFilterMaxAgeType, m_pFilter->getHoursAgo() );
	}
	else if (m_pFilter->isDateRange())
	{
		m_pFilterAgeRangeCheck->set(true);

		time_t timeCurrent = time_corrected();

		U32 nSecondsStart = (U32)llmax(timeCurrent - m_pFilter->getMinDate(), (time_t)0) + 30 * 60;
		U32 nSecondsEnd = (U32)llmax(timeCurrent - m_pFilter->getMaxDate(), (time_t)0) + 30 * 60;

		U32 nHoursStart = llmin(nSecondsStart, nSecondsEnd) / (60 * 60);
		U32 nHoursEnd = llmax(nSecondsStart, nSecondsEnd) / (60 * 60);

		if ( (0 == nHoursStart) && (0 == nHoursEnd) )
		{
			m_pFilterAgeRangeStart->setValue(0);
			m_pFilterAgeRangeEnd->setValue(0);
		}
		else if ( (0 == nHoursStart % 24) && (0 == nHoursEnd % 24) )
		{
			m_pFilterAgeRangeStart->setValue((S32)(nHoursStart / 24));
			m_pFilterAgeRangeEnd->setValue((S32)(nHoursEnd / 24));
			m_pFilterAgeRangeType->setValue("days");
		}
		else
		{
			m_pFilterAgeRangeStart->setValue((S32)(nHoursStart));
			m_pFilterAgeRangeEnd->setValue((S32)(nHoursEnd));
			m_pFilterAgeRangeType->setValue("hours");
		}

		pActiveDateCtrl = m_pFilterAgeRangeCheck;;
	}
	// Enables/disables date-related controls
	if ( (m_pFilter->areDateLimitsSet()) || (areDateLimitsSet()) )
		onFilterByDate(pActiveDateCtrl, false);

	//
	// General options
	//
	m_pFilterShowEmpty->setValue(m_pFilter->getShowFolderState() == LLInventoryFilter::SHOW_ALL_FOLDERS);

	// Disable the "Show Links" option if the user is currently using "Find All Links"
	m_pFilterShowLinks->setEnabled(!m_pFilter->isFilterUUID());
	m_pFilterShowLinks->setValue(LLInventoryFilter::FILTERLINK_EXCLUDE_LINKS != m_pFilter->getFilterLinks());
}

void LLFloaterInventoryFinder::refreshFilter(const LLUICtrl* pCtrl /*=nullptr*/)
{
	LLInventoryPanel* pInvPanel = m_pPanelMainInventory->getPanel();
	m_FilterRefreshing = true;

	//
	// Filter by name/description/creator
	//
	m_pFilter->setFilterSubString(getNameFilterValue());
	m_pFilter->setFilterDescriptionSubString(m_pFilterDescription->getValue());
	m_pFilter->setFilterCreatorUUID(m_pFilterCreator->getValue().asUUID());

	//
	// Filter by permissions
	//
	PermissionMask maskPerm = m_pFilter->getFilterPermissionsDeny();
	if (m_pFilterPermModify->get())
		maskPerm |= PERM_MODIFY;
	else
		maskPerm &= ~PERM_MODIFY;
	if (m_pFilterPermCopy->get())
		maskPerm |= PERM_COPY;
	else
		maskPerm &= ~PERM_COPY;
	if (m_pFilterPermTransfer->get())
		maskPerm |= PERM_TRANSFER;
	else
		maskPerm &= ~PERM_TRANSFER;
	m_pFilter->setFilterPermissionsDeny(maskPerm);

	//
	// Filter by (inventory) type
	//
	U64 filter = 0xffffffffffffffffULL;
	bool filtered_by_all_types = true;

	for (int idxType = 0; idxType < LLInventoryType::IT_COUNT; idxType++)
	{
		if ( (m_pFilterTypeCtrls[idxType]) && (!m_pFilterTypeCtrls[idxType]->getValue()) )
		{
			filter &= ~(0x1 << idxType);
			filtered_by_all_types = false;
		}
	}

	if ( (!filtered_by_all_types) || (pInvPanel->getFilter().getFilterTypes() & LLInventoryFilter::FILTERTYPE_DATE) )
	{
		// Don't include folders in filter, unless I've selected everything
		filter &= ~(0x1 << LLInventoryType::IT_CATEGORY);
	}
	pInvPanel->setFilterTypes(filter);

	//
	// Filter by date
	//
	if (m_pFilterSinceLogoff->get())
	{
		pInvPanel->setSinceLogoff(true);
	}
	else if ( (m_pFilterMinAgeCheck->get()) || (m_pFilterMaxAgeCheck->get()) )
	{
		bool fOlderThan = m_pFilterMinAgeCheck->get();
		LLSpinCtrl* pActiveSpinCtrl = (fOlderThan) ? m_pFilterMinAgeSpin : m_pFilterMaxAgeSpin;
		LLComboBox* pActiveTypeCtrl = (fOlderThan) ? m_pFilterMinAgeType : m_pFilterMaxAgeType;

		U32 nHours = 0;
		F32 nValue = pActiveSpinCtrl->getValue().asReal();

		if (pActiveTypeCtrl->getSelectedValue().asString() == "hours")
			nHours = (U32)nValue;
		else
			nHours = nValue * 24;

		pInvPanel->setHoursAgo(nHours);
		pInvPanel->setDateSearchDirection( (fOlderThan) ? LLInventoryFilter::FILTERDATEDIRECTION_OLDER : LLInventoryFilter::FILTERDATEDIRECTION_NEWER );
	}
	else if (m_pFilterAgeRangeCheck->get())
	{
		F64 nValueStart = m_pFilterAgeRangeStart->getValue().asReal(),
			nValueEnd = m_pFilterAgeRangeEnd->getValue().asReal();
		if ( (pCtrl == m_pFilterAgeRangeStart) && (nValueStart > nValueEnd) )
			nValueEnd = nValueStart;
		else if ( (pCtrl == m_pFilterAgeRangeEnd) && (nValueEnd < nValueStart) )
			nValueStart = nValueEnd;

		time_t timeCurrent = time_corrected();

		time_t timeStart = time_min(), timeEnd = time_max();
		if (m_pFilterAgeRangeType->getSelectedValue().asString() == "hours")
		{
			timeStart = timeCurrent - (llmax(nValueStart, nValueEnd) * 60 * 60);
			timeEnd = timeCurrent - (llmin(nValueStart, nValueEnd) * 60 * 60);
		}
		else
		{
			timeStart = timeCurrent - (llmax(nValueStart, nValueEnd) * 24 * 60 * 60);
			timeEnd = timeCurrent - (llmin(nValueStart, nValueEnd) * 24 * 60 * 60);
		}

		// If we filter [0, 10] hours then new items that arrive will be excluded so we cheat and make it [-24, 10] instead
		if ( (timeStart != timeCurrent) && (timeEnd == timeCurrent) )
			timeEnd += 24 * 60 * 60;

		pInvPanel->setDateRange(timeStart, timeEnd);
	}
	else
	{
		pInvPanel->setDateRange(time_min(), time_max());
	}

	//
	// General options
	//
	pInvPanel->setShowFolderState( (m_pFilterShowEmpty->get()) ? LLInventoryFilter::SHOW_ALL_FOLDERS : LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS );
	if (m_pFilterShowLinks->getEnabled())
		pInvPanel->setFilterLinks( (m_pFilterShowLinks->get()) ? LLInventoryFilter::FILTERLINK_INCLUDE_LINKS : LLInventoryFilter::FILTERLINK_EXCLUDE_LINKS, true);
	m_pPanelMainInventory->setFilterSubStringFromFilter();
	m_pPanelMainInventory->setFilterTextFromFilter();

	m_FilterRefreshing = false;
}

void LLFloaterInventoryFinder::selectAllTypes()
{
	// Always refresh our current state on external calls (filter may have changed without an event trigger)
	refreshControls();
	onFilterAllTypes(true);
}

void LLFloaterInventoryFinder::selectNoTypes()
{
	// Always refresh our current state on external calls (filter may have changed without an event trigger)
	refreshControls();
	onFilterAllTypes(false);
}

void LLFloaterInventoryFinder::setFilterFromPanel()
{
	LLInventoryPanel* pInvPanel = m_pPanelMainInventory->getPanel();
	if (!pInvPanel)
		return;

	m_pFilter = &pInvPanel->getFilter();

	if (m_FilterModifiedConnection.connected())
		m_FilterModifiedConnection.disconnect();
	m_FilterModifiedConnection = pInvPanel->getRootFolder()->setFilterStateChangedCallback(boost::bind(&LLFloaterInventoryFinder::refreshControls, this));

	resetDateControls();
	refreshControls();
}

bool LLFloaterInventoryFinder::areDateLimitsSet() const
{
	return
		(m_pFilterSinceLogoff->get()) ||
		( (m_pFilterMinAgeCheck->get()) && (m_pFilterMinAgeSpin->getValue().asInteger()) ) ||
		( (m_pFilterMaxAgeCheck->get()) && (m_pFilterMaxAgeSpin->getValue().asInteger()) ) ||
		( (m_pFilterAgeRangeCheck->get()) && ((m_pFilterAgeRangeStart->getValue().asInteger()) || (m_pFilterAgeRangeEnd->getValue().asInteger())) );

}

void LLFloaterInventoryFinder::resetDateControls()
{
	onFilterByDate(nullptr, false);
	m_pFilterMinAgeSpin->setValue(0);
	m_pFilterMaxAgeSpin->setValue(0);
	m_pFilterAgeRangeStart->setValue(0);
	m_pFilterAgeRangeEnd->setValue(0);
}

void LLFloaterInventoryFinder::setDateSpinnerValue(LLSpinCtrl* pDateSpinCtrl, LLComboBox* pDateTypeCtrl, U32 nHoursValue)
{
	// Don't reapply the same value (keeps us from switching from hours -> days when going from 23 to 24)
	U32 nCurValue = pDateSpinCtrl->getValue().asInteger() * ((pDateTypeCtrl->getSelectedValue().asString() == "hours") ? 1 : 24);
	if (nCurValue == nHoursValue)
		return;

	if (0 == nHoursValue % 24)
	{
		pDateSpinCtrl->setValue((S32)(nHoursValue / 24));
		pDateTypeCtrl->setValue("days");
	}
	else
	{
		pDateSpinCtrl->setValue((S32)nHoursValue);
		pDateTypeCtrl->setValue("hours");
	}
}

std::string LLFloaterInventoryFinder::getNameFilterValue() const
{
	const std::string strFilterType = m_pFilterNameMatchType->getSelectedValue().asString();
	if (PREFIX_MATCH_EXACT == strFilterType)
	{
		return m_pFilterName->getText();
	}
	else if (PREFIX_MATCH_ALL == strFilterType)
	{
		std::string strFilter(PREFIX_MATCH_ALL":");
		boost::replace_all(strFilter.append(m_pFilterName->getText()), " ", "|");
		return strFilter;
	}
	else if (PREFIX_MATCH_ANY == strFilterType)
	{
		std::string strFilter(PREFIX_MATCH_ANY":");
		boost::replace_all(strFilter.append(m_pFilterName->getText()), " ", "|");
		return strFilter;
	}
	return LLStringUtil::null;
}

void LLFloaterInventoryFinder::setNameFilterValue(std::string strFilter)
{
	std::string strFilterType = (strFilter.size() >= 4) ? strFilter.substr(0, 4) : LLStringUtil::null;
	if ( (PREFIX_MATCH_ALL":" == strFilterType) || (PREFIX_MATCH_ANY":" == strFilterType) )
	{
		strFilter = (strFilter.size() > 4) ? strFilter.substr(4) : LLStringUtil::null;
		boost::replace_all(strFilter, "|", " ");
		m_pFilterName->setText(strFilter);
		strFilterType.pop_back();
		m_pFilterNameMatchType->selectByValue(strFilterType);
	}
	else
	{
		m_pFilterName->setText(strFilter);
		m_pFilterNameMatchType->selectByValue(PREFIX_MATCH_EXACT);
	}
}

void LLFloaterInventoryFinder::onExpandCollapse(bool fExpand)
{
	gSavedSettings.setBOOL("InventoryFinderExpanded", fExpand);

	LLHandle<LLFloater> hSnapTarget = getSnapTarget();
	LLRect curRect = getRect();

	m_pPanelMainInventory->showFindOptions(false);
	m_pPanelMainInventory->showFindOptions(true);

	LLFloaterInventoryFinder* pNewInst = m_pPanelMainInventory->getFinder();
	if (!hSnapTarget.isDead())
		pNewInst->setSnappedTo(hSnapTarget.get());
	else
		pNewInst->clearSnapTarget();
	pNewInst->translate(curRect.mLeft - pNewInst->getRect().mLeft, curRect.mTop - pNewInst->getRect().mTop);
}

void LLFloaterInventoryFinder::onFilterAllTypes(bool fSelectAll)
{
	for (int idxType = 0; idxType < LLInventoryType::IT_COUNT; idxType++)
	{
		if (m_pFilterTypeCtrls[idxType])
			m_pFilterTypeCtrls[idxType]->set(fSelectAll);
	}
	refreshFilter();
}

void LLFloaterInventoryFinder::onFilterByDate(const LLUICtrl* pCheckCtrl, bool fRefreshFilter)
{
	// Since logoff
	if (pCheckCtrl != m_pFilterSinceLogoff)
		m_pFilterSinceLogoff->set(false);

	// Min age
	if (pCheckCtrl != m_pFilterMinAgeCheck)
		m_pFilterMinAgeCheck->set(false);
	m_pFilterMinAgeSpin->setEnabled(m_pFilterMinAgeCheck->get());
	m_pFilterMinAgeType->setEnabled(m_pFilterMinAgeCheck->get());

	// Max age
	if (pCheckCtrl != m_pFilterMaxAgeCheck)
		m_pFilterMaxAgeCheck->set(false);
	m_pFilterMaxAgeSpin->setEnabled(m_pFilterMaxAgeCheck->get());
	m_pFilterMaxAgeType->setEnabled(m_pFilterMaxAgeCheck->get());

	// Date range
	if (pCheckCtrl != m_pFilterAgeRangeCheck)
		m_pFilterAgeRangeCheck->set(false);
	m_pFilterAgeRangeStart->setEnabled(m_pFilterAgeRangeCheck->get());
	m_pFilterAgeRangeEnd->setEnabled(m_pFilterAgeRangeCheck->get());
	m_pFilterAgeRangeType->setEnabled(m_pFilterAgeRangeCheck->get());

	if (fRefreshFilter)
	{
		m_pFilter->resetDateLimits();
		refreshFilter(pCheckCtrl);
	}
}

// ====================================================================================
