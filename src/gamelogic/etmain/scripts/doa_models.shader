// DOA custom models - FireFly

models/doa/search_light/s_light_main
{ 	qer_editorimage models/doa/search_light/s_light_main
  	implicitMap models/doa/search_light/s_light_main
}

models/doa/search_light/s_light_lamp
{ 	qer_editorimage models/doa/search_light/s_light_lamp
  	implicitMap models/doa/search_light/s_light_lamp
}

models/doa/doa_cabinet/ammo_crate
{
	qer_editorimage map models/doa/doa_cabinet/ammo_outside
    	//cull twosided
	{
		map textures/effects/envmap_slate_90
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/doa/doa_cabinet/ammo_outside
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/doa/doa_cabinet/health_crate
{
	qer_editorimage map models/doa/doa_cabinet/health_outside
    	//cull twosided
	{
		map textures/effects/envmap_slate_90
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/doa/doa_cabinet/health_outside
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/doa/doa_cabinet/ammo_inside
{
	qer_editorimage map models/doa/doa_cabinet/ammo_inside
    	cull twosided
	{
		map textures/effects/envmap_slate_90
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/doa/doa_cabinet/ammo_inside
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}

models/doa/doa_cabinet/health_inside
{
	qer_editorimage models/doa/doa_cabinet/health_inside
    	//cull twosided
	{
		map textures/effects/envmap_slate_90
		rgbGen lightingdiffuse
		tcGen environment
	}
	{
		map models/doa/doa_cabinet/health_inside
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
		rgbGen lightingdiffuse
	}
}


