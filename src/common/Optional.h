/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef COMMON_OPTIONAL_H_
#define COMMON_OPTIONAL_H_

#include "../engine/qcommon/q_shared.h"
#include <type_traits>
#include <initializer_list>
#include <memory>
#include <stdexcept>

// optional class based on http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3793.html
// The implementation is heavily based on experimental/optional in libc++, but
// rewritten to work with older compilers.

namespace Opt {

struct nullopt_t {
	explicit nullopt_t(int) {}
};
const nullopt_t nullopt{0};

struct inplace_t {};
const inplace_t inplace;

class bad_optional_access: public std::logic_error
{
public:
	explicit bad_optional_access(const std::string& __arg)
		: logic_error(__arg) {}
	explicit bad_optional_access(const char* __arg)
		: logic_error(__arg) {}
};

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
			init(*other);
	}
	optional(optional&& other) NOEXCEPT_IF(std::is_nothrow_move_constructible<T>::value)
		: engaged(other.engaged)
	{
		if (other)
			init(std::move(*other));
	}
	optional(const value_type& value)
		: engaged(true)
	{
		init(value);
	}
	optional(value_type&& value)
		: engaged(true)
	{
		init(std::move(value));
	}
	~optional()
	{
		if (engaged)
			destroy();
	}

	template<typename... Args, typename = typename std::enable_if<std::is_constructible<value_type, Args...>::value>::type>
	explicit optional(inplace_t, Args&&... args)
		: engaged(true)
	{
		init(std::forward<Args>(args)...);
	}
	template<typename U, typename... Args, typename = typename std::enable_if<std::is_constructible<value_type, std::initializer_list<U>&, Args...>::value>::type>
	explicit optional(inplace_t, std::initializer_list<U> il, Args&&... args)
		: engaged(true)
	{
		init(il, std::forward<Args>(args)...);
	}

	optional& operator=(nullopt_t)
	{
		if (engaged) {
			engaged = false;
			destroy();
		}
		return *this;
	}
	optional& operator=(const optional& other)
	{
		if (engaged == other.engaged) {
			if (engaged)
				assign(*other);
		} else {
			if (engaged)
				destroy();
			else
				init(*other);
			engaged = other.engaged;
		}
		return *this;
	}
	optional& operator=(optional&& other) NOEXCEPT_IF(std::is_nothrow_move_assignable<T>::value && std::is_nothrow_move_constructible<T>::value)
	{
		if (engaged == other.engaged) {
			if (engaged)
				assign(std::move(*other));
		} else {
			if (engaged)
				destroy();
			else
				init(std::move(*other));
			engaged = other.engaged;
		}
		return *this;
	}
	template<typename U, typename = typename std::enable_if<std::is_same<typename std::decay<U>::type, value_type>::value &&
	                                                        std::is_constructible<value_type, U>::value &&
	                                                        std::is_assignable<value_type&, U>::value>::type>
	optional& operator=(U&& value)
	{
		if (engaged) {
			assign(std::forward<U>(value));
		} else {
			init(std::forward<U>(value));
			engaged = true;
		}
		return *this;
	}

	template<typename... Args, typename = typename std::enable_if<std::is_constructible<value_type, Args...>::value>::type>
	void emplace(Args&&... args)
	{
		if (engaged)
			destroy();
		init(std::forward<Args>(args)...);
		engaged = true;
	}
	template<typename U, typename... Args, typename = typename std::enable_if<std::is_constructible<value_type, std::initializer_list<U>&, Args...>::value>::type>
	void emplace(std::initializer_list<U> il, Args&&... args)
	{
		if (engaged)
			destroy();
		init(il, std::forward<Args>(args)...);
		engaged = true;
	}

	void swap(optional& other) NOEXCEPT_IF(std::is_nothrow_move_constructible<T>::value && NOEXCEPT_EXPR(swap(std::declval<T&>(), std::declval<T&>())))
	{
		using std::swap;
		if (engaged == other.engaged) {
			if (engaged)
				swap(get_value(), *other);
		} else {
			if (engaged) {
				other.init(std::move(get_value()));
				destroy();
			} else {
				init(std::move(*other));
				other.destroy();
			}
			swap(engaged, other.engaged);
		}
	}

	value_type* operator->()
	{
		return std::addressof(get_value());
	}
	const value_type* operator->() const
	{
		return std::addressof(get_value());
	}
	value_type& operator*()
	{
		return get_value();
	}
	const value_type& operator*() const
	{
		return get_value();
	}

	explicit operator bool() const
	{
		return engaged;
	}

	value_type& value()
	{
		if (!engaged)
			throw bad_optional_access("optional<T>::value: not engaged");
		return get_value();
	}
	const value_type& value() const
	{
		if (!engaged)
			throw bad_optional_access("optional<T>::value: not engaged");
		return get_value();
	}

	// Because we don't have rvalue references for *this, split into 2 functions
	template<typename U> value_type value_or(U&& value) const
	{
		return engaged ? get_value() : static_cast<value_type>(std::forward<U>(value));
	}
	template<typename U> value_type move_or(U&& value)
	{
		return engaged ? std::move(get_value()) : static_cast<value_type>(std::forward<U>(value));
	}

private:
	typename std::aligned_storage<sizeof(value_type), std::alignment_of<value_type>::value>::type data;
	bool engaged;

	template<typename... Args> void init(Args&&... args)
	{
		new(&data) value_type(std::forward<Args>(args)...);
	}
	template<typename Arg> void assign(Arg&& arg)
	{
		get_value() = std::forward<Arg>(arg);
	}
	void destroy()
	{
		get_value().~value_type();
	}
	T& get_value()
	{
		return *reinterpret_cast<value_type*>(&data);
	}
	const T& get_value() const
	{
		return *reinterpret_cast<const value_type*>(&data);
	}
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
template<typename T> struct hash<Opt::optional<T>> {
	size_t operator()(const Opt::optional<T>& value) const
	{
		return static_cast<bool>(value) ? std::hash<T>()(*value) : 0;
	}
};

} // namespace std

#endif // COMMON_OPTIONAL_H_
