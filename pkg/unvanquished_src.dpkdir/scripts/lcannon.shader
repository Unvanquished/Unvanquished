models/weapons/lcannon/flash
{
	sort additive
	cull disable
	{
		map	models/weapons/lcannon/flash
		blendfunc GL_ONE GL_ONE
	}
}

models/weapons/lcannon/skin
{
	diffuseMap models/weapons/lcannon/skin_d
	normalMap models/weapons/lcannon/skin_n
	specularMap models/weapons/lcannon/skin_s
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

models/weapons/lcannon/pulse
{
	diffuseMap models/weapons/lcannon/pulse_d
	normalMap models/weapons/lcannon/pulse_n
	specularMap models/weapons/lcannon/pulse_s
}
