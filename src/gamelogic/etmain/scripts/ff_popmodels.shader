models/pop/bunker/pop_reading_ff 
{ 
   qer_editorimage models/pop/bunker/pop_book_ff
   q3map_nonplanar
   q3map_shadeAngle 75 
   cull none
   { 
      map $lightmap 
      rgbGen identity 
    } 
    { 
      map models/pop/bunker/pop_book_ff 
      blendFunc GL_DST_COLOR GL_ZERO 
      rgbGen identity
   } 
}

models/pop/lights/pop_slight_mammut
{
	qer_editorimage models/pop/lights/pop_slight
	q3map_lightimage models/pop/lights/pop_slight_blend
	q3map_surfacelight 5000
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map models/pop/lights/pop_slight
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map models/pop/lights/pop_slight_blend
		blendFunc GL_ONE GL_ONE
	}
}