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

// ============================================================================

class LLPanelPlacesSearch : public LLPanel
{
public:
	LLPanelPlacesSearch();
	/*virtual*/ ~LLPanelPlacesSearch();
	
	/*virtual*/ BOOL postBuild();

	/*
	 * Member functions
	 */
protected:
	void onResultSelect();
	void onSearchStart();
	void onSearchResult(const LLUUID& idQuery, U32 nStatus, const LLSearchDirectory::places_results_vec_t& lResults);
	void onToggleMaturity();

	/*
	 * Member variables
	 */
protected:
	LLUUID m_idCurQuery;
	S32    m_nCurIndex;

	LLLineEditor*      m_pSearchEditor;
	LLComboBox*        m_pSearchCategory;
	LLCheckBoxCtrl*    m_pSearchPG;
	LLCheckBoxCtrl*    m_pSearchMature;
	LLCheckBoxCtrl*    m_pSearchAdult;
	LLPanelParcelInfo* m_pParcelInfo;
	LLScrollListCtrl*  m_pResultsList;
};

// ============================================================================

#endif //LL_LLPANELPLACESSEARCH_H
