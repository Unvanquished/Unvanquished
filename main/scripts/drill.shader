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

// Used for the glowing effect of the low poly drill model
// The low poly drill steals its body shader from the low poly repeater (lazy, lazy Ishq!)
models/buildables/drill/energy
{
        {
                map models/buildables/drill/energy.jpg
                rgbGen wave sawtooth 0.3 1 0 0.5
                tcMod scale 2 1
                tcMod scroll 0 1
        }
}
