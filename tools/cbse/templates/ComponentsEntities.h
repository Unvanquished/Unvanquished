// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

#ifndef COMPONENTS_ENTITIES_H_
#define COMPONENTS_ENTITIES_H_

#include "{{files['declaration']}}"
#include "{{files['includehelper']}}"

// Entity definitions

{% for entity in entities %}

    // Definition of {{entity.get_type_name()}}

    class {{entity.get_type_name()}}: public Entity {
        private:
            //* The vtables for each entities are statically defined
            static const MessageHandler messageHandlers[];
            static const int componentOffsets[];

        public:
            //* Default constructor
            {{entity.get_type_name()}}(
                {%- for (i, attrib) in enumerate(general.common_entity_attributes) -%}
                    {%- if i != 0 -%}
                        ,
                    {%- endif -%}
                    {{attrib.get_declaration()}}
                {%- endfor -%}
            );
            virtual ~{{entity.get_type_name()}}();

            //* The components
            {% for component in entity.get_components() %}
                {{component.get_type_name()}} {{component.get_variable_name()}};
            {% endfor %}
    };

{% endfor %}

#endif // COMPONENTS_ENTITIES_H_
