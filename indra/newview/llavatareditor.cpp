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

#include "llviewerprecompiledheaders.h"

#include "llavatarnamecache.h"
#include "llfloateravatarpicker.h"

#include "llavatareditor.h"

// ====================================================================================
// LLAvatarEditor class
//

static LLDefaultChildRegistry::Register<LLAvatarEditor> register_avatar_editor("avatar_editor");

LLAvatarEditor::LLAvatarEditor(const LLAvatarEditor::Params& p)
	: LLSearchEditor(p)
{
	// Reset the left text padding since we're moving the search button out of the way
	S32 nTextPadRight = 0;
	mSearchEditor->getTextPadding(nullptr, &nTextPadRight);
	mSearchEditor->setTextPadding(p.text_pad_left, nTextPadRight);

	// Disable the text editor since we don't want anyone to type into it (instead left-clicks will trigger the search button)
	mSearchEditor->setEnabled(false);
	mSearchEditor->setFocusReceivedCallback(boost::bind(&LLAvatarEditor::onSearchButtonClick, this));

	// Move the search button so it covers the clear button
	mSearchButton->translate(mClearButton->getRect().mRight - mSearchButton->getRect().mLeft - mSearchButton->getRect().getWidth() - p.search_button.left_pad, 1);
}

LLAvatarEditor::~LLAvatarEditor()
{
	if (m_AvatarNameConnection.connected())
		m_AvatarNameConnection.disconnect();
}

// ====================================================================================
// LLAvatarEditor base class overrides
//

// virtual
LLSD LLAvatarEditor::getValue() const
{
	return LLSD(getAvatarId());
}

// virtual
void LLAvatarEditor::setValue(const LLSD& sdValue)
{
	setAvatarId(sdValue.asUUID());
}

// virtual
void LLAvatarEditor::onClearButtonClick(const LLSD& sdData)
{
	setAvatarId(LLUUID::null);
	onCommit();
}

// virtual
void LLAvatarEditor::onSearchButtonClick()
{
	// Clear focus so the next click will trigger focus received
	if (mSearchEditor->hasFocus())
		mSearchEditor->setFocus(false);

	// Assume we'll always be owned by a floater
	LLFloater* pParentFloater = getParentByType<LLFloater>();
	LLFloater* pPickerFloater = LLFloaterAvatarPicker::show(boost::bind(&LLAvatarEditor::onAvatarPick, this, _1, _2), FALSE, TRUE);
	if ( (pParentFloater) && (pPickerFloater) )
		pParentFloater->addDependentFloater(pPickerFloater);
}

// ====================================================================================
// LLAvatarEditor member functions
//

void LLAvatarEditor::setAvatarId(const LLUUID& idAvatar)
{
	m_idAvatar = idAvatar;

	LLSearchEditor::setValue(LLSD());
	mClearButton->setVisible(false);
	mSearchButton->setVisible(m_idAvatar.isNull());
	if (m_idAvatar.notNull())
	{
		if (m_AvatarNameConnection.connected())
			m_AvatarNameConnection.disconnect();
		m_AvatarNameConnection = LLAvatarNameCache::get(m_idAvatar, boost::bind(&LLAvatarEditor::onNameCallback, this, _1, _2));
	}
}

void LLAvatarEditor::onAvatarPick(const uuid_vec_t& idAvatars, const std::vector<LLAvatarName>& nameAvatars)
{
	if (!idAvatars.empty())
	{
		m_idAvatar = idAvatars.front();
		mSearchEditor->setValue(nameAvatars.front().getCompleteName());
		mClearButton->setVisible(true);
		mSearchButton->setVisible(false);
		onCommit();
	}
}

void LLAvatarEditor::onNameCallback(const LLUUID& idAvatar, const LLAvatarName& avName)
{
	if (m_idAvatar == idAvatar)
	{
		mSearchEditor->setValue(avName.getCompleteName());
	}
	mClearButton->setVisible(!mSearchEditor->getWText().empty());
	mSearchButton->setVisible(!mClearButton->getVisible());
	m_AvatarNameConnection.disconnect();
}

// ============================================================================
