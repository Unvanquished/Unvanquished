models/buildables/medistat/base
{
	diffuseMap  models/buildables/medistat/medistat_d
	normalMap   models/buildables/medistat/medistat_n
	specularMap models/buildables/medistat/medistat_s

	// displays
	{
		map   models/buildables/medistat/medistat_g1
		blend add
		rgb   0.5 0.5 0.5
	}

	// hexagons
	{
		map    models/buildables/medistat/medistat_g2
		blend  add
		rgbGen wave sin 0.375 0.1875 0 0.25
	}

	when unpowered models/buildables/medistat/base_down
	when destroyed models/buildables/medistat/base_dead
}

models/buildables/medistat/base_dead
{
	diffuseMap  models/buildables/medistat/medistat_d
	normalMap   models/buildables/medistat/medistat_n
	specularMap models/buildables/medistat/medistat_s

	{
		map   models/buildables/medistat/medistat_dead
		blend filter
	}
}

models/buildables/medistat/base_down
{
	diffuseMap  models/buildables/medistat/medistat_d
	normalMap   models/buildables/medistat/medistat_n
	specularMap models/buildables/medistat/medistat_s
}

models/buildables/medistat/healing
{
	cull none
	surfaceparm nolightmap

	{
		map   models/buildables/medistat/active
		blend blend
	}

	{
		map    models/buildables/medistat/active_noise
		blend  add
		rgb    0.05 0.05 0.05

		tcMod rotate 60
		tcmod scale 2 4
		tcmod stretch sin 1 0.5 0 0.5
		tcMod scroll -1 0.5
	}
}
