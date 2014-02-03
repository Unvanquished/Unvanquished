models/buildables/telenode/energy
{
	{
		map models/buildables/telenode/energy
		rgbGen wave inversesawtooth 0.2 0.4 0 1
		tcMod rotate 10
	}
}

models/buildables/telenode/rep_cyl
{
	cull disable
	{
		map models/buildables/telenode/rep_cyl
		blendfunc add
		rgbGen lightingDiffuse
		tcMod scroll 0.2 0
	}
	{
		map models/buildables/telenode/lines2
		blendfunc add
		rgbGen identity
		tcMod scroll 0 0.2
	}
}

models/buildables/telenode/telenode_parts
{
	{
		map models/buildables/telenode/telenode_parts
		rgbGen lightingDiffuse
	}
	{
		map models/buildables/overmind/ref2
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
}

models/buildables/telenode/telenode_top
{
	{
		map models/buildables/telenode/telenode_top
		rgbGen lightingDiffuse
	}
	{
		map models/buildables/overmind/ref2
		blendfunc filter
		rgbGen identity
		tcGen environment
	}
}

