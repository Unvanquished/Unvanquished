models/buildables/drill/drill_dead
{
	diffuseMap  models/buildables/drill/drill_d
	normalMap   models/buildables/drill/drill_n
	specularMap models/buildables/drill/drill_s
}

models/buildables/drill/drill
{
	diffuseMap  models/buildables/drill/drill_d
	normalMap   models/buildables/drill/drill_n
	specularMap models/buildables/drill/drill_s
	{
		map models/buildables/drill/drill_a
		blend add
		rgbGen wave sin 0.0 0.75 0.0 0.75
	}

        when unpowered models/buildables/drill/drill_dead
        when destroyed models/buildables/drill/drill_dead
}
