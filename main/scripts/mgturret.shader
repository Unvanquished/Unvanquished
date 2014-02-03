models/buildables/mgturret/t_flash
{
	cull disable
	{
		map models/buildables/mgturret/t_flash
		blendfunc add
		rgbGen wave square 0 1 0 10
	}
}

models/buildables/mgturret/turret
{
	qer_editorimage models/buildables/mgturret/turret_diff
	diffuseMap models/buildables/mgturret/turret_diff
	normalMap models/buildables/mgturret/turret_norm
	specularMap models/buildables/mgturret/turret_spec
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

