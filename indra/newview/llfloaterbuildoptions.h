/** 
 * @file llfloaterbuildoptions.h
 * @brief LLFloaterBuildOptions class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

/**
 * Panel for setting global object-editing options, specifically
 * grid size and spacing.
 */

#ifndef LL_LLFLOATERBUILDOPTIONS_H
#define LL_LLFLOATERBUILDOPTIONS_H

#include "llfloater.h"
#include "llselectmgr.h"

class LLComboBox;
class LLObjectSelection;

typedef LLSafeHandle<LLObjectSelection> LLObjectSelectionHandle;

class LLFloaterBuildOptions
	:	public LLFloater
{
public:

	virtual BOOL postBuild();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/	void onClose(bool app_quitting);

	void setGridMode(EGridMode mode);
	void updateGridMode();

private:

	friend class LLFloaterReg;

	LLFloaterBuildOptions(const LLSD& key);
	~LLFloaterBuildOptions();

	LLComboBox*	mComboGridMode;
	LLObjectSelectionHandle	mObjectSelection;
};

// [SL:KB] - Patch: Build-AxisAtRoot | Checked: 2011-12-06 (Catznip-3.2.0d) | Added: 3.2.0d
//
// LLFloaterBuildAxis
//

class LLFloaterBuildAxis : public LLFloater
{
	friend class LLFloaterReg;
protected:
	LLFloaterBuildAxis(const LLSD& sdKey);
public:
	/*virtual*/ ~LLFloaterBuildAxis();

public:
	/*virtual*/ void onOpen(const LLSD& sdKey);
	/*virtual*/ void onClose(bool fQuiting);
	/*virtual*/ BOOL postBuild();
	            void refresh();
protected:
	static void onAxisPosChanged(const LLSD& sdValue, U32 idxAxis);
	static void onAxisPosCenter();
	static void onAxisOffsetChanged(const LLSD& sdValue, U32 idxAxis);

protected:
	boost::signals2::connection m_AxisPosConn;
	boost::signals2::connection m_AxisOffsetConn;
};

// [/SL:KB]

#endif
