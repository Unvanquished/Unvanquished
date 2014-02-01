gfx/blaster/orange_particle
{
	cull disable
	{
		map gfx/blaster/orange_particle
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
		rgbGen vertex
	}
}

models/weapons/blaster/blasterz
{
	diffuseMap models/weapons/blaster/blasterz_COLOR
	normalMap  models/weapons/blaster/blasterz_NRM
	specularMap models/weapons/blaster/blasterz_SPEC
	glowMap     models/weapons/blaster/blaster_glow
}

models/weapons/blaster/flash
{
	sort additive
	cull disable
	{
		map	models/weapons/blaster/flash
		blendfunc GL_ONE GL_ONE
	}
}

