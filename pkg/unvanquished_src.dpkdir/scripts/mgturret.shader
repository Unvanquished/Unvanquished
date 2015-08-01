models/buildables/mgturret/turret
{
	diffuseMap  models/buildables/mgturret/turret_d
	normalMap   models/buildables/mgturret/turret_n
	specularMap models/buildables/mgturret/turret_s
}

// legacy turret
// -------------

models/buildables/mgturret/t_flash
{
	cull disable
	{
		map models/buildables/mgturret/t_flash
		blendfunc add
		rgbGen wave square 0 1 0 10
	}
}

models/buildables/mgturret/turret_coil
{
	cull disable
	{
		map models/buildables/mgturret/turret_coil
		rgbGen lightingDiffuse
		alphaFunc GE128
	}
}

models/buildables/mgturret/turret_shiny
{
	{
		map models/buildables/mgturret/turret_shiny
		rgbGen lightingDiffuse
	}
	{
		map models/buildables/mgturret/ref_map
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
}

