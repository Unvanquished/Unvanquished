/////////////////////////////////////////////////////////////////////
// HORIZON SKY - BY AVOC @ eft-clan.com
// If you wish to use this sky, please leave this part in the shader
// or give proper credit in the readme
/////////////////////////////////////////////////////////////////////

textures/horizon_sky/horizon_sky
{
	qer_editorimage textures/horizon_sky/horizon_up
	
	skyParms textures/horizon_sky/horizon 2048 -

	q3map_sunExt 1.00 0.80 0.70 200 90 35	// R G B Intensity Angle Pitch


	q3map_skylight 100 4                    //amount iterations
	q3map_noFog

	surfaceparm sky                         //flags compiler that this is sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight
	nopicmip
	nomipmaps

}