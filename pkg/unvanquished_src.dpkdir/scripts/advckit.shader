models/weapons/ackit/rep_cyl
{
	cull disable
	{
		map models/weapons/ackit/rep_cyl
		blendfunc add
		rgbGen lightingDiffuse
		tcMod scroll 0.2 0
	}
	{
		map models/weapons/ackit/lines2
		blendfunc add
		rgbGen identity
		tcMod scroll 0 -0.2
	}
}

models/weapons/ackit/particle
{
	cull disable
	{
		map models/weapons/ackit/particle
		blendfunc add
		rgbGen identity
		tcMod scroll 0.02 -0.4
	}
}

models/weapons/ackit/screen
{
	{
		map models/weapons/ackit/screen
	}

	{
		map models/weapons/ackit/scroll
		blendfunc add
		rgbGen lightingDiffuse
		tcMod scroll 10.0 -0.2
	}
}


models/weapons/ackit/screen2
{
	{
		map models/weapons/ackit/screen2
	}

	{
		map models/weapons/ackit/scroll2
		blendfunc add
		rgbGen lightingDiffuse
		tcMod scroll 0.2 -10.0
	}
}