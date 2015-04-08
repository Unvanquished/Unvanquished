// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

#ifndef COMPONENTS_ENTITIES_H_
#define COMPONENTS_ENTITIES_H_

#include "{{files['components']}}"

// Entity definitions

{% for entity in entities %}

	// Definition of {{entity.get_type_name()}}

	class {{entity.get_type_name()}}: public Entity {
		private:
			//* The vtables for each entities are statically defined
			static const MessageHandler messageHandlers[];
			static const int componentOffsets[];

		public:
			{% set user_params = entity.get_user_params() %}
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

			//* Default constructor
			{{entity.get_type_name()}}(Params params);
			virtual ~{{entity.get_type_name()}}();

			//* The components
			{% for component in entity.get_components() %}
				{{component.get_type_name()}} {{component.get_variable_name()}};
			{% endfor %}
	};

{% endfor %}

#endif // COMPONENTS_ENTITIES_H_

//* vi:ai:ts=4
