models/players/level2/level2adv
{
  {
    map models/players/level2/lvl2_fx
    blendFunc GL_ONE GL_ZERO
    tcmod scale 7 7
    tcMod scroll 5 -5
    tcmod rotate 360
    rgbGen identity
  }

  {
    map models/players/level2/adv
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen lightingDiffuse
  }
}

models/players/level2/electric_s
{
  {
    map models/players/level2/electric
    blendfunc add
    tcMod scroll 10.0 0.5
  }
}

models/weapons/level2/zzap
{
	{
		map models/weapons/level2/zzap
		blendFunc add
	}
}

models/weapons/level2/zzap2
{
	{
		map models/weapons/level2/zzap2
		blendFunc add
	}
}

models/players/level2/level2
{
	qer_editorimage models/players/level2/level2

	diffuseMap models/players/level2/level2
	normalMap models/players/level2/level2_n
}

models/players/level2/level2upg
{
	qer_editorimage models/players/level2/level2_u

	diffuseMap models/players/level2/level2_u
	normalMap models/players/level2/level2_n
}
