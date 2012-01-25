////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-09-20 04:04:56 +0200 (Mo, 20 Sep 2010) $
// $LastChangedRevision: 153 $
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __OMNICOLOR_H__
#define __OMNICOLOR_H__

#include "Omni-Bot_BasicTypes.h"

// class: obColor
//		Helper class for defining color values.
class obColor
{
public:
	obColor()
	{
		// initialize to white
		cdata.m_RGBA[0] = 255;
		cdata.m_RGBA[1] = 255;
		cdata.m_RGBA[2] = 255;
		cdata.m_RGBA[3] = 255; // 255 is opaque, 0 is transparent
	}
	obColor(obint32 _color)
	{
		cdata.m_RGBAi = _color;
	}
	obColor(obuint8 _r, obuint8 _g, obuint8 _b, obuint8 _a = 255)
	{
		cdata.m_RGBA[0] = _r;
		cdata.m_RGBA[1] = _g;
		cdata.m_RGBA[2] = _b;
		cdata.m_RGBA[3] = _a; // 255 is opaque, 0 is transparent
	}
	void FromFloat(float _r, float _g, float _b, float _a = 1)
	{
		cdata.m_RGBA[0] = (obuint8)(_r * 255.f);
		cdata.m_RGBA[1] = (obuint8)(_g * 255.f);
		cdata.m_RGBA[2] = (obuint8)(_b * 255.f);
		cdata.m_RGBA[3] = (obuint8)(_a * 255.f);// 255 is opaque, 0 is transparent
	}
	operator int() const
	{
		return cdata.m_RGBAi;
	}

	inline obuint8 r() const	{ return cdata.m_RGBA[0]; }
	inline obuint8 g() const	{ return cdata.m_RGBA[1]; }
	inline obuint8 b() const	{ return cdata.m_RGBA[2]; }
	inline obuint8 a() const	{ return cdata.m_RGBA[3]; }

	inline float rF() const	{ return (float)cdata.m_RGBA[0] / 255.0f; }
	inline float gF() const	{ return (float)cdata.m_RGBA[1] / 255.0f; }
	inline float bF() const	{ return (float)cdata.m_RGBA[2] / 255.0f; }
	inline float aF() const	{ return (float)cdata.m_RGBA[3] / 255.0f; }

	inline obColor fade(obuint8 _a) const { obColor c(cdata.m_RGBAi); c.cdata.m_RGBA[3]=_a; return c; }

	inline obint32 rgba() const { return cdata.m_RGBAi; }
	inline obint32 argb() const { return obColor( a(), r(), g(), b() ); }
private:
	union cdatatype
	{
		obuint8		m_RGBA[4];
		obint32		m_RGBAi;
	} cdata;
};

#endif
