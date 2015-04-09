{% if header is defined %}
	/*
	{% for line in header %}
		 * {{line}}
	{% endfor %}
	 */

{% endif %}
// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

#ifndef CBSE_COMPONENTS_H_
#define CBSE_COMPONENTS_H_

{% for component in components -%}
	#include "../{{dirs['components']}}/{{component.get_type_name()}}.h"
{% endfor %}

#endif // CBSE_COMPONENTS_H_

//* vi:ai:ts=4
