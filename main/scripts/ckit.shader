models/weapons/ckit/demoncore
{
	diffuseMap models/weapons/ckit/demoncore_COLOR
	normalMap models/weapons/ckit/demoncore_NRM
	specularMap models/weapons/ckit/demoncore_SPEC
}

models/weapons/ckit/rep_cyl
{
	cull disable
	{
		map models/weapons/ckit/rep_cyl
		blendfunc add
		tcMod scroll 0.2 0
	}
	{
		map models/weapons/ckit/lines2
		blendfunc add
		tcMod scroll 0 -0.2
	}
}

models/weapons/ckit/screen
{
	{
		map models/weapons/ckit/screen
	}
	{
		map models/weapons/ckit/scroll
		blendfunc add
		tcMod scroll 10 -0.4
	}
}

models/weapons/ckit/weapon
{
	diffuseMap models/weapons/ckit/weapon_COLOR
	normalMap models/weapons/ckit/weapon_NRM
	specularMap models/weapons/ckit/weapon_SPEC
}

