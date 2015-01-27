models/buildables/repeater/repeater
{
	diffuseMap models/buildables/repeater/repeater_d.tga
	normalMap models/buildables/repeater/repeater_n.tga
	{
		stage specularMap
		map models/buildables/repeater/repeater_s.tga
	}
	// white lamp on top
	{
		map models/buildables/repeater/repeater_g2.tga
		blendfunc add
		rgbGen    wave sin 1.0 0.85 0.5 0.08
	}
	// small yellow lamps
	{
		map models/buildables/repeater/repeater_g1.tga
		blendfunc add
		rgb       0.85 0.85 0.85
	}

	when destroyed models/buildables/repeater/repeater_dead
}

models/buildables/repeater/repeater_dead
{
	diffuseMap models/buildables/repeater/repeater_d.tga
	normalMap models/buildables/repeater/repeater_n.tga
	{
		stage specularMap
		map models/buildables/repeater/repeater_s.tga
	}
}