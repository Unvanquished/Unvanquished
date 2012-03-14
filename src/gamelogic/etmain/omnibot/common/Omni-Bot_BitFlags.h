////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-08-28 07:12:05 +0200 (Sa, 28 Aug 2010) $
// $LastChangedRevision: 32 $
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __USERFLAGS_H__
#define __USERFLAGS_H__

#include "Omni-Bot_BasicTypes.h"

class BitFlag32
{
public:
	bool AnyFlagSet() const
	{
		return m_Flags != 0;
	}

	bool CheckFlag ( obint32 _flag ) const
	{
		return ( m_Flags & ( ( obint32 ) 1 << _flag ) ) != 0;
	}

	void SetFlag ( obint32 _flag )
	{
		m_Flags |= ( ( obint32 ) 1 << _flag );
	}

	void SetFlag ( obint32 _flag, bool _set )
	{
		if ( _set )
		{
			SetFlag ( _flag );
		}
		else
		{
			ClearFlag ( _flag );
		}
	}

	void ClearFlag ( obint32 _flag )
	{
		m_Flags &= ~ ( ( obint32 ) 1 << _flag );
	}

	void ClearAll()
	{
		m_Flags = 0;
	}

	BitFlag32 &operator&= ( const BitFlag32 &rhs )
	{
		m_Flags &= rhs.m_Flags;
		return *this;
	}

	BitFlag32 operator& ( const BitFlag32 &rhs ) const
	{
		BitFlag32 bf ( *this );
		bf &= rhs;
		return bf;
	}

	BitFlag32 &operator|= ( const BitFlag32 &rhs )
	{
		m_Flags |= rhs.m_Flags;
		return *this;
	}

	BitFlag32 operator| ( const BitFlag32 &rhs ) const
	{
		BitFlag32 bf ( *this );
		bf |= rhs;
		return bf;
	}

	BitFlag32 operator~() const
	{
		BitFlag32 bf ( *this );
		bf.m_Flags = ~bf.m_Flags;
		return bf;
	}

	bool operator< ( obint32 _rhs ) const
	{
		return m_Flags < _rhs;
	}

	bool operator== ( const BitFlag32 &r ) const
	{
		return m_Flags == r.m_Flags;
	}

	bool operator!= ( const BitFlag32 &r ) const
	{
		return m_Flags != r.m_Flags;
	}

	obint32 GetRawFlags() const
	{
		return m_Flags;
	}

	explicit BitFlag32 ( obint32 flgs = 0 ) : m_Flags ( flgs )
	{
	}

private:
	obint32 m_Flags;
};

class BitFlag64
{
public:
	bool AnyFlagSet() const
	{
		return m_Flags != 0;
	}

	bool CheckFlag ( obint32 _flag ) const
	{
		obint64 flg = ( obint64 ) 1 << _flag;
		return ( m_Flags & flg ) != 0;
	}

	void SetFlag ( obint32 _flag )
	{
		m_Flags |= ( ( obint64 ) 1 << _flag );
	}

	void SetFlag ( obint32 _flag, bool _set )
	{
		if ( _set )
		{
			SetFlag ( _flag );
		}
		else
		{
			ClearFlag ( _flag );
		}
	}

	void ClearFlag ( obint32 _flag )
	{
		m_Flags &= ~ ( ( obint64 ) 1 << _flag );
	}

	void ClearAll()
	{
		m_Flags = 0;
	}

	BitFlag64 &operator&= ( const BitFlag64 &rhs )
	{
		m_Flags &= rhs.m_Flags;
		return *this;
	}

	BitFlag64 operator& ( const BitFlag64 &rhs ) const
	{
		BitFlag64 bf ( *this );
		bf &= rhs;
		return bf;
	}

	BitFlag64 &operator|= ( const BitFlag64 &rhs )
	{
		m_Flags |= rhs.m_Flags;
		return *this;
	}

	BitFlag64 operator| ( const BitFlag64 &rhs ) const
	{
		BitFlag64 bf ( *this );
		bf |= rhs;
		return bf;
	}

	BitFlag64 operator~() const
	{
		BitFlag64 bf ( *this );
		bf.m_Flags = ~bf.m_Flags;
		return bf;
	}

	bool operator< ( obint64 _rhs ) const
	{
		return m_Flags < _rhs;
	}

	bool operator== ( const BitFlag64 &r ) const
	{
		return m_Flags == r.m_Flags;
	}

	bool operator!= ( const BitFlag64 &r ) const
	{
		return m_Flags != r.m_Flags;
	}

	explicit BitFlag64 ( obint64 flgs = 0 ) : m_Flags ( flgs )
	{
	}

private:
	obint64 m_Flags;
};

#endif
