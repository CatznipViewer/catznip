/** 
 *
 * Copyright (c) 2013, Kitty Barnett
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
#include "llfloaterownedobjects.h"
#include "lllineeditor.h"
#include "llpathfindinglinksetlist.h"
#include "llspinctrl.h"
#include "llviewerparcelmgr.h"

#include <boost/algorithm/string.hpp>

// ============================================================================
// LLFloaterOwnedObjects member functions
//

LLFloaterOwnedObjects::LLFloaterOwnedObjects(const LLSD& sdSeed)
	: LLFloaterPathfindingObjects(sdSeed)
	, m_pLimitAgentParcel(NULL)
	, m_pFilterObjects(NULL)
	, m_pNameFilter(NULL)
	, m_pDescrFilter(NULL)
	, m_pFilterHeight(NULL)
	, m_pMinHeightFilter(NULL)
	, m_pMaxHeightFilter(NULL)
{
}

LLFloaterOwnedObjects::~LLFloaterOwnedObjects()
{
}

BOOL LLFloaterOwnedObjects::postBuild()
{
	m_pLimitAgentParcel = findChild<LLCheckBoxCtrl>("show_agent_parcel");
	m_pFilterObjects = findChild<LLCheckBoxCtrl>("filter_objects");
	m_pFilterObjects->setCommitCallback(boost::bind(&LLFloaterOwnedObjects::onToggleFilter, this));
	m_pNameFilter = findChild<LLLineEditor>("filter_by_name");
	m_pNameFilter->setKeystrokeCallback(boost::bind(&LLFloaterOwnedObjects::refreshFilterButtons, this), NULL);
	m_pDescrFilter = findChild<LLLineEditor>("filter_by_description");
	m_pDescrFilter->setKeystrokeCallback(boost::bind(&LLFloaterOwnedObjects::refreshFilterButtons, this), NULL);
	m_pFilterHeight = findChild<LLCheckBoxCtrl>("filter_height");
	m_pFilterHeight->setCommitCallback(boost::bind(&LLFloaterOwnedObjects::refreshFilterButtons, this));
	m_pMinHeightFilter = findChild<LLSpinCtrl>("filter_height_min");
	m_pMinHeightFilter->setCommitCallback(boost::bind(&LLFloaterOwnedObjects::onMinHeightFilterChanged, this));
	m_pMaxHeightFilter = findChild<LLSpinCtrl>("filter_height_max");
	m_pMinHeightFilter->setCommitCallback(boost::bind(&LLFloaterOwnedObjects::refreshFilterButtons, this));

	findChild<LLUICtrl>("apply_filters")->setCommitCallback(boost::bind(&LLFloaterOwnedObjects::onApplyFilter, this));
	findChild<LLUICtrl>("clear_filters")->setCommitCallback(boost::bind(&LLFloaterOwnedObjects::onClearFilter, this));

	return LLFloaterPathfindingObjects::postBuild();
}

void LLFloaterOwnedObjects::requestGetObjects()
{
	LLPathfindingManager::getInstance()->requestGetLinksets(getNewRequestId(), boost::bind(&LLFloaterOwnedObjects::handleNewObjectList, this, _1, _2, _3));
}

void LLFloaterOwnedObjects::buildObjectsScrollList(const LLPathfindingObjectListPtr pObjectList)
{
	bool fFilterAgentParcel = m_pLimitAgentParcel->get();
	bool fFilterResults = m_pFilterObjects->get();
	const std::string& strNameFilter = (fFilterResults) ? m_pNameFilter->getText() : LLStringUtil::null;
	const std::string& strDescrFilter = (fFilterResults) ? m_pDescrFilter->getText() : LLStringUtil::null;
	bool fFilterHeight = m_pFilterHeight->get();
	F32 nMinHeight = (fFilterHeight) ? m_pMinHeightFilter->get() : 0.0f;
	F32 nMaxHeight = (fFilterHeight) ? m_pMaxHeightFilter->get() : 0.0f;

	const LLVector3& posAgent = gAgent.getPositionAgent();

	for (LLPathfindingObjectList::const_iterator itObj = pObjectList->begin(); itObj != pObjectList->end(); ++itObj)
	{
		const LLPathfindingObjectPtr pObj = itObj->second;

		const LLPathfindingLinkset* pLinkset = dynamic_cast<const LLPathfindingLinkset*>(pObj.get());
		llassert(pLinkset != NULL);
		
		if ( (pLinkset->isTerrain()) || (pLinkset->getOwnerID() != gAgentID) || 
		     ((fFilterAgentParcel) && (!LLViewerParcelMgr::getInstance()->inAgentParcel(pLinkset->getLocation()))) )
		{
			continue;
		}

		if ( ((!strNameFilter.empty()) && (!boost::icontains(pLinkset->getName(), strNameFilter))) ||
		     ((!strDescrFilter.empty()) && (!boost::icontains(pLinkset->getDescription(), strDescrFilter))) )
		{
			continue;
		}

		F32 nHeight = pLinkset->getLocation()[VZ];
		if ( (fFilterHeight) && ((nHeight < nMinHeight) || (nHeight > nMaxHeight)) )
		{
			continue;
		}

		LLSD sdRow = LLSD::emptyArray();

		// Name
		sdRow[0]["column"] = "name";
		sdRow[0]["value"] = pLinkset->getName();

		// Description
		sdRow[1]["column"] = "description";
		sdRow[1]["value"] = pLinkset->getDescription();

		// Land Impact
		sdRow[2]["column"] = "land_impact";
		sdRow[2]["value"] = llformat("%1d", pLinkset->getLandImpact());

		// Distance
		sdRow[3]["column"] = "dist_from_you";
		sdRow[3]["value"] = llformat("%1.0f m", dist_vec(posAgent, pLinkset->getLocation()));

		addObjectToScrollList(pObj, sdRow);
	}

	// Make all filter controls pristine again
	m_pNameFilter->resetDirty();
	m_pDescrFilter->resetDirty();
}

LLPathfindingObjectListPtr LLFloaterOwnedObjects::getEmptyObjectList() const
{
	LLPathfindingObjectListPtr objectListPtr(new LLPathfindingLinksetList());
	return objectListPtr;
}

S32 LLFloaterOwnedObjects::getNameColumnIndex() const
{
	return 0;
}

S32 LLFloaterOwnedObjects::getOwnerNameColumnIndex() const
{
	// We're not showing the object's owner name
	return -1;
}

void LLFloaterOwnedObjects::onMinHeightFilterChanged()
{
	m_pMaxHeightFilter->setMinValue(m_pMinHeightFilter->getValueF32() + 1);
	if (m_pMaxHeightFilter->getValueF32() < m_pMaxHeightFilter->getMinValue())
	{
		m_pMaxHeightFilter->setValue(m_pMaxHeightFilter->getMinValue());
	}
}

void LLFloaterOwnedObjects::onApplyFilter()
{
	refreshFilterButtons();

	requestGetObjects();
}

void LLFloaterOwnedObjects::onClearFilter()
{
	m_pNameFilter->clear();
	m_pDescrFilter->clear();
	m_pFilterHeight->set(false);
	m_pMinHeightFilter->set(0.0f);
	m_pMaxHeightFilter->set(0.0f);
	refreshFilterButtons();

	requestGetObjects();
}

void LLFloaterOwnedObjects::onToggleFilter()
{
	bool fFilter = m_pFilterObjects->get();

	LLUICtrl* pFilterPanel = findChild<LLUICtrl>("filter_panel");
	if (pFilterPanel)
	{
		const child_list_t* pChildren = pFilterPanel->getChildList();
		for (child_list_t::const_iterator itChild = pChildren->begin(); itChild != pChildren->end(); ++itChild)
		{
			(*itChild)->setEnabled(fFilter);
		}
	}

	if (fFilter)
	{
		refreshFilterButtons();
	}

	requestGetObjects();
}

void LLFloaterOwnedObjects::refreshFilterButtons()
{
	bool fFilter = m_pFilterObjects->get();
	bool fDirty = 
		m_pNameFilter->isDirty() || m_pDescrFilter->isDirty() ||
		m_pMinHeightFilter->isDirty() || m_pMaxHeightFilter->isDirty();
	bool fHasFilter = 
		!m_pNameFilter->getText().empty() || !m_pDescrFilter->getText().empty() || 
		m_pMinHeightFilter->getValueF32() > 0.f || m_pMaxHeightFilter->getValueF32() > 0.f;

	findChild<LLUICtrl>("apply_filters")->setEnabled(fFilter && fDirty);
	findChild<LLUICtrl>("clear_filters")->setEnabled(fFilter && fHasFilter);
}
