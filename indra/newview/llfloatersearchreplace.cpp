/** 
 * @file llfloatersearchreplace.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersearchreplace.h"

#include "llcheckboxctrl.h"
#include "llpreviewscript.h"
#include "lltexteditor.h"

LLFloaterScriptSearch* LLFloaterScriptSearch::sInstance = NULL;

LLFloaterScriptSearch::LLFloaterScriptSearch(LLScriptEdCore* editor_core)
:	LLFloater(LLSD()),
	mEditorCore(editor_core)
{
	buildFromFile("floater_script_search.xml");

	sInstance = this;
	
	// find floater in which script panel is embedded
	LLView* viewp = (LLView*)editor_core;
	while(viewp)
	{
		LLFloater* floaterp = dynamic_cast<LLFloater*>(viewp);
		if (floaterp)
		{
			floaterp->addDependentFloater(this);
			break;
		}
		viewp = viewp->getParent();
	}
}

BOOL LLFloaterScriptSearch::postBuild()
{
	childSetAction("search_btn", onBtnSearch,this);
	childSetAction("replace_btn", onBtnReplace,this);
	childSetAction("replace_all_btn", onBtnReplaceAll,this);

	setDefaultBtn("search_btn");

	return TRUE;
}

//static 
void LLFloaterScriptSearch::show(LLScriptEdCore* editor_core)
{
	if (sInstance && sInstance->mEditorCore && sInstance->mEditorCore != editor_core)
	{
		sInstance->closeFloater();
		delete sInstance;
	}

	if (!sInstance)
	{
		// sInstance will be assigned in the constructor.
		new LLFloaterScriptSearch(editor_core);
	}

	sInstance->openFloater();
}

LLFloaterScriptSearch::~LLFloaterScriptSearch()
{
	sInstance = NULL;
}

// static 
void LLFloaterScriptSearch::onBtnSearch(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnSearch();
}

void LLFloaterScriptSearch::handleBtnSearch()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->selectNext(getChild<LLUICtrl>("search_text")->getValue().asString(), caseChk->get());
}

// static 
void LLFloaterScriptSearch::onBtnReplace(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplace();
}

void LLFloaterScriptSearch::handleBtnReplace()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->replaceText(getChild<LLUICtrl>("search_text")->getValue().asString(), getChild<LLUICtrl>("replace_text")->getValue().asString(), caseChk->get());
}

// static 
void LLFloaterScriptSearch::onBtnReplaceAll(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplaceAll();
}

void LLFloaterScriptSearch::handleBtnReplaceAll()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->replaceTextAll(getChild<LLUICtrl>("search_text")->getValue().asString(), getChild<LLUICtrl>("replace_text")->getValue().asString(), caseChk->get());
}

bool LLFloaterScriptSearch::hasAccelerators() const
{
	if (mEditorCore)
	{
		return mEditorCore->hasAccelerators();
	}
	return FALSE;
}

BOOL LLFloaterScriptSearch::handleKeyHere(KEY key, MASK mask)
{
	if (mEditorCore)
	{
		return mEditorCore->handleKeyHere(key, mask);
	}

	return FALSE;
}
