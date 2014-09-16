//* Message and enum list used to tag each message sent to an entity with its 'type'.
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

// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK
//%L
#ifndef COMPONENTS_H_
#define COMPONENTS_H_
//%L
// /////////// //
// Message IDs //
// /////////// //
//%L
enum {
{% for message in messages %}
	{{message.get_enum_name()}},
{% endfor %}
};
//%L
// //////////////////// //
// Forward declarations //
// //////////////////// //
//%L
class Entity;
//%L
{% for component in components %}
class {{component.get_type_name()}};
{% endfor %}
//%L
/** Message handler declaration. */
typedef void (*MessageHandler)(Entity*, const void*);
//%L
namespace detail {
	template<typename T> struct ComponentPriority;
	{% for component in components %}
	//%L
	template<> struct ComponentPriority<{{component.get_type_name()}}> {
		static const int value = {{component.get_priority()}};
	};
	{% endfor %}
};
//%L
/** Base entity class. */
class Entity {
	public:
		/**
		 * @brief Base entity constructor.
		 * @param messageHandlers Message handler vtable.
		 * @param componentOffsets Component offset vtable.
		 */
		Entity(const MessageHandler* messageHandlers, const int* componentOffsets
		{%- for attrib in general.common_entity_attributes -%}
			, {{attrib.get_declaration()}}
		{%- endfor -%}
		);
		//%L
		/**
		 * @brief Base entity deconstructor.
		 */
		virtual ~Entity();
		//%L
		/**
		 * @brief Generic message dispatcher.
		 * @note Should not be called directly, use message helpers instead.
		 */
		bool SendMessage(int msg, const void* data);
		//%L
		//* Handy functions to send a specific message to the components of Entity e.g. (returns true if the message was handled)
		//*   bool Damage(int amount);
		// /////////////// //
		// Message helpers //
		// /////////////// //
		//%L
		{% for message in messages %}
		bool {{message.name}}({{message.get_function_args()}}); /**< Sends the {{message.name}} message to all interested components. */
		{% endfor %}
		//%L
		/**
		 * @brief Returns a component of this entity, if available.
		 * @tparam T Type of component to ask for.
		 * @return Pointer to component of type T or nullptr.
		 */
		template<typename T> T* Get() {
			int index = detail::ComponentPriority<T>::value;
			int offset = componentOffsets[index];
			if (offset) {
				return (T*) (((char*) this) + offset);
			} else {
				return nullptr;
			}
		}
		//%L
	private:
		/** Message handler vtable. */
		const MessageHandler* messageHandlers;
		//%L
		/** Component offset vtable. */
		const int* componentOffsets;
		//%L
	public:
		// ///////////////////// //
		// Shared public members //
		// ///////////////////// //
		//%L
		{% for attrib in general.common_entity_attributes %}
		{{attrib.get_declaration()}};
		{% endfor %}
};
//%L
// ////////////////////////// //
// Base component definitions //
// ////////////////////////// //
//%L
{% for component in components %}
/** Base class of {{component.get_type_name()}}. */
class {{component.get_base_type_name()}} {
	public:
		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;
		//%L
	protected:
		{% for declaration in component.get_own_param_declarations() %}
		const {{declaration}}; /**< An initialization parameter. */
		{% endfor %}
		{% if component.get_own_param_declarations() %}//%L{% endif %}
		{% for declaration in component.get_own_required_component_declarations() %}
		{{declaration}}; /**< A component of the owning entity that this component depends on. */
		{% endfor %}
		{% if component.get_own_required_component_declarations() %}//%L{% endif %}
	public:
		/**
		 * @brief {{component.get_base_type_name()}} constructor.
		 * @param entity The entity that owns this component.
		{% for param in component.get_param_names() %}
		 * @param {{param}} An initialization parameter.
		{% endfor %}
		{% for required in component.get_required_components() %}
		 * @param r_{{required.get_type_name()}} A {{required.get_type_name()}} instance that this component depends on.
		{% endfor %}
		 */
		{{component.get_base_type_name()}}(Entity& entity
		{%- for declaration in component.get_param_declarations() -%}
			, {{declaration}}
		{%- endfor -%}
		{%- for declaration in component.get_required_component_declarations() -%}
			, {{declaration}}
		{%- endfor -%}
		)
			: entity(entity)
		{%- for name in component.get_own_param_names() -%}
			, {{name}}({{name}})
		{%- endfor -%}
		{%- for name in component.get_own_required_component_names() -%}
			, {{name}}({{name}})
		{%- endfor %}
		{}
		{% for required in component.get_own_required_components() %}
		//%L
		/**
		 * @return A reference to the {{required.get_type_name()}} of the owning entity.
		 */
		{{required.get_type_name()}}& Get{{required.get_type_name()}}() {
			return r_{{required.get_type_name()}};
		}
		{% endfor %}
};
//%L
{% endfor %}

#endif //COMPONENTS_H_

//* vi:ai:filetype=jinja:
