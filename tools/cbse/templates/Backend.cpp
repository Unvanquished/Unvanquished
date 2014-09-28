// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

/*
This file contains:
	- Implementation of the base entity.
	- Helper functions for sending messages to entities.
	- Implementations of the specific entities, including necessary helpers.
*/

#include "{{files['backend']}}"
#include "{{files['entities']}}"
#include <tuple>

#define myoffsetof(st, m) static_cast<int>((size_t)(&((st *)0)->m))

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

// Base entity deconstructor.
Entity::~Entity()
{}

// Base entity's message dispatcher.
bool Entity::SendMessage(int msg, const void* data) {
	MessageHandler handler = messageHandlers[msg];
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
			return SendMessage({{message.get_enum_name()}}, nullptr);
		{% else %}
			{{message.get_tuple_type()}} data({{message.get_args_names()}});
			return SendMessage({{message.get_enum_name()}}, &data);
		{% endif %}
	}
{% endfor %}

{% for component in components %}
	std::set<{{component.get_type_name()}}*> {{component.get_base_type_name()}}::allSet;
{% endfor %}

// ////////////////////// //
// Entity implementations //
// ////////////////////// //
//* TODO: Move to new file Entities.cpp?
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
		void {{entity.get_message_handler_name(message)}}(Entity* _entity, const void* {% if message.get_num_args() > 0 %} _data{% endif %}) {
			//* Cast the entity to the correct type (receive an Entity*)
			{{entity.get_type_name()}}* entity = ({{entity.get_type_name()}}*) _entity;
			{% if message.get_num_args() == 0 %}
				{% for component in entity.get_components() %}
					{% if message in component.get_messages_to_handle() %}
						//* No argument for the message, just call the handlers of all the components
						entity->{{component.get_variable_name()}}.{{message.get_handler_name()}}();
					{% endif %}
				{% endfor %}
			{% else %}
				//* Cast the message content to the right type (receive a const void*)
				const {{message.get_tuple_type()}}* data = (const {{message.get_tuple_type()}}*) _data;
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
	{{entity.get_type_name()}}::{{entity.get_type_name()}}(
		{%- for (i, attrib) in enumerate(general.common_entity_attributes) -%}
			{%- if i != 0 -%}
				,
			{%- endif -%}
			{{attrib.get_declaration()}}
		{%- endfor -%})
		: Entity(messageHandlers, componentOffsets
		{%- for attrib in general.common_entity_attributes -%}
			, {{attrib.get_name()}}
		{%- endfor -%})
		{%- for component in entity.get_components() -%}
			, {{component.get_variable_name()}}(*this
			{%- for param in component.get_param_names() -%}
				, {{entity.get_params()[component.name][param]}}
			{%- endfor -%}
			{%- for required in component.get_required_components() -%}
				, *{{required.get_variable_name()}}
			{%- endfor -%})
		{%- endfor %}

	{}

	// {{entity.get_type_name()}}'s deconstructor.
	//* Destroys all the components in reverse order.
	{{entity.get_type_name()}}::~{{entity.get_type_name()}}()
	{}
{% endfor %}

#undef myoffsetof

//* vi:ai:ts=4:filetype=jinja
