// model projection shadow shaders

models/mapobjects/tanks_sd/shadow_tank
{
	polygonOffset
	{
		clampmap models/mapobjects/tanks_sd/shadow_tank.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}

