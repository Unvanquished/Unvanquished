models/buildables/dcc/comp_pipes
{
	{
		map models/buildables/dcc/comp_pipes.tga
		rgbGen lightingDiffuse
		tcGen environment
	}
}

models/buildables/dcc/comp_display
{
	{
		map models/buildables/dcc/comp_grad.tga
		rgbGen identity
		tcMod scroll 0 1
	}
	{
		map models/buildables/dcc/comp_display.tga
		blendfunc gl_one gl_src_alpha
		rgbGen identity
	}
}

