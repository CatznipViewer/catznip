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
// Helper functions
//

// NOTE: returns C_ANY for an invalid category since we're only using this for search
LLParcel::ECategory getParcelCategoryFromString(const std::string& strCategory)
{
	typedef std::pair<std::string, LLParcel::ECategory> category_pair_t;
	typedef std::map<std::string, LLParcel::ECategory> category_map_t;
	static category_map_t s_Categories;
	if (s_Categories.empty())
	{
		s_Categories.insert(category_pair_t("any", LLParcel::C_ANY));
		s_Categories.insert(category_pair_t("linden", LLParcel::C_LINDEN));
		s_Categories.insert(category_pair_t("adult", LLParcel::C_ADULT));
		s_Categories.insert(category_pair_t("arts", LLParcel::C_ARTS));
		s_Categories.insert(category_pair_t("store", LLParcel::C_BUSINESS));
		s_Categories.insert(category_pair_t("educational", LLParcel::C_EDUCATIONAL));
		s_Categories.insert(category_pair_t("game", LLParcel::C_GAMING));
		s_Categories.insert(category_pair_t("gather", LLParcel::C_HANGOUT));
		s_Categories.insert(category_pair_t("newcomer", LLParcel::C_NEWCOMER));
		s_Categories.insert(category_pair_t("park", LLParcel::C_PARK));
		s_Categories.insert(category_pair_t("home", LLParcel::C_RESIDENTIAL));
		s_Categories.insert(category_pair_t("shopping", LLParcel::C_SHOPPING));
		s_Categories.insert(category_pair_t("rental", LLParcel::C_RENTAL));
		s_Categories.insert(category_pair_t("other", LLParcel::C_OTHER));
	}
	category_map_t::const_iterator itCategory = s_Categories.find(strCategory);
	return (s_Categories.end() != itCategory) ? itCategory->second : LLParcel::C_ANY;
}

// ============================================================================
// LLPanelPlacesSearch
//

static LLRegisterPanelClassWrapper<LLPanelPlacesSearch> t_panel_places_search("panel_places_search");

LLPanelPlacesSearch::LLPanelPlacesSearch()
	: LLPanel()
	, m_nCurIndex(0)
	, m_pSearchEditor(NULL)
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
	m_pSearchEditor = findChild<LLLineEditor>("search_query");
	m_pSearchCategory = findChild<LLComboBox>("search_category");
	findChild<LLUICtrl>("search_start")->setCommitCallback(boost::bind(&LLPanelPlacesSearch::onSearchStart, this));

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

void LLPanelPlacesSearch::onSearchStart()
{
	// Clean up any pending queries
	if (m_idCurQuery.notNull())
	{
		LLSearchDirectory::instance().cancelQuery(m_idCurQuery);
		m_idCurQuery.setNull();
	}

	m_pResultsList->clearRows();
	m_pResultsList->setCommentText(getString("searching"));
	m_pParcelInfo->setParcelFromId(LLUUID::null);

	U32 nSearchFlags = 0;
	if (m_pSearchPG->get())
		nSearchFlags |= DFQ_INC_PG;
	if (m_pSearchMature->get())
		nSearchFlags |= DFQ_INC_MATURE;
	if (m_pSearchAdult->get())
		nSearchFlags |= DFQ_INC_ADULT;

	m_nCurIndex = 0;
	m_strCurQuery = m_pSearchEditor->getText();
	m_idCurQuery = LLSearchDirectory::instance().queryPlaces(
		m_strCurQuery, getParcelCategoryFromString(m_pSearchCategory->getSelectedValue().asString()), 
		nSearchFlags, m_nCurIndex, boost::bind(&LLPanelPlacesSearch::onSearchResult, this, _1, _2, _3));
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

	m_pResultsList->clearRows();
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

// ============================================================================
