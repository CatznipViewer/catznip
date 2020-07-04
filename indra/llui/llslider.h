/** 
 * @file llslider.h
 * @brief A simple slider with no label.
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

#ifndef LL_LLSLIDER_H
#define LL_LLSLIDER_H

#include "llf32uictrl.h"
#include "v4color.h"
#include "lluiimage.h"

class LLSlider : public LLF32UICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLF32UICtrl::Params>
	{
		Optional<std::string> orientation;

		Optional<LLUIColor>	track_color,
// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
							divet_color,
// [/SL:KB]
							thumb_outline_color,
							thumb_center_color;

		Optional<LLUIImage*>	thumb_image,
								thumb_image_pressed,
								thumb_image_disabled,
								track_image_horizontal,
								track_image_vertical,
// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
								divet_image,
// [/SL:KB]
								track_highlight_horizontal_image,
								track_highlight_vertical_image;

// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
		Optional<bool> show_default_value;
		Optional<F32>  default_value;
		Optional<S32>  default_value_dead_zone;
// [/SL:KB]

		Optional<CommitCallbackParam>	mouse_down_callback,
										mouse_up_callback;


		Params();
	};
protected:
	LLSlider(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLSlider();
	void			setValue( F32 value, BOOL from_event = FALSE );
    // overrides for LLF32UICtrl methods
	virtual void	setValue(const LLSD& value )	{ setValue((F32)value.asReal(), TRUE); }
	
	virtual void 	setMinValue(const LLSD& min_value) { setMinValue((F32)min_value.asReal()); }
	virtual void 	setMaxValue(const LLSD& max_value) { setMaxValue((F32)max_value.asReal()); }
//	virtual void	setMinValue(F32 min_value) { LLF32UICtrl::setMinValue(min_value); updateThumbRect(); }
//	virtual void	setMaxValue(F32 max_value) { LLF32UICtrl::setMaxValue(max_value); updateThumbRect(); }
// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
	        void	setMinValue(F32 min_value) override { LLF32UICtrl::setMinValue(min_value); updateThumbRect(); setDefaultValue(mDefaultValue); }
	        void	setMaxValue(F32 max_value) override { LLF32UICtrl::setMaxValue(max_value); updateThumbRect(); setDefaultValue(mDefaultValue); }
			void	setControlVariable(LLControlVariable* control) override;
			void	setDefaultValue(F32 default_value);
			void	setShowDefaultValue(bool show_default_value);
// [/SL:KB]

	boost::signals2::connection setMouseDownCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setMouseUpCallback(	const commit_signal_t::slot_type& cb );

	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void	draw();

private:
	void			setValueAndCommit(F32 value);
	void			updateThumbRect();
// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
	LLRect			calcThumbRect(F32 value, S32 thumb_width, S32 thumb_height) const;
	void			updateDivetRect();
// [/SL:KB]

	BOOL			mVolumeSlider;
	S32				mMouseOffset;
	LLRect			mDragStartThumbRect;

	LLPointer<LLUIImage>	mThumbImage;
// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
	boost::signals2::scoped_connection mThumbImageLoadedConnection;
// [/SL:KB]
	LLPointer<LLUIImage>	mThumbImagePressed;
	LLPointer<LLUIImage>	mThumbImageDisabled;
	LLPointer<LLUIImage>	mTrackImageHorizontal;
	LLPointer<LLUIImage>	mTrackImageVertical;
// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
	LLPointer<LLUIImage>    mDivetImage;
// [/SL:KB]
	LLPointer<LLUIImage>	mTrackHighlightHorizontalImage;
	LLPointer<LLUIImage>	mTrackHighlightVerticalImage;

	const EOrientation	mOrientation;

// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
	bool        mShowDefaultValue = false;
	F32         mDefaultValue = F32_MAX;
	S32         mDefaultValueDeadZone = 10;
// [/SL:KB]
	LLRect		mThumbRect;
// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
	LLRect		mDivetRect;
// [/SL:KB]
	LLUIColor	mTrackColor;
// [SL:KB] - Control-SliderDefaultValue | Checked: Catznip-6.4
	LLUIColor	mDivetColor;
// [/SL:KB]
	LLUIColor	mThumbOutlineColor;
	LLUIColor	mThumbCenterColor;
	
	commit_signal_t*	mMouseDownSignal;
	commit_signal_t*	mMouseUpSignal;
};

#endif  // LL_LLSLIDER_H
