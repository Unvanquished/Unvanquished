gfx/prifle/red_blob
{
	cull disable
	{
		map gfx/prifle/red_blob
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
	}
}

gfx/prifle/red_streak
{
	nomipmaps
	cull disable
	{
		map gfx/prifle/red_streak
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen vertex
	}
}

models/weapons/prifle/flash
{
	sort additive
	cull disable
	{
		map	models/weapons/prifle/flash
		blendfunc GL_ONE GL_ONE
	}
}

models/weapons/prifle/lense
{
	{
		map models/weapons/prifle/lense
		blendfunc add
		tcGen environment
	}
}

models/weapons/prifle/misc
{
	diffuseMap models/weapons/prifle/misc_COLOR
	normalMap models/weapons/prifle/misc_NRM
	specularMap models/weapons/prifle/misc_SPEC
}

models/weapons/prifle/sight
{
	diffuseMap models/weapons/prifle/sight_COLOR
	normalMap models/weapons/prifle/sight_NRM
	specularMap models/weapons/prifle/sight_SPEC
}

models/weapons/prifle/zprifle
{
	diffuseMap models/weapons/prifle/zprifle_COLOR
	normalMap models/weapons/prifle/zprifle_NRM
	specularMap models/weapons/prifle/zprifle_SPEC
}

