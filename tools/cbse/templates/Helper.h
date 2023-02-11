//* Daemon CBSE Source Code
//* Copyright (c) 2014-2015, Daemon Developers
//* All rights reserved.
//*
//* Redistribution and use in source and binary forms, with or without
//* modification, are permitted provided that the following conditions are met:
//*
//* * Redistributions of source code must retain the above copyright notice, this
//*   list of conditions and the following disclaimer.
//*
//* * Redistributions in binary form must reproduce the above copyright notice,
//*   this list of conditions and the following disclaimer in the documentation
//*   and/or other materials provided with the distribution.
//*
//* * Neither the name of Daemon CBSE nor the names of its
//*   contributors may be used to endorse or promote products derived from
//*   this software without specific prior written permission.
//*
//* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
{% if header is defined %}
	/*
	{% for line in header %}
		 * {{line}}
	{% endfor %}
	 */

{% endif %}
// This header includes the rest of the CBSE system.
// It also provides to the CBSE backend all types that it needs to know about.

#ifndef CBSE_H_
#define CBSE_H_

// Add here any definitions, forward declarations and includes that provide all
// the types used in the entities definition file (and thus the CBSE backend).
// Make sure none of the includes in this file includes any header that is part
// of the CBSE system.
// You can also define helper macros for use in all components here.
// ----------------



// ----------------

// Include the backend. These should be the last lines in this header.
#ifndef CBSE_INCLUDE_TYPES_ONLY
#include "{{dirs['backend']}}/{{files['entities']}}"
#endif // CBSE_INCLUDE_TYPES_ONLY

#endif // CBSE_H_
//* vi:ai:ts=4
