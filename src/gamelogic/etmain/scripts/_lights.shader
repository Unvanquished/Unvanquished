

table neontable2 { snap { 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1 } }

lights/redglow
{	
	{
		forceHighQuality
		map	lights/squarelight
		red 	bathroomtable [ time * .02 ] * Parm0
		green 	bathroomtable [ time * .02 ] * Parm1
		blue 	bathroomtable [ time * .02 ] * Parm2
		//colored
		zeroClamp
	}
}

lights/roundfire2
{
	{
		forceHighQuality
		map	lights/round.tga
		red 	( firetable2 [Parm4 + (time / 6) ]) * Parm0
		green 	( firetable2 [Parm4 + (time / 6) ]) * Parm1
		blue 	( firetable2 [Parm4 + (time / 6) ]) * Parm2
		rotate	firelightrot [ time * (2 * parm3) ]
		zeroClamp
	}
}

lights/stormy1
{	

	{
		forceHighQuality
		map	lights/stormy2.tga
		colored
	    rotate	time * .1
		zeroClamp
		rgb		stormtable[ time * .3 ]
	}

}

// an ambient light will do non-directional bump mapping, and won't have specular lighting
// or shadows
lights/ambientLight
{
	ambientLight
	lightFalloffImage	makeintensity( lights/squarelight1a.tga )
	{
		forceHighQuality
		map lights/squarelight1.tga
		colored
		zeroClamp
	}
}

lights/ambientLight2
{
	ambientLight
	lightFalloffImage	makeintensity( lights/squarelight1a.tga )
	{
		forceHighQuality
		map lights/squarelight.tga
		colored
		zeroClamp
	}
}

lights/square_blast
{
	{
		forceHighQuality
		map		lights/squarelight1.tga
		red 		( blasttable [Parm4 + (time / 6 * (Parm3)) ]) * Parm0
		green 		( blasttable [Parm4 + (time / 6 * (Parm3)) ]) * Parm1
		blue 		( blasttable [Parm4 + (time / 6 * (Parm3)) ]) * Parm2
		zeroclamp
	}
}

lights/square_strobe
{
	{
		forceHighQuality
		map		lights/squarelight1.tga
		red 		( blinktable2 [Parm4 + (time * (6 * Parm3)) ]) * Parm0
		green 		( blinktable2 [Parm4 + (time * (6 * Parm3)) ]) * Parm1
		blue 		( blinktable2 [Parm4 + (time * (6 * Parm3)) ]) * Parm2
		zeroclamp
	}
}

lights/square_brokenneon2
{	
	{
		forceHighQuality
		map		lights/squarelight1.tga
		red 		( neontable2 [Parm4 + (time * (.15 * Parm3)) ]) * Parm0
		green 		( neontable2 [Parm4 + (time * (.15 * Parm3)) ]) * Parm1
		blue 		( neontable2 [Parm4 + (time * (.15 * Parm3)) ]) * Parm2
		zeroclamp
	}
}

lights/square_brokenneon1
{	
	{
		forceHighQuality
		map		lights/squarelight1.tga
		red 		( neontable1 [Parm4 + (time * (.2 * Parm3)) ]) * Parm0
		green 		( neontable1 [Parm4 + (time * (.2 * Parm3)) ]) * Parm1
		blue 		( neontable1 [Parm4 + (time * (.2 * Parm3)) ]) * Parm2
		zeroclamp
	}
}

lights/square_flicker2
{	
	{
		forceHighQuality
		map		lights/squarelight1.tga
		red 		( flashtable [Parm4 + (time * (.25 * Parm3)) ]) * Parm0
		green 		( flashtable [Parm4 + (time * (.25 * Parm3)) ]) * Parm1
		blue 		( flashtable [Parm4 + (time * (.25 * Parm3)) ]) * Parm2
		zeroclamp
	}
}

