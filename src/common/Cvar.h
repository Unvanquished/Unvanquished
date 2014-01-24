/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#include "../engine/qcommon/q_shared.h"
#include "String.h"
#include <string>
#include <functional>

#ifndef COMMON_CVAR_H_
#define COMMON_CVAR_H_

namespace Cvar {

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
     * want to use Cvar::Cvar. It is basically a callback for when the value of the cvar changes.
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
     *   static Cvar<bool> my_bool_cvar("my_bool_cvar", "bool - a cvar", Cvar::ARCHIVE, false);
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
            // Implemented by subtypes to validate the value of the cvar (for example for Range)
            virtual OnValueChangedResult Validate(const T& value);
            // Returns the new description of the cvar given the current value and the description
            // given at the creation of the cvar.
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
            Callback(std::string name, std::string description, int flags, value_type defaultValue, std::function<void(value_type)> callback, Args&& ... args);

            virtual OnValueChangedResult OnValueChanged(Str::StringRef newValue);

        private:
            std::function<void(value_type)> callback;
    };

    /*
     * Modified<CvarType> allow to query atomically if the cvar has been modified and the new value.
     * (resets the modified flag to false)
     */

    template<typename Base> class Modified : public Base {
        public:
            typedef typename Base::value_type value_type;

            template<typename ... Args>
            Modified(std::string name, std::string description, int flags, value_type defaultValue, Args ... args);

            virtual OnValueChangedResult OnValueChanged(Str::StringRef newValue);

            //TODO change it when we have optional
            bool GetModifiedValue(value_type& value);

        private:
            bool modified;
    };

    /*
     * Range<CvarType> forces the cvar to stay in a range of values.
     */

    template<typename Base> class Range : public Base {
        public:
            typedef typename Base::value_type value_type;

            template<typename ... Args>
            Range(std::string name, std::string description, int flags, value_type defaultValue, value_type min, value_type max, Args ... args);

        private:
            virtual OnValueChangedResult Validate(const value_type& value);
            virtual std::string GetDescription(Str::StringRef value, Str::StringRef originalDescription);

            value_type min;
            value_type max;
    };

    // Implement Cvar<T> for T = bool, int, string
    template<typename T>
    std::string GetCvarTypeName();

    bool ParseCvarValue(Str::StringRef value, bool& result);
    std::string SerializeCvarValue(bool value);
    template<>
    std::string GetCvarTypeName<bool>();
    bool ParseCvarValue(Str::StringRef value, int& result);
    std::string SerializeCvarValue(int value);
    template<>
    std::string GetCvarTypeName<int>();
    bool ParseCvarValue(Str::StringRef value, float& result);
    std::string SerializeCvarValue(float value);
    template<>
    std::string GetCvarTypeName<float>();
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
        if (Validate(newValue).success) {
            SetValue(SerializeCvarValue(newValue));
        }
    }

    template<typename T>
    OnValueChangedResult Cvar<T>::OnValueChanged(Str::StringRef text) {
        if (Parse(text, value)) {
            OnValueChangedResult validationResult = Validate(value);
            if (validationResult.success) {
                return {true, GetDescription(text, description)};
            } else {
                return validationResult;
            }
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
    Callback<Base>::Callback(std::string name, std::string description, int flags, value_type defaultValue, std::function<void(value_type)> callback, Args&& ... args)
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

    // Modified<Base>

    template <typename Base>
    template <typename ... Args>
    Modified<Base>::Modified(std::string name, std::string description, int flags, value_type defaultValue, Args ... args)
    : Base(std::move(name), std::move(description), flags, std::move(defaultValue), std::forward<Args>(args) ...), modified(false) {
    }

    template <typename Base>
    OnValueChangedResult Modified<Base>::OnValueChanged(Str::StringRef newValue) {
        OnValueChangedResult rec = Base::OnValueChanged(newValue);

        if (rec.success) {
            modified = true;
        }
        return std::move(rec);
    }

    template<typename Base>
    bool Modified<Base>::GetModifiedValue(value_type& value) {
        value = this->Get();
        bool res = modified;
        modified = false;
        return res;
    }

    // Range<Base>

    template <typename Base>
    template <typename ... Args>
    Range<Base>::Range(std::string name, std::string description, int flags, value_type defaultValue, value_type min, value_type max, Args ... args)
    : Base(std::move(name), std::move(description), flags, std::move(defaultValue), std::forward<Args>(args) ...), min(min), max(max) {
    }

    template <typename Base>
    OnValueChangedResult Range<Base>::Validate(const value_type& value) {
        bool inBounds = value <= max and value >= min;
        if (inBounds) {
            return {true, ""};
        } else {
            return {false, Str::Format("%s is not between %s and %s", SerializeCvarValue(value), SerializeCvarValue(min), SerializeCvarValue(max))};
        }
    }

    template <typename Base>
    std::string Range<Base>::GetDescription(Str::StringRef value, Str::StringRef originalDescription) {
        return Base::GetDescription(value, Str::Format("%s - between %s and %s", originalDescription, SerializeCvarValue(min), SerializeCvarValue(max)));
    }

}

#endif // COMMON_CVAR_H_
