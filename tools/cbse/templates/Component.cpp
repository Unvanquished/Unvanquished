//* TODO: Add license from input file.
// TODO: Insert license statement.

#include "{{component.get_type_name()}}.h"

{{component.get_type_name()}}::{{component.get_type_name()}}(Entity& entity
{%- for declaration in component.get_param_declarations() -%}
	, {{declaration}}
{%- endfor -%}
{%- for declaration in component.get_required_component_declarations() -%}
	, {{declaration}}
{%- endfor -%}
)
	: {{component.get_base_type_name()}}(entity
{%- for name in component.get_param_names() -%}
	, {{name}}
{%- endfor -%}
{%- for name in component.get_required_component_names() -%}
	, {{name}}
{%- endfor -%}
)
{}

{% for message in component.get_messages_to_handle() %}
	{{message.get_return_type()}} {{component.get_type_name()}}::{{message.get_handler_declaration()}} {
		// TODO: Implement {{component.get_type_name()}}::{{message.get_name()}}
	}
{% endfor %}

//* vi:ai:ts=4
