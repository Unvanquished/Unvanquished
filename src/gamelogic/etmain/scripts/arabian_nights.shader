/////////////////////////////////////////////////////////////////////
// ARABIAN NIGHTS SKY - BY AVOC @ eft-clan.com
// If you wish to use this sky, please leave this part in the shader
// or give proper credit in the readme
/////////////////////////////////////////////////////////////////////

textures/arabian_nights/arabian_nights_sky
{
	qer_editorimage textures/arabian_nights/arabian_nights_up
	
	skyParms textures/arabian_nights/arabian_nights 2048 -

	q3map_sunExt 0.5 0.35 0.34 65 270 15	// R G B Intensity Angle Pitch

	q3map_skylight 100 4                    //amount iterations
	q3map_noFog

	surfaceparm sky                         //flags compiler that this is sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight

	nopicmip
	nomipmaps

}