lights/square_flicker
{	
	{
		forceHighQuality
		map		lights/squarelight1.tga
		red 		((.25 * blinktable [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm0
		green 		((.25 * blinktable [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm1
		blue 		((.25 * blinktable [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm2
		zeroclamp
	}
}

lights/square_flicker3
{	
	{
		forceHighQuality
		map		lights/squarelight1.tga
		red 		((.25 * blinktable3 [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm0
		green 		((.25 * blinktable3 [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm1
		blue 		((.25 * blinktable3 [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm2
		zeroclamp
	}
}

lights/square_flicker3_spectrum
{	
	spectrum 1
	{
		forceHighQuality
		map		lights/squarelight1.tga
		red 		((.25 * blinktable3 [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm0
		green 		((.25 * blinktable3 [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm1
		blue 		((.25 * blinktable3 [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm2
		zeroclamp
	}
}

lights/square_flicker4
{	
	{
		forceHighQuality
		map		lights/squarelight1.tga
		red 		((.25 * blinktable4 [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm0
		green 		((.25 * blinktable4 [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm1
		blue 		((.25 * blinktable4 [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm2
		zeroclamp
	}
}

lights/round_sin
{	
	{
		forceHighQuality
		map			lights/round.tga
		red 		( sintable [Parm4 + (time * Parm3)] ) * Parm0
		green 		( sintable [Parm4 + (time * Parm3)] ) * Parm1
		blue 		( sintable [Parm4 + (time * Parm3)] ) * Parm2
		zeroclamp
	}
}

lights/round_strobe
{	
	{
		forceHighQuality
		map		lights/round.tga
		red 		( blinktable2 [Parm4 + (time * (6 * Parm3)) ]) * Parm0
		green 		( blinktable2 [Parm4 + (time * (6 * Parm3)) ]) * Parm1
		blue 		( blinktable2 [Parm4 + (time * (6 * Parm3)) ]) * Parm2
		zeroclamp
	}
}

lights/round_brokenneon2
{	
	{
		forceHighQuality
		map		lights/round.tga
		red 		( neontable2 [Parm4 + (time * (.15 * Parm3)) ]) * Parm0
		green 	( neontable2 [Parm4 + (time * (.15 * Parm3)) ]) * Parm1
		blue 		( neontable2 [Parm4 + (time * (.15 * Parm3)) ]) * Parm2
		zeroclamp
	}
}

lights/round_brokenneon1
{	
	{
		forceHighQuality
		map		lights/round.tga
		red 		( neontable1 [Parm4 + (time * (.2 * Parm3)) ]) * Parm0
		green 	( neontable1 [Parm4 + (time * (.2 * Parm3)) ]) * Parm1
		blue 		( neontable1 [Parm4 + (time * (.2 * Parm3)) ]) * Parm2
		zeroclamp
	}
}

lights/round_flicker2
{	
	{
		forceHighQuality
		map		lights/round.tga
		red 		( flashtable [Parm4 + (time * (.25 * Parm3)) ]) * Parm0
		green 		( flashtable [Parm4 + (time * (.25 * Parm3)) ]) * Parm1
		blue 		( flashtable [Parm4 + (time * (.25 * Parm3)) ]) * Parm2
		zeroclamp
	}
}

lights/round_flicker
{	
	{
		forceHighQuality
		map		lights/round.tga
		red 		((.25 * blinktable [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm0
		green 		((.25 * blinktable [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm1
		blue 		((.25 * blinktable [Parm4 + (time * (15 * Parm3)) ]) +.75) * Parm2
		zeroclamp
	}
}

lights/squarelight
{
	{
		forceHighQuality
		map	lights/squarelight.tga
		zeroClamp
		colored
	}
}


table spark_neontable1 { { 1.3, 0, 0, 0, 0, 1.3, 0, 0, 0, 0, 1.3, 0, 0, 1.1, .075, 1.15, 1.22, 1.3, 1.3, 1.45, 1.52, .6, .67, .75, .82, .9, .95, 1, 0, 0, 0, .3, 0, 0, 0, 0, 0, 0, 0, .3, 0, 0, 0, 0, 0, 0, 0, 0, 1, .6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, .3, 0, 0, 0, 0 } }

//
// Tr3B - new lights
//

lights/defaultDynamicLight
{
	{
		stage attenuationMapZ
		map makeintensity(lights/squarelight1a.tga)
		edgeClamp
	}
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/round.tga
		colored
		zeroClamp
	}
}

lights/defaultPointLight
{
	// this will also be the falloff for any
	// point light shaders that don't specify one
	lightFalloffImage	makeintensity( lights/squarelight1a.tga )
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/squarelight1.tga
		colored
		zeroClamp
	}
}

lights/defaultProjectedLight
{
	// by default, stay bright almost all the way to the end
//	lightFalloffImage	_noFalloff

//	lightFalloffImage	lights/skyline1.png
	lightFalloffImage	makeintensity(lights/squarelight1b.png)
//	lightFalloffImage	makeintensity(lights/flashoff.tga)
	
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/squarelight1.tga
		colored
		zeroClamp
	}
}


lights/roqVideoLight
{
	lightFalloffImage	makeintensity(lights/squarelight1b.tga)
//	lightFalloffImage	lights/skyline1.png
//	lightFalloffImage	_noFalloff
	{
		stage attenuationMapXY
		colored
		videoMap video/intro.RoQ
		
		//scale 1, 0.9
		//scroll time * 0.1, 0
		//scroll 0, time * 0.1
		rotate 180
	}
}

lights/stormLight
{
	{
		if(storm3Table[time * 0.1])
	
		stage attenuationMapXY
		forceHighQuality		
		map	lights/squarelight.tga
		colored
	    rotate	time * .1
		zeroClamp
		rgb storm3Table[time * 0.1]
	}
}

lights/flashLight
{
//	lightFalloffImage	_noFalloff
//	lightFalloffImage	lights/skyline1.png
	lightFalloffImage	makeintensity(lights/squarelight1b.png)
	
	{
		stage attenuationMapXY
		forceHighQuality
		map	lights/round.png
		zeroClamp
		colored
	}
}

lights/flashLight1
{
	lightFalloffImage	makeintensity(lights/squarelight1b.png)
	
	{
		stage attenuationMapXY
		forceHighQuality
		map	lights/flashlight1.png
		zeroClamp
		colored
	}
}

lights/roundFire
{
	lightFalloffImage	makeintensity(lights/squarelight1a.tga)
	{
		stage attenuationMapXY
		forceHighQuality
		map	lights/round.tga
		red 	( firetable [Parm4 + (time / 6 * (Parm3)) ]) * Parm0
		green 	( firetable [Parm4 + (time / 6 * (Parm3)) ]) * Parm1
		blue 	( firetable [Parm4 + (time / 6 * (Parm3)) ]) * Parm2
		rotate	firelightrot [ time * 2 ]
		zeroClamp
	}
}

lights/volumetricLight
{
	volumetricLight
	lightFalloffImage	makeintensity( lights/squarelight1a.tga )
	
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/squarelight1.tga
		colored
		zeroClamp
	}
}

lights/shadowProjectedLight
{
	lightFalloffImage	makeintensity(lights/squarelight1b.png)
	
	{
		stage attenuationMapXY
		forceHighQuality
		map lights/squarelight1.tga
		colored
		//red 	Parm0 * Parm3
		//green 	Parm0 * Parm3
		//blue 	Parm0 * Parm3
		//alpha	1.0
		zeroClamp
	}
}
