// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 EpsilonBSP

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

/**
 * @file smsdk_config.h
 * @brief Contains macros for configuring basic extension information.
 */

/* Basic information exposed publicly */
#define SMEXT_CONF_NAME         "Bzip2 Extension"
#define SMEXT_CONF_DESCRIPTION  "Extension for compressing and extracting files"
#define SMEXT_CONF_VERSION      "1.0.0.0"
#define SMEXT_CONF_AUTHOR       "EpsilonBSP"
#define SMEXT_CONF_URL          "https://github.com/epsilonbsp/sm_bzip2"
#define SMEXT_CONF_LOGTAG       "BZIP2"
#define SMEXT_CONF_LICENSE      "GPL"
#define SMEXT_CONF_DATESTRING   __DATE__

/** 
 * @brief Exposes plugin's main interface.
 */
#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

/** Enable interfaces you want to use here by uncommenting lines */
#define SMEXT_ENABLE_FORWARDSYS
#define SMEXT_ENABLE_THREADER

#endif // _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
