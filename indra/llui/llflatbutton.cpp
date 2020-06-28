/**
 *
 * Copyright (c) 2020, Kitty Barnett
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

#include "linden_common.h"

#define LLFLATBUTTON_CPP
#include "llflatbutton.h"
#include "lluictrlfactory.h"

// ============================================================================
// LLFlatButton class - WIP (Do not use)
//

//static LLDefaultChildRegistry::Register<LLFlatButton> r("flat_button");

// Compiler optimization, generate extern template
template class LLFlatButton* LLView::getChild<class LLFlatButton>(const std::string& name, BOOL recurse) const;

LLFlatButton::Params::Params()
	: bg_hover("bg_hover")
	, bg_selected("bg_selected")
	, image("image")
	, image_selected("image_selected")
	, label("label")
	, commit_callback("commit_callback")
{
}

LLFlatButton::LLFlatButton(const LLFlatButton::Params& p)
	: LLPanel(p)
{
	LLIconCtrl::Params icon_params = p.bg_hover;
	icon_params.rect.top = p.rect.height;
	icon_params.rect.width = p.rect.width;
	icon_params.rect.height = p.rect.height;
	m_pBgHoverIcon = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(m_pBgHoverIcon);

	icon_params = p.bg_selected;
	icon_params.rect.top = p.rect.height;
	icon_params.rect.width = p.rect.width;
	icon_params.rect.height = p.rect.height;
	m_pBgSelectIcon = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(m_pBgSelectIcon);

	icon_params = p.image;
	icon_params.rect.top = p.rect.height;
	icon_params.rect.width = p.rect.width;
	icon_params.rect.height = p.rect.height;
	m_pImage = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(m_pImage);

	icon_params = p.image_selected;
	icon_params.rect.top = p.rect.height;
	icon_params.rect.width = p.rect.width;
	icon_params.rect.height = p.rect.height;
	m_pImageSelected = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(m_pImageSelected);

	LLTextBox::Params text_params = p.label;
	if (text_params.initial_value.isProvided())
	{
		m_pLabel = LLUICtrlFactory::create<LLTextBox>(text_params);
		addChild(m_pLabel);
	}

	if (p.commit_callback.isProvided())
	{
		setCommitCallback(initCommitCallback(p.commit_callback));
	}
}

BOOL LLFlatButton::postBuild()
{
	setMouseEnterCallback(std::bind([this]() { m_pBgHoverIcon->setVisible(true); }));
	setMouseLeaveCallback(std::bind([this]() { m_pBgHoverIcon->setVisible(false); }));
	setMouseDownCallback(std::bind(&LLFlatButton::onClicked, this));

	return TRUE;
}

void LLFlatButton::onClicked()
{
	setValue(LLSD(!getValue().asBoolean()));

	if (mCommitSignal)
	{
		(*mCommitSignal)(this, LLSD());
	}
}

void LLFlatButton::setValue(const LLSD& sdValue)
{
	LLPanel::setValue(sdValue);

	bool fIsSelected = sdValue.asBoolean();
	m_pBgSelectIcon->setVisible(fIsSelected);
	m_pImage->setVisible(!fIsSelected);
	m_pImageSelected->setVisible(fIsSelected);
}

// ============================================================================
