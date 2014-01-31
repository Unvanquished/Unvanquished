//deep ocean
textures/misc/dark_water
{
	{
		map textures/misc/dark_water
		tcMod rotate 1
	}
	{
		map textures/misc/caustic
		blendfunc add
		rgbGen wave sin 0 1 0 0.05
		tcMod scale 0.5 0.5
		tcMod rotate 1
	}
	{
		map textures/misc/caustic
		blendfunc add
		rgbGen wave sin 0 1 0 -0.05
		tcMod scale -0.5 -0.5
		tcMod rotate 1
	}
}

//tank bubbles
textures/misc/bubbles
{
	cull disable
	{
		map textures/misc/bubbles
		blendfunc add
		rgbGen wave noise 0 1 0 0.02
		tcMod scroll -0.01 0.05
		tcMod scale 2 2
	}
	{
		map textures/misc/bubbles
		blendfunc add
		rgbGen wave noise 0 1 0 0.02
		tcMod scroll 0.01 0.02
		tcMod scale -2 2
	}
}

//foamy water top
textures/misc/foam
{
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm water
	deformVertexes wave 128 sin 0 5 6 0.5
	tessSize 32
	cull disable
	{
		map textures/misc/foam
		blendfunc add
	}
}

