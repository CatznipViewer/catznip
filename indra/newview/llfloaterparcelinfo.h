/** 
 *
 * Copyright (c) 2012-2013, Kitty Barnett
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
#ifndef LL_LLFLOATERPARCELINFO_H
#define LL_LLFLOATERPARCELINFO_H

#include "llfloater.h"

class LLButton;
class LLPanelParcelInfo;

// ============================================================================

class LLFloaterParcelInfo : public LLFloater
{
	friend class LLFloaterReg;
private:
	LLFloaterParcelInfo(const LLSD& sdKey);
	/*virtual*/ ~LLFloaterParcelInfo();

	/*
	 * LLView overrides
	 */
public:
	/*virtual*/ void onOpen(const LLSD& sdKey);
	/*virtual*/ BOOL postBuild();

	/*
	 * Member functions
	 */
protected:
	const LLUUID getItemId() /*const*/;
	void         refreshControls();
	// Parcel/landmark information
	void         onClickShowOnMap();
	void         onClickTeleport();
	// Landmark editing
	void         onClickSave();
	void         onClickDiscard();

	/*
	 * Member variables
	 */
protected:
	LLPanelParcelInfo* m_pParcelInfo;
	LLButton*          m_pTeleportBtn;
	LLButton*          m_pMapBtn;
	LLButton*          m_pSaveBtn;
	LLButton*          m_pDiscardBtn;
};

// ============================================================================

#endif // LL_LLFLOATERPARCELINFO_H
