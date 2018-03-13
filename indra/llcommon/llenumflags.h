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

// ============================================================================
// Bit mask operators for C++0x enums
//

template<typename TEnum>
struct EnableEnumFlags : std::false_type
{
};

template<typename TEnum>
typename std::enable_if<EnableEnumFlags<TEnum>::value, bool>::type isSet(TEnum value, TEnum flag)
{
	using underlying = typename std::underlying_type<TEnum>::type;
	return static_cast<underlying>(value) & static_cast<underlying>(flag);
}

template<typename TEnum>
typename std::enable_if<EnableEnumFlags<TEnum>::value, TEnum>::type operator |(TEnum lhs, TEnum rhs)
{
	using underlying = typename std::underlying_type<TEnum>::type;
	return static_cast<TEnum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs) );
}

template<typename TEnum>
typename std::enable_if<EnableEnumFlags<TEnum>::value, TEnum>::type operator &(TEnum lhs, TEnum rhs)
{
	using underlying = typename std::underlying_type<TEnum>::type;
	return static_cast<TEnum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename TEnum>
typename std::enable_if<EnableEnumFlags<TEnum>::value, TEnum>::type operator ^(TEnum lhs, TEnum rhs)
{
	using underlying = typename std::underlying_type<TEnum>::type;
	return static_cast<TEnum>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template<typename TEnum>
typename std::enable_if<EnableEnumFlags<TEnum>::value, TEnum>::type operator ~(TEnum rhs)
{
	using underlying = typename std::underlying_type<TEnum>::type;
	return static_cast<TEnum>(~static_cast<underlying>(rhs));
}

template<typename TEnum>
typename std::enable_if<EnableEnumFlags<TEnum>::value, TEnum>::type& operator |=(TEnum& lhs, TEnum rhs)
{
	using underlying = typename std::underlying_type<TEnum>::type;
	lhs = static_cast<TEnum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
	return lhs;
}

template<typename TEnum>
typename std::enable_if<EnableEnumFlags<TEnum>::value, TEnum>::type& operator &=(TEnum& lhs, TEnum rhs)
{
	using underlying = typename std::underlying_type<TEnum>::type;
	lhs = static_cast<TEnum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
	return lhs;
}

template<typename TEnum>
typename std::enable_if<EnableEnumFlags<TEnum>::value, TEnum>::type& operator ^=(TEnum& lhs, TEnum rhs)
{
	using underlying = typename std::underlying_type<TEnum>::type;
	lhs = static_cast<TEnum>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
	return lhs;
}

// ============================================================================
