//* Daemon CBSE Source Code
//* Copyright (c) 2014-2015, Daemon Developers
//* All rights reserved.
//*
//* Redistribution and use in source and binary forms, with or without
//* modification, are permitted provided that the following conditions are met:
//*
//* * Redistributions of source code must retain the above copyright notice, this
//*   list of conditions and the following disclaimer.
//*
//* * Redistributions in binary form must reproduce the above copyright notice,
//*   this list of conditions and the following disclaimer in the documentation
//*   and/or other materials provided with the distribution.
//*
//* * Neither the name of Daemon CBSE nor the names of its
//*   contributors may be used to endorse or promote products derived from
//*   this software without specific prior written permission.
//*
//* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
{% if header is defined %}
	/*
	{% for line in header %}
		 * {{line}}
	{% endfor %}
	 */

{% endif %}
// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

/*
 * This file contains:
 *   - Declaration of the base entity.
 *   - Implementations of the base components.
 *   - Helpers to access entities and components.
 */
//* Message enum list used to tag each message sent to an entity with its 'type'.
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

#ifndef CBSE_BACKEND_H_
#define CBSE_BACKEND_H_

#include <set>

#define CBSE_INCLUDE_TYPES_ONLY
#include "../{{files['helper']}}"
#undef CBSE_INCLUDE_TYPES_ONLY

// /////////// //
// Message IDs //
// /////////// //

//* An enum of message IDs used to index the vtable.
enum class EntityMessage {
	{% for message in messages %}
		{{message.get_name()}},
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
using MessageHandler = void (*)(Entity*, const void* /*_data*/);

// //////////////////// //
// Component priorities //
// //////////////////// //

//* This is a trait that declares a compile-time index for components
//* it is used to index the offset table.
namespace detail {
	template<typename T> struct ComponentPriority;
	{% for component in components %}

		template<> struct ComponentPriority<{{component.get_type_name()}}> {
			static const int value = {{component.get_priority()}};
		};
	{% endfor %}
};

// ////////////////////////////// //
// Declaration of the base Entity //
// ////////////////////////////// //

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
		virtual ~Entity() = default;

		// /////////////// //
		// Message helpers //
		// /////////////// //

		//* Member functions to send a specific message to the components of Entity e.g. (returns true if the message was handled)
		//*   bool Damage(int amount);
		{% for message in messages %}
			bool {{message.name}}({{message.get_function_args()}}); /**< Sends the {{message.name}} message to all interested components. */
		{% endfor %}

		/**
		 * @brief Returns a component of this entity, if available.
		 * @tparam T Type of component to ask for.
		 * @return Pointer to component of type T or nullptr.
		 */
		template<typename T> const T* Get() const {
			int index = detail::ComponentPriority<T>::value;
			int offset = componentOffsets[index];
			if (offset) {
				return (const T*) (((char*) this) + offset);
			} else {
				return nullptr;
			}
		}

		/**
		 * @brief Returns a component of this entity, if available.
		 * @tparam T Type of component to ask for.
		 * @return Pointer to component of type T or nullptr.
		 */
		template<typename T> T* Get() {
			return const_cast<T*>(static_cast<const Entity*>(this)->Get<T>());
		}

	private:
		/** Message handler vtable. */
		const MessageHandler* messageHandlers;

		/** Component offset vtable. */
		const int* componentOffsets;

		/**
		 * @brief Generic message dispatcher.
		 * @note Should not be called directly, use message helpers instead.
		 */
		bool SendMessage(EntityMessage msg, const void* data);

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

template<typename C>
class AllComponents {
	public:
		AllComponents(std::set<C*>& all): all(all) {}

		typename std::set<C*>::iterator begin() {
			return all.begin();
		}

		typename std::set<C*>::iterator end() {
			return all.end();
		}

	private:
		std::set<C*>& all;
};

{% for component in components %}
	/** Base class of {{component.get_type_name()}}. */
	class {{component.get_base_type_name()}} {
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
				allSet.insert(reinterpret_cast<{{component.get_type_name()}}*>(this));
			}

			~{{component.get_base_type_name()}}() {
				allSet.erase(reinterpret_cast<{{component.get_type_name()}}*>(this));
			}

			{% for required in component.get_own_required_components() %}
				/**
				 * @return A reference to the {{required.get_type_name()}} of the owning entity.
				 */
				{{required.get_type_name()}}& Get{{required.get_type_name()}}();
				const {{required.get_type_name()}}& Get{{required.get_type_name()}}() const;

			{% endfor %}
			{% for (dependency, firstLevel) in component.get_own_further_dependencies().items() %}
				/**
				 * @return A reference to the {{dependency.get_type_name()}} of the owning entity.
				 */
				{{dependency.get_type_name()}}& Get{{dependency.get_type_name()}}();
				const {{dependency.get_type_name()}}& Get{{dependency.get_type_name()}}() const;

			{% endfor %}
			/** A reference to the entity that owns the component instance. Allows sending back messages. */
			Entity& entity;

			static AllComponents<{{component.get_type_name()}}> GetAll() {
				return {allSet};
			}

		protected:
			{% for declaration in component.get_own_param_declarations() %}
				{{declaration}}; /**< An initialization parameter. */
			{% endfor %}

		private:
			{% for declaration in component.get_own_required_component_declarations() %}
				{{declaration}}; /**< A component of the owning entity that this component depends on. */
			{% endfor %}

			static std::set<{{component.get_type_name()}}*> allSet;
	};

{% endfor %}

// ////////////////////////// //
// Definitions of ForEntities //
// ////////////////////////// //

template <typename Component> bool HasComponents(const Entity& ent) {
    return ent.Get<Component>() != nullptr;
}

template <typename Component1, typename Component2, typename ... Components> bool HasComponents(const Entity& ent) {
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

#endif // CBSE_BACKEND_H_
//* vi:ai:ts=4
