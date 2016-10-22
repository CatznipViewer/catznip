/** 
 *
 * Copyright (c) 2016, Kitty Barnett
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

#ifndef LL_LLFLOATERFEEDBACK_H
#define LL_LLFLOATERFEEDBACK_H

#include "llfloater.h"

// =========================================================================
// Forward declarations
//

class LLMediaCtrl;

// =========================================================================
// LLFloaterFeedback class
//

class LLFloaterFeedback : public LLFloater
{
	friend class LLFloaterReg;
	/*
	 * Constructor
	 */
private:
	LLFloaterFeedback(const LLSD& sdKey);
	/*virtual*/ ~LLFloaterFeedback();

	/*
	 * Base class overrides
	 */
public:
	BOOL postBuild();
	
	/*
	 * Member variables
	 */
protected:
	LLMediaCtrl* m_pWebBrowser;
};

// =========================================================================

#endif // LL_LLFLOATERFEEDBACK_H
