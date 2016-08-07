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
#ifndef LL_LLPANELPLACESSEARCH_H
#define LL_LLPANELPLACESSEARCH_H

#include "llpanel.h"
#include "llsearchdirectory.h"

class LLCheckBoxCtrl;
class LLComboBox;
class LLLineEditor;
class LLPanelParcelInfo;
class LLScrollListCtrl;
class LLTextBox;

// ============================================================================
// LLPanelPlacesSearch class
//

class LLPanelPlacesSearch : public LLPanel
{
public:
	LLPanelPlacesSearch();
	/*virtual*/ ~LLPanelPlacesSearch();
	
	/*
	 * LLView overrides
	 */
public:
	/*virtual*/ BOOL postBuild();

	/*
	 * Member functions
	 */
public:
	const LLUUID&               getCurrentParcelId() const;
	const LLVector3d&           getCurrentParcelPos() const;
	bool                        hasCurrentParcel() const	{ return getCurrentParcelId().notNull(); }
	void                        setRefreshOnCategoryChange(bool refresh) { m_fRefreshOnCategory = refresh; }
	void                        setRefreshOnMaturityToggle(bool refresh) { m_fRefreshOnMaturity = refresh; }
	boost::signals2::connection setSelectCallback(const commit_signal_t::slot_type& cb);

	void searchClear();
	void searchStart(std::string strQuery, bool fQuiet = false);
	void searchPrevious();
	void searchNext();

protected:
	void onResultSelect();
	void onSearchBtn();
	void onSearchResult(const LLUUID& idQuery, U32 nStatus, const LLSearchDirectory::places_results_vec_t& lResults);
	void onSelectCategory();
	void onShowOnMapBtn();
	void onTeleportBtn();
	void onToggleMaturity();
	void performSearch();
	void updateButtons();

	/*
	 * Member variables
	 */
protected:
	bool        m_fRefreshOnCategory;
	bool        m_fRefreshOnMaturity;

	std::string m_strCurQuery;
	LLUUID      m_idCurQuery;
	U32         m_nCurIndex;
	U32         m_nCurResults;

	LLComboBox*        m_pSearchCategory;
	LLCheckBoxCtrl*    m_pSearchPG;
	LLCheckBoxCtrl*    m_pSearchMature;
	LLCheckBoxCtrl*    m_pSearchAdult;
	LLPanelParcelInfo* m_pParcelInfo;
	LLScrollListCtrl*  m_pResultsList;
	LLTextBox*         m_pResultsCount;
	LLButton*          m_pResultsPrevious;
	LLButton*          m_pResultsNext;
};

// ============================================================================

#endif //LL_LLPANELPLACESSEARCH_H
