models/buildables/medistat/medistat
{
	diffuseMap models/buildables/medistat/medipad_lp_d
    	normalMap models/buildables/medistat/medipad_lp_n
	{
		stage specularMap
		map models/buildables/medistat/medipad_lp_s
		specularExponentMin 10
		specularExponentMax 25

	}
	{
		map models/buildables/medistat/medipad_lp_e
		blendfunc add
	}
}
models/buildables/medistat/rings2
{
	{
		map models/buildables/medistat/noise
		tcMod scale 6 6
		blendfunc filter
		tcMod scroll -0.1 0.1
	}
	{
		map models/buildables/medistat/noise
		tcMod scale 4 4
		blendfunc add
		tcMod scroll 0.1 0.1
		rgbGen const ( 0 0.572549 0.690196 )
	}
}
models/buildables/medistat/display
{
	{
		AnimMap 0.8 models/buildables/medistat/display1 models/buildables/medistat/display2 models/buildables/medistat/display3 models/buildables/medistat/display4 models/buildables/medistat/display5 models/buildables/medistat/display6 models/buildables/medistat/display7 models/buildables/medistat/display8
		blendfunc add
	}
	when idle2 models/buildables/medistat/displayH
	when destroyed models/buildables/medistat/displayD
}
models/buildables/medistat/displayH
{
	{
		AnimMap 1 models/buildables/medistat/display-h1 models/buildables/medistat/display-h2 models/buildables/medistat/display-h3
		blendfunc add
	}
}
models/buildables/medistat/displayD
{
	{
		AnimMap 1 models/buildables/medistat/display1 models/buildables/medistat/display-d1 models/buildables/medistat/display-d2 models/buildables/medistat/display1 models/buildables/medistat/display-d1 models/buildables/medistat/display-d2 models/buildables/medistat/display-d1 models/buildables/medistat/display-d2
		blendfunc add
	}
}

models/buildables/medistat/rings
{
	{
		map models/buildables/medistat/noise
		tcMod scale 5 5
		blendfunc filter
		rgbGen const ( 0.678431 0.933333 0.960784 )
		tcMod scroll -0.1 0.1
		//tcGen environment
	}
	{
		map models/buildables/medistat/noise2
		tcMod scale 1 1
		blendfunc add
		tcMod scroll -0.1 -0.1
		rgbGen const ( 0 0.572549 0.690196 )
	}
	{
		map models/buildables/medistat/h_grid
		tcMod scale 1 1
		blendfunc add
		tcMod scroll 0 0.1
		rgbGen const ( 0 0.572549 0.690196 )
	}
		{
		map models/buildables/medistat/v_grid
		tcMod scale 1 1
		blendfunc add
		tcMod scroll 0.1 0
		rgbGen const ( 0 0.572549 0.690196 )
	}
}
models/buildables/medistat/scan
{
	{
		map models/buildables/medistat/noise
		tcMod scale 5 5
		blendfunc filter
		rgbGen const ( 0.678431 0.933333 0.960784 )
		tcMod scroll -0.1 0.1
	}
	{
		map models/buildables/medistat/noise2
		tcMod scale 1 1
		blendfunc add
		tcMod scroll -0.1 -0.1
		rgbGen const ( 0 0.572549 0.690196 )
	}
	{
		map models/buildables/medistat/h_grid
		tcMod scale 1 1
		blendfunc add
		tcMod scroll 0 0.1
		rgbGen const ( 0 0.572549 0.690196 )
	}
		{
		map models/buildables/medistat/v_grid
		tcMod scale 1 1
		blendfunc add
		tcMod scroll 0.1 0
		rgbGen const ( 0 0.572549 0.690196 )
	}
}
