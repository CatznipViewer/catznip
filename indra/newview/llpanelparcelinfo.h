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
#ifndef LL_LLPANELPARCELINFO_H
#define LL_LLPANELPARCELINFO_H

#include "llpanel.h"
#include "llremoteparcelrequest.h"

class LLIconCtrl;
class LLLandmark;
class LLLineEditor;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;

class LLPanelParcelInfo : public LLPanel, public LLRemoteParcelInfoObserver
{
public:
	LLPanelParcelInfo();
	/*virtual*/ ~LLPanelParcelInfo();

	/*
	 * LLView overrides
	 */
public:
	/*virtual*/ BOOL postBuild();

	/*
	 * LLRemoteParcelInfoObserver overrides
	 */
protected:
	/*virtual*/ void setParcelID(const LLUUID& idParcel);
	/*virtual*/ void processParcelInfo(const LLParcelData& parcelData);
	/*virtual*/ void setErrorStatus(U32 nStatus, const std::string& strReason);

	/*
	 * Member functions
	 */
public:
	void                clearLocation();
	const LLUUID&       getCurrentParcelId() const		{ return m_idCurParcel; }
	bool                getEditMode() const				{ return m_fEditMode; }
	const std::string&  getEditName() const;
	const std::string   getEditDescription() const;
	void                setEditMode(bool fEditMode);
	const LLVector3d&   getGlobalPos() const			{ return (!m_posCurGlobal.isExactlyZero()) ? m_posCurGlobal : m_posGlobalRequest; }
	const LLUUID&       getItemId() const				{ return m_idItem; }
	bool                isLandmark() const				{ return m_idItem.notNull(); }
	void                setParcelFromPos(const LLVector3d posGlobal);
	void                setParcelFromId(const LLUUID& idParcel);
	void                setParcelFromItem(const LLUUID& idItem);
protected:
	void clearControls(const std::string& strGeneral, const std::string& strDescription);
	void clearPendingRequest();
	void requestRemoteParcel(const LLSD& sdBody);
	void requestRemoteParcel(const LLVector3d& posGlobal, const LLUUID& idRegion = LLUUID::null);
	void requestRemoteParcel(const LLUUID& idRegion, const LLVector3& posRegion);
	void updateFromInventoryItem();
	void updateFromParcelData();
	static void onLandmarkLoaded(LLLandmark* pLandmark, LLHandle<LLPanel> hPanel);

	/*
	 * Member variables
	 */
protected:
	// Request tracking
	enum ERequestType { REQUEST_NONE, REQUEST_PARCEL_ID, REQUEST_PARCEL_INFO, REQUEST_LANDMARK };
	ERequestType m_eRequestType;
	LLVector3d   m_posGlobalRequest;
	LLVector3    m_posRegionRequest;

	// Parcel information
	LLUUID       m_idItem;
	LLUUID       m_idCurParcel;
	LLVector3d   m_posCurGlobal;
	LLParcelData m_CurParcelData;

	// Controls
	bool           m_fEditMode;
	LLTextureCtrl* m_pParcelSnapshot;
	LLTextBox*     m_pParcelName;
	LLLineEditor*  m_pParcelNameEdit;
	LLIconCtrl*    m_pRegionMaturityIcon;
	LLTextBox*     m_pParcelLocation;
	LLTextBox*     m_pParcelNumbers;
	LLTextEditor*  m_pParcelDescription;
	LLTextEditor*  m_pParcelDescriptionEdit;
};

#endif // LL_LLPANELPARCELINFO_H
