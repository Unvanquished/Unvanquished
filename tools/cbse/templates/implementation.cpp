// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

#include <tuple>

// Implementation of the base entity class

Entity::Entity(void (**messageHandlers)(Entity*, const void*), const int* componentOffsets): messageHandlers(messageHandlers), componentOffsets(componentOffsets) {
}

void Entity::SendMessage(int msg, const void* data) {
    void (*handler)(Entity*, const void*) = messageHandlers[msg];
    if (handler) {
        handler(this, data);
    }
}

{% for message in messages %}
    void Entity::{{message.name}}({{message.get_function_args()}}) {
        {% if message.get_num_args() == 0 %}
            SendMessage({{message.get_enum_name()}}, nullptr);
        {% else %}
            {{message.get_tuple_type()}}* data = new {{message.get_tuple_type()}}({{message.get_args_names()}});
            SendMessage({{message.get_enum_name()}}, data);
            delete data;
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
            entity->SendMessage({{attrib.get_change_enum_name()}}, new {{attrib.typ}}(value));
        }
    {% endfor %}
{% endfor %}
