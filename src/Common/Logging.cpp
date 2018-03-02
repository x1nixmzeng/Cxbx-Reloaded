// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Logging.cpp
// *
// *  This file is part of the Cxbx project.
// *
// *  Cxbx and Cxbe are free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Bostom, MA 02111-1307, USA.
// *
// *  (c) 2016 Patrick van Logchem <pvanlogchem@gmail.com>
// *
// *  All rights reserved
// *
// ******************************************************************

#include "Logging.h"

// For thread_local, see : http://en.cppreference.com/w/cpp/language/storage_duration
// TODO : Use Boost.Format http://www.boost.org/doc/libs/1_53_0/libs/format/index.html
thread_local std::string _logPrefix;

const bool needs_escape(const Native::wint_t _char)
{
	// Escaping is needed for control characters,
	// for double quote, and for backslash :
	return Native::iswcntrl(_char) || (_char == '"') || (_char == '\\');
}

inline void output_char(std::ostream& os, char c)
{
	if (needs_escape((int)c))
	{
		switch (c)
		{
			// Render escaped double quote as \", and escaped backslash as \\ :
		case '"': os << "\\\""; break;
		case '\\': os << "\\\\"; break;
			// See https://en.wikipedia.org/wiki/Escape_sequences_in_C#Table_of_escape_sequences
		case '\a': os << "\\t"; break;
		case '\b': os << "\\b"; break;
		case '\f': os << "\\f"; break;
		case '\n': os << "\\n"; break;
		case '\r': os << "\\r"; break;
		case '\t': os << "\\t"; break;
		case '\v': os << "\\v"; break;
			// All other to-escape-characters are rendered as hexadecimal :
		default: os << "\\x" << std::setfill('0') << std::setw(2) << std::right << std::hex << std::uppercase << (int)c;
		}
	}
	else
		os << c;
}

inline void output_wchar(std::ostream& os, wchar_t c)
{
	if (needs_escape((Native::wint_t)c))
	{
		switch (c)
		{
			// Render escaped double quote as \", and escaped backslash as \\ :
		case '"': os << "\\\""; break;
		case '\\': os << "\\\\"; break;
			// See https://en.wikipedia.org/wiki/Escape_sequences_in_C#Table_of_escape_sequences
		case '\a': os << "\\t"; break;
		case '\b': os << "\\b"; break;
		case '\f': os << "\\f"; break;
		case '\n': os << "\\n"; break;
		case '\r': os << "\\r"; break;
		case '\t': os << "\\t"; break;
		case '\v': os << "\\v"; break;
			// All other to-escape-characters are rendered as hexadecimal :
		default: os << "\\x" << std::setfill('0') << std::setw(4) << std::right << std::hex << std::uppercase << (Native::wint_t)c;
		}
	}
	else
		os << c;
}

LOG_SANITIZE_HEADER(hex1, uint8_t)
{
	return os << hexstring8 << (int)container.value;
}

LOG_SANITIZE_HEADER(hex2, uint16_t)
{
	return os << hexstring16 << (int)container.value;
}

LOG_SANITIZE_HEADER(hex4, uint32_t)
{
	return os << hexstring32 << (int)container.value;
}

LOG_SANITIZE_HEADER(sanitized_char, char)
{
	output_char(os, container.value);
	return os;
}

LOG_SANITIZE_HEADER(sanitized_wchar, wchar_t)
{
	output_wchar(os, container.value);
	return os;
}

LOG_SANITIZE_HEADER(sanitized_char_pointer, char *)
{
	char *v = container.value;

	os << "(char *)";
	if (v == nullptr)
		return os << "NULL";

	bool needsEscaping = false;

	while (*v)
		if (needs_escape(*v++))
		{
			needsEscaping = true;
			break;
		}

	v = container.value;
	os << hexstring32 << (uint32_t)v << " = \"";
	if (needsEscaping)
	{
		while (*v)
			output_char(os, *v++);
	}
	else
		os << v;

	return os << "\"";
}

LOG_SANITIZE_HEADER(sanitized_wchar_pointer, wchar_t *)
{
	wchar_t *v = container.value;

	os << "(wchar *)";
	if (v == nullptr)
		return os << "NULL";

	bool needsEscaping = false;

	while (*v)
		if (needs_escape(*v++))
		{
			needsEscaping = true;
			break;
		}

	v = container.value;
	os << hexstring32 << (uint32_t)v << " = \"";
	if (needsEscaping)
	{
		while (*v)
			output_wchar(os, *v++);
	}
	else
		os << v; // TODO : FIXME - VS2015 doesn''t render this string (instead, it shows a hexadecimal memory address)

	return os << "\"";
}

LOGRENDER_HEADER_BY_REF(Native::PVOID)
{
	return os << hex4((uint32_t)value);
}
