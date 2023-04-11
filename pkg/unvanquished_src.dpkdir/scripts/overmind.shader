// TODO: Remove MD3 model shader
models/buildables/overmind/over_spike
{
	{
		map models/buildables/overmind/over_spike
		rgbGen lightingDiffuse
	}
	{
		map models/buildables/overmind/ref2
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
}

// TODO: Remove MD3 model shader
models/buildables/overmind/pod_strands
{
	cull disable
	{
		map models/buildables/eggpod/pod_strands
		rgbGen lightingDiffuse
		alphaFunc GE128
	}
}

models/buildables/overmind/overmind
{
	diffuseMap  models/buildables/overmind/overmind_d
	normalMap   models/buildables/overmind/overmind_n
	specularMap models/buildables/overmind/overmind_s
	glowMap     models/buildables/overmind/overmind_g
}
