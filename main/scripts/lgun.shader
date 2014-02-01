models/weapons/lgun/flash
{
	sort additive
	cull disable
	{
		map	models/weapons/lgun/flash
		blendfunc GL_ONE GL_ONE
	}
}

models/weapons/lgun/grill
{
	diffuseMap models/weapons/lgun/grill
}

models/weapons/lgun/heat
{
	{
		map models/weapons/lgun/heat
		blendfunc add
	}
}

models/weapons/lgun/sight
{
	{
		map models/weapons/rifle/lense
		blendfunc add
		tcGen environment
	}
}

models/weapons/lgun/zlasgun
{
	diffuseMap models/weapons/lgun/zlasgun
	normalMap models/weapons/lgun/zlasgun_NRM
	specularMap models/weapons/lgun/zlasgun_SPEC
}

weapons/lgun/sight
{
	{
		map models/weapons/rifle/lense
		blendfunc add
		tcGen environment
	}
}

