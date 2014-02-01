MarkBlasterBullet
{
	polygonOffset
	{
		map gfx/marks/mark_blaster
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen exactVertex
		alphaGen vertex
	}
}

MarkPulseRifleBullet
{
	polygonOffset
	{
		map gfx/marks/mark_pulserifle
		//map models/weapons/rifle/lense
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}

MarkRifleBullet
{
	polygonOffset
	{
		map gfx/marks/mark_rifle
		//map models/weapons/rifle/lense
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}

MarkShotgunBullet
{
	polygonOffset
	{
		map gfx/marks/mark_shotgun
		//map models/weapons/rifle/lense
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}

blasterbullet
{
	cull disable
	{
		map gfx/weapons/blasterbullet
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		rgbGen vertex
	}
}

console
{
	nopicmip
	nomipmaps
	{
		map gfx/colors/black
	}
}

console2
{
	nopicmip
	nomipmaps
	{
		map gfx/colors/black
	}
}

creep
{
	nopicmip
	polygonoffset
	twoSided
	{
		clampmap gfx/creep/creep
		blendfunc blend
		blend diffusemap
		rgbGen identity
		alphaGen vertex
		alphaFunc GE128
	}
	specularMap gfx/creep/creep_s
	normalMap gfx/creep/creep_n
}

//  BEST FLAM THROEWR EVAR!
flame1
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame00
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame10
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame09
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame11
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame10
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame12
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame11
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame13
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame12
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame14
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame13
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame15
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame14
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame16
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame15
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame17
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame16
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame18
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame17
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame19
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame18
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame2
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame01
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame20
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame19
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame21
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame20
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame22
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame21
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame23
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame22
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame24
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame23
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame25
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame24
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame3
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame02
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame4
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame03
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame5
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame04
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame6
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame05
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame7
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame06
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame8
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame07
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flame9
{
	nopicmip
	cull disable
	{
		map gfx/flame/flame08
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer0
{
	cull none
	entityMergable
	{
		map gfx/flamer/0
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer1
{
	cull none
	entityMergable
	{
		map gfx/flamer/1
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer10
{
	cull none
	entityMergable
	{
		map gfx/flamer/10
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer11
{
	cull none
	entityMergable
	{
		map gfx/flamer/11
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer12
{
	cull none
	entityMergable
	{
		map gfx/flamer/12
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer13
{
	cull none
	entityMergable
	{
		map gfx/flamer/13
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer14
{
	cull none
	entityMergable
	{
		map gfx/flamer/14
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer15
{
	cull none
	entityMergable
	{
		map gfx/flamer/15
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer16
{
	cull none
	entityMergable
	{
		map gfx/flamer/16
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer17
{
	cull none
	entityMergable
	{
		map gfx/flamer/17
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer18
{
	cull none
	entityMergable
	{
		map gfx/flamer/18
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer19
{
	cull none
	entityMergable
	{
		map gfx/flamer/19
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer2
{
	cull none
	entityMergable
	{
		map gfx/flamer/2
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer20
{
	cull none
	entityMergable
	{
		map gfx/flamer/20
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer21
{
	cull none
	entityMergable
	{
		map gfx/flamer/21
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer22
{
	cull none
	entityMergable
	{
		map gfx/flamer/22
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer23
{
	cull none
	entityMergable
	{
		map gfx/flamer/23
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer24
{
	cull none
	entityMergable
	{
		map gfx/flamer/24
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer3
{
	cull none
	entityMergable
	{
		map gfx/flamer/3
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer4
{
	cull none
	entityMergable
	{
		map gfx/flamer/4
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer5
{
	cull none
	entityMergable
	{
		map gfx/flamer/5
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer6
{
	cull none
	entityMergable
	{
		map gfx/flamer/6
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer7
{
	cull none
	entityMergable
	{
		map gfx/flamer/7
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer8
{
	cull none
	entityMergable
	{
		map gfx/flamer/8
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

flamer9
{
	cull none
	entityMergable
	{
		map gfx/flamer/9
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}

lucybullet
{
	cull disable
	{
		map gfx/weapons/lucybullet
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		rgbGen vertex
	}
}

outline
{
	cull none
	nopicmip
	nomipmaps
	{
		map gfx/2d/outline
		blendfunc	GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

//  projectionShadow is used for cheap squashed model shadows
projectionShadow
{
	polygonOffset
	deformVertexes projectionShadow
	{
		map			*white
		blendFunc GL_ONE GL_ZERO
		rgbGen wave square 0 0 0 0
	}
}

scope
{
	{
		map gfx/2d/scope/bg
		alphaGen vertex
		blend blend
	}
	{
		map gfx/2d/scope/circle1
		alphaGen vertex
		blend blend
	}
	{
		clampmap gfx/2d/scope/circle2
		alphaGen vertex
		blend blend
		tcMod rotate 45
	}
	{
		clampmap gfx/2d/scope/crosshair
		alphaGen vertex
		blend blend
	}
	{
		clampmap gfx/2d/scope/zoom
		alphaGen vertex
		blend blend
	}
}

white
{
	{
		map *white
		blendfunc	GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

