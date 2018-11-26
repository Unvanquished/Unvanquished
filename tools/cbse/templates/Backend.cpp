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
 *   - Implementation of the base entity.
 *   - Implementations of the specific entities and related helpers.
 */

#include "{{files['entities']}}"
#include <tuple>

#define myoffsetof(st, m) static_cast<int>((size_t)(&((st *)1)->m))-1

// /////////// //
// Base entity //
// /////////// //

// Base entity constructor.
Entity::Entity(const MessageHandler *messageHandlers, const int* componentOffsets
	{%- for attrib in general.common_entity_attributes -%}
		, {{attrib.get_declaration()}}
	{%- endfor -%})
	: messageHandlers(messageHandlers), componentOffsets(componentOffsets)
	{%- for attrib in general.common_entity_attributes -%}
		, {{attrib.get_initializer()}}
	{%- endfor %}

{}

// Base entity's message dispatcher.
bool Entity::SendMessage(EntityMessage msg, const void* data) {
	MessageHandler handler = messageHandlers[static_cast<int>(msg)];
	if (handler) {
		handler(this, data);
		return true;
	}
	return false;
}

// /////////////// //
// Message helpers //
// /////////////// //
{% for message in messages %}

	bool Entity::{{message.name}}({{message.get_function_args()}}) {
		{% if message.get_num_args() == 0 %}
			return SendMessage(EntityMessage::{{message.get_name()}}, nullptr);
		{% else %}
			{{message.get_tuple_type()}} data({{message.get_args_names()}});
			return SendMessage(EntityMessage::{{message.get_name()}}, &data);
		{% endif %}
	}
{% endfor %}

// ///////////////////////// //
// Component implementations //
// ///////////////////////// //
{% for component in components %}
    {% for required in component.get_own_required_components() %}
        /**
         * @return A reference to the {{required.get_type_name()}} of the owning entity.
         */
        {{required.get_type_name()}}& {{component.get_base_type_name()}}::Get{{required.get_type_name()}}() {
            return r_{{required.get_type_name()}};
        }
        const {{required.get_type_name()}}& {{component.get_base_type_name()}}::Get{{required.get_type_name()}}() const {
            return r_{{required.get_type_name()}};
        }

    {% endfor %}

    {% for (dependency, firstLevel) in component.get_own_further_dependencies().items() %}
        /**
         * @return A reference to the {{dependency.get_type_name()}} of the owning entity.
         */
        {{dependency.get_type_name()}}& {{component.get_base_type_name()}}::Get{{dependency.get_type_name()}}() {
            return r_{{firstLevel.get_type_name()}}.Get{{dependency.get_type_name()}}();
        }
        const {{dependency.get_type_name()}}& {{component.get_base_type_name()}}::Get{{dependency.get_type_name()}}() const {
            return r_{{firstLevel.get_type_name()}}.Get{{dependency.get_type_name()}}();
        }

    {% endfor %}

	std::set<{{component.get_type_name()}}*> {{component.get_base_type_name()}}::allSet;

{% endfor %}

// ////////////////////// //
// Entity implementations //
// ////////////////////// //
{% for entity in entities %}

	// {{entity.get_type_name()}}
	// ---------

	// {{entity.get_type_name()}}'s component offset vtable.
	//* TODO: doesn't handle component inheritance?
	const int {{entity.get_type_name()}}::componentOffsets[] = {
		{% for component in components %}
			{% if component in entity.get_components() %}
				myoffsetof({{entity.get_type_name()}}, {{component.get_variable_name()}}),
			{% else %}
				0,
			{% endif %}
		{% endfor %}
	};
	{% for message in entity.get_messages_to_handle() %}

		// {{entity.get_type_name()}}'s {{message.get_name()}} message dispatcher.
		void {{entity.get_message_handler_name(message)}}(Entity* _entity, const void*{% if message.get_num_args() > 0 %} _data{% endif %}) {
			//* Cast the entity to the correct type (receive an Entity*)
			auto* entity = static_cast<{{entity.get_type_name()}}*>(_entity);
			{% if message.get_num_args() == 0 %}
				{% for component in entity.get_components() %}
					{% if message in component.get_messages_to_handle() %}
						//* No argument for the message, just call the handlers of all the components
						entity->{{component.get_variable_name()}}.{{message.get_handler_name()}}();
					{% endif %}
				{% endfor %}
			{% else %}
				//* Cast the message content to the right type (received a const void*)
				const auto* data = static_cast<const {{message.get_tuple_type()}}*>(_data);
				{% for component in entity.get_components() %}
					{% if message in component.get_messages_to_handle() %}
						entity->{{component.get_variable_name()}}.{{message.get_handler_name()}}({{message.get_unpacked_tuple_args('*data')}});
					{% endif %}
				{% endfor %}
			{% endif %}
		}
	{% endfor%}

	// {{entity.get_type_name()}}'s message dispatcher vtable.
	const MessageHandler {{entity.get_type_name()}}::messageHandlers[] = {
		{% for message in messages %}
			{% if message in entity.get_messages_to_handle() %}
				{{entity.get_message_handler_name(message)}},
			{% else %}
				nullptr,
			{% endif %}
		{% endfor %}
	};

	// {{entity.get_type_name()}}'s constructor.
	{% set user_params = entity.get_user_params() %}
	{{entity.get_type_name()}}::{{entity.get_type_name()}}(Params params)
		: Entity(messageHandlers, componentOffsets
			{%- for attrib in general.common_entity_attributes -%}
				, params.{{attrib.get_name()}}
			{%- endfor -%}
		)
		{% for component in entity.get_components() %}
			, {{component.get_variable_name()}}(*this
			{%- for param in component.get_param_names() -%}
				{%- if param in user_params[component.get_name()] -%}
					, params.{{component.get_name()}}_{{param}}
				{%- else-%}
					, {{entity.get_params()[component.name][param]}}
				{%- endif -%}
			{%- endfor -%}
			{%- for required in component.get_required_components() -%}
				, {{required.get_variable_name()}}
			{%- endfor -%})
		{% endfor %}
	{}
{% endfor %}

#undef myoffsetof
//* vi:ai:ts=4
