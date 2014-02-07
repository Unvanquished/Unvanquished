gfx/damage/blood
{
	cull disable
	{
		map gfx/damage/blood
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/damage/fullscreen_painblend
{
	{
		map gfx/damage/fullscreen_painblend
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
		tcMod rotate 90
	}
	{
		map gfx/damage/fullscreen_painblend
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
		tcMod rotate -90
	}
}

