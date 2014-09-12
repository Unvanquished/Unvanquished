// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

//% L

#ifndef COMPONENTS_ENTITIES_H_
#define COMPONENTS_ENTITIES_H_

//% L

#include "Components.h"
#include "ComponentImplementationInclude.h"

//% L
// Entity definitions
//% L

{% for entity in entities %}

    //% L
    // Definition of {{entity.get_type_name()}}
    //% L

    class {{entity.get_type_name()}}: public Entity {
            //* The vtables for each entities are statically defined
            static const MessageHandler messageHandlers[];
            static const int componentOffsets[];

            //% L

        public:
            //* Default constructor
            {{entity.get_type_name()}}(
                {% for (i, attrib) in enumerate(general.common_entity_attributes) %}
                    {% if i != 0 %}, {% endif %} {{attrib.get_declaration()}}
                {% endfor %}
            );
            virtual ~{{entity.get_type_name()}}();

            //% L

            //* The components
            {% for component in entity.get_components() %}
                {{component.get_type_name()}} {{component.get_variable_name()}};
            {% endfor %}
    };
    //% L
{% endfor %}

//% LL

#endif // COMPONENTS_ENTITIES_H_

//% L
