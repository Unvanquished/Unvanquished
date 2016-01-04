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

#ifndef COMMON_OPTIONAL_H_
#define COMMON_OPTIONAL_H_

// optional class based on http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3793.html
// The implementation is heavily based on experimental/optional in libc++, but
// rewritten to work with older compilers.

namespace Util {

struct nullopt_t {
	explicit nullopt_t(int) {}
};
const nullopt_t nullopt{0};

struct inplace_t {};
const inplace_t inplace{};

class bad_optional_access: public std::logic_error
{
public:
	explicit bad_optional_access(const std::string& __arg)
		: logic_error(__arg) {}
	explicit bad_optional_access(const char* __arg)
		: logic_error(__arg) {}
};

// Missing type traits
namespace detail {

// GCC 4.6 is missing some type traits
#ifdef LIBSTDCXX_BROKEN_CXX11
typedef char one[1];
typedef char two[2];
template<typename T, typename Arg, typename = decltype(std::declval<T>() = std::declval<Arg>())>
two& is_assignable_helper(int);
template<typename T, typename Arg>
one& is_assignable_helper(...);

template<typename T, typename Arg> struct is_assignable: public std::integral_constant<bool, sizeof(is_assignable_helper<T, Arg>(0)) - 1> {};

template<typename T> struct is_nothrow_move_assignable: public std::integral_constant<bool, NOEXCEPT_EXPR(std::declval<T&>() = std::declval<T>())> {};
template<typename T> struct is_nothrow_move_constructible: public std::integral_constant<bool, NOEXCEPT_EXPR(T(std::declval<T>()))> {};

#else
using std::is_assignable;
using std::is_nothrow_move_assignable;
using std::is_nothrow_move_constructible;
#endif

using std::swap;
template<typename T> struct is_nothrow_swappable: public std::integral_constant<bool, NOEXCEPT_EXPR(swap(std::declval<T&>(), std::declval<T&>()))> {};

} // namespace detail

template<typename T> class optional {
public:
	typedef T value_type;

	optional()
		: engaged(false) {}
	optional(nullopt_t)
		: engaged(false) {}
	optional(const optional& other)
		: engaged(other.engaged)
	{
		if (other)
			storage.construct(*other);
	}
	optional(optional&& other) NOEXCEPT_IF(detail::is_nothrow_move_constructible<value_type>::value)
		: engaged(other.engaged)
	{
		if (other)
			storage.construct(std::move(*other));
	}
	optional(const value_type& value)
		: engaged(true)
	{
		storage.construct(value);
	}
	optional(value_type&& value)
		: engaged(true)
	{
		storage.construct(std::move(value));
	}
	~optional()
	{
		if (engaged)
			storage.destroy();
	}

	template<typename... Args, typename = typename std::enable_if<std::is_constructible<value_type, Args...>::value>::type>
	explicit optional(inplace_t, Args&&... args)
		: engaged(true)
	{
		storage.construct(std::forward<Args>(args)...);
	}
	template<typename U, typename... Args, typename = typename std::enable_if<std::is_constructible<value_type, std::initializer_list<U>&, Args...>::value>::type>
	explicit optional(inplace_t, std::initializer_list<U> il, Args&&... args)
		: engaged(true)
	{
		storage.construct(il, std::forward<Args>(args)...);
	}

	optional& operator=(nullopt_t)
	{
		if (engaged) {
			engaged = false;
			storage.destroy();
		}
		return *this;
	}
	optional& operator=(const optional& other)
	{
		if (engaged == other.engaged) {
			if (engaged)
				storage.assign(*other);
		} else {
			if (engaged)
				storage.destroy();
			else
				storage.construct(*other);
			engaged = other.engaged;
		}
		return *this;
	}
	optional& operator=(optional&& other) NOEXCEPT_IF(detail::is_nothrow_move_assignable<value_type>::value && detail::is_nothrow_move_constructible<value_type>::value)
	{
		if (engaged == other.engaged) {
			if (engaged)
				storage.assign(std::move(*other));
		} else {
			if (engaged)
				storage.destroy();
			else
				storage.construct(std::move(*other));
			engaged = other.engaged;
		}
		return *this;
	}
	template<typename U, typename = typename std::enable_if<std::is_same<typename std::decay<U>::type, value_type>::value &&
	                                                        std::is_constructible<value_type, U>::value &&
	                                                        detail::is_assignable<value_type&, U>::value>::type>
	optional& operator=(U&& value)
	{
		if (engaged) {
			storage.assign(std::forward<U>(value));
		} else {
			storage.construct(std::forward<U>(value));
			engaged = true;
		}
		return *this;
	}

	template<typename... Args, typename = typename std::enable_if<std::is_constructible<value_type, Args...>::value>::type>
	void emplace(Args&&... args)
	{
		if (engaged)
			storage.destroy();
		storage.construct(std::forward<Args>(args)...);
		engaged = true;
	}
	template<typename U, typename... Args, typename = typename std::enable_if<std::is_constructible<value_type, std::initializer_list<U>&, Args...>::value>::type>
	void emplace(std::initializer_list<U> il, Args&&... args)
	{
		if (engaged)
			storage.destroy();
		storage.construct(il, std::forward<Args>(args)...);
		engaged = true;
	}

	void swap(optional& other) NOEXCEPT_IF(detail::is_nothrow_move_constructible<value_type>::value && detail::is_nothrow_swappable<value_type>::value)
	{
		using std::swap;
		if (engaged == other.engaged) {
			if (engaged)
				swap(storage.get(), *other);
		} else {
			if (engaged) {
				other.storage.construct(std::move(storage.get()));
				storage.destroy();
			} else {
				storage.construct(std::move(*other));
				other.storage.destroy();
			}
			swap(engaged, other.engaged);
		}
	}

	value_type* operator->()
	{
		return std::addressof(storage.get());
	}
	const value_type* operator->() const
	{
		return std::addressof(storage.get());
	}
	value_type& operator*()
	{
		return storage.get();
	}
	const value_type& operator*() const
	{
		return storage.get();
	}

	explicit operator bool() const
	{
		return engaged;
	}

	value_type& value()
	{
		if (!engaged)
			throw bad_optional_access("optional<T>::value: not engaged");
		return storage.get();
	}
	const value_type& value() const
	{
		if (!engaged)
			throw bad_optional_access("optional<T>::value: not engaged");
		return storage.get();
	}

	// Because we don't have rvalue references for *this, split into 2 functions
	template<typename U> value_type value_or(U&& value) const
	{
		return engaged ? storage.get() : static_cast<value_type>(std::forward<U>(value));
	}
	template<typename U> value_type move_or(U&& value)
	{
		return engaged ? std::move(storage.get()) : static_cast<value_type>(std::forward<U>(value));
	}

private:
	uninitialized<T> storage;
	bool engaged;
};

template<typename T> void swap(optional<T>& a, optional<T>& b) NOEXCEPT_IF(NOEXCEPT_EXPR(a.swap(b)))
{
	a.swap(b);
}

template<typename T> optional<typename std::decay<T>::type> make_optional(T&& value)
{
	return optional<typename std::decay<T>::type>(std::forward<T>(value));
}

template<typename T> bool operator==(const optional<T>& a, const optional<T>& b)
{
	if (static_cast<bool>(a) != static_cast<bool>(b))
		return false;
	if (!static_cast<bool>(a))
		return true;
	return *a == *b;
}
template<typename T> bool operator!=(const optional<T>& a, const optional<T>& b)
{
	return !(a == b);
}
template<typename T> bool operator<(const optional<T>& a, const optional<T>& b)
{
	if (!static_cast<bool>(b))
		return false;
	if (!static_cast<bool>(a))
		return true;
	return *a < *b;
}
template<typename T> bool operator>=(const optional<T>& a, const optional<T>& b)
{
	return !(a < b);
}
template<typename T> bool operator<=(const optional<T>& a, const optional<T>& b)
{
	return !(b < a);
}
template<typename T> bool operator>(const optional<T>& a, const optional<T>& b)
{
	return b < a;
}

template<typename T> bool operator==(const optional<T>& a, nullopt_t)
{
	return !static_cast<bool>(a);
}
template<typename T> bool operator==(nullopt_t, const optional<T>& b)
{
	return !static_cast<bool>(b);
}
template<typename T> bool operator!=(const optional<T>& a, nullopt_t)
{
	return !(a == nullopt);
}
template<typename T> bool operator!=(nullopt_t, const optional<T>& b)
{
	return !(nullopt == b);
}
template<typename T> bool operator<(const optional<T>&, nullopt_t)
{
	return false;
}
template<typename T> bool operator<(nullopt_t, const optional<T>& b)
{
	return static_cast<bool>(b);
}
template<typename T> bool operator>=(const optional<T>& a, nullopt_t)
{
	return !(a < nullopt);
}
template<typename T> bool operator>=(nullopt_t, const optional<T>& b)
{
	return !(nullopt < b);
}
template<typename T> bool operator<=(const optional<T>& a, nullopt_t)
{
	return !(nullopt < a);
}
template<typename T> bool operator<=(nullopt_t, const optional<T>& b)
{
	return !(b < nullopt);
}
template<typename T> bool operator>(const optional<T>& a, nullopt_t)
{
	return nullopt < a;
}
template<typename T> bool operator>(nullopt_t, const optional<T>& b)
{
	return b < nullopt;
}

template<typename T> bool operator==(const optional<T>& a, const T& b)
{
	return a ? *a == b : false;
}
template<typename T> bool operator==(const T& a, const optional<T>& b)
{
	return b ? a == *b : false;
}
template<typename T> bool operator!=(const optional<T>& a, const T& b)
{
	return !(a == b);
}
template<typename T> bool operator!=(const T& a, const optional<T>& b)
{
	return !(a == b);
}
template<typename T> bool operator<(const optional<T>& a, const T& b)
{
	return a ? *a < b : true;
}
template<typename T> bool operator<(const T& a, const optional<T>& b)
{
	return b ? a < *b : false;
}
template<typename T> bool operator>=(const optional<T>& a, const T& b)
{
	return !(a < b);
}
template<typename T> bool operator>=(const T& a, const optional<T>& b)
{
	return !(a < b);
}
template<typename T> bool operator<=(const optional<T>& a, const T& b)
{
	return !(b < a);
}
template<typename T> bool operator<=(const T& a, const optional<T>& b)
{
	return !(b < a);
}
template<typename T> bool operator>(const optional<T>& a, const T& b)
{
	return b < a;
}
template<typename T> bool operator>(const T& a, const optional<T>& b)
{
	return b < a;
}

} // namespace Opt

namespace std {

// Hash support for optional
template<typename T> struct hash<Util::optional<T>> {
	size_t operator()(const Util::optional<T>& value) const
	{
		return static_cast<bool>(value) ? std::hash<T>()(*value) : 0;
	}
};

} // namespace Util

#endif // COMMON_OPTIONAL_H_
