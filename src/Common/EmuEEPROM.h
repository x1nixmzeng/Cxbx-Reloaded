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
// *   Common->EmuEEPROM.h
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
// *  (c) 2017 Patrick van Logchem <pvanlogchem@gmail.com>
// *
// *  All rights reserved
// *
// ******************************************************************
#ifndef EMU_EEPROM_H
#define EMU_EEPROM_H

#include "XDK.h"

namespace Xbox
{
#define EEPROM_SIZE sizeof(XBOX_EEPROM)

typedef struct EEPROMInfo {
	XDK::XC_VALUE_INDEX index;
	int value_offset;
	int value_type;
	int value_length;
} EEPROMInfo;

#define XC_END_MARKER (XC_VALUE_INDEX)-1

#define EEPROM_INFO_ENTRY(XC, Member, REG_Type) \
	{ ##XC, offsetof(XBOX_EEPROM, Member), REG_Type, sizeof(((XBOX_EEPROM *)0)->Member) }

static const EEPROMInfo EEPROMInfos[] = {
	EEPROM_INFO_ENTRY(XDK::XC_TIMEZONE_BIAS,         UserSettings.TimeZoneBias,           XDK::REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_TZ_STD_NAME,           UserSettings.TimeZoneStdName,             REG_BINARY),
	EEPROM_INFO_ENTRY(XDK::XC_TZ_STD_DATE,           UserSettings.TimeZoneStdDate,             REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_TZ_STD_BIAS,           UserSettings.TimeZoneStdBias,             REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_TZ_DLT_NAME,           UserSettings.TimeZoneDltName,             REG_BINARY),
	EEPROM_INFO_ENTRY(XDK::XC_TZ_DLT_DATE,           UserSettings.TimeZoneDltDate,             REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_TZ_DLT_BIAS,           UserSettings.TimeZoneDltBias,             REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_LANGUAGE,              UserSettings.Language,                    REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_VIDEO,                 UserSettings.VideoFlags,                  REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_AUDIO,                 UserSettings.AudioFlags,                  REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_P_CONTROL_GAMES,       UserSettings.ParentalControlGames,        REG_DWORD), // Zapper queries this. TODO : Should this be REG_NONE?
	EEPROM_INFO_ENTRY(XDK::XC_P_CONTROL_PASSWORD,    UserSettings.ParentalControlPassword,     REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_P_CONTROL_MOVIES,      UserSettings.ParentalControlMovies,       REG_DWORD), // Xbox Dashboard queries this.
	EEPROM_INFO_ENTRY(XDK::XC_ONLINE_IP_ADDRESS,     UserSettings.OnlineIpAddress,             REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_ONLINE_DNS_ADDRESS,    UserSettings.OnlineDnsAddress,            REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_ONLINE_DEFAULT_GATEWAY_ADDRESS, UserSettings.OnlineDefaultGatewayAddress, REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_ONLINE_SUBNET_ADDRESS, UserSettings.OnlineSubnetMask,            REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_MISC,                  UserSettings.MiscFlags,                   REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_DVD_REGION,            UserSettings.DvdRegion,                   REG_DWORD),
	EEPROM_INFO_ENTRY(XDK::XC_MAX_OS,                UserSettings,                             REG_BINARY),
	// XC_MAX_OS is called to return a complete XBOX_USER_SETTINGS structure
	//
	// One example is from XapipQueryTimeZoneInformation(, REG_DWORD, sizeof(DWORD), where it is used to
	// detect the local timezone information.
	EEPROM_INFO_ENTRY(XC_FACTORY_SERIAL_NUMBER, FactorySettings.SerialNumber,             REG_BINARY),
	EEPROM_INFO_ENTRY(XC_FACTORY_ETHERNET_ADDR, FactorySettings.EthernetAddr,             REG_BINARY),
	EEPROM_INFO_ENTRY(XC_FACTORY_ONLINE_KEY,    FactorySettings.OnlineKey,                REG_BINARY),
	EEPROM_INFO_ENTRY(XC_FACTORY_AV_REGION,     FactorySettings.AVRegion,                 REG_DWORD),
	// Note : XC_FACTORY_GAME_REGION is linked to a separate ULONG XboxFactoryGameRegion (of type REG_DWORD)
	EEPROM_INFO_ENTRY(XC_ENCRYPTED_SECTION,     EncryptedSettings,                        REG_BINARY),
	{ XC_MAX_ALL,                     0,                                        REG_BINARY, sizeof(XBOX_EEPROM) },
	{ XC_END_MARKER }
};

extern XBOX_EEPROM *CxbxRestoreEEPROM(char *szFilePath_EEPROM_bin);

extern const EEPROMInfo* EmuFindEEPROMInfo(XC_VALUE_INDEX index);

extern XBOX_EEPROM *EEPROM;

extern ULONG XboxFactoryGameRegion;

void gen_section_CRCs(XBOX_EEPROM*);

} // Xbox

#endif // EMU_EEPROM_H
