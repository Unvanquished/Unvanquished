///Rufus_sky - BY AVOC @ eft-clan.com /////////
//////////////////////////////////////////////

textures/rufus_sky/rufus_sky
{
	qer_editorimage textures/rufus_sky/rufus_sky_up
	
	skyParms textures/rufus_sky/rufus_sky 2048 -

	q3map_sunExt 0.90 0.83 0.71 190 225 25	// R G B Intensity Angle Pitch


	q3map_skylight 100 4                    //amount iterations
	q3map_noFog

	surfaceparm sky                         //flags compiler that this is sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight

	nopicmip
	nomipmaps

}
