models/buildables/overmind/pod_strands
{
	cull disable
	{
		map models/buildables/eggpod/pod_strands
		rgbGen lightingDiffuse
		alphaFunc GE128
	}
}

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

