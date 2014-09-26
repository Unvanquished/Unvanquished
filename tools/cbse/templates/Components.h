// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

#ifndef COMPONENT_IMPLEMENTATION_INCLUDE_H_
#define COMPONENT_IMPLEMENTATION_INCLUDE_H_

{% for component in components -%}
	#include "{{dirs['components']}}/{{component.get_type_name()}}.h"
{% endfor %}

#endif // COMPONENT_IMPLEMENTATION_INCLUDE_H_

//* vi:ai:ts=4:filetype=jinja
