// shaders required by engine

white
{
	cull none
	{
		map $whiteimage
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

console
{
	nopicmip
	nomipmaps
	{
		map $blackimage
	}
}

// console font fallback
gfx/2d/bigchars
{
	nopicmip
	nomipmaps
	{
		map gfx/2d/bigchars
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

lights/defaultDynamicLight
{
	{
		stage attenuationMapZ
		map lights/mkintsquarelight1a
		edgeClamp
	}
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/round
		colored
		zeroClamp
	}
}

lights/defaultPointLight
{
	// this will also be the falloff for any
	// point light shaders that don't specify one
	lightFalloffImage lights/mkintsquarelight1a
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/squarelight1
		colored
		zeroClamp
	}
}

lights/defaultProjectedLight
{
	// by default, stay bright almost all the way to the end
	lightFalloffImage lights/mkintsquarelight1b
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/squarelight1
		colored
		zeroClamp
	}
}
