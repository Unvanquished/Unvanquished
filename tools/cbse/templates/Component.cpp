//* TODO: Add license from input file.
// TODO: Insert license statement.
//%L
#include "{{component.get_type_name()}}.h"
//%L
{{component.get_type_name()}}::{{component.get_constructor_declaration()}}
	: {{component.get_super_call()}}
{}
//%L
{% for message in component.get_messages_to_handle() %}
{{message.get_return_type()}} {{component.get_type_name()}}::{{message.get_handler_declaration()}} {
	// TODO: Implement {{component.get_type_name()}}::{{message.get_name()}}
}
//%L
{% endfor %}
//%L
