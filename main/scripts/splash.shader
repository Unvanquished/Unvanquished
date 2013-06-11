models/splash/bright_star
{
	nopicmip
	{
		map models/splash/bright_star.tga
		blendfunc add
		rgbGen identity
	}
	{
		map models/splash/bright_star_2.tga
		blendfunc add
		rgbGen wave sin 0 0.5 0 0.09
	}
}

models/splash/nebula
{
	nopicmip
	{
		map models/splash/nebula.tga
		blendfunc add
		rgbGen wave sin 0.5 1 5 0.05
		tcMod scale -1 1
	}
	{
		map models/splash/nebula_2.tga
		blendfunc add
		rgbGen wave sin 0.3 1 2.5 0.05
		tcMod scale -1 1
	}
}

models/splash/trem_black
{
	nopicmip
//	{
//		map models/splash/highlights.tga
//		blendfunc add
//		rgbGen wave sin 0 0.2 0 0.2
//		tcMod scroll -0.2 0
//	}

	{
		map ui/assets/title.tga
		blendfunc blend
		//alphaFunc GE128
		rgbGen identity
	}
}

