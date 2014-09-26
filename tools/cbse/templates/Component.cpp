//* TODO: Add license from input file.
// TODO: Insert license statement.

#include "{{component.get_type_name()}}.h"

{{component.get_type_name()}}::{{component.get_constructor_declaration()}}
	: {{component.get_super_call()}}
{}

{% for message in component.get_messages_to_handle() %}
	{{message.get_return_type()}} {{component.get_type_name()}}::{{message.get_handler_declaration()}} {
		// TODO: Implement {{component.get_type_name()}}::{{message.get_name()}}
	}
{% endfor %}

//* vi:ai:ts=4:filetype=jinja
