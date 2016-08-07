/** 
 *
 * Copyright (c) 2014, Kitty Barnett
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

#ifndef LL_PANELEDITORINLINESEARCH_H
#define LL_PANELEDITORINLINESEARCH_H

#include "llpanel.h"

// ============================================================================
// Forward declarations
//

class LLLineEditor;
class LLTextEditor;

// ============================================================================
// LLTextSearchCtrl class
//

class LLTextSearchCtrl : public LLPanel
{
	/*
	 * Constructor
	 */
public:
	LLTextSearchCtrl(LLTextEditor* pEditor);
	virtual ~LLTextSearchCtrl();

	/*
	 * Base class overrides
	 */
public:
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setVisible(BOOL fVisible);
	/*virtual*/ LLSD getValue() const;
	/*virtual*/ void setValue(const LLSD& sdValue);

	/*
	 * Event handlers
	 */
protected:
	void onRefreshHighlight();
	void onSearchClicked(bool fSearchDown);

	/*
	 * Member variables
	 */
protected:
	LLLineEditor* m_pSearchEditor;
	boost::signals2::scoped_connection m_CaseInsensitiveConn;
};

// ============================================================================

#endif // LL_PANELEDITORINLINESEARCH_H
