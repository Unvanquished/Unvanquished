// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

#include <tuple>

// Implementation of the base entity class

Entity::Entity(MessageHandler *messageHandlers, const int* componentOffsets): messageHandlers(messageHandlers), componentOffsets(componentOffsets) {
}

void Entity::SendMessage(int msg, const void* data) {
    MessageHandler handler = messageHandlers[msg];
    if (handler) {
        handler(this, data);
    }
}

{% for message in messages %}
    void Entity::{{message.name}}({{message.get_function_args()}}) {
        {% if message.get_num_args() == 0 %}
            SendMessage({{message.get_enum_name()}}, nullptr);
        {% else %}
            {{message.get_tuple_type()}} data({{message.get_args_names()}});
            SendMessage({{message.get_enum_name()}}, &data);
        {% endif %}
    }
{% endfor %}

{% for component in components %}
    {{component.get_type_name()}}* Entity::Get{{component.get_type_name()}}() {
        int index = {{component.get_priority()}};
        int offset = componentOffsets[index];
        if (offset) {
            return ({{component.get_type_name()}}*) (((char*) this) + offset);
        } else {
            return nullptr;
        }
    }
{% endfor %}

// Implementation of the components

{% for component in components %}
    {% for attrib in component.get_own_attribs() %}
        void {{component.get_base_type_name()}}::{{attrib.get_setter_name()}}({{attrib.typ}} value) {
            entity->SendMessage({{attrib.get_message().get_enum_name()}}, new {{attrib.typ}}(value));
        }
    {% endfor %}
{% endfor %}

// Implementation of the entities

{% for entity in entities %}
    static int {{entity.get_type_name()}}componentOffsets[] = {
        {% for component in components %}
            {% if component in entity.get_components() %}
                offsetof({{entity.get_type_name()}}, {{component.get_variable_name()}}),
            {% else %}
                0,
            {% endif %}
        {% endfor %}
    };

    {% for message in entity.get_messages_to_handle() %}
        void {{entity.get_message_handler_name(message)}}(Entity* _entity, const void* _data) {
            {{entity.get_type_name()}}* entity = ({{entity.get_type_name()}}*) _entity;
            {% if message.get_num_args == 0 %}
                {% for component in entity.get_components() %}
                    {% if message in component.get_messages_to_handle() %}
                        entity->{{component.get_variable_name()}}->{{message.get_handler_name()}}();
                    {% endif %}
                {% endfor %}
            {% else %}
                const {{message.get_tuple_type()}}* data = (const {{message.get_tuple_type()}}*) _data;
                {% for component in entity.get_components() %}
                    {% if message in component.get_messages_to_handle() %}
                        entity->{{component.get_variable_name()}}->{{message.get_handler_name()}}({{message.get_unpacked_tuple_args('*data')}});
                    {% endif %}
                {% endfor %}
            {% endif %}
        }
    {% endfor%}

    static MessageHandler {{entity.get_type_name()}}messageHandlers[] = {
        {% for message in messages %}
            {% if message in entity.get_messages_to_handle() %}
                {{entity.get_message_handler_name(message)}},
            {% else %}
                nullptr,
            {% endif %}
        {% endfor%}
    };

    {{entity.get_type_name()}}::{{entity.get_type_name()}}(): Entity(messageHandlers, componentOffsets)
    //TODO make sure it is in order
    {% for component in entity.get_components() %}
        , {{component.get_variable_name()}}(new {{component.get_type_name()}}(
            this
            {% for param in component.get_param_names() %}
                , {{entity.get_params()[component.name][param]}}
            {% endfor %}
            {% for attrib in component.get_attribs() %}
                , {{attrib.get_variable_name()}}
            {% endfor %}
            {% for required in component.get_required_components() %}
                , {{required.get_variable_name()}}
            {% endfor %}
        ))
    {% endfor %}
    {}

{% endfor %}
