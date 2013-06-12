scope
{
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
		blendFunc add
		alphaFunc GE128
	}
	{
		clampmap gfx/2d/scope/zoom
		alphaGen vertex
		blend blend
	}
	{
		map gfx/2d/scope/bg
		alphaGen vertex
		blend blend
	}

}
