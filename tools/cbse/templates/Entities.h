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
		private:
			/** {{entity.get_type_name()}}'s message handler vtable. */
			static const MessageHandler messageHandlers[];

			/** {{entity.get_type_name()}}'s component offset table. */
			static const int componentOffsets[];

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
			virtual ~{{entity.get_type_name()}}();

			{% for component in entity.get_components() %}
				{{component.get_type_name()}} {{component.get_variable_name()}}; /**< {{entity.get_type_name()}}'s {{component.get_type_name()}} instance. */
			{% endfor %}
	};

{% endfor %}

#endif // CBSE_ENTITIES_H_

//* vi:ai:ts=4
