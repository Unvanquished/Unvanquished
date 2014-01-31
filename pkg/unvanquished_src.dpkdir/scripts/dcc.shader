models/buildables/dcc/comp_pipes
{
	{
		map models/buildables/dcc/comp_pipes
		rgbGen lightingDiffuse
		tcGen environment
	}
}

models/buildables/dcc/comp_display
{
	{
		map models/buildables/dcc/comp_grad
		rgbGen identity
		tcMod scroll 0 1
	}
	{
		map models/buildables/dcc/comp_display
		blendfunc gl_one gl_src_alpha
		rgbGen identity
	}
}

