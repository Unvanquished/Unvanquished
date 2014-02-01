gfx/mdriver/green_particle
{
	cull disable
	{
		map gfx/mdriver/green_particle
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/mdriver/trail
{
	nomipmaps
	cull disable
	{
		map gfx/mdriver/trail
		blendFunc blend
	}
}

models/weapons/mdriver/core
{
	diffuseMap models/weapons/mdriver/core_COLOR
	normalMap models/weapons/mdriver/core_NRM
	specularMap models/weapons/mdriver/core_SPEC
}

models/weapons/mdriver/flash
{
	sort additive
	cull disable
	{
		map	models/weapons/mdriver/flash
		blendfunc GL_ONE GL_ONE
	}
}

models/weapons/mdriver/glow
{
	cull disable
	{
		map models/weapons/mdriver/glow
		blendfunc GL_ONE GL_ONE
		tcMod scroll -9.0 9.0
	}
}

models/weapons/mdriver/lens
{
	diffuseMap models/weapons/mdriver/lens
	specularMap models/weapons/mdriver/lens_SPEC
}

models/weapons/mdriver/zmdriver
{
	diffuseMap models/weapons/mdriver/zmdriver_COLOR
	normalMap models/weapons/mdriver/zmdriver_NRM
	specularMap models/weapons/mdriver/zmdriver_SPEC
}

