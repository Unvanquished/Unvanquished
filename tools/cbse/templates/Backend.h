// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

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

#ifndef COMPONENTS_H_
#define COMPONENTS_H_

// /////////// //
// Message IDs //
// /////////// //

enum {
	{% for message in messages %}
		{{message.get_enum_name()}},
	{% endfor %}
};

// //////////////////// //
// Forward declarations //
// //////////////////// //

class Entity;

{% for component in components %}
	class {{component.get_type_name()}};
{% endfor %}

/** Message handler declaration. */
typedef void (*MessageHandler)(Entity*, const void*);

namespace detail {
	template<typename T> struct ComponentPriority;
	{% for component in components %}

		template<> struct ComponentPriority<{{component.get_type_name()}}> {
			static const int value = {{component.get_priority()}};
		};
	{% endfor %}
};

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

		/**
		 * @brief Base entity deconstructor.
		 */
		virtual ~Entity();

		/**
		 * @brief Generic message dispatcher.
		 * @note Should not be called directly, use message helpers instead.
		 */
		bool SendMessage(int msg, const void* data);

		//* Handy functions to send a specific message to the components of Entity e.g. (returns true if the message was handled)
		//*   bool Damage(int amount);
		// /////////////// //
		// Message helpers //
		// /////////////// //

		{% for message in messages %}
			bool {{message.name}}({{message.get_function_args()}}); /**< Sends the {{message.name}} message to all interested components. */
		{% endfor %}

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

	private:
		/** Message handler vtable. */
		const MessageHandler* messageHandlers;

		/** Component offset vtable. */
		const int* componentOffsets;

	public:
		// ///////////////////// //
		// Shared public members //
		// ///////////////////// //

		{% for attrib in general.common_entity_attributes %}
			{{attrib.get_declaration()}};
		{% endfor %}
};

// ////////////////////////// //
// Base component definitions //
// ////////////////////////// //
//* TODO: Move to Components.h

template<typename C> class AllComponents {
		std::set<C*>& all;

	public:
		AllComponents(std::set<C*>& all): all(all) {}

		typename std::set<C*>::iterator begin() {
			return all.begin();
		}

		typename std::set<C*>::iterator end() {
			return all.end();
		}
};

{% for component in components %}
	/** Base class of {{component.get_type_name()}}. */
	class {{component.get_base_type_name()}} {
		public:
			/** A reference to the entity that owns the component instance. Allows sending back messages. */
			Entity& entity;

		private:
			static std::set<{{component.get_type_name()}}*> allSet;

		public:
			static AllComponents<{{component.get_type_name()}}> GetAll() {
				return {allSet};
			}

		protected:
			{% for declaration in component.get_own_param_declarations() %}
				const {{declaration}}; /**< An initialization parameter. */
			{% endfor %}
			{% for declaration in component.get_own_required_component_declarations() %}
				{{declaration}}; /**< A component of the owning entity that this component depends on. */
			{% endfor %}
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
			{%- endfor -%}
			{
				allSet.insert(({{component.get_type_name()}}*)((char*) this - (char*) ({{component.get_base_type_name()}}*) ({{component.get_type_name()}}*) nullptr));
			}

			~{{component.get_base_type_name()}}() {
				allSet.erase(({{component.get_type_name()}}*)((char*) this - (char*) ({{component.get_base_type_name()}}*) ({{component.get_type_name()}}*) nullptr));
			}

			{% for required in component.get_own_required_components() %}

				/**
				 * @return A reference to the {{required.get_type_name()}} of the owning entity.
				 */
				{{required.get_type_name()}}& Get{{required.get_type_name()}}() {
					return r_{{required.get_type_name()}};
				}
			{% endfor %}
	};

{% endfor %}

// ////////////////////////// //
// Definitions of ForEntities //
// ////////////////////////// //

template <typename Component1> bool HasComponents(Entity& ent) {
    return ent.Get<Component1>() != nullptr;
}

template <typename Component1, typename Component2, typename ... Components> bool HasComponents(Entity& ent) {
    return HasComponents<Component1>(ent) && HasComponents<Component2, Components...>(ent);
}

template <typename Component1, typename ... Components, typename FuncType>
void ForEntities(FuncType f) {
    for(auto* component1: Component1::GetAll()) {
        Entity& ent = component1->entity;

        if (HasComponents<Component1, Components...>(ent)) {
            f(ent, *component1, *ent.Get<Components>()...);
        }
    }
}

#endif //COMPONENTS_H_

//* vi:ai:ts=4:filetype=jinja
