// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

// Implementation of the base entity class

{% for message in messages %}
    void Entity::{{message.name}}({{message.get_function_args()}}) {
        SendMessage({{message.get_enum_name()}}, new {{message.get_tuple_type()}}({{message.get_args_names()}}));
    }
{% endfor %}

{% for component in components %}
    {{component.get_type_name()}}* Entity::Get{{component.get_type_name()}}() {
        return nullptr;
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
