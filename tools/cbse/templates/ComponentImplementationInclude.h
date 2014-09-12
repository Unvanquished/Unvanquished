// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK
//%L
#ifndef COMPONENT_IMPLEMENTATION_INCLUDE_H_
#define COMPONENT_IMPLEMENTATION_INCLUDE_H_
//%L
{% for component in components %}
#include "{{dirs['components']}}/{{component.get_type_name()}}.h"
{% endfor %}
//%L
#endif // COMPONENT_IMPLEMENTATION_INCLUDE_H_
//%L
