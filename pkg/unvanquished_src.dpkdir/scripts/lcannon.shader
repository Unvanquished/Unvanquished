gfx/lcannon/primary
{
	cull disable
	{
		animmap 24 gfx/lcannon/primary_1 gfx/lcannon/primary_2 gfx/lcannon/primary_3 gfx/lcannon/primary_4
		blendFunc GL_ONE GL_ONE
	}
}

gfx/lcannon/ripple
{
	cull disable
	entityMergable
	{
		map gfx/lcannon/ripple
		blendFunc filter
	}
}

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

