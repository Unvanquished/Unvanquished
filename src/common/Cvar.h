/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "../engine/qcommon/q_shared.h"
#include "String.h"
#include <string>
#include <functional>

#ifndef COMMON_CVAR_H_
#define COMMON_CVAR_H_

namespace Cvar {

    //TODO more doc

    /*
     * Cvars can have different flags that trigger specific behavior.
     */
    enum {
        NONE       = 0,
        ARCHIVE    = BIT(0), // The cvar is saved to the configuration file
        USERINFO   = BIT(1), // The cvar is sent to the server as part of the client state
        SERVERINFO = BIT(2), // The cvar is send to the client as part of the server state
        SYSTEMINFO = BIT(3), // ???
        ROM        = BIT(6), // The cvar cannot be changed by the user
        CHEAT      = BIT(9)  // The cvar is a cheat and should stay at its default value on pure servers.
    };

    // Internal to the Cvar system
    struct OnValueChangedResult {
        bool success;
        std::string description;
    };

    /*
     * All cvars created by the code inherit from this class although most of the time you'll
     * want to use Cvar::Cvar. It is basically a callback for hen the value of the cvar changes.
     * A single CvarProxy can be registered for a given cvar name.
     */
    class CvarProxy {
        public:
            CvarProxy(std::string name, std::string description, int flags, std::string defaultValue);

            // Called when the value of the cvar changes, returns success=true if the new value
            // is valid, false otherwise. If true is returned, the description will be the new
            // description of the cvar. If false is returned, the cvar wil keep its old
            // value. And the description will be the description of the problem printed to
            // the user.
            virtual OnValueChangedResult OnValueChanged(Str::StringRef newValue) = 0;

        protected:
            std::string name;

            // Will trigger another OnValueChanged after a roundtrip in the cvar system.
            void SetValue(std::string value);
    };

    /*
     * Cvar::Cvar<T> represents a type-checked cvar of type T. The parsed T can
     * be accessed with .Get() and .Set() will serialize T before setting the value.
     * It is also automatically registered when created so you can write:
     *   static Cvar<bool> my_bool_cvar("my_bool_cvar", "bool - a cvar", Cvar::Archive, false);
     *
     * The functions bool ParseCvarValue(string, T& res), string SerializeCvarValue(T)
     * and string GetCvarTypeName<T>() must be implemented for Cvar<T> to work.
     */
    template<typename T> class Cvar : public CvarProxy{
        public:
            typedef T value_type;

            Cvar(std::string name, std::string description, int flags, value_type defaultValue);

            //Outside code accesses the Cvar value by doing my_cvar.Get() or *my_cvar
            T Get();
            T operator*();

            //Outside code can also change the value but it won't be seen immediately after with a .Get()
            void Set(T newValue);

            //Called by the cvar system when the value is changed
            virtual OnValueChangedResult OnValueChanged(Str::StringRef text);

        protected:
            // Used by classes that extend Cvar<T>
            bool Parse(std::string text, T& value);
            virtual OnValueChangedResult Validate(const T& value);
            virtual std::string GetDescription(Str::StringRef value, Str::StringRef originalDescription);

            T value;
            std::string description;
    };

    /*
     * Cvar::Cvar<T> can be augmented using the following classes
     */

    //TODO do not force people to include functional?
    /*
     * Callback<CvarType> adds a callback that is called with the parsed value when
     * the value changes
     */
    template<typename Base> class Callback : public Base {
        public:
            typedef typename Base::value_type value_type;

            template <typename ... Args>
            Callback(std::string name, std::string description, int flags, value_type, std::function<void(value_type)> callback, Args ... args);

            virtual OnValueChangedResult OnValueChanged(Str::StringRef newValue);

        private:
            std::function<void(value_type)> callback;
    };

    // Implement Cvar<T> for T = bool, int, string
    template<typename T>
    std::string GetCvarTypeName();

    bool ParseCvarValue(std::string value, bool& result);
    std::string SerializeCvarValue(bool value);
    template<>
    std::string GetCvarTypeName<bool>();
    bool ParseCvarValue(std::string value, int& result);
    std::string SerializeCvarValue(int value);
    template<>
    std::string GetCvarTypeName<int>();
    bool ParseCvarValue(std::string value, std::string& result);
    std::string SerializeCvarValue(std::string value);
    template<>
    std::string GetCvarTypeName<std::string>();

    // Implementation of templates

    // Cvar<T>

    template<typename T>
    Cvar<T>::Cvar(std::string name, std::string description, int flags, value_type defaultValue)
    : CvarProxy(std::move(name), GetDescription(SerializeCvarValue(defaultValue), description), flags, SerializeCvarValue(defaultValue)), description(std::move(description)) {
        value = std::move(defaultValue);
    }

    template<typename T>
    T Cvar<T>::Get() {
        return value;
    }

    template<typename T>
    T Cvar<T>::operator*() {
        return this->Get();
    }

    template<typename T>
    void Cvar<T>::Set(T newValue) {
        if (Validate(newValue)) {
            SetValue(SerializeCvarValue(value));
        }
    }

    template<typename T>
    OnValueChangedResult Cvar<T>::OnValueChanged(Str::StringRef text) {
        if (Parse(text, value)) {
            return {true, GetDescription(text, description)};
        } else {
            return {false, Str::Format("value \"%s\" is not of type '%s' as expected", text, GetCvarTypeName<T>())};
        }
    }

    template<typename T>
    bool Cvar<T>::Parse(std::string text, T& value) {
        return ParseCvarValue(std::move(text), value);
    }

    template<typename T>
    OnValueChangedResult Cvar<T>::Validate(const T&) {
        return {true, ""};
    }

    template<typename T>
    std::string Cvar<T>::GetDescription(Str::StringRef value, Str::StringRef originalDescription) {
        return Str::Format("\"%s\" - %s - %s", value, GetCvarTypeName<T>(), originalDescription);
    }


    // Callback<Base>

    template <typename Base>
    template <typename ... Args>
    Callback<Base>::Callback(std::string name, std::string description, int flags, value_type defaultValue, std::function<void(value_type)> callback, Args ... args)
    : Base(std::move(name), std::move(description), flags, std::move(defaultValue), std::forward<Args>(args) ...),
    callback(callback) {
    }

    template <typename Base>
    OnValueChangedResult Callback<Base>::OnValueChanged(Str::StringRef newValue) {
        OnValueChangedResult rec = Base::OnValueChanged(newValue);

        if (rec.success) {
            callback(this->Get());
        }
        return std::move(rec);
    }
}

#endif // COMMON_CVAR_H_
