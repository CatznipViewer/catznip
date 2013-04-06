/** 
 *
 * Copyright (c) 2012, Kitty Barnett
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

#include "llfloaterreg.h"
#include "llfloatersearch.h"
#include "llfloatersearchcontainer.h"

// ============================================================================
// LLFloaterSearchContainer class
//

LLFloaterSearchContainer::LLFloaterSearchContainer(const LLSD& sdKey)
	: LLMultiFloater(sdKey)
	, m_pWebSearch(NULL)
	, m_pPlacesSearch(NULL)
{
	mAutoResize = true;
	mCloseFloaters = false;
}

LLFloaterSearchContainer::~LLFloaterSearchContainer()
{
}

BOOL LLFloaterSearchContainer::postBuild()
{
	mTabContainer = getChild<LLTabContainer>("search_container");

	m_pWebSearch = LLFloaterReg::getTypedInstance<LLFloaterSearch>("search_web");
	addFloater(m_pWebSearch, true);

	m_pPlacesSearch = LLFloaterReg::getTypedInstance<LLFloater>("search_places");
	addFloater(m_pPlacesSearch, false);

	return LLMultiFloater::postBuild();
}

void LLFloaterSearchContainer::onOpen(const LLSD& sdKey)
{
	LLMultiFloater::onOpen(LLSD());

	if (sdKey.has("category"))
	{
		// This is meant for the web search tab
		S32 idxTab = mTabContainer->getIndexForPanel(m_pWebSearch);
		if (-1 != idxTab)
		{
			mTabContainer->selectTab(idxTab);
			m_pWebSearch->onOpen(sdKey);
		}
	}
	else
	{
		// TODO-Catznip: should we call onOpen only for the active tab, or all tabs?
		// (Right now we're using it to initialize LLFloaterSearch's first navigation when search is opened)
		mTabContainer->getCurrentPanel()->onOpen(sdKey);
	}
}

void LLFloaterSearchContainer::addFloater(LLFloater* pFloater, BOOL fSelect, LLTabContainer::eInsertionPoint eInsert)
{
	if (pFloater)
	{
		pFloater->setTitleVisible(false);
		pFloater->setButtonsVisible(false);

		LLRect rctFloater = pFloater->getRect();
		rctFloater.mTop -= pFloater->getHeaderHeight();
		pFloater->setRect(rctFloater);

		LLMultiFloater::addFloater(pFloater, fSelect, eInsert);
	}
}

// ============================================================================
