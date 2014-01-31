models/buildables/tesla/tesla_main
{
	{
		map models/buildables/tesla/tesla_main
		rgbGen lightingDiffuse
	}
	{
		map models/buildables/overmind/ref2
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
}

models/buildables/tesla/tesla_ball
{
	{
		map models/buildables/tesla/tesla_ball
		rgbGen lightingDiffuse
		tcGen environment
	}
}

models/buildables/tesla/tesla_grill
{
	{
		map models/buildables/tesla/tesla_grill
		rgbGen wave sin 0 1 0 0.4
	}
}

models/buildables/tesla/tesla_spark
{
	cull disable
	{
		map models/buildables/tesla/tesla_spark
		blendfunc add
		rgbGen identity
	}
}

models/ammo/tesla/tesla_bolt
{
	cull disable
	{
		map models/ammo/tesla/tesla_bolt
		blendfunc add
		rgbGen vertex
		tcMod scroll 0.2 0
	}
	{
		map models/ammo/tesla/tesla_bolt
		blendfunc add
		rgbGen wave sin 0 1 0 5
		tcMod scroll 0.5 0
		tcMod scale -1 1
	}
}

