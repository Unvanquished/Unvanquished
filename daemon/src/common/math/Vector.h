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

#ifndef COMMON_VECTOR_H_
#define COMMON_VECTOR_H_

#include <ostream>

namespace Math {

	// Forward declarations
	template<size_t Dimension, typename Type, typename Enable> class VectorOps;
	template<size_t Dimension, typename Type> class Vector;
	template<size_t Dimension> class BoolVectorEq;
	template<size_t Dimension> class BoolVectorNe;

	// Shortcut typedefs
	typedef Vector<2, float> Vec2;
	typedef Vector<3, float> Vec3;
	typedef Vector<4, float> Vec4;
	typedef Vector<2, bool> Vec2b;
	typedef Vector<3, bool> Vec3b;
	typedef Vector<4, bool> Vec4b;

	// Base vector class containing operations common to all vector types
	template<size_t Dimension, typename Type> class VectorBase {
	protected:
		Type data[Dimension];
		template<size_t L, typename T> friend class VectorBase;

		// Vector construction helpers
		template<size_t Index> void FillFromList()
		{
			static_assert(Index == Dimension, "Invalid arguments for vector constructor");
		}
		template<size_t Index, typename... Args> void FillFromList(Type x, Args... args)
		{
			data[Index] = x;
			FillFromList<Index + 1>(args...);
		}
		template<size_t Index, size_t Len, typename... Args> void FillFromList(Vector<Len, Type> vec, Args... args)
		{
			for (size_t i = 0; i < Len; i++)
				data[Index + i] = vec.data[i];
			FillFromList<Index + Len>(args...);
		}
		void FillFromElem(Type x)
		{
			for (size_t i = 0; i < Dimension; i++)
				data[i] = x;
		}

	public:
		// Expose internal data
		const Type* Data() const {return data;}
		Type* Data() {return data;}

		// Swizzle operations
		template<size_t x> Type Swizzle() const;
		template<size_t x, size_t y> Vector<2, Type> Swizzle() const;
		template<size_t x, size_t y, size_t z> Vector<3, Type> Swizzle() const;
		template<size_t x, size_t y, size_t z, size_t w> Vector<4, Type> Swizzle() const;

		// Load/store from pointer
		static Vector<Dimension, Type> Load(const Type* ptr);
		void Store(Type* ptr) const;

		// Apply a function on all elements of the vector
		template<typename Func> Vector<Dimension, typename std::result_of<Func(Type)>::type> Apply(Func func) const;
		template<typename Func, typename Type2> Vector<Dimension, typename std::result_of<Func(Type, Type2)>::type> Apply2(Func func, Vector<Dimension, Type2> other) const;
		template<typename Func, typename Type2, typename Type3> Vector<Dimension, typename std::result_of<Func(Type, Type2, Type3)>::type> Apply3(Func func, Vector<Dimension, Type2> other1, Vector<Dimension, Type3> other2) const;

		// Reduce the elements of the vector to a single value
		template<typename Func> Type Reduce(Type init, Func func);

		// Get an element of the vector
		Type& operator[](size_t index) {return data[index];}
		const Type& operator[](size_t index) const {return data[index];}

		// Convert a vector to a string
		friend std::ostream& operator<<(std::ostream& os, Vector<Dimension, Type> vec)
		{
			os << '(';
			for (size_t i = 0; i < Dimension; i++) {
				if (i != 0)
					os << ", ";
				os << vec.data[i];
			}
			os << ')';
			return os;
		}
	};

	// Type-specific vector operations
	template<size_t Dimension, typename Type, typename Enable = void> class VectorOps: public VectorBase<Dimension, Type> {};
	template<size_t Dimension, typename Type> class VectorOps<Dimension, Type, typename std::enable_if<std::is_floating_point<Type>::value>::type>: public VectorBase<Dimension, Type> {
	public:
		// Comparison operators, == and != can be implicitly converted to bool
		BoolVectorEq<Dimension> operator==(Vector<Dimension, Type> other) const {return BoolVectorEq<Dimension>(this->Apply2([](Type a, Type b) {return a == b;}, other));}
		BoolVectorNe<Dimension> operator!=(Vector<Dimension, Type> other) const {return BoolVectorNe<Dimension>(this->Apply2([](Type a, Type b) {return a != b;}, other));}
		Vector<Dimension, bool> operator>=(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a >= b;}, other);}
		Vector<Dimension, bool> operator<=(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a <= b;}, other);}
		Vector<Dimension, bool> operator>(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a > b;}, other);}
		Vector<Dimension, bool> operator<(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a < b;}, other);}

		// Arithmetic operators
		Vector<Dimension, Type> operator+() const {return this->Apply([](Type a) {return a;});}
		Vector<Dimension, Type> operator-() const {return this->Apply([](Type a) {return -a;});}
		Vector<Dimension, Type> operator+(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a + b;}, other);}
		Vector<Dimension, Type> operator-(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a - b;}, other);}
		Vector<Dimension, Type> operator*(Type x) const {return this->Apply([x](Type a) {return a * x;});}
		Vector<Dimension, Type> operator*(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a * b;}, other);}
		friend Vector<Dimension, Type> operator*(Type x, Vector<Dimension, Type> other) {return other.Apply([x](Type a) {return x * a;});}
		Vector<Dimension, Type> operator/(Type x) const {return this->Apply([x](Type a) {return a / x;});}
		Vector<Dimension, Type> operator/(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a / b;}, other);}
		friend Vector<Dimension, Type> operator/(Type x, Vector<Dimension, Type> other) {return other.Apply([x](Type a) {return x / a;});}

		// Compound arithmetic operators
		Vector<Dimension, Type>& operator+=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) + other;}
		Vector<Dimension, Type>& operator-=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) - other;}
		Vector<Dimension, Type>& operator*=(Type x) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) * x;}
		Vector<Dimension, Type>& operator*=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) * other;}
		Vector<Dimension, Type>& operator/=(Type x) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) / x;}
		Vector<Dimension, Type>& operator/=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) / other;}
	};
	template<size_t Dimension, typename Type> class VectorOps<Dimension, Type, typename std::enable_if<std::is_integral<Type>::value>>: public VectorBase<Dimension, Type> {
	public:
		// Comparison operators, == and != can be implicitly converted to bool
		BoolVectorEq<Dimension> operator==(Vector<Dimension, Type> other) const {return BoolVectorEq<Dimension>(this->Apply2([](Type a, Type b) {return a == b;}, other));}
		BoolVectorNe<Dimension> operator!=(Vector<Dimension, Type> other) const {return BoolVectorNe<Dimension>(this->Apply2([](Type a, Type b) {return a != b;}, other));}
		Vector<Dimension, bool> operator>=(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a >= b;}, other);}
		Vector<Dimension, bool> operator<=(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a <= b;}, other);}
		Vector<Dimension, bool> operator>(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a > b;}, other);}
		Vector<Dimension, bool> operator<(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a < b;}, other);}

		// Arithmetic operators
		Vector<Dimension, Type> operator+() const {return this->Apply([](Type a) {return a;});}
		Vector<Dimension, Type> operator-() const {return this->Apply([](Type a) {return -a;});}
		Vector<Dimension, Type> operator+(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a + b;}, other);}
		Vector<Dimension, Type> operator-(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a - b;}, other);}
		Vector<Dimension, Type> operator*(Type x) const {return this->Apply([x](Type a) {return a * x;});}
		Vector<Dimension, Type> operator*(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a * b;}, other);}
		friend Vector<Dimension, Type> operator*(Type x, Vector<Dimension, Type> other) {return other.Apply([x](Type a) {return x * a;});}
		Vector<Dimension, Type> operator/(Type x) const {return this->Apply([x](Type a) {return a / x;});}
		Vector<Dimension, Type> operator/(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a / b;}, other);}
		friend Vector<Dimension, Type> operator/(Type x, Vector<Dimension, Type> other) {return other.Apply([x](Type a) {return x / a;});}
		Vector<Dimension, Type> operator%(Type x) const {return this->Apply([x](Type a) {return a % x;});}
		Vector<Dimension, Type> operator%(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a % b;}, other);}
		friend Vector<Dimension, Type> operator%(Type x, Vector<Dimension, Type> other) {return other.Apply([x](Type a) {return x % a;});}

		// Compound arithmetic operators
		Vector<Dimension, Type>& operator+=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) + other;}
		Vector<Dimension, Type>& operator-=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) - other;}
		Vector<Dimension, Type>& operator*=(Type x) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) * x;}
		Vector<Dimension, Type>& operator*=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) * other;}
		Vector<Dimension, Type>& operator/=(Type x) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) / x;}
		Vector<Dimension, Type>& operator/=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) / other;}
		Vector<Dimension, Type>& operator%=(Type x) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) % x;}
		Vector<Dimension, Type>& operator%=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) % other;}

		// Bitwise operators
		Vector<Dimension, Type> operator~() const {return this->Apply([](Type a) {return ~a;});}
		Vector<Dimension, Type> operator&(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a & b;}, other);}
		Vector<Dimension, Type> operator|(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a | b;}, other);}
		Vector<Dimension, Type> operator^(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a ^ b;}, other);}
		Vector<Dimension, Type> operator<<(Type x) const {return this->Apply([x](Type a) {return a << x;});}
		Vector<Dimension, Type> operator<<(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a << b;}, other);}
		Vector<Dimension, Type> operator>>(Type x) const {return this->Apply([x](Type a) {return a >> x;});}
		Vector<Dimension, Type> operator>>(Vector<Dimension, Type> other) const {return this->Apply2([](Type a, Type b) {return a >> b;}, other);}

		// Compound bitwise operators
		Vector<Dimension, Type>& operator&=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) & other;}
		Vector<Dimension, Type>& operator|=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) | other;}
		Vector<Dimension, Type>& operator^=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) ^ other;}
		Vector<Dimension, Type>& operator<<=(Type x) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) << x;}
		Vector<Dimension, Type>& operator<<=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) << other;}
		Vector<Dimension, Type>& operator>>=(Type x) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) >> x;}
		Vector<Dimension, Type>& operator>>=(Vector<Dimension, Type> other) {return static_cast<Vector<Dimension, Type>&>(*this) = static_cast<Vector<Dimension, Type>&>(*this) >> other;}
	};
	template<size_t Dimension> class VectorOps<Dimension, bool>: public VectorBase<Dimension, bool> {
	public:
		// Comparison operators, == and != can be implicitly converted to bool
		BoolVectorEq<Dimension> operator==(Vector<Dimension, bool> other) const {return BoolVectorEq<Dimension>(this->Apply2([](bool a, bool b) {return a == b;}, other));}
		BoolVectorNe<Dimension> operator!=(Vector<Dimension, bool> other) const {return BoolVectorNe<Dimension>(this->Apply2([](bool a, bool b) {return a != b;}, other));}

		// Bitwise operators
		Vector<Dimension, bool> operator~() const {return this->Apply([](bool a) {return !a;});}
		Vector<Dimension, bool> operator&(Vector<Dimension, bool> other) const {return this->Apply2([](bool a, bool b) {return a && b;}, other);}
		Vector<Dimension, bool> operator|(Vector<Dimension, bool> other) const {return this->Apply2([](bool a, bool b) {return a || b;}, other);}
		Vector<Dimension, bool> operator^(Vector<Dimension, bool> other) const {return this->Apply2([](bool a, bool b) {return a != b;}, other);}

		// Compound bitwise operators
		Vector<Dimension, bool>& operator&=(Vector<Dimension, bool> other) {return static_cast<Vector<Dimension, bool>&>(*this) = static_cast<Vector<Dimension, bool>&>(*this) & other;}
		Vector<Dimension, bool>& operator|=(Vector<Dimension, bool> other) {return static_cast<Vector<Dimension, bool>&>(*this) = static_cast<Vector<Dimension, bool>&>(*this) | other;}
		Vector<Dimension, bool>& operator^=(Vector<Dimension, bool> other) {return static_cast<Vector<Dimension, bool>&>(*this) = static_cast<Vector<Dimension, bool>&>(*this) ^ other;}

		// Group testing
		bool All() const {return this->Reduce(true, [](bool a, bool b) {return a && b;});}
		bool Any() const {return this->Reduce(false, [](bool a, bool b) {return a || b;});}
		bool NotAll() const {return !All();}
		bool None() const {return !Any();}
	};

	// Size-specific vector operations
	template<size_t Dimension, typename Type> class Vector: public VectorOps<Dimension, Type> {
	public:
		// Constructors
		Vector() = default;
		explicit Vector(Type x)
		{
			this->FillFromElem(x);
		}
		template<typename Type2, typename = typename std::enable_if<std::is_convertible<Type2, Type>::value>::type> explicit Vector(Vector<Dimension, Type2> vec)
		{
			*this = vec.Apply([](Type2 a) {return static_cast<Type>(a);});
		}
		template<typename Arg1, typename Arg2, typename... Args> Vector(Arg1 arg1, Arg2 arg2, Args... args)
		{
			this->template FillFromList<0>(arg1, arg2, args...);
		}
	};
	template<typename Type> class Vector<2, Type>: public VectorOps<2, Type> {
	public:
		// Constructors
		Vector() = default;
		explicit Vector(Type x)
		{
			this->FillFromElem(x);
		}
		template<typename Type2, typename = typename std::enable_if<std::is_convertible<Type2, Type>::value>::type> explicit Vector(Vector<2, Type2> vec)
		{
			*this = vec.Apply([](Type2 a) {return static_cast<Type>(a);});
		}
		template<typename Arg1, typename Arg2, typename... Args> Vector(Arg1 arg1, Arg2 arg2, Args... args)
		{
			this->template FillFromList<0>(arg1, arg2, args...);
		}

		// Swizzle shortcuts
		Type x() const {return this->template Swizzle<0>();}
		Type y() const {return this->template Swizzle<1>();}
		Vector<2, Type> xx() const {return this->template Swizzle<0, 0>();}
		Vector<2, Type> xy() const {return this->template Swizzle<0, 1>();}
		Vector<2, Type> yx() const {return this->template Swizzle<1, 0>();}
		Vector<2, Type> yy() const {return this->template Swizzle<1, 1>();}
		Vector<3, Type> xxx() const {return this->template Swizzle<0, 0, 0>();}
		Vector<3, Type> xxy() const {return this->template Swizzle<0, 0, 1>();}
		Vector<3, Type> xyx() const {return this->template Swizzle<0, 1, 0>();}
		Vector<3, Type> xyy() const {return this->template Swizzle<0, 1, 1>();}
		Vector<3, Type> yxx() const {return this->template Swizzle<1, 0, 0>();}
		Vector<3, Type> yxy() const {return this->template Swizzle<1, 0, 1>();}
		Vector<3, Type> yyx() const {return this->template Swizzle<1, 1, 0>();}
		Vector<3, Type> yyy() const {return this->template Swizzle<1, 1, 1>();}
		Vector<4, Type> xxxx() const {return this->template Swizzle<0, 0, 0, 0>();}
		Vector<4, Type> xxxy() const {return this->template Swizzle<0, 0, 0, 1>();}
		Vector<4, Type> xxyx() const {return this->template Swizzle<0, 0, 1, 0>();}
		Vector<4, Type> xxyy() const {return this->template Swizzle<0, 0, 1, 1>();}
		Vector<4, Type> xyxx() const {return this->template Swizzle<0, 1, 0, 0>();}
		Vector<4, Type> xyxy() const {return this->template Swizzle<0, 1, 0, 1>();}
		Vector<4, Type> xyyx() const {return this->template Swizzle<0, 1, 1, 0>();}
		Vector<4, Type> xyyy() const {return this->template Swizzle<0, 1, 1, 1>();}
		Vector<4, Type> yxxx() const {return this->template Swizzle<1, 0, 0, 0>();}
		Vector<4, Type> yxxy() const {return this->template Swizzle<1, 0, 0, 1>();}
		Vector<4, Type> yxyx() const {return this->template Swizzle<1, 0, 1, 0>();}
		Vector<4, Type> yxyy() const {return this->template Swizzle<1, 0, 1, 1>();}
		Vector<4, Type> yyxx() const {return this->template Swizzle<1, 1, 0, 0>();}
		Vector<4, Type> yyxy() const {return this->template Swizzle<1, 1, 0, 1>();}
		Vector<4, Type> yyyx() const {return this->template Swizzle<1, 1, 1, 0>();}
		Vector<4, Type> yyyy() const {return this->template Swizzle<1, 1, 1, 1>();}
	};
	template<typename Type> class Vector<3, Type>: public VectorOps<3, Type> {
	public:
		// Constructors
		Vector() = default;
		explicit Vector(Type x)
		{
			this->FillFromElem(x);
		}
		template<typename Type2, typename = typename std::enable_if<std::is_convertible<Type2, Type>::value>::type> explicit Vector(Vector<3, Type2> vec)
		{
			*this = vec.Apply([](Type2 a) {return a;});
		}
		template<typename Arg1, typename Arg2, typename... Args> Vector(Arg1 arg1, Arg2 arg2, Args... args)
		{
			this->template FillFromList<0>(arg1, arg2, args...);
		}

		// Swizzle shortcuts
		Type x() const {return this->template Swizzle<0>();}
		Type y() const {return this->template Swizzle<1>();}
		Type z() const {return this->template Swizzle<2>();}
		Vector<2, Type> xx() const {return this->template Swizzle<0, 0>();}
		Vector<2, Type> xy() const {return this->template Swizzle<0, 1>();}
		Vector<2, Type> xz() const {return this->template Swizzle<0, 2>();}
		Vector<2, Type> yx() const {return this->template Swizzle<1, 0>();}
		Vector<2, Type> yy() const {return this->template Swizzle<1, 1>();}
		Vector<2, Type> yz() const {return this->template Swizzle<1, 2>();}
		Vector<2, Type> zx() const {return this->template Swizzle<2, 0>();}
		Vector<2, Type> zy() const {return this->template Swizzle<2, 1>();}
		Vector<2, Type> zz() const {return this->template Swizzle<2, 2>();}
		Vector<3, Type> xxx() const {return this->template Swizzle<0, 0, 0>();}
		Vector<3, Type> xxy() const {return this->template Swizzle<0, 0, 1>();}
		Vector<3, Type> xxz() const {return this->template Swizzle<0, 0, 2>();}
		Vector<3, Type> xyx() const {return this->template Swizzle<0, 1, 0>();}
		Vector<3, Type> xyy() const {return this->template Swizzle<0, 1, 1>();}
		Vector<3, Type> xyz() const {return this->template Swizzle<0, 1, 2>();}
		Vector<3, Type> xzx() const {return this->template Swizzle<0, 2, 0>();}
		Vector<3, Type> xzy() const {return this->template Swizzle<0, 2, 1>();}
		Vector<3, Type> xzz() const {return this->template Swizzle<0, 2, 2>();}
		Vector<3, Type> yxx() const {return this->template Swizzle<1, 0, 0>();}
		Vector<3, Type> yxy() const {return this->template Swizzle<1, 0, 1>();}
		Vector<3, Type> yxz() const {return this->template Swizzle<1, 0, 2>();}
		Vector<3, Type> yyx() const {return this->template Swizzle<1, 1, 0>();}
		Vector<3, Type> yyy() const {return this->template Swizzle<1, 1, 1>();}
		Vector<3, Type> yyz() const {return this->template Swizzle<1, 1, 2>();}
		Vector<3, Type> yzx() const {return this->template Swizzle<1, 2, 0>();}
		Vector<3, Type> yzy() const {return this->template Swizzle<1, 2, 1>();}
		Vector<3, Type> yzz() const {return this->template Swizzle<1, 2, 2>();}
		Vector<3, Type> zxx() const {return this->template Swizzle<2, 0, 0>();}
		Vector<3, Type> zxy() const {return this->template Swizzle<2, 0, 1>();}
		Vector<3, Type> zxz() const {return this->template Swizzle<2, 0, 2>();}
		Vector<3, Type> zyx() const {return this->template Swizzle<2, 1, 0>();}
		Vector<3, Type> zyy() const {return this->template Swizzle<2, 1, 1>();}
		Vector<3, Type> zyz() const {return this->template Swizzle<2, 1, 2>();}
		Vector<3, Type> zzx() const {return this->template Swizzle<2, 2, 0>();}
		Vector<3, Type> zzy() const {return this->template Swizzle<2, 2, 1>();}
		Vector<3, Type> zzz() const {return this->template Swizzle<2, 2, 2>();}
		Vector<4, Type> xxxx() const {return this->template Swizzle<0, 0, 0, 0>();}
		Vector<4, Type> xxxy() const {return this->template Swizzle<0, 0, 0, 1>();}
		Vector<4, Type> xxxz() const {return this->template Swizzle<0, 0, 0, 2>();}
		Vector<4, Type> xxyx() const {return this->template Swizzle<0, 0, 1, 0>();}
		Vector<4, Type> xxyy() const {return this->template Swizzle<0, 0, 1, 1>();}
		Vector<4, Type> xxyz() const {return this->template Swizzle<0, 0, 1, 2>();}
		Vector<4, Type> xxzx() const {return this->template Swizzle<0, 0, 2, 0>();}
		Vector<4, Type> xxzy() const {return this->template Swizzle<0, 0, 2, 1>();}
		Vector<4, Type> xxzz() const {return this->template Swizzle<0, 0, 2, 2>();}
		Vector<4, Type> xyxx() const {return this->template Swizzle<0, 1, 0, 0>();}
		Vector<4, Type> xyxy() const {return this->template Swizzle<0, 1, 0, 1>();}
		Vector<4, Type> xyxz() const {return this->template Swizzle<0, 1, 0, 2>();}
		Vector<4, Type> xyyx() const {return this->template Swizzle<0, 1, 1, 0>();}
		Vector<4, Type> xyyy() const {return this->template Swizzle<0, 1, 1, 1>();}
		Vector<4, Type> xyyz() const {return this->template Swizzle<0, 1, 1, 2>();}
		Vector<4, Type> xyzx() const {return this->template Swizzle<0, 1, 2, 0>();}
		Vector<4, Type> xyzy() const {return this->template Swizzle<0, 1, 2, 1>();}
		Vector<4, Type> xyzz() const {return this->template Swizzle<0, 1, 2, 2>();}
		Vector<4, Type> xzxx() const {return this->template Swizzle<0, 2, 0, 0>();}
		Vector<4, Type> xzxy() const {return this->template Swizzle<0, 2, 0, 1>();}
		Vector<4, Type> xzxz() const {return this->template Swizzle<0, 2, 0, 2>();}
		Vector<4, Type> xzyx() const {return this->template Swizzle<0, 2, 1, 0>();}
		Vector<4, Type> xzyy() const {return this->template Swizzle<0, 2, 1, 1>();}
		Vector<4, Type> xzyz() const {return this->template Swizzle<0, 2, 1, 2>();}
		Vector<4, Type> xzzx() const {return this->template Swizzle<0, 2, 2, 0>();}
		Vector<4, Type> xzzy() const {return this->template Swizzle<0, 2, 2, 1>();}
		Vector<4, Type> xzzz() const {return this->template Swizzle<0, 2, 2, 2>();}
		Vector<4, Type> yxxx() const {return this->template Swizzle<1, 0, 0, 0>();}
		Vector<4, Type> yxxy() const {return this->template Swizzle<1, 0, 0, 1>();}
		Vector<4, Type> yxxz() const {return this->template Swizzle<1, 0, 0, 2>();}
		Vector<4, Type> yxyx() const {return this->template Swizzle<1, 0, 1, 0>();}
		Vector<4, Type> yxyy() const {return this->template Swizzle<1, 0, 1, 1>();}
		Vector<4, Type> yxyz() const {return this->template Swizzle<1, 0, 1, 2>();}
		Vector<4, Type> yxzx() const {return this->template Swizzle<1, 0, 2, 0>();}
		Vector<4, Type> yxzy() const {return this->template Swizzle<1, 0, 2, 1>();}
		Vector<4, Type> yxzz() const {return this->template Swizzle<1, 0, 2, 2>();}
		Vector<4, Type> yyxx() const {return this->template Swizzle<1, 1, 0, 0>();}
		Vector<4, Type> yyxy() const {return this->template Swizzle<1, 1, 0, 1>();}
		Vector<4, Type> yyxz() const {return this->template Swizzle<1, 1, 0, 2>();}
		Vector<4, Type> yyyx() const {return this->template Swizzle<1, 1, 1, 0>();}
		Vector<4, Type> yyyy() const {return this->template Swizzle<1, 1, 1, 1>();}
		Vector<4, Type> yyyz() const {return this->template Swizzle<1, 1, 1, 2>();}
		Vector<4, Type> yyzx() const {return this->template Swizzle<1, 1, 2, 0>();}
		Vector<4, Type> yyzy() const {return this->template Swizzle<1, 1, 2, 1>();}
		Vector<4, Type> yyzz() const {return this->template Swizzle<1, 1, 2, 2>();}
		Vector<4, Type> yzxx() const {return this->template Swizzle<1, 2, 0, 0>();}
		Vector<4, Type> yzxy() const {return this->template Swizzle<1, 2, 0, 1>();}
		Vector<4, Type> yzxz() const {return this->template Swizzle<1, 2, 0, 2>();}
		Vector<4, Type> yzyx() const {return this->template Swizzle<1, 2, 1, 0>();}
		Vector<4, Type> yzyy() const {return this->template Swizzle<1, 2, 1, 1>();}
		Vector<4, Type> yzyz() const {return this->template Swizzle<1, 2, 1, 2>();}
		Vector<4, Type> yzzx() const {return this->template Swizzle<1, 2, 2, 0>();}
		Vector<4, Type> yzzy() const {return this->template Swizzle<1, 2, 2, 1>();}
		Vector<4, Type> yzzz() const {return this->template Swizzle<1, 2, 2, 2>();}
		Vector<4, Type> zxxx() const {return this->template Swizzle<2, 0, 0, 0>();}
		Vector<4, Type> zxxy() const {return this->template Swizzle<2, 0, 0, 1>();}
		Vector<4, Type> zxxz() const {return this->template Swizzle<2, 0, 0, 2>();}
		Vector<4, Type> zxyx() const {return this->template Swizzle<2, 0, 1, 0>();}
		Vector<4, Type> zxyy() const {return this->template Swizzle<2, 0, 1, 1>();}
		Vector<4, Type> zxyz() const {return this->template Swizzle<2, 0, 1, 2>();}
		Vector<4, Type> zxzx() const {return this->template Swizzle<2, 0, 2, 0>();}
		Vector<4, Type> zxzy() const {return this->template Swizzle<2, 0, 2, 1>();}
		Vector<4, Type> zxzz() const {return this->template Swizzle<2, 0, 2, 2>();}
		Vector<4, Type> zyxx() const {return this->template Swizzle<2, 1, 0, 0>();}
		Vector<4, Type> zyxy() const {return this->template Swizzle<2, 1, 0, 1>();}
		Vector<4, Type> zyxz() const {return this->template Swizzle<2, 1, 0, 2>();}
		Vector<4, Type> zyyx() const {return this->template Swizzle<2, 1, 1, 0>();}
		Vector<4, Type> zyyy() const {return this->template Swizzle<2, 1, 1, 1>();}
		Vector<4, Type> zyyz() const {return this->template Swizzle<2, 1, 1, 2>();}
		Vector<4, Type> zyzx() const {return this->template Swizzle<2, 1, 2, 0>();}
		Vector<4, Type> zyzy() const {return this->template Swizzle<2, 1, 2, 1>();}
		Vector<4, Type> zyzz() const {return this->template Swizzle<2, 1, 2, 2>();}
		Vector<4, Type> zzxx() const {return this->template Swizzle<2, 2, 0, 0>();}
		Vector<4, Type> zzxy() const {return this->template Swizzle<2, 2, 0, 1>();}
		Vector<4, Type> zzxz() const {return this->template Swizzle<2, 2, 0, 2>();}
		Vector<4, Type> zzyx() const {return this->template Swizzle<2, 2, 1, 0>();}
		Vector<4, Type> zzyy() const {return this->template Swizzle<2, 2, 1, 1>();}
		Vector<4, Type> zzyz() const {return this->template Swizzle<2, 2, 1, 2>();}
		Vector<4, Type> zzzx() const {return this->template Swizzle<2, 2, 2, 0>();}
		Vector<4, Type> zzzy() const {return this->template Swizzle<2, 2, 2, 1>();}
		Vector<4, Type> zzzz() const {return this->template Swizzle<2, 2, 2, 2>();}
	};
	template<typename Type> class Vector<4, Type>: public VectorOps<4, Type> {
	public:
		// Constructors
		Vector() = default;
		explicit Vector(Type x)
		{
			this->FillFromElem(x);
		}
		template<typename Type2, typename = typename std::enable_if<std::is_convertible<Type2, Type>::value>::type> explicit Vector(Vector<4, Type2> vec)
		{
			*this = vec.Apply([](Type2 a) {return static_cast<Type>(a);});
		}
		template<typename Arg1, typename Arg2, typename... Args> Vector(Arg1 arg1, Arg2 arg2, Args... args)
		{
			this->template FillFromList<0>(arg1, arg2, args...);
		}

		// Swizzle shortcuts
		Type x() const {return this->template Swizzle<0>();}
		Type y() const {return this->template Swizzle<1>();}
		Type z() const {return this->template Swizzle<2>();}
		Type w() const {return this->template Swizzle<3>();}
		Vector<2, Type> xx() const {return this->template Swizzle<0, 0>();}
		Vector<2, Type> xy() const {return this->template Swizzle<0, 1>();}
		Vector<2, Type> xz() const {return this->template Swizzle<0, 2>();}
		Vector<2, Type> xw() const {return this->template Swizzle<0, 3>();}
		Vector<2, Type> yx() const {return this->template Swizzle<1, 0>();}
		Vector<2, Type> yy() const {return this->template Swizzle<1, 1>();}
		Vector<2, Type> yz() const {return this->template Swizzle<1, 2>();}
		Vector<2, Type> yw() const {return this->template Swizzle<1, 3>();}
		Vector<2, Type> zx() const {return this->template Swizzle<2, 0>();}
		Vector<2, Type> zy() const {return this->template Swizzle<2, 1>();}
		Vector<2, Type> zz() const {return this->template Swizzle<2, 2>();}
		Vector<2, Type> zw() const {return this->template Swizzle<2, 3>();}
		Vector<2, Type> wx() const {return this->template Swizzle<3, 0>();}
		Vector<2, Type> wy() const {return this->template Swizzle<3, 1>();}
		Vector<2, Type> wz() const {return this->template Swizzle<3, 2>();}
		Vector<2, Type> ww() const {return this->template Swizzle<3, 3>();}
		Vector<3, Type> xxx() const {return this->template Swizzle<0, 0, 0>();}
		Vector<3, Type> xxy() const {return this->template Swizzle<0, 0, 1>();}
		Vector<3, Type> xxz() const {return this->template Swizzle<0, 0, 2>();}
		Vector<3, Type> xxw() const {return this->template Swizzle<0, 0, 3>();}
		Vector<3, Type> xyx() const {return this->template Swizzle<0, 1, 0>();}
		Vector<3, Type> xyy() const {return this->template Swizzle<0, 1, 1>();}
		Vector<3, Type> xyz() const {return this->template Swizzle<0, 1, 2>();}
		Vector<3, Type> xyw() const {return this->template Swizzle<0, 1, 3>();}
		Vector<3, Type> xzx() const {return this->template Swizzle<0, 2, 0>();}
		Vector<3, Type> xzy() const {return this->template Swizzle<0, 2, 1>();}
		Vector<3, Type> xzz() const {return this->template Swizzle<0, 2, 2>();}
		Vector<3, Type> xzw() const {return this->template Swizzle<0, 2, 3>();}
		Vector<3, Type> xwx() const {return this->template Swizzle<0, 3, 0>();}
		Vector<3, Type> xwy() const {return this->template Swizzle<0, 3, 1>();}
		Vector<3, Type> xwz() const {return this->template Swizzle<0, 3, 2>();}
		Vector<3, Type> xww() const {return this->template Swizzle<0, 3, 3>();}
		Vector<3, Type> yxx() const {return this->template Swizzle<1, 0, 0>();}
		Vector<3, Type> yxy() const {return this->template Swizzle<1, 0, 1>();}
		Vector<3, Type> yxz() const {return this->template Swizzle<1, 0, 2>();}
		Vector<3, Type> yxw() const {return this->template Swizzle<1, 0, 3>();}
		Vector<3, Type> yyx() const {return this->template Swizzle<1, 1, 0>();}
		Vector<3, Type> yyy() const {return this->template Swizzle<1, 1, 1>();}
		Vector<3, Type> yyz() const {return this->template Swizzle<1, 1, 2>();}
		Vector<3, Type> yyw() const {return this->template Swizzle<1, 1, 3>();}
		Vector<3, Type> yzx() const {return this->template Swizzle<1, 2, 0>();}
		Vector<3, Type> yzy() const {return this->template Swizzle<1, 2, 1>();}
		Vector<3, Type> yzz() const {return this->template Swizzle<1, 2, 2>();}
		Vector<3, Type> yzw() const {return this->template Swizzle<1, 2, 3>();}
		Vector<3, Type> ywx() const {return this->template Swizzle<1, 3, 0>();}
		Vector<3, Type> ywy() const {return this->template Swizzle<1, 3, 1>();}
		Vector<3, Type> ywz() const {return this->template Swizzle<1, 3, 2>();}
		Vector<3, Type> yww() const {return this->template Swizzle<1, 3, 3>();}
		Vector<3, Type> zxx() const {return this->template Swizzle<2, 0, 0>();}
		Vector<3, Type> zxy() const {return this->template Swizzle<2, 0, 1>();}
		Vector<3, Type> zxz() const {return this->template Swizzle<2, 0, 2>();}
		Vector<3, Type> zxw() const {return this->template Swizzle<2, 0, 3>();}
		Vector<3, Type> zyx() const {return this->template Swizzle<2, 1, 0>();}
		Vector<3, Type> zyy() const {return this->template Swizzle<2, 1, 1>();}
		Vector<3, Type> zyz() const {return this->template Swizzle<2, 1, 2>();}
		Vector<3, Type> zyw() const {return this->template Swizzle<2, 1, 3>();}
		Vector<3, Type> zzx() const {return this->template Swizzle<2, 2, 0>();}
		Vector<3, Type> zzy() const {return this->template Swizzle<2, 2, 1>();}
		Vector<3, Type> zzz() const {return this->template Swizzle<2, 2, 2>();}
		Vector<3, Type> zzw() const {return this->template Swizzle<2, 2, 3>();}
		Vector<3, Type> zwx() const {return this->template Swizzle<2, 3, 0>();}
		Vector<3, Type> zwy() const {return this->template Swizzle<2, 3, 1>();}
		Vector<3, Type> zwz() const {return this->template Swizzle<2, 3, 2>();}
		Vector<3, Type> zww() const {return this->template Swizzle<2, 3, 3>();}
		Vector<3, Type> wxx() const {return this->template Swizzle<3, 0, 0>();}
		Vector<3, Type> wxy() const {return this->template Swizzle<3, 0, 1>();}
		Vector<3, Type> wxz() const {return this->template Swizzle<3, 0, 2>();}
		Vector<3, Type> wxw() const {return this->template Swizzle<3, 0, 3>();}
		Vector<3, Type> wyx() const {return this->template Swizzle<3, 1, 0>();}
		Vector<3, Type> wyy() const {return this->template Swizzle<3, 1, 1>();}
		Vector<3, Type> wyz() const {return this->template Swizzle<3, 1, 2>();}
		Vector<3, Type> wyw() const {return this->template Swizzle<3, 1, 3>();}
		Vector<3, Type> wzx() const {return this->template Swizzle<3, 2, 0>();}
		Vector<3, Type> wzy() const {return this->template Swizzle<3, 2, 1>();}
		Vector<3, Type> wzz() const {return this->template Swizzle<3, 2, 2>();}
		Vector<3, Type> wzw() const {return this->template Swizzle<3, 2, 3>();}
		Vector<3, Type> wwx() const {return this->template Swizzle<3, 3, 0>();}
		Vector<3, Type> wwy() const {return this->template Swizzle<3, 3, 1>();}
		Vector<3, Type> wwz() const {return this->template Swizzle<3, 3, 2>();}
		Vector<3, Type> www() const {return this->template Swizzle<3, 3, 3>();}
		Vector<4, Type> xxxx() const {return this->template Swizzle<0, 0, 0, 0>();}
		Vector<4, Type> xxxy() const {return this->template Swizzle<0, 0, 0, 1>();}
		Vector<4, Type> xxxz() const {return this->template Swizzle<0, 0, 0, 2>();}
		Vector<4, Type> xxxw() const {return this->template Swizzle<0, 0, 0, 3>();}
		Vector<4, Type> xxyx() const {return this->template Swizzle<0, 0, 1, 0>();}
		Vector<4, Type> xxyy() const {return this->template Swizzle<0, 0, 1, 1>();}
		Vector<4, Type> xxyz() const {return this->template Swizzle<0, 0, 1, 2>();}
		Vector<4, Type> xxyw() const {return this->template Swizzle<0, 0, 1, 3>();}
		Vector<4, Type> xxzx() const {return this->template Swizzle<0, 0, 2, 0>();}
		Vector<4, Type> xxzy() const {return this->template Swizzle<0, 0, 2, 1>();}
		Vector<4, Type> xxzz() const {return this->template Swizzle<0, 0, 2, 2>();}
		Vector<4, Type> xxzw() const {return this->template Swizzle<0, 0, 2, 3>();}
		Vector<4, Type> xxwx() const {return this->template Swizzle<0, 0, 3, 0>();}
		Vector<4, Type> xxwy() const {return this->template Swizzle<0, 0, 3, 1>();}
		Vector<4, Type> xxwz() const {return this->template Swizzle<0, 0, 3, 2>();}
		Vector<4, Type> xxww() const {return this->template Swizzle<0, 0, 3, 3>();}
		Vector<4, Type> xyxx() const {return this->template Swizzle<0, 1, 0, 0>();}
		Vector<4, Type> xyxy() const {return this->template Swizzle<0, 1, 0, 1>();}
		Vector<4, Type> xyxz() const {return this->template Swizzle<0, 1, 0, 2>();}
		Vector<4, Type> xyxw() const {return this->template Swizzle<0, 1, 0, 3>();}
		Vector<4, Type> xyyx() const {return this->template Swizzle<0, 1, 1, 0>();}
		Vector<4, Type> xyyy() const {return this->template Swizzle<0, 1, 1, 1>();}
		Vector<4, Type> xyyz() const {return this->template Swizzle<0, 1, 1, 2>();}
		Vector<4, Type> xyyw() const {return this->template Swizzle<0, 1, 1, 3>();}
		Vector<4, Type> xyzx() const {return this->template Swizzle<0, 1, 2, 0>();}
		Vector<4, Type> xyzy() const {return this->template Swizzle<0, 1, 2, 1>();}
		Vector<4, Type> xyzz() const {return this->template Swizzle<0, 1, 2, 2>();}
		Vector<4, Type> xyzw() const {return this->template Swizzle<0, 1, 2, 3>();}
		Vector<4, Type> xywx() const {return this->template Swizzle<0, 1, 3, 0>();}
		Vector<4, Type> xywy() const {return this->template Swizzle<0, 1, 3, 1>();}
		Vector<4, Type> xywz() const {return this->template Swizzle<0, 1, 3, 2>();}
		Vector<4, Type> xyww() const {return this->template Swizzle<0, 1, 3, 3>();}
		Vector<4, Type> xzxx() const {return this->template Swizzle<0, 2, 0, 0>();}
		Vector<4, Type> xzxy() const {return this->template Swizzle<0, 2, 0, 1>();}
		Vector<4, Type> xzxz() const {return this->template Swizzle<0, 2, 0, 2>();}
		Vector<4, Type> xzxw() const {return this->template Swizzle<0, 2, 0, 3>();}
		Vector<4, Type> xzyx() const {return this->template Swizzle<0, 2, 1, 0>();}
		Vector<4, Type> xzyy() const {return this->template Swizzle<0, 2, 1, 1>();}
		Vector<4, Type> xzyz() const {return this->template Swizzle<0, 2, 1, 2>();}
		Vector<4, Type> xzyw() const {return this->template Swizzle<0, 2, 1, 3>();}
		Vector<4, Type> xzzx() const {return this->template Swizzle<0, 2, 2, 0>();}
		Vector<4, Type> xzzy() const {return this->template Swizzle<0, 2, 2, 1>();}
		Vector<4, Type> xzzz() const {return this->template Swizzle<0, 2, 2, 2>();}
		Vector<4, Type> xzzw() const {return this->template Swizzle<0, 2, 2, 3>();}
		Vector<4, Type> xzwx() const {return this->template Swizzle<0, 2, 3, 0>();}
		Vector<4, Type> xzwy() const {return this->template Swizzle<0, 2, 3, 1>();}
		Vector<4, Type> xzwz() const {return this->template Swizzle<0, 2, 3, 2>();}
		Vector<4, Type> xzww() const {return this->template Swizzle<0, 2, 3, 3>();}
		Vector<4, Type> xwxx() const {return this->template Swizzle<0, 3, 0, 0>();}
		Vector<4, Type> xwxy() const {return this->template Swizzle<0, 3, 0, 1>();}
		Vector<4, Type> xwxz() const {return this->template Swizzle<0, 3, 0, 2>();}
		Vector<4, Type> xwxw() const {return this->template Swizzle<0, 3, 0, 3>();}
		Vector<4, Type> xwyx() const {return this->template Swizzle<0, 3, 1, 0>();}
		Vector<4, Type> xwyy() const {return this->template Swizzle<0, 3, 1, 1>();}
		Vector<4, Type> xwyz() const {return this->template Swizzle<0, 3, 1, 2>();}
		Vector<4, Type> xwyw() const {return this->template Swizzle<0, 3, 1, 3>();}
		Vector<4, Type> xwzx() const {return this->template Swizzle<0, 3, 2, 0>();}
		Vector<4, Type> xwzy() const {return this->template Swizzle<0, 3, 2, 1>();}
		Vector<4, Type> xwzz() const {return this->template Swizzle<0, 3, 2, 2>();}
		Vector<4, Type> xwzw() const {return this->template Swizzle<0, 3, 2, 3>();}
		Vector<4, Type> xwwx() const {return this->template Swizzle<0, 3, 3, 0>();}
		Vector<4, Type> xwwy() const {return this->template Swizzle<0, 3, 3, 1>();}
		Vector<4, Type> xwwz() const {return this->template Swizzle<0, 3, 3, 2>();}
		Vector<4, Type> xwww() const {return this->template Swizzle<0, 3, 3, 3>();}
		Vector<4, Type> yxxx() const {return this->template Swizzle<1, 0, 0, 0>();}
		Vector<4, Type> yxxy() const {return this->template Swizzle<1, 0, 0, 1>();}
		Vector<4, Type> yxxz() const {return this->template Swizzle<1, 0, 0, 2>();}
		Vector<4, Type> yxxw() const {return this->template Swizzle<1, 0, 0, 3>();}
		Vector<4, Type> yxyx() const {return this->template Swizzle<1, 0, 1, 0>();}
		Vector<4, Type> yxyy() const {return this->template Swizzle<1, 0, 1, 1>();}
		Vector<4, Type> yxyz() const {return this->template Swizzle<1, 0, 1, 2>();}
		Vector<4, Type> yxyw() const {return this->template Swizzle<1, 0, 1, 3>();}
		Vector<4, Type> yxzx() const {return this->template Swizzle<1, 0, 2, 0>();}
		Vector<4, Type> yxzy() const {return this->template Swizzle<1, 0, 2, 1>();}
		Vector<4, Type> yxzz() const {return this->template Swizzle<1, 0, 2, 2>();}
		Vector<4, Type> yxzw() const {return this->template Swizzle<1, 0, 2, 3>();}
		Vector<4, Type> yxwx() const {return this->template Swizzle<1, 0, 3, 0>();}
		Vector<4, Type> yxwy() const {return this->template Swizzle<1, 0, 3, 1>();}
		Vector<4, Type> yxwz() const {return this->template Swizzle<1, 0, 3, 2>();}
		Vector<4, Type> yxww() const {return this->template Swizzle<1, 0, 3, 3>();}
		Vector<4, Type> yyxx() const {return this->template Swizzle<1, 1, 0, 0>();}
		Vector<4, Type> yyxy() const {return this->template Swizzle<1, 1, 0, 1>();}
		Vector<4, Type> yyxz() const {return this->template Swizzle<1, 1, 0, 2>();}
		Vector<4, Type> yyxw() const {return this->template Swizzle<1, 1, 0, 3>();}
		Vector<4, Type> yyyx() const {return this->template Swizzle<1, 1, 1, 0>();}
		Vector<4, Type> yyyy() const {return this->template Swizzle<1, 1, 1, 1>();}
		Vector<4, Type> yyyz() const {return this->template Swizzle<1, 1, 1, 2>();}
		Vector<4, Type> yyyw() const {return this->template Swizzle<1, 1, 1, 3>();}
		Vector<4, Type> yyzx() const {return this->template Swizzle<1, 1, 2, 0>();}
		Vector<4, Type> yyzy() const {return this->template Swizzle<1, 1, 2, 1>();}
		Vector<4, Type> yyzz() const {return this->template Swizzle<1, 1, 2, 2>();}
		Vector<4, Type> yyzw() const {return this->template Swizzle<1, 1, 2, 3>();}
		Vector<4, Type> yywx() const {return this->template Swizzle<1, 1, 3, 0>();}
		Vector<4, Type> yywy() const {return this->template Swizzle<1, 1, 3, 1>();}
		Vector<4, Type> yywz() const {return this->template Swizzle<1, 1, 3, 2>();}
		Vector<4, Type> yyww() const {return this->template Swizzle<1, 1, 3, 3>();}
		Vector<4, Type> yzxx() const {return this->template Swizzle<1, 2, 0, 0>();}
		Vector<4, Type> yzxy() const {return this->template Swizzle<1, 2, 0, 1>();}
		Vector<4, Type> yzxz() const {return this->template Swizzle<1, 2, 0, 2>();}
		Vector<4, Type> yzxw() const {return this->template Swizzle<1, 2, 0, 3>();}
		Vector<4, Type> yzyx() const {return this->template Swizzle<1, 2, 1, 0>();}
		Vector<4, Type> yzyy() const {return this->template Swizzle<1, 2, 1, 1>();}
		Vector<4, Type> yzyz() const {return this->template Swizzle<1, 2, 1, 2>();}
		Vector<4, Type> yzyw() const {return this->template Swizzle<1, 2, 1, 3>();}
		Vector<4, Type> yzzx() const {return this->template Swizzle<1, 2, 2, 0>();}
		Vector<4, Type> yzzy() const {return this->template Swizzle<1, 2, 2, 1>();}
		Vector<4, Type> yzzz() const {return this->template Swizzle<1, 2, 2, 2>();}
		Vector<4, Type> yzzw() const {return this->template Swizzle<1, 2, 2, 3>();}
		Vector<4, Type> yzwx() const {return this->template Swizzle<1, 2, 3, 0>();}
		Vector<4, Type> yzwy() const {return this->template Swizzle<1, 2, 3, 1>();}
		Vector<4, Type> yzwz() const {return this->template Swizzle<1, 2, 3, 2>();}
		Vector<4, Type> yzww() const {return this->template Swizzle<1, 2, 3, 3>();}
		Vector<4, Type> ywxx() const {return this->template Swizzle<1, 3, 0, 0>();}
		Vector<4, Type> ywxy() const {return this->template Swizzle<1, 3, 0, 1>();}
		Vector<4, Type> ywxz() const {return this->template Swizzle<1, 3, 0, 2>();}
		Vector<4, Type> ywxw() const {return this->template Swizzle<1, 3, 0, 3>();}
		Vector<4, Type> ywyx() const {return this->template Swizzle<1, 3, 1, 0>();}
		Vector<4, Type> ywyy() const {return this->template Swizzle<1, 3, 1, 1>();}
		Vector<4, Type> ywyz() const {return this->template Swizzle<1, 3, 1, 2>();}
		Vector<4, Type> ywyw() const {return this->template Swizzle<1, 3, 1, 3>();}
		Vector<4, Type> ywzx() const {return this->template Swizzle<1, 3, 2, 0>();}
		Vector<4, Type> ywzy() const {return this->template Swizzle<1, 3, 2, 1>();}
		Vector<4, Type> ywzz() const {return this->template Swizzle<1, 3, 2, 2>();}
		Vector<4, Type> ywzw() const {return this->template Swizzle<1, 3, 2, 3>();}
		Vector<4, Type> ywwx() const {return this->template Swizzle<1, 3, 3, 0>();}
		Vector<4, Type> ywwy() const {return this->template Swizzle<1, 3, 3, 1>();}
		Vector<4, Type> ywwz() const {return this->template Swizzle<1, 3, 3, 2>();}
		Vector<4, Type> ywww() const {return this->template Swizzle<1, 3, 3, 3>();}
		Vector<4, Type> zxxx() const {return this->template Swizzle<2, 0, 0, 0>();}
		Vector<4, Type> zxxy() const {return this->template Swizzle<2, 0, 0, 1>();}
		Vector<4, Type> zxxz() const {return this->template Swizzle<2, 0, 0, 2>();}
		Vector<4, Type> zxxw() const {return this->template Swizzle<2, 0, 0, 3>();}
		Vector<4, Type> zxyx() const {return this->template Swizzle<2, 0, 1, 0>();}
		Vector<4, Type> zxyy() const {return this->template Swizzle<2, 0, 1, 1>();}
		Vector<4, Type> zxyz() const {return this->template Swizzle<2, 0, 1, 2>();}
		Vector<4, Type> zxyw() const {return this->template Swizzle<2, 0, 1, 3>();}
		Vector<4, Type> zxzx() const {return this->template Swizzle<2, 0, 2, 0>();}
		Vector<4, Type> zxzy() const {return this->template Swizzle<2, 0, 2, 1>();}
		Vector<4, Type> zxzz() const {return this->template Swizzle<2, 0, 2, 2>();}
		Vector<4, Type> zxzw() const {return this->template Swizzle<2, 0, 2, 3>();}
		Vector<4, Type> zxwx() const {return this->template Swizzle<2, 0, 3, 0>();}
		Vector<4, Type> zxwy() const {return this->template Swizzle<2, 0, 3, 1>();}
		Vector<4, Type> zxwz() const {return this->template Swizzle<2, 0, 3, 2>();}
		Vector<4, Type> zxww() const {return this->template Swizzle<2, 0, 3, 3>();}
		Vector<4, Type> zyxx() const {return this->template Swizzle<2, 1, 0, 0>();}
		Vector<4, Type> zyxy() const {return this->template Swizzle<2, 1, 0, 1>();}
		Vector<4, Type> zyxz() const {return this->template Swizzle<2, 1, 0, 2>();}
		Vector<4, Type> zyxw() const {return this->template Swizzle<2, 1, 0, 3>();}
		Vector<4, Type> zyyx() const {return this->template Swizzle<2, 1, 1, 0>();}
		Vector<4, Type> zyyy() const {return this->template Swizzle<2, 1, 1, 1>();}
		Vector<4, Type> zyyz() const {return this->template Swizzle<2, 1, 1, 2>();}
		Vector<4, Type> zyyw() const {return this->template Swizzle<2, 1, 1, 3>();}
		Vector<4, Type> zyzx() const {return this->template Swizzle<2, 1, 2, 0>();}
		Vector<4, Type> zyzy() const {return this->template Swizzle<2, 1, 2, 1>();}
		Vector<4, Type> zyzz() const {return this->template Swizzle<2, 1, 2, 2>();}
		Vector<4, Type> zyzw() const {return this->template Swizzle<2, 1, 2, 3>();}
		Vector<4, Type> zywx() const {return this->template Swizzle<2, 1, 3, 0>();}
		Vector<4, Type> zywy() const {return this->template Swizzle<2, 1, 3, 1>();}
		Vector<4, Type> zywz() const {return this->template Swizzle<2, 1, 3, 2>();}
		Vector<4, Type> zyww() const {return this->template Swizzle<2, 1, 3, 3>();}
		Vector<4, Type> zzxx() const {return this->template Swizzle<2, 2, 0, 0>();}
		Vector<4, Type> zzxy() const {return this->template Swizzle<2, 2, 0, 1>();}
		Vector<4, Type> zzxz() const {return this->template Swizzle<2, 2, 0, 2>();}
		Vector<4, Type> zzxw() const {return this->template Swizzle<2, 2, 0, 3>();}
		Vector<4, Type> zzyx() const {return this->template Swizzle<2, 2, 1, 0>();}
		Vector<4, Type> zzyy() const {return this->template Swizzle<2, 2, 1, 1>();}
		Vector<4, Type> zzyz() const {return this->template Swizzle<2, 2, 1, 2>();}
		Vector<4, Type> zzyw() const {return this->template Swizzle<2, 2, 1, 3>();}
		Vector<4, Type> zzzx() const {return this->template Swizzle<2, 2, 2, 0>();}
		Vector<4, Type> zzzy() const {return this->template Swizzle<2, 2, 2, 1>();}
		Vector<4, Type> zzzz() const {return this->template Swizzle<2, 2, 2, 2>();}
		Vector<4, Type> zzzw() const {return this->template Swizzle<2, 2, 2, 3>();}
		Vector<4, Type> zzwx() const {return this->template Swizzle<2, 2, 3, 0>();}
		Vector<4, Type> zzwy() const {return this->template Swizzle<2, 2, 3, 1>();}
		Vector<4, Type> zzwz() const {return this->template Swizzle<2, 2, 3, 2>();}
		Vector<4, Type> zzww() const {return this->template Swizzle<2, 2, 3, 3>();}
		Vector<4, Type> zwxx() const {return this->template Swizzle<2, 3, 0, 0>();}
		Vector<4, Type> zwxy() const {return this->template Swizzle<2, 3, 0, 1>();}
		Vector<4, Type> zwxz() const {return this->template Swizzle<2, 3, 0, 2>();}
		Vector<4, Type> zwxw() const {return this->template Swizzle<2, 3, 0, 3>();}
		Vector<4, Type> zwyx() const {return this->template Swizzle<2, 3, 1, 0>();}
		Vector<4, Type> zwyy() const {return this->template Swizzle<2, 3, 1, 1>();}
		Vector<4, Type> zwyz() const {return this->template Swizzle<2, 3, 1, 2>();}
		Vector<4, Type> zwyw() const {return this->template Swizzle<2, 3, 1, 3>();}
		Vector<4, Type> zwzx() const {return this->template Swizzle<2, 3, 2, 0>();}
		Vector<4, Type> zwzy() const {return this->template Swizzle<2, 3, 2, 1>();}
		Vector<4, Type> zwzz() const {return this->template Swizzle<2, 3, 2, 2>();}
		Vector<4, Type> zwzw() const {return this->template Swizzle<2, 3, 2, 3>();}
		Vector<4, Type> zwwx() const {return this->template Swizzle<2, 3, 3, 0>();}
		Vector<4, Type> zwwy() const {return this->template Swizzle<2, 3, 3, 1>();}
		Vector<4, Type> zwwz() const {return this->template Swizzle<2, 3, 3, 2>();}
		Vector<4, Type> zwww() const {return this->template Swizzle<2, 3, 3, 3>();}
		Vector<4, Type> wxxx() const {return this->template Swizzle<3, 0, 0, 0>();}
		Vector<4, Type> wxxy() const {return this->template Swizzle<3, 0, 0, 1>();}
		Vector<4, Type> wxxz() const {return this->template Swizzle<3, 0, 0, 2>();}
		Vector<4, Type> wxxw() const {return this->template Swizzle<3, 0, 0, 3>();}
		Vector<4, Type> wxyx() const {return this->template Swizzle<3, 0, 1, 0>();}
		Vector<4, Type> wxyy() const {return this->template Swizzle<3, 0, 1, 1>();}
		Vector<4, Type> wxyz() const {return this->template Swizzle<3, 0, 1, 2>();}
		Vector<4, Type> wxyw() const {return this->template Swizzle<3, 0, 1, 3>();}
		Vector<4, Type> wxzx() const {return this->template Swizzle<3, 0, 2, 0>();}
		Vector<4, Type> wxzy() const {return this->template Swizzle<3, 0, 2, 1>();}
		Vector<4, Type> wxzz() const {return this->template Swizzle<3, 0, 2, 2>();}
		Vector<4, Type> wxzw() const {return this->template Swizzle<3, 0, 2, 3>();}
		Vector<4, Type> wxwx() const {return this->template Swizzle<3, 0, 3, 0>();}
		Vector<4, Type> wxwy() const {return this->template Swizzle<3, 0, 3, 1>();}
		Vector<4, Type> wxwz() const {return this->template Swizzle<3, 0, 3, 2>();}
		Vector<4, Type> wxww() const {return this->template Swizzle<3, 0, 3, 3>();}
		Vector<4, Type> wyxx() const {return this->template Swizzle<3, 1, 0, 0>();}
		Vector<4, Type> wyxy() const {return this->template Swizzle<3, 1, 0, 1>();}
		Vector<4, Type> wyxz() const {return this->template Swizzle<3, 1, 0, 2>();}
		Vector<4, Type> wyxw() const {return this->template Swizzle<3, 1, 0, 3>();}
		Vector<4, Type> wyyx() const {return this->template Swizzle<3, 1, 1, 0>();}
		Vector<4, Type> wyyy() const {return this->template Swizzle<3, 1, 1, 1>();}
		Vector<4, Type> wyyz() const {return this->template Swizzle<3, 1, 1, 2>();}
		Vector<4, Type> wyyw() const {return this->template Swizzle<3, 1, 1, 3>();}
		Vector<4, Type> wyzx() const {return this->template Swizzle<3, 1, 2, 0>();}
		Vector<4, Type> wyzy() const {return this->template Swizzle<3, 1, 2, 1>();}
		Vector<4, Type> wyzz() const {return this->template Swizzle<3, 1, 2, 2>();}
		Vector<4, Type> wyzw() const {return this->template Swizzle<3, 1, 2, 3>();}
		Vector<4, Type> wywx() const {return this->template Swizzle<3, 1, 3, 0>();}
		Vector<4, Type> wywy() const {return this->template Swizzle<3, 1, 3, 1>();}
		Vector<4, Type> wywz() const {return this->template Swizzle<3, 1, 3, 2>();}
		Vector<4, Type> wyww() const {return this->template Swizzle<3, 1, 3, 3>();}
		Vector<4, Type> wzxx() const {return this->template Swizzle<3, 2, 0, 0>();}
		Vector<4, Type> wzxy() const {return this->template Swizzle<3, 2, 0, 1>();}
		Vector<4, Type> wzxz() const {return this->template Swizzle<3, 2, 0, 2>();}
		Vector<4, Type> wzxw() const {return this->template Swizzle<3, 2, 0, 3>();}
		Vector<4, Type> wzyx() const {return this->template Swizzle<3, 2, 1, 0>();}
		Vector<4, Type> wzyy() const {return this->template Swizzle<3, 2, 1, 1>();}
		Vector<4, Type> wzyz() const {return this->template Swizzle<3, 2, 1, 2>();}
		Vector<4, Type> wzyw() const {return this->template Swizzle<3, 2, 1, 3>();}
		Vector<4, Type> wzzx() const {return this->template Swizzle<3, 2, 2, 0>();}
		Vector<4, Type> wzzy() const {return this->template Swizzle<3, 2, 2, 1>();}
		Vector<4, Type> wzzz() const {return this->template Swizzle<3, 2, 2, 2>();}
		Vector<4, Type> wzzw() const {return this->template Swizzle<3, 2, 2, 3>();}
		Vector<4, Type> wzwx() const {return this->template Swizzle<3, 2, 3, 0>();}
		Vector<4, Type> wzwy() const {return this->template Swizzle<3, 2, 3, 1>();}
		Vector<4, Type> wzwz() const {return this->template Swizzle<3, 2, 3, 2>();}
		Vector<4, Type> wzww() const {return this->template Swizzle<3, 2, 3, 3>();}
		Vector<4, Type> wwxx() const {return this->template Swizzle<3, 3, 0, 0>();}
		Vector<4, Type> wwxy() const {return this->template Swizzle<3, 3, 0, 1>();}
		Vector<4, Type> wwxz() const {return this->template Swizzle<3, 3, 0, 2>();}
		Vector<4, Type> wwxw() const {return this->template Swizzle<3, 3, 0, 3>();}
		Vector<4, Type> wwyx() const {return this->template Swizzle<3, 3, 1, 0>();}
		Vector<4, Type> wwyy() const {return this->template Swizzle<3, 3, 1, 1>();}
		Vector<4, Type> wwyz() const {return this->template Swizzle<3, 3, 1, 2>();}
		Vector<4, Type> wwyw() const {return this->template Swizzle<3, 3, 1, 3>();}
		Vector<4, Type> wwzx() const {return this->template Swizzle<3, 3, 2, 0>();}
		Vector<4, Type> wwzy() const {return this->template Swizzle<3, 3, 2, 1>();}
		Vector<4, Type> wwzz() const {return this->template Swizzle<3, 3, 2, 2>();}
		Vector<4, Type> wwzw() const {return this->template Swizzle<3, 3, 2, 3>();}
		Vector<4, Type> wwwx() const {return this->template Swizzle<3, 3, 3, 0>();}
		Vector<4, Type> wwwy() const {return this->template Swizzle<3, 3, 3, 1>();}
		Vector<4, Type> wwwz() const {return this->template Swizzle<3, 3, 3, 2>();}
		Vector<4, Type> wwww() const {return this->template Swizzle<3, 3, 3, 3>();}
	};

	// Boolean vector types with implicit conversions to bool, used for the == and
	// != operators.
	template<size_t Dimension> class BoolVectorEq: public Vector<Dimension, bool> {
	public:
		explicit BoolVectorEq(Vector<Dimension, bool> vec)
			: Vector<Dimension, bool>(vec) {}
		explicit operator bool() const {return this->All();}
	};
	template<size_t Dimension> class BoolVectorNe: public Vector<Dimension, bool> {
	public:
		explicit BoolVectorNe(Vector<Dimension, bool> vec)
			: Vector<Dimension, bool>(vec) {}
		explicit operator bool() const {return this->Any();}
	};

	// VectorBase::Swizzle
	template<size_t Dimension, typename Type> template<size_t x>
	Type VectorBase<Dimension, Type>::Swizzle() const
	{
		return data[x];
	}
	template<size_t Dimension, typename Type> template<size_t x, size_t y>
	Vector<2, Type> VectorBase<Dimension, Type>::Swizzle() const
	{
		return Vector<2, Type>(data[x], data[y]);
	}
	template<size_t Dimension, typename Type> template<size_t x, size_t y, size_t z>
	Vector<3, Type> VectorBase<Dimension, Type>::Swizzle() const
	{
		return Vector<3, Type>(data[x], data[y], data[z]);
	}
	template<size_t Dimension, typename Type> template<size_t x, size_t y, size_t z, size_t w>
	Vector<4, Type> VectorBase<Dimension, Type>::Swizzle() const
	{
		return Vector<4, Type>(data[x], data[y], data[z], data[w]);
	}

	// VectorBase::Load/Store
	template<size_t Dimension, typename Type>
	Vector<Dimension, Type> VectorBase<Dimension, Type>::Load(const Type* ptr)
	{
		Vector<Dimension, Type> out;
		for (size_t i = 0; i < Dimension; i++)
			out.data[i] = ptr[i];
		return out;
	}
	template<size_t Dimension, typename Type>
	void VectorBase<Dimension, Type>::Store(Type* ptr) const
	{
		for (size_t i = 0; i < Dimension; i++)
			ptr[i] = data[i];
	}

	// VectorBase::Apply/Apply2/Apply3
	template<size_t Dimension, typename Type> template<typename Func>
	Vector<Dimension, typename std::result_of<Func(Type)>::type> VectorBase<Dimension, Type>::Apply(Func func) const
	{
		Vector<Dimension, typename std::result_of<Func(Type)>::type> out;
		for (size_t i = 0; i < Dimension; i++)
			out.data[i] = func(data[i]);
		return out;
	}
	template<size_t Dimension, typename Type> template<typename Func, typename Type2>
	Vector<Dimension, typename std::result_of<Func(Type, Type2)>::type> VectorBase<Dimension, Type>::Apply2(Func func, Vector<Dimension, Type2> other) const
	{
		Vector<Dimension, typename std::result_of<Func(Type, Type2)>::type> out;
		for (size_t i = 0; i < Dimension; i++)
			out.data[i] = func(data[i], other.data[i]);
		return out;
	}
	template<size_t Dimension, typename Type> template<typename Func, typename Type2, typename Type3>
	Vector<Dimension, typename std::result_of<Func(Type, Type2, Type3)>::type> VectorBase<Dimension, Type>::Apply3(Func func, Vector<Dimension, Type2> other1, Vector<Dimension, Type3> other2) const
	{
		Vector<Dimension, typename std::result_of<Func(Type, Type2, Type3)>::type> out;
		for (size_t i = 0; i < Dimension; i++)
			out.data[i] = func(data[i], other1.data[i], other2.data[i]);
		return out;
	}

	// VectorBase::Reduce
	template<size_t Dimension, typename Type> template<typename Func>
	Type VectorBase<Dimension, Type>::Reduce(Type init, Func func)
	{
		for (size_t i = 0; i < Dimension; i++)
			init = func(init, data[i]);
		return init;
	}

	// Vector minimum/maximum and clamping
	template<size_t Dimension, typename Type>
	Vector<Dimension, Type> Min(Vector<Dimension, Type> a, Vector<Dimension, Type> b)
	{
		return a.Apply2([](Type a, Type b) {return std::min(a, b);}, b);
	}
	template<size_t Dimension, typename Type>
	Vector<Dimension, Type> Max(Vector<Dimension, Type> a, Vector<Dimension, Type> b)
	{
		return a.Apply2([](Type a, Type b) {return std::max(a, b);}, b);
	}
	template<size_t Dimension, typename Type>
	Vector<Dimension, Type> Clamp(Vector<Dimension, Type> x, Vector<Dimension, Type> low, Vector<Dimension, Type> high)
	{
		return Min(Max(x, low), high);
	}
	template<size_t Dimension, typename Type>
	Vector<Dimension, Type> Clamp(Vector<Dimension, Type> x, Type low, Type high)
	{
		return Clamp(x, Vector<Dimension, Type>(low), Vector<Dimension, Type>(high));
	}

	// Vector dot product, length and distance
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Type Dot(Vector<Dimension, Type> a, Vector<Dimension, Type> b)
	{
		return (a * b).Reduce(0, [](Type a, Type b) {return a + b;});
	}
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Type LengthSq(Vector<Dimension, Type> x)
	{
		return Dot(x, x);
	}
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Type Length(Vector<Dimension, Type> x)
	{
		return sqrt(LengthSq(x));
	}
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Type DistanceSq(Vector<Dimension, Type> a, Vector<Dimension, Type> b)
	{
		return LengthSq(a - b);
	}
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Type Distance(Vector<Dimension, Type> a, Vector<Dimension, Type> b)
	{
		return sqrt(DistanceSq(a, b));
	}

	// Vector cross product
	template<typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Vector<3, Type> Cross(Vector<3, Type> a, Vector<3, Type> b)
	{
		return a.yzx() * b.zxy() - a.zxy() * b.yzx();
	}

	// Vector normalization
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Vector<Dimension, Type> Normalize(Vector<Dimension, Type> x)
	{
		return x / Length(x);
	}

	// Vector interpolation
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Vector<Dimension, Type> Mix(Vector<Dimension, Type> a, Vector<Dimension, Type> b, Vector<Dimension, Type> frac)
	{
		return a + (b - a) * frac;
	}
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Vector<Dimension, Type> Mix(Vector<Dimension, Type> a, Vector<Dimension, Type> b, Type frac)
	{
		return a + (b - a) * frac;
	}
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Vector<Dimension, Type> Mix(Vector<Dimension, Type> a, Vector<Dimension, Type> b, Vector<Dimension, bool> sel)
	{
		return a.Apply3([](Type a, Type b, bool sel) {return sel ? b : a;}, b, sel);
	}

	// Vector projection
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Vector<Dimension, Type> Project(Vector<Dimension, Type> a, Vector<Dimension, Type> n)
	{
		return Dot(a, n) * n;
	}

	// Vector rejection
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Vector<Dimension, Type> Reject(Vector<Dimension, Type> a, Vector<Dimension, Type> n)
	{
		return a - Project(a, n);
	}

	// Vector reflection
	template<size_t Dimension, typename Type, typename Enable = typename std::enable_if<std::is_floating_point<Type>::value>::type>
	Vector<Dimension, Type> Reflect(Vector<Dimension, Type> a, Vector<Dimension, Type> n)
	{
		return a - 2 * Project(a, n);
	}
}

#endif // COMMON_VECTOR_H_
