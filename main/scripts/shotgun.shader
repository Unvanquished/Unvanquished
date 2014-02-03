models/weapons/shotgun/flash
{
	sort additive
	cull disable
	{
		map	models/weapons/shotgun/flash
		blendfunc GL_ONE GL_ONE
	}
}

models/weapons/shotgun/shotgun
{
	diffuseMap models/weapons/shotgun/shotgun_COLOR
	normalMap models/weapons/shotgun/shotgun_NRM
	specularMap models/weapons/shotgun/shotgun_SPEC
}

MarkShotgunBullet
{
	polygonOffset
	{
		map gfx/marks/mark_shotgun
		//map models/weapons/rifle/lense
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}
