models/mapobjects/stasis/chamber
{
	{
		map models/mapobjects/stasis/chamber.tga
		rgbGen vertex
	}
}

models/mapobjects/stasis/lifemeter
{
	{
		map models/mapobjects/stasis/lifemeter.tga
		rgbGen vertex
	}
}

models/mapobjects/stasis/lifemeter2
{
	{
		map models/mapobjects/stasis/meters.tga
		rgbGen identityLighting
		tcMod scroll 0.5 0
	}
	{
		map models/mapobjects/stasis/lifemeter2.tga
		alphaFunc GE128
	}
}

models/mapobjects/stasis/window
{
	{
		map models/mapobjects/stasis/window.tga
		blendfunc filter
	}
	{
		map models/mapobjects/stasis/bubbles.tga
		blendfunc add

		tcMod scroll 0 0.2
		tcMod scale 2 1
	}
}

models/mapobjects/stasis/flowpipe
{
	{
		map models/mapobjects/stasis/flowpipe.tga

		tcMod scale 5 5
		tcMod scroll 1 0
	}
	{
		map models/buildables/overmind/ref2.tga
		blendfunc filter

		tcGen environment
	}
}

models/mapobjects/stasis/bubbles
{
	{
		map models/mapobjects/stasis/bubbles.tga
		rgbGen wave noise 0 1 0 1
		tcMod scroll 0 0.5
	}
}

