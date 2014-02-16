models/buildables/reactor/reactor1
{
	qer_editorimage models/buildables/reactor/reactor_core_d
	diffuseMap models/buildables/reactor/reactor_core_d
	normalMap models/buildables/reactor/reactor_core_n
	{
		stage specularMap
		map models/buildables/reactor/reactor_core_s
		specularExponentMin 12
		specularExponentMax 128
	}
	{
		map       models/buildables/reactor/reactor_core_glow
		blendfunc add
		rgbGen    wave sin 1.0 0.85 0.5 0.08
	}
}

models/buildables/reactor/reactor2
{
	qer_editorimage models/buildables/reactor/reactor_arm_d
	diffuseMap models/buildables/reactor/reactor_arm_d
	normalMap models/buildables/reactor/reactor_arm_n
	{
		stage specularMap
		map models/buildables/reactor/reactor_arm_s
		specularExponentMin 12
		specularExponentMax 128
	}
	// big lamps
	{
		map       models/buildables/reactor/reactor_arm_glow
		blendfunc add
		//rgbGen    wave sawtooth -0.2 2.0 0.0 0.4
		rgbGen    wave inversesawtooth 0.375 0.75 0.0 0.4
	}
	// small lamps
	{
		map       models/buildables/reactor/reactor_arm_glow2
		blendfunc add
		rgb       0.85 0.85 0.85
	}
}

