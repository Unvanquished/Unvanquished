models/buildables/overmind/pod_strands
{
	cull disable
	{
		map models/buildables/eggpod/pod_strands.tga
		rgbGen lightingDiffuse
		alphaFunc GE128
	}
}

models/buildables/overmind/over_spike
{
	{
		map models/buildables/overmind/over_spike.tga
		rgbGen lightingDiffuse
	}
	{
		map models/buildables/overmind/ref2.tga
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
}

