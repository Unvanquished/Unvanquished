// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

//* Message and attribute change enum list used to tag each message sent to an entity with its 'type'.
//* Components could have been implemented using multiple (virtual?) inheritance except:
//*  - We want to be able to allocate them in different pools and we would need ugly
//*  new operator overloads to do that.
//*  - Several components can register to the same message contrary to only one using multiple inheritance
//*
//* We opted to implement component messaging using a custom vtable-like structure as follows: for each
//* entity, for each message it handles, we have a static function dispatching the message to the right
//* components in that entity. For each entity we have a vtable [message number] -> [function pointer].
//*
//* Likewise we want to be able to ask an entity for a specific component it might have, we have another
//* vtable containing the offset of the components, if any, for each entity.


#ifndef COMPONENTS_H_
#define COMPONENTS_H_

enum {
    {% for message in messages%}
        {{message.get_enum_name()}},
    {% endfor %}
};

// Component forward declarations

{% for component in components %}
    class {{component.get_type_name()}};
{% endfor %}

class Entity;
typedef void (*MessageHandler)(Entity*, const void*);

// Base entity class

class Entity {
    public:
        //* Entity takes as argument for the constructor both vtables
        Entity(const MessageHandler* messageHandlers, const int* componentOffsets
            {% for attrib in general.common_entity_attributes %}
                ,{{attrib.get_declaration()}}
            {% endfor %}
        );
        virtual ~Entity();

        //* This function should never actually be used directly, it dispatches
        //* the messages to the right components
        bool SendMessage(int msg, const void* data);

        //* Handy functions to send a specific message to the components of Entity e.g. (returns true if the message was handled)
        //*   bool Damage(int amount);
        {% for message in messages %}
            bool {{message.name}}({{message.get_function_args()}});
        {% endfor %}

        //* Getter for the different components, returns nullptr if it doesn't have the component e.g.
        //*   HealthComponent* GetHealthComponent();
        {% for component in components %}
            {{component.get_type_name()}}* Get{{component.get_type_name()}}();
        {% endfor %}

    private:
        //* The vtables for the entity
        const MessageHandler* messageHandlers;
        const int* componentOffsets;

    public:
        {% for attrib in general.common_entity_attributes %}
            {{attrib.get_declaration()}};
        {% endfor %}
};

// Component definitions

{% for component in components %}
    class {{component.get_base_type_name()}} {
        public:
            //* Every component has a pointer back to the entity (say to be able to send back messages etc.)
            Entity* entity;

        protected:
            //* Contains as a value its default parameters e.g.
            //*   const int maxHealth;
            {% for declaration in component.get_own_param_declarations() %}
                const {{declaration}};
            {% endfor %}

            //* Has a pointer to the other components it requires e.g.
            //*   HealthComponent* const r_CompComponent
            {% for declaration in component.get_own_required_component_declarations() %}
                {{declaration}};
            {% endfor %}

        public:
            //* The constructor takes the default values and required components
            {{component.get_base_type_name()}}(
                Entity* entity
                {% for declaration in component.get_param_declarations() %},{{declaration}}{% endfor %}
                {% for declaration in component.get_required_component_declarations() %},{{declaration}}{% endfor %}
            ): entity(entity)
            {% for name in component.get_own_param_names() %},{{name}}({{name}}){% endfor %}
            {% for name in component.get_own_required_component_names() %},{{name}}({{name}}){% endfor %}
            {}

            //* Components have getters to access the components they require quickly e.g.
            //*   HealthComponent* GetHealthComponent();
            {% for required in component.get_own_required_components() %}
                {{required.get_type_name()}}* Get{{required.get_type_name()}}() {
                    return r_{{required.get_type_name()}};
                }
            {% endfor %}
    };
{% endfor %}

// Entity definitions

{% for entity in entities %}
    class {{entity.get_type_name()}}: public Entity {
            //* The vtables for each entities are statically defined
            static const MessageHandler messageHandlers[];
            static const int componentOffsets[];

        public:
            //* Default constructor
            {{entity.get_type_name()}}(
                {% for (i, attrib) in enumerate(general.common_entity_attributes) %}
                    {% if i != 0 %}, {% endif %} {{attrib.get_declaration()}}
                {% endfor %}
            );
            virtual ~{{entity.get_type_name()}}();

            //* Pointer to the components
            {% for component in entity.get_components() %}
                {{component.get_type_name()}}* {{component.get_variable_name()}};
            {% endfor %}
    };
{% endfor %}

#endif //COMPONENTS_H_
