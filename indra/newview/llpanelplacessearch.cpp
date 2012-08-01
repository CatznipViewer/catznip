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

#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpanelparcelinfo.h"
#include "llpanelplacessearch.h"
#include "llparcel.h"
#include "llqueryflags.h"
#include "llscrolllistctrl.h"
#include "lltrans.h"

// ============================================================================

static LLRegisterPanelClassWrapper<LLPanelPlacesSearch> t_panel_places_search("panel_places_search");

LLPanelPlacesSearch::LLPanelPlacesSearch()
	: LLPanel()
	, m_nCurIndex(0)
	, m_pSearchEditor(NULL), m_pResultsList(NULL)
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
	findChild<LLUICtrl>("search_start")->setCommitCallback(boost::bind(&LLPanelPlacesSearch::onSearchStart, this));

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
	m_pResultsList->clearRows();
	m_nCurIndex = 0;
	if (m_idCurQuery.notNull())
	{
		LLSearchDirectory::instance().cancelQuery(m_idCurQuery);
		m_idCurQuery.setNull();
	}

	m_idCurQuery = LLSearchDirectory::instance().queryPlaces(m_pSearchEditor->getText(), LLParcel::C_ANY, DFQ_INC_PG | DFQ_INC_MATURE, m_nCurIndex, 
	                                                         boost::bind(&LLPanelPlacesSearch::onSearchResult, this, _1, _2, _3));
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
		LLNotificationsUtil::add("SearchWordBanned");

	LLSD sdRow; LLSD& sdColumns = sdRow["columns"];
	sdColumns[0]["column"] = "name";    sdColumns[0]["type"] = "text";
	sdColumns[1]["column"] = "traffic"; sdColumns[1]["type"] = "text";

	for (LLSearchDirectory::places_results_vec_t::const_iterator itResult = lResults.cbegin(); itResult != lResults.cend(); ++itResult)
	{
		sdRow["value"] = itResult->mParcelId;
		sdColumns[0]["value"] = itResult->mParcelName;
		sdColumns[1]["value"] = llformat("%.0f", itResult->mDwell);

		m_pResultsList->addElement(sdRow, ADD_BOTTOM);
	}
}

// ============================================================================
