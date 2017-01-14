/**
 *
 * Copyright (c) 2017, Kitty Barnett
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

#pragma once

#include "llsearcheditor.h"

// ====================================================================================
// Forward declarations
//

class LLAvatarName;

// ====================================================================================
// LLAvatarEditor class
//

class LLAvatarEditor : public LLSearchEditor
{
	friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLSearchEditor::Params> {};
	LLAvatarEditor(const Params& p);
	~LLAvatarEditor();

	/*
	 * Base class overrides
	 */
public:
	LLSD getValue() const override;
	void setValue(const LLSD& sdValue) override;
protected:
	void onClearButtonClick(const LLSD& sdData) override;
	void onSearchButtonClick() override;

	/*
	 * Member functions
	 */
public:
	const LLUUID& getAvatarId()	const	{ return m_idAvatar; }
	void          setAvatarId(const LLUUID& idAvatar);
protected:
	void onAvatarPick(const uuid_vec_t& idAvatars, const std::vector<LLAvatarName>& nameAvatars);
	void onNameCallback(const LLUUID& idAvatar, const LLAvatarName& avName);

	/*
	 * Member variables
	 */
protected:
	LLUUID m_idAvatar;
	boost::signals2::connection m_AvatarNameConnection;
};

// ============================================================================
