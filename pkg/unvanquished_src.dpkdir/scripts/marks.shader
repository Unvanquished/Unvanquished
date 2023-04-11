gfx/marks/bullet_mrk
{
	polygonOffset
	{
		map gfx/marks/bullet_mrk
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}

gfx/marks/burn_mrk
{
	polygonOffset
	{
		map gfx/marks/burn_mrk
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}

gfx/marks/plasma_mrk
{
	polygonOffset
	{
		map gfx/marks/plasma_mrk
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

//   markShadow is the very cheap blurry blob underneath the player
gfx/marks/shadow
{
	polygonOffset
	{
		map gfx/marks/shadow
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

//   wake is the mark on water surfaces for paddling players
gfx/marks/wake
{
	polygonOffset
	{
		clampmap gfx/marks/splash
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcmod rotate 250
		tcMod stretch sin .9 0.1 0 0.7
		rgbGen wave sin .7 .3 .25 .5
	}
	{
		clampmap gfx/marks/splash
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcmod rotate -230
		tcMod stretch sin .9 0.05 0 0.9
		rgbGen wave sin .7 .3 .25 .4
	}
}

