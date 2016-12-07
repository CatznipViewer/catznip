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

#pragma once

#include "llfloater.h"
#include "llviewermediaobserver.h"

// =========================================================================
// Forward declarations
//

class LLMediaCtrl;

// =========================================================================
// LLFloaterFeedback class
//

class LLFloaterFeedback : public LLFloater, public LLViewerMediaObserver
{
	friend class LLFloaterReg;

	/*
	 * Constructor
	 */
private:
	LLFloaterFeedback(const LLSD& sdKey);
	~LLFloaterFeedback() override;

	/*
	 * Base class overrides
	 */
public:
	BOOL postBuild() override;
	void handleMediaEvent(LLPluginClassMedia* pSelf, EMediaEvent eEvent) override;

	/*
	 * Member variables
	 */
protected:
	LLMediaCtrl* m_pWebBrowser;
};

// =========================================================================
