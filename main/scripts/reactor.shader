models/buildables/reactor/reactor1
{
	qer_editorimage models/buildables/reactor/reactor_core_d.tga
	diffuseMap models/buildables/reactor/reactor_core_d.tga
	{
		stage specularMap
		map models/buildables/reactor/reactor_core_s.tga
		specularExponentMin 10
		specularExponentMax 25
		
	}
	normalMap models/buildables/reactor/reactor_core_n.tga
	{
		map       models/buildables/reactor/reactor_core_glow.tga
		blendfunc add
		rgbGen    wave sin 1.0 0.85 0.5 0.08
	}	
}

models/buildables/reactor/reactor2
{
	qer_editorimage models/buildables/reactor/reactor_arm_d.tga
	diffuseMap models/buildables/reactor/reactor_arm_d.tga
	normalMap models/buildables/reactor/reactor_arm_n.tga
	{
		stage specularMap
		map models/buildables/reactor/reactor_arm_s.tga
		specularExponentMin 10
		specularExponentMax 25

	}
	// big lamps
	{
		map       models/buildables/reactor/reactor_arm_glow1.tga
		blendfunc add
		//rgbGen    wave sawtooth -0.2 2.0 0.0 0.4
		rgbGen    wave inversesawtooth 0.0 1.0 0.0 0.4
	}
	// small lamps
	{
		map       models/buildables/reactor/reactor_arm_glow2.tga
		blendfunc add
		rgb       0.85 0.85 0.85
	}

}

