{% if header is defined %}
	/*
	{% for line in header %}
		 * {{line}}
	{% endfor %}
	 */

{% endif %}
// This header includes the rest of the CBSE system.
// It won't be overwritten on backend generation but there should be no need to
// add any content here.

#ifndef CBSE_H_
#define CBSE_H_

#include "{{dirs['backend']}}/{{files['entities']}}"

#endif // CBSE_H_

//* vi:ai:ts=4
