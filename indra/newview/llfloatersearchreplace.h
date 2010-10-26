/** 
 * @file llfloatersearchreplace.h
 * @brief LLFloaterSearchReplace class implementation
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

#ifndef LL_FLOATERSEARCHREPLACE_H
#define LL_FLOATERSEARCHREPLACE_H

#include "llfloater.h"

class LLTextEditor;

class LLFloaterSearchReplace : public LLFloater
{
public:
	LLFloaterSearchReplace(LLTextEditor* editor);
	~LLFloaterSearchReplace();

	/*virtual*/	BOOL	postBuild();
	static void show(LLTextEditor* editor_core);
	static void onBtnSearch(void* userdata);
	void handleBtnSearch();

	static void onBtnReplace(void* userdata);
	void handleBtnReplace();

	static void onBtnReplaceAll(void* userdata);
	void handleBtnReplaceAll();

	LLTextEditor* getEditor() { return mEditor; }
	static LLFloaterSearchReplace* getInstance() { return sInstance; }

	virtual bool hasAccelerators() const;
	virtual BOOL handleKeyHere(KEY key, MASK mask);

private:

	LLTextEditor* mEditor;

	static LLFloaterSearchReplace*	sInstance;
};

#endif // LL_FLOATERSEARCHREPLACE_H

