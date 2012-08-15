/** 
 *
 * Copyright (c) 2012, Kitty Barnett
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

#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpanelparcelinfo.h"
#include "llpanelplacessearch.h"
#include "llparcel.h"
#include "llqueryflags.h"
#include "llscrolllistctrl.h"
#include "lltrans.h"

// ============================================================================
// LLPanelPlacesSearch
//

static LLRegisterPanelClassWrapper<LLPanelPlacesSearch> t_panel_places_search("panel_places_search");

LLPanelPlacesSearch::LLPanelPlacesSearch()
	: LLPanel()
	, m_nCurIndex(0)
	, m_pSearchCategory(NULL)
	, m_pSearchPG(NULL)
	, m_pSearchMature(NULL)
	, m_pSearchAdult(NULL)
	, m_pResultsList(NULL)
{
}

LLPanelPlacesSearch::~LLPanelPlacesSearch()
{
	if (m_idCurQuery.notNull())
	{
		LLSearchDirectory::instance().cancelQuery(m_idCurQuery);
		m_idCurQuery.setNull();
	}
}

BOOL LLPanelPlacesSearch::postBuild()
{
	LLButton* pSearchBtn = findChild<LLButton>("search_start");
	if (pSearchBtn)
	{
		pSearchBtn->setCommitCallback(boost::bind(&LLPanelPlacesSearch::onSearchBtn, this));
	}

	m_pSearchCategory = findChild<LLComboBox>("search_category");
	m_pSearchCategory->add(getString("all_categories"), LLSD("any"));
	m_pSearchCategory->addSeparator();
	for (int idxCategory = LLParcel::C_LINDEN; idxCategory < LLParcel::C_COUNT; idxCategory++)
	{
		LLParcel::ECategory eCategory = (LLParcel::ECategory)idxCategory;
		m_pSearchCategory->add(LLParcel::getCategoryUIString(eCategory), LLParcel::getCategoryString(eCategory));
	}

	m_pSearchPG = findChild<LLCheckBoxCtrl>("search_pg_check");
	m_pSearchPG->setCommitCallback(boost::bind(&LLPanelPlacesSearch::onToggleMaturity, this));
	m_pSearchMature = findChild<LLCheckBoxCtrl>("search_mature_check");
	m_pSearchMature->setCommitCallback(boost::bind(&LLPanelPlacesSearch::onToggleMaturity, this));
	m_pSearchAdult = findChild<LLCheckBoxCtrl>("search_adult_check");
	m_pSearchAdult->setCommitCallback(boost::bind(&LLPanelPlacesSearch::onToggleMaturity, this));

	m_pResultsList = findChild<LLScrollListCtrl>("search_results");
	m_pResultsList->setCommitOnSelectionChange(true);
	m_pResultsList->setCommitOnKeyboardMovement(true);
	m_pResultsList->setCommitCallback(boost::bind(&LLPanelPlacesSearch::onResultSelect, this));

	m_pParcelInfo = findChild<LLPanelParcelInfo>("search_parcel");

	return TRUE;
}

void LLPanelPlacesSearch::onResultSelect()
{
	m_pParcelInfo->setParcelFromId(m_pResultsList->getSelectedValue().asUUID());
}

void LLPanelPlacesSearch::onSearchBtn()
{
	LLLineEditor* pSearchEditor = findChild<LLLineEditor>("search_query");
	if (pSearchEditor)
	{
		searchStart(pSearchEditor->getText());
	}
}

void LLPanelPlacesSearch::searchStart(const std::string& strQuery)
{
	// Clean up any pending queries
	if (m_idCurQuery.notNull())
	{
		LLSearchDirectory::instance().cancelQuery(m_idCurQuery);
		m_idCurQuery.setNull();
	}

	m_pResultsList->clearRows();
	m_pParcelInfo->setParcelFromId(LLUUID::null);

	m_nCurIndex = 0;
	m_strCurQuery = strQuery;
	if (!strQuery.empty())
	{
		U32 nSearchFlags = 0;
		if (m_pSearchPG->get())
			nSearchFlags |= DFQ_INC_PG;
		if (m_pSearchMature->get())
			nSearchFlags |= DFQ_INC_MATURE;
		if (m_pSearchAdult->get())
			nSearchFlags |= DFQ_INC_ADULT;

		m_idCurQuery = LLSearchDirectory::instance().queryPlaces(
			m_strCurQuery, LLParcel::getCategoryFromString(m_pSearchCategory->getSelectedValue().asString()), 
			nSearchFlags, m_nCurIndex, boost::bind(&LLPanelPlacesSearch::onSearchResult, this, _1, _2, _3));
		m_pResultsList->setCommentText(getString("searching"));
	}
	else
	{
		m_idCurQuery.setNull();
		m_pResultsList->setCommentText(getString("no_results"));
	}
}

void LLPanelPlacesSearch::onSearchResult(const LLUUID& idQuery, U32 nStatus, const LLSearchDirectory::places_results_vec_t& lResults)
{
	// Sanity check - only process results from the last submitted query
	if (m_idCurQuery != idQuery)
	{
		LLSearchDirectory::instance().cancelQuery(idQuery);
		return;
	}

	if (nStatus & STATUS_SEARCH_PLACES_BANNEDWORD)
	{
		LLNotificationsUtil::add("SearchWordBanned");
	}

	if (!lResults.empty())
	{
		LLSD sdRow; LLSD& sdColumns = sdRow["columns"];
		sdColumns[0]["column"] = "name";    sdColumns[0]["type"] = "text";
		sdColumns[1]["column"] = "traffic"; sdColumns[1]["type"] = "text";

		for (LLSearchDirectory::places_results_vec_t::const_iterator itResult = lResults.cbegin(); itResult != lResults.cend(); ++itResult)
		{
			sdColumns[0]["value"] = itResult->mParcelName;
			sdColumns[1]["value"] = llformat("%.0f", itResult->mDwell);
			sdRow["value"] = itResult->mParcelId;

			m_pResultsList->addElement(sdRow, ADD_BOTTOM);
		}
	}
	else
	{
		LLStringUtil::format_map_t args;
		args["TEXT"] = m_strCurQuery;
		m_pResultsList->setCommentText(getString("not_found", args));
	}
}

void LLPanelPlacesSearch::onToggleMaturity()
{
	// Make sure at least one of the checkboxes is always checked
	if ( (!m_pSearchPG->get()) && (!m_pSearchMature->get()) && (!m_pSearchAdult->get()) )
	{
		m_pSearchPG->set(TRUE);
	}
}

const LLUUID& LLPanelPlacesSearch::getCurrentParcelId() const
{
	return m_pParcelInfo->getCurrentParcelId();
}

const LLVector3d& LLPanelPlacesSearch::getCurrentParcelPos() const
{
	return m_pParcelInfo->getCurrentParcelPos();
}

boost::signals2::connection LLPanelPlacesSearch::setSelectCallback(const commit_signal_t::slot_type& cb)
{
	return m_pResultsList->setCommitCallback(cb);
}

// ============================================================================
