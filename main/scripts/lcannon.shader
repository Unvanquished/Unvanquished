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
	diffuseMap models/weapons/lcannon/skin_COLOR
	normalMap models/weapons/lcannon/skin_NRM
	specularMap models/weapons/lcannon/skin_SPEC
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
