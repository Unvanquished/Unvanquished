models/buildables/medistat/base
{
	qer_editorimage models/buildables/medistat/medistat_d
	diffuseMap models/buildables/medistat/medistat_d
	normalMap models/buildables/medistat/medistat_n
	{
		stage specularMap
		map models/buildables/medistat/medistat_s
		specularExponentMin 12
		specularExponentMax 128
	}
	// displays
	{
		map models/buildables/medistat/medistat_g1
		blendfunc add
		rgb 0.5 0.5 0.5
	}
	// hexagons
	{
		map models/buildables/medistat/medistat_g2
		blendfunc add
		rgbGen wave sin 0.375 0.1875 0 0.25
	}
	when unpowered models/buildables/medistat/base_down
	when destroyed models/buildables/medistat/base_dead
}

models/buildables/medistat/base_dead
{
	diffuseMap models/buildables/medistat/medistat_d
	normalMap models/buildables/medistat/medistat_n
	{
		stage specularMap
		map models/buildables/medistat/medistat_s
		specularExponentMin 12
		specularExponentMax 128
	}
	{
		map models/buildables/medistat/medistat_dead
		blendfunc filter
	}
}

models/buildables/medistat/base_down
{
	diffuseMap models/buildables/medistat/medistat_d
	normalMap models/buildables/medistat/medistat_n
	{
		stage specularMap
		map models/buildables/medistat/medistat_s
		specularExponentMin 12
		specularExponentMax 128
	}
}

models/buildables/medistat/healing
{
	cull none
	{
		map models/buildables/medistat/active
		blendfunc blend
		rgbGen lightingDiffuse
	}
	{
		map models/buildables/medistat/active_noise
		blendfunc GL_SRC_ALPHA GL_ONE
		rgbGen wave noise 0.0546875 0.0234375 35 0.25
		// let's have some fun with this
		tcMod rotate 59
		tcmod scale 2 4
		tcmod stretch sin 1 0.5 0 0.497
		tcMod scroll -0.997 0.5
	}
}

