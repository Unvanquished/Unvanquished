models/buildables/reactor/reactor_main
{
	{
		map models/buildables/reactor/reactor_main.tga
		rgbGen lightingDiffuse
	}
	{
		map models/buildables/reactor/reactor_glow.tga
		blendfunc add
		rgbGen wave sin 0 1 0 0.5
	}
}

models/buildables/reactor/reactor_meter
{
	{
		map models/buildables/reactor/reactor_meter.tga
		rgbGen lightingDiffuse
	}
}

models/buildables/reactor/reactor_display
{
	{
		map models/buildables/reactor/reactor_display.tga
		rgbGen identity
	}
}

models/buildables/reactor/reactor_bolt
{
	cull disable
	{
		map models/buildables/reactor/reactor_bolt.tga
		blendfunc add
		rgbGen identity
		tcMod scroll 2 0
	}
}

models/buildables/repeater/energy
{
	{
		map models/buildables/repeater/energy.tga
		rgbGen wave sawtooth 0.3 1 0 0.5
		tcMod scale 2 1
		tcMod scroll 0 1
	}
}

models/buildables/repeater/repeator_panel
{
	{
		map models/buildables/repeater/repeator_panel.tga
		rgbGen identity
	}
}

models/buildables/arm/arm_panel2
{
	{
		map models/buildables/arm/arm_panel2.tga
		rgbGen identity
	}
}

models/buildables/arm/arm_panel3
{
	{
		map models/buildables/arm/arm_panel3.tga
		rgbGen identity
	}
}

