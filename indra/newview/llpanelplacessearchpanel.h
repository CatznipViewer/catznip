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
#ifndef LLPANELPLACESSEARCHPANEL_H
#define LLPANELPLACESSEARCHPANEL_H

#include "llpanelplacestab.h"

class LLPanelPlacesSearch;

// ============================================================================

class LLPanelPlacesSearchPanel : public LLPanelPlacesTab
{
public:
	LLPanelPlacesSearchPanel();
	virtual ~LLPanelPlacesSearchPanel();

	/*
	 * LLView overrides
	 */
public:
	/*virtual*/ BOOL postBuild();

	/*
	 * LLPanelPlacesTab overrides
	 */
public:
	/*virtual*/ bool isSingleItemSelected();
	/*virtual*/ void onSearchEdit(const std::string& strQuery);
	/*virtual*/ void onShowOnMap();
	/*virtual*/ void onShowProfile();
	/*virtual*/ void onTeleport();
	/*virtual*/ void updateVerbs();

	/*
	 * Member functions
	 */
protected:
	void onSearchResultSelect();

	/*
	 * Member variables
	 */
protected:
	LLPanelPlacesSearch* m_pSearchPanel;
};

// ============================================================================

#endif // LLPANELPLACESSEARCHPANEL_H
