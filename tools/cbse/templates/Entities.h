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
// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

/*
 * This file contains:
 *   - Declarations of the specific entities.
 */

#ifndef CBSE_ENTITIES_H_
#define CBSE_ENTITIES_H_

#include "{{files['components']}}"

// ///////////////// //
// Specific Entities //
// ///////////////// //

{% for entity in entities %}
	/** A specific entity. */
	class {{entity.get_type_name()}}: public Entity {
		public:
			{% set user_params = entity.get_user_params() %}
			/** Initialization parameters for {{entity.get_type_name()}}. */
			struct Params {
				{% for attrib in general.common_entity_attributes %}
					{{attrib.get_declaration()}};
				{% endfor %}
				{% for component in entity.get_components() %}
					{% for param_name in user_params[component.name] %}
						{% set param = user_params[component.name][param_name] %}
						{{param.typ}} {{component.get_name()}}_{{param.name}};
					{% endfor %}
				{% endfor %}
			};

			/** Default constructor of {{entity.get_type_name()}}. */
			{{entity.get_type_name()}}(Params params);

			/** Default destructor of {{entity.get_type_name()}}. */
			virtual ~{{entity.get_type_name()}}() = default;

			{% for component in entity.get_components() %}
				{{component.get_type_name()}} {{component.get_variable_name()}}; /**< {{entity.get_type_name()}}'s {{component.get_type_name()}} instance. */
			{% endfor %}

		private:
			/** {{entity.get_type_name()}}'s message handler vtable. */
			static const MessageHandler messageHandlers[];

			/** {{entity.get_type_name()}}'s component offset table. */
			static const int componentOffsets[];
	};

{% endfor %}

#endif // CBSE_ENTITIES_H_
//* vi:ai:ts=4
