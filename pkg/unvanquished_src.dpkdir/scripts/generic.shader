models/generic/sphere
{
	cull disable
	{
		map $whiteimage
		rgbGen const ( 0.5 0.5 0.5 )
		alphaGen const 0.3
		blendFunc blend
	}
}

models/generic/sphericalCone64
{
	cull disable
	{
		map $whiteimage
		rgbGen const ( 0.5 0.5 0.5 )
		alphaGen const 0.3
		blendFunc blend
	}
}

models/generic/sphericalCone240
{
	cull disable
	{
		map $whiteimage
		rgbGen const ( 0.5 0.5 0.5 )
		alphaGen const 0.3
		blendFunc blend
	}
}

gfx/plainColor
{
	cull disable
	{
		map $whiteimage
		rgbGen entity
		alphaGen entity
		blendFunc GL_SRC_ALPHA GL_ONE
	}
}

