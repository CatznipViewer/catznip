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

#ifndef LL_LLPANELIMCONTROLPANEL_H
#define LL_LLPANELIMCONTROLPANEL_H

#include "llavatarpropertiesprocessor.h"
#include "llpanel.h"

// ============================================================================
// LLPanelIMControlPanel class
//

class LLPanelIMControlPanel : public LLPanel, public LLAvatarPropertiesObserver
{
	/*
	 * Constructor
	 */
public:
	LLPanelIMControlPanel(const LLUUID& idAvatar);
	virtual ~LLPanelIMControlPanel();

	/*
	 * Member functions
	 */
protected:
	void refreshFromProperties();

	/*
	 * LLPanel overrides
	 */
public:
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void handleVisibilityChange(BOOL fVisible);

	/*
	 * LLAvatarPropertiesObserver overrides
	 */
public:
	/*virtual*/ void processProperties(void* pData, EAvatarProcessorType eType);

	/*
	 * Member variables
	 */
protected:
	LLUUID mAvatarId;
	bool   mRequestSent;
};

// ============================================================================

#endif // LL_LLPANELIMCONTROLPANEL_H
