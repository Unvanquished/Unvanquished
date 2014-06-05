// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

// Message and attribute change enum list

enum {
    {% for message in messages%}
        {{message.get_enum_name()}},
    {% endfor %}
    {% for attribute in attributes %}
        {{attribute.get_change_enum_name()}},
    {% endfor %}
};

// Component forward declarations

{% for component in components %}
    class {{component.get_type_name()}};
{% endfor %}

// Base entity class

class Entity {
    public:
        virtual void SendMessage(int msg, void* data) = 0;
        {% for message in messages %}
            void {{message.name}}({{message.get_function_args()}});
        {% endfor %}
        {% for component in components %}
            {{component.get_type_name()}}* Get{{component.get_type_name()}}();
        {% endfor %}
};

// Component definitions

{% for component in components %}
    class {{component.get_base_type_name()}} {
        private:
            Entity* entity;
        protected:
            {% for declaration in component.get_own_param_declarations() %}
                const {{declaration}};
            {% endfor %}
            {% for declaration in component.get_own_attrib_declarations() %}
                {{declaration}};
            {% endfor %}
        public:
            {{component.get_base_type_name()}}(
                Entity* entity
                {% for declaration in component.get_param_declarations() %},{{declaration}}{% endfor %}
                {% for declaration in component.get_attrib_declarations() %},{{declaration}}{% endfor %}
            ): entity(entity)
            {% for names in component.get_own_param_names() %},{{names}}({{names}}){% endfor %}
            {% for names in component.get_own_attrib_names() %},{{names}}({{names}}){% endfor %}
            {}

            {% for attrib in component.get_own_attribs() %}
                void {{attrib.get_setter_name()}}({{attrib.typ}} value);
            {% endfor %}

    };
{% endfor %}
