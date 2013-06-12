models/players/level2/level2adv
{
  {
    map models/players/level2/lvl2_fx.tga
    blendFunc GL_ONE GL_ZERO
    tcmod scale 7 7
    tcMod scroll 5 -5
    tcmod rotate 360
    rgbGen identity
  }

  {
    map models/players/level2/adv.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen lightingDiffuse
  }
}

models/players/level2/electric_s
{
  {
    map models/players/level2/electric.jpg
    blendfunc add
    tcMod scroll 10.0 0.5
  }
}

models/weapons/level2/zzap
{
	{
		map models/weapons/level2/zzap.tga
		blendFunc add
	}
}

models/weapons/level2/zzap2
{
	{
		map models/weapons/level2/zzap2.tga
		blendFunc add
	}
}
