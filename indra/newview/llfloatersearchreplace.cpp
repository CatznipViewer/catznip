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

LLFloaterSearchReplace* LLFloaterSearchReplace::sInstance = NULL;

LLFloaterSearchReplace::LLFloaterSearchReplace(LLTextEditor* editor)
:	LLFloater(LLSD()),
	mEditor(editor)
{
	buildFromFile("floater_script_search.xml");

	sInstance = this;
	
	// find floater in which editor is embedded
	LLView* viewp = (LLView*)editor;
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

BOOL LLFloaterSearchReplace::postBuild()
{
	childSetAction("search_btn", onBtnSearch,this);
	childSetAction("replace_btn", onBtnReplace,this);
	childSetAction("replace_all_btn", onBtnReplaceAll,this);

	setDefaultBtn("search_btn");

	return TRUE;
}

//static 
void LLFloaterSearchReplace::show(LLTextEditor* editor)
{
	if (sInstance && sInstance->mEditor && sInstance->mEditor != editor)
	{
		sInstance->closeFloater();
		delete sInstance;
	}

	if (!sInstance)
	{
		// sInstance will be assigned in the constructor.
		new LLFloaterSearchReplace(editor);
	}

	sInstance->openFloater();
}

LLFloaterSearchReplace::~LLFloaterSearchReplace()
{
	sInstance = NULL;
}

// static 
void LLFloaterSearchReplace::onBtnSearch(void *userdata)
{
	LLFloaterSearchReplace* self = (LLFloaterSearchReplace*)userdata;
	self->handleBtnSearch();
}

void LLFloaterSearchReplace::handleBtnSearch()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditor->selectNext(getChild<LLUICtrl>("search_text")->getValue().asString(), caseChk->get());
}

// static 
void LLFloaterSearchReplace::onBtnReplace(void *userdata)
{
	LLFloaterSearchReplace* self = (LLFloaterSearchReplace*)userdata;
	self->handleBtnReplace();
}

void LLFloaterSearchReplace::handleBtnReplace()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditor->replaceText(getChild<LLUICtrl>("search_text")->getValue().asString(), getChild<LLUICtrl>("replace_text")->getValue().asString(), caseChk->get());
}

// static 
void LLFloaterSearchReplace::onBtnReplaceAll(void *userdata)
{
	LLFloaterSearchReplace* self = (LLFloaterSearchReplace*)userdata;
	self->handleBtnReplaceAll();
}

void LLFloaterSearchReplace::handleBtnReplaceAll()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditor->replaceTextAll(getChild<LLUICtrl>("search_text")->getValue().asString(), getChild<LLUICtrl>("replace_text")->getValue().asString(), caseChk->get());
}

bool LLFloaterSearchReplace::hasAccelerators() const
{
	// Pass this on to the editor we're operating on (or any view up along its hierarchy 
	// (allows Ctrl-F to work when the floater itself has focus - see changeset 0c8947e5f433)
	const LLView* pView = (LLView*)mEditor;
	while (pView)
	{
		if (pView->hasAccelerators())
			return true;
		pView = pView->getParent();
	}
	return false;
}

BOOL LLFloaterSearchReplace::handleKeyHere(KEY key, MASK mask)
{
	// Pass this on to the editor we're operating on (or any view up along its hierarchy 
	// (allows Ctrl-F to work when the floater itself has focus - see changeset 0c8947e5f433)
	LLView* pView = (LLView*)mEditor;
	while (pView)
	{
		if (pView->hasAccelerators())
			return pView->handleKeyHere(key, mask);
		pView = pView->getParent();
	}
	return FALSE;
}
