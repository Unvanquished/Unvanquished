// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK
//% L

#include <tuple>
#include "ComponentImplementationInclude.h"

//% L

#define myoffsetof(st, m) static_cast<int>((size_t)(&((st *)0)->m))

//% L
// Implementation of the base entity class
//% L

//* Constructor of entity
Entity::Entity(const MessageHandler *messageHandlers, const int* componentOffsets
    {% for attrib in general.common_entity_attributes %}
        ,{{attrib.get_declaration()}}
    {% endfor %}
): messageHandlers(messageHandlers), componentOffsets(componentOffsets)
    {% for attrib in general.common_entity_attributes %}
        ,{{attrib.get_initializer()}}
    {% endfor %}
{
}

//% L

Entity::~Entity() {
}

//% L

bool Entity::SendMessage(int msg, const void* data) {
    MessageHandler handler = messageHandlers[msg];
    if (handler) {
        handler(this, data);
        return true;
    }
    return false;
}

//% L
//* Entity helper functions to send message e.g.
//*   void Entity::Damage(int value);
{% for message in messages %}
    bool Entity::{{message.name}}({{message.get_function_args()}}) {
        {% if message.get_num_args() == 0 %}
            return SendMessage({{message.get_enum_name()}}, nullptr);
        {% else %}
            {{message.get_tuple_type()}} data({{message.get_args_names()}});
            return SendMessage({{message.get_enum_name()}}, &data);
        {% endif %}
    }
{% endfor %}

//% L
//* Entity helper functions to get the components e.g.
//*   HealthComponent* GetHealthComponent();
{% for component in components %}
    {{component.get_type_name()}}* Entity::Get{{component.get_type_name()}}() {
        int index = {{component.get_priority()}};
        int offset = componentOffsets[index];
        if (offset) {
            return *({{component.get_type_name()}}**) (((char*) this) + offset);
        } else {
            return nullptr;
        }
    }
{% endfor %}

//% L
// Implementation of the entities
//% L

{% for entity in entities %}

    //% L
    // Implementation of {{entity.get_type_name()}}
    //% L    

    //* The vtable of offset of components in an entity
    //* TODO: doesn't handle component inheritance?
    const int {{entity.get_type_name()}}::componentOffsets[] = {
        {% for component in components %}
            {% if component in entity.get_components() %}
                myoffsetof({{entity.get_type_name()}}, {{component.get_variable_name()}}),
            {% else %}
                0,
            {% endif %}
        {% endfor %}
    };

    //% L

    //* The static message handlers put in the vtable
    {% for message in entity.get_messages_to_handle() %}
        void {{entity.get_message_handler_name(message)}}(Entity* _entity, const void* {% if message.get_num_args() > 0 %} _data {% endif %} ) {
            //* Cast the entity to the correct type (receive an Entity*)
            {{entity.get_type_name()}}* entity = ({{entity.get_type_name()}}*) _entity;

            //% L

            {% if message.get_num_args() == 0 %}
                //* No argument for the message, just call the handlers of all the components
                {% for component in entity.get_components() %}
                    {% if message in component.get_messages_to_handle() %}
                        entity->{{component.get_variable_name()}}->{{message.get_handler_name()}}();
                    {% endif %}
                {% endfor %}
            {% else %}
                //* Cast the message content to the right type (receive a const void*)
                const {{message.get_tuple_type()}}* data = (const {{message.get_tuple_type()}}*) _data;
                {% for component in entity.get_components() %}
                    {% if message in component.get_messages_to_handle() %}
                        entity->{{component.get_variable_name()}}->{{message.get_handler_name()}}({{message.get_unpacked_tuple_args('*data')}});
                    {% endif %}
                {% endfor %}
            {% endif %}
        }
    {% endfor%}

    //% L

    //* The vtable of message handlers for an entity
    const MessageHandler {{entity.get_type_name()}}::messageHandlers[] = {
        {% for message in messages %}
            {% if message in entity.get_messages_to_handle() %}
                {{entity.get_message_handler_name(message)}},
            {% else %}
                nullptr,
            {% endif %}
        {% endfor%}
    };

    //% L

    //* Fat constructor for the entity that initializes the components.
    {{entity.get_type_name()}}::{{entity.get_type_name()}}(
        {% for (i, attrib) in enumerate(general.common_entity_attributes) %}
            {% if i != 0 %}, {% endif %} {{attrib.get_declaration()}}
        {% endfor %}
    ): Entity(messageHandlers, componentOffsets
        {% for attrib in general.common_entity_attributes %}
            ,{{attrib.get_name()}}
        {% endfor %}
    )
    {% for component in entity.get_components() %}
        //* Each component takes the entity it is in, its parameters and the components it requires
        , {{component.get_variable_name()}}(new {{component.get_type_name()}}(
            this
            {% for param in component.get_param_names() %}
                , {{entity.get_params()[component.name][param]}}
            {% endfor %}
            {% for required in component.get_required_components() %}
                , {{required.get_variable_name()}}
            {% endfor %}
        ))
    {% endfor %}
    {}

    //% L

    //* The destructor of the entity destroys all the components in reverse order
    {{entity.get_type_name()}}::~{{entity.get_type_name()}}() {
        {% for component in entity.get_components()[::-1] %}
            delete {{component.get_variable_name()}};
        {% endfor %}
    }
    //% L
{% endfor %}

//% L

#undef myoffsetof

