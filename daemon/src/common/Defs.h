/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#ifndef COMMON_DEFS_H_
#define COMMON_DEFS_H_

#define PRODUCT_NAME        "Unvanquished"
/** Case, No spaces */
#define PRODUCT_NAME_UPPER  "UNVANQUISHED"
/** No case, No spaces */
#define PRODUCT_NAME_LOWER  "unvanquished"

#define PRODUCT_VERSION     "0.50"

/** Default base package */
#define DEFAULT_BASE_PAK    PRODUCT_NAME_LOWER

#define DAEMON_FS_LEGACY_PAKS false

/** URI scheme and server browser filter */
#define GAMENAME_STRING     "unv"
#define GAMENAME_FOR_MASTER PRODUCT_NAME_UPPER

#define MASTER_SERVER_NAME  "master.unvanquished.net"

#define IRC_SERVER          "irc.freenode.org"
#define IRC_CHANNEL         "unv-lobby"

#define WWW_BASEURL         "dl.unvanquished.net/pkg"

#define AUTOEXEC_NAME       "autoexec.cfg"

#define CONFIG_NAME         "autogen.cfg"
#define KEYBINDINGS_NAME    "keybindings.cfg"
#define TEAMCONFIG_NAME     "teamconfig.cfg"
#define SERVERCONFIG_NAME   "autogen_server.cfg"

#define UNNAMED_PLAYER      "UnnamedPlayer"
#define UNNAMED_SERVER      PRODUCT_NAME " " PRODUCT_VERSION " Server"

/** file containing our RSA public and private keys */
#define RSAKEY_FILE        "pubkey"

#endif // COMMON_DEFS_H_
