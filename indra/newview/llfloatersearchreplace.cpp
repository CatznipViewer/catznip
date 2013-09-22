/** 
 *
 * Copyright (c) 2010-2013, Kitty Barnett
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

#include "llviewerprecompiledheaders.h"

#include "llcheckboxctrl.h"
#include "llfloaterreg.h"
#include "llfloatersearchreplace.h"
#include "lllineeditor.h"
#include "llmultifloater.h"
#include "lltexteditor.h"

LLFloaterSearchReplace::LLFloaterSearchReplace(const LLSD& sdKey)
	: LLFloater(sdKey)
	, mSearchEditor(NULL)
	, mReplaceEditor(NULL)
	, mCaseInsensitiveCheck(NULL)
	, mSearchUpCheck(NULL)
{
}

LLFloaterSearchReplace::~LLFloaterSearchReplace()
{
}

//static 
void LLFloaterSearchReplace::show(LLTextEditor* pEditor)
{
	LLFloaterSearchReplace* pSelf = LLFloaterReg::getTypedInstance<LLFloaterSearchReplace>("search_replace");
	if ( (!pSelf) || (!pEditor) )
		return;

	pSelf->mEditorHandle = pEditor->getHandle();
	if (pEditor)
	{
		LLFloater *pDependeeNew = NULL, *pDependeeOld = pSelf->getDependee();
		LLView* pView = pEditor->getParent();
		while (pView)
		{
			pDependeeNew = dynamic_cast<LLFloater*>(pView);
			if (pDependeeNew)
			{
				if (pDependeeNew != pDependeeOld)
				{
					if (pDependeeOld)
						pDependeeOld->removeDependentFloater(pSelf);

					if (!pDependeeNew->getHost())
						pDependeeNew->addDependentFloater(pSelf);
					else
						pDependeeNew->getHost()->addDependentFloater(pSelf);
				}
				break;
			}
			pView = pView->getParent();
		}

		pSelf->getChildView("replace_text")->setEnabled(!pEditor->getReadOnly());
		pSelf->getChildView("replace_btn")->setEnabled(!pEditor->getReadOnly());
		pSelf->getChildView("replace_all_btn")->setEnabled(!pEditor->getReadOnly());

		pSelf->openFloater();
		pSelf->mSearchEditor->setFocus(TRUE);
	}
}

BOOL LLFloaterSearchReplace::postBuild()
{
	childSetAction("search_btn", boost::bind(&LLFloaterSearchReplace::onBtnSearch, this));
	childSetAction("replace_btn", boost::bind(&LLFloaterSearchReplace::onBtnReplace, this));
	childSetAction("replace_all_btn", boost::bind(&LLFloaterSearchReplace::onBtnReplaceAll, this));

	setDefaultBtn("search_btn");

	mSearchEditor = getChild<LLLineEditor>("search_text");
	mSearchEditor->setCommitCallback(boost::bind(&LLFloaterSearchReplace::onBtnSearch, this));
	mSearchEditor->setCommitOnFocusLost(false);
	mReplaceEditor = getChild<LLLineEditor>("replace_text");
	mCaseInsensitiveCheck = getChild<LLCheckBoxCtrl>("case_text");
	mSearchUpCheck = getChild<LLCheckBoxCtrl>("find_previous");

	return TRUE;
}

void LLFloaterSearchReplace::onBtnSearch()
{
	LLTextEditor* pEditor = dynamic_cast<LLTextEditor*>(mEditorHandle.get());
	if (pEditor)
	{
		pEditor->selectNext(mSearchEditor->getText(), mCaseInsensitiveCheck->get(), TRUE, mSearchUpCheck->get());
	}
}

void LLFloaterSearchReplace::onBtnReplace()
{
	LLTextEditor* pEditor = dynamic_cast<LLTextEditor*>(mEditorHandle.get());
	if (pEditor)
	{
		pEditor->replaceText(mSearchEditor->getText(), mReplaceEditor->getText(), mCaseInsensitiveCheck->get(), TRUE, mSearchUpCheck->get());
	}
}

void LLFloaterSearchReplace::onBtnReplaceAll()
{
	LLTextEditor* pEditor = dynamic_cast<LLTextEditor*>(mEditorHandle.get());
	if (pEditor)
	{
		pEditor->replaceTextAll(mSearchEditor->getText(), mReplaceEditor->getText(), mCaseInsensitiveCheck->get());
	}
}

bool LLFloaterSearchReplace::hasAccelerators() const
{
	const LLView* pView = dynamic_cast<LLTextEditor*>(mEditorHandle.get());
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
	// Pass this on to the editor we're operating on (or any view up along its hierarchy) if we don't handle the key ourselves 
	// (allows Ctrl-F to work when the floater itself has focus - see changeset 0c8947e5f433)
	if (!LLFloater::handleKeyHere(key, mask))
	{
		LLView* pView = mEditorHandle.get();
		while (pView)
		{
			if ( (pView->hasAccelerators()) && (pView->handleKeyHere(key, mask)) )
				return TRUE;
			pView = pView->getParent();
		}
	}
	return FALSE;
}
