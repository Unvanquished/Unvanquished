/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

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
        SERVERINFO = BIT(2), // The cvar is sent to the client when doing server status request and in a config string (mostly for mapname)
        SYSTEMINFO = BIT(3), // The cvar is sent to the client when changed, to synchronize some global game options (for example pmove config)
        ROM        = BIT(6), // The cvar cannot be changed by the user
        TEMPORARY  = BIT(8), // The cvar is temporary and is not to be archived (overrides archive flags)
        CHEAT      = BIT(9), // The cvar is a cheat and should stay at its default value on pure servers.
        USER_ARCHIVE = BIT(14), // The cvar is saved to the configuration file at user request
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
     * When extending CvarProxy you should call Register in the constructor to perform the registration
     * of the proxy. It should be done by the first class of the inheritance chain in order to have the
     * right vtable when the Cvar system tries to validate the value it already has for that cvar.
     */
    class CvarProxy {
        public:
            CvarProxy(std::string name, int flags, std::string defaultValue);

            // Called when the value of the cvar changes, returns success=true if the new value
            // is valid, false otherwise. If true is returned, the description will be the new
            // description of the cvar. If false is returned, the cvar wil keep its old
            // value. And the description will be the description of the problem printed to
            // the user.
            virtual OnValueChangedResult OnValueChanged(Str::StringRef newValue) = 0;

            const std::string& Name() const {
                return name;
            }

        protected:
            std::string name;

            // Will trigger another OnValueChanged after a roundtrip in the cvar system.
            void SetValue(std::string value);
            bool Register(std::string description);

        private:
            int flags;
            std::string defaultValue;
    };

    struct NoRegisterTag {};

    /*
     * Cvar::Cvar<T> represents a type-checked cvar of type T. The parsed T can
     * be accessed with .Get() and .Set() will serialize T before setting the value.
     * It is also automatically registered when created so you can write:
     *   static Cvar<bool> my_bool_cvar("my_bool_cvar", "bool - a cvar", Cvar::NONE, false);
     *
     * The functions bool ParseCvarValue(string, T& res), string SerializeCvarValue(T)
     * and string GetCvarTypeName<T>() must be implemented for Cvar<T> to work.
     */
    template<typename T> class Cvar : public CvarProxy{
        public:
            using value_type = T;

            // See the comment near the extension classes about that double constructor.
            Cvar(std::string name, std::string description, int flags, value_type defaultValue);
            Cvar(NoRegisterTag, std::string name, std::string description, int flags, value_type defaultValue);

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
            virtual std::string GetAdditionalDescription();

            void Register();

            T value;
            std::string description;

        private:
            std::string GetDescription();
    };

    /*
     * Cvar::Cvar<T> can be extended using the following classes.
     *
     * If you want to create you own extension you should take care of calling the right constructor
     * of the base class in order to prevent the base class from calling Register (which will be
     * problematic because the vtable isn't set up for your class). To do that, pass to the base
     * class a first argument NoRegisterTag(). Likewise if your class should be extandable, you need to
     * make two constructors: a normal one that calls Register and a one that takes NoRegisterTag without
     * registering.
     */

    //TODO do not force people to include functional?
    /*
     * Callback<CvarType> adds a callback that is called with the parsed value when
     * the value changes
     */
    template<typename Base> class Callback : public Base {
        public:
            using value_type = typename Base::value_type;

            template <typename ... Args>
            Callback(std::string name, std::string description, int flags, value_type defaultValue, std::function<void(value_type)> callback, Args&& ... args);
            template <typename ... Args>
            Callback(NoRegisterTag, std::string name, std::string description, int flags, value_type defaultValue, std::function<void(value_type)> callback, Args&& ... args);

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
            using value_type = typename Base::value_type;

            template<typename ... Args>
            Modified(std::string name, std::string description, int flags, value_type defaultValue, Args ... args);
            template<typename ... Args>
            Modified(NoRegisterTag, std::string name, std::string description, int flags, value_type defaultValue, Args ... args);

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
            using value_type = typename Base::value_type;

            template<typename ... Args>
            Range(std::string name, std::string description, int flags, value_type defaultValue, value_type min, value_type max, Args ... args);
            template<typename ... Args>
            Range(NoRegisterTag, std::string name, std::string description, int flags, value_type defaultValue, value_type min, value_type max, Args ... args);

        private:
            virtual OnValueChangedResult Validate(const value_type& value);
            virtual std::string GetAdditionalDescription();

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
    std::string SerializeCvarValue(std::size_t value);
    template<>
    std::string GetCvarTypeName<std::size_t>();
    bool ParseCvarValue(Str::StringRef value, std::size_t& result);


    // Engine calls available everywhere

    bool Register(CvarProxy* proxy, const std::string& name, std::string description, int flags, const std::string& defaultValue);
    std::string GetValue(const std::string& cvarName);
    void SetValue(const std::string& cvarName, const std::string& value);
    bool AddFlags(const std::string& cvarName, int flags);

    // Implementation of templates

    // Cvar<T>

    template<typename T>
    Cvar<T>::Cvar(std::string name, std::string description, int flags, value_type defaultValue)
    : CvarProxy(std::move(name), flags, SerializeCvarValue(defaultValue)), description(std::move(description)) {
        value = std::move(defaultValue);
        Register();
    }

    template<typename T>
    Cvar<T>::Cvar(NoRegisterTag, std::string name, std::string description, int flags, value_type defaultValue)
    : CvarProxy(std::move(name), flags, SerializeCvarValue(defaultValue)), description(std::move(description)) {
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
                return OnValueChangedResult{true, GetDescription()};
            } else {
                return validationResult;
            }
        } else {
            return OnValueChangedResult{false, Str::Format("value \"%s\" is not of type '%s' as expected", text, GetCvarTypeName<T>())};
        }
    }

    template<typename T>
    bool Cvar<T>::Parse(std::string text, T& value) {
        return ParseCvarValue(std::move(text), value);
    }

    template<typename T>
    OnValueChangedResult Cvar<T>::Validate(const T&) {
        return OnValueChangedResult{true, ""};
    }

    template<typename T>
    std::string Cvar<T>::GetAdditionalDescription() {
        return "";
    }

    template<typename T>
    void Cvar<T>::Register() {
        CvarProxy::Register(GetDescription());
    }

    template<typename T>
    std::string Cvar<T>::GetDescription() {
        return Str::Format("%s - %s%s", GetCvarTypeName<T>(), description, GetAdditionalDescription());
    }

    // Callback<Base>

    template <typename Base>
    template <typename ... Args>
    Callback<Base>::Callback(std::string name, std::string description, int flags, value_type defaultValue, std::function<void(value_type)> callback, Args&& ... args)
    : Base(NoRegisterTag(), std::move(name), std::move(description), flags, std::move(defaultValue), std::forward<Args>(args) ...),
    callback(callback) {
        Cvar<value_type>::Register();
    }

    template <typename Base>
    template <typename ... Args>
    Callback<Base>::Callback(NoRegisterTag, std::string name, std::string description, int flags, value_type defaultValue, std::function<void(value_type)> callback, Args&& ... args)
    : Base(NoRegisterTag(), std::move(name), std::move(description), flags, std::move(defaultValue), std::forward<Args>(args) ...),
    callback(callback) {
    }

    template <typename Base>
    OnValueChangedResult Callback<Base>::OnValueChanged(Str::StringRef newValue) {
        OnValueChangedResult rec = Base::OnValueChanged(newValue);

        if (rec.success) {
            callback(this->Get());
        }
        return rec;
    }

    // Modified<Base>

    template <typename Base>
    template <typename ... Args>
    Modified<Base>::Modified(std::string name, std::string description, int flags, value_type defaultValue, Args ... args)
    : Base(NoRegisterTag(), std::move(name), std::move(description), flags, std::move(defaultValue), std::forward<Args>(args) ...), modified(false) {
        Cvar<value_type>::Register();
    }

    template <typename Base>
    template <typename ... Args>
    Modified<Base>::Modified(NoRegisterTag, std::string name, std::string description, int flags, value_type defaultValue, Args ... args)
    : Base(NoRegisterTag(), std::move(name), std::move(description), flags, std::move(defaultValue), std::forward<Args>(args) ...), modified(false) {
    }

    template <typename Base>
    OnValueChangedResult Modified<Base>::OnValueChanged(Str::StringRef newValue) {
        OnValueChangedResult rec = Base::OnValueChanged(newValue);

        if (rec.success) {
            modified = true;
        }
        return rec;
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
    : Base(NoRegisterTag(), std::move(name), std::move(description), flags, std::move(defaultValue), std::forward<Args>(args) ...), min(min), max(max) {
        Cvar<value_type>::Register();
    }

    template <typename Base>
    template <typename ... Args>
    Range<Base>::Range(NoRegisterTag, std::string name, std::string description, int flags, value_type defaultValue, value_type min, value_type max, Args ... args)
    : Base(NoRegisterTag(), std::move(name), std::move(description), flags, std::move(defaultValue), std::forward<Args>(args) ...), min(min), max(max) {
    }

    template <typename Base>
    OnValueChangedResult Range<Base>::Validate(const value_type& value) {
        bool inBounds = value <= max and value >= min;
        if (inBounds) {
            return OnValueChangedResult{true, ""};
        } else {
            return OnValueChangedResult{false, Str::Format("%s is not between %s and %s", SerializeCvarValue(value), SerializeCvarValue(min), SerializeCvarValue(max))};
        }
    }

    template <typename Base>
    std::string Range<Base>::GetAdditionalDescription() {
        return Str::Format(" - between %s and %s", SerializeCvarValue(min), SerializeCvarValue(max));
    }

}

#endif // COMMON_CVAR_H_
