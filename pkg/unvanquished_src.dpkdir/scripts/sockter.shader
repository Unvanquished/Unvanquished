//   ======================================================================
//   Placeholder for fire caused by flamer & firebomb
//   Wall Torch Stuff - Thanks to Sockter / dasprid
//   ======================================================================
textures/citadel/sockter/torchflame
{
	qer_editorimage	textures/citadel/sockter/flame1_editor
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	polygonOffset
	cull none
	//	q3map_surfacelight 1000
	sort nearest
	{
		animMap 10 textures/citadel/sockter/flame1 textures/citadel/sockter/flame2 textures/citadel/sockter/flame3 textures/citadel/sockter/flame4 textures/citadel/sockter/flame5 textures/citadel/sockter/flame6 textures/citadel/sockter/flame7 textures/citadel/sockter/flame8
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
	}
	{
		animMap 10 textures/citadel/sockter/flame2 textures/citadel/sockter/flame3 textures/citadel/sockter/flame4 textures/citadel/sockter/flame5 textures/citadel/sockter/flame6 textures/citadel/sockter/flame7 textures/citadel/sockter/flame8 textures/citadel/sockter/flame1
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}
	{
		map textures/citadel/sockter/flameball
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6
	}
}

