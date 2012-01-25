

/*
author Patrick Hoesly
license http://creativecommons.org/licenses/by/2.0/
source http://www.flickr.com/photos/zooboing/4278406515/sizes/o/in/photostream/
*/
textures/tr3b/lava/lava1
{
	qer_editorImage	 textures/tr3b/lava/lava1
	
	noSelfShadow
	noshadows
	forceOpaque
	nolightmap
	lava
	 
	q3map_surfaceLight 8000
	q3map_lightRGB 1.0 0.58 0
	q3map_lightImage textures/tr3b/lava/lava1
	q3map_lightSubdivide 8

	{
		//blend add
		map textures/tr3b/lava/lava1
		translate	time * -0.02 , time * 0.05
	}
}




	
/*
author Patrick Hoesly
license http://creativecommons.org/licenses/by/2.0/
source http://www.flickr.com/photos/zooboing/4279154528/sizes/o/in/set-72157622833572432/
*/
textures/tr3b/lava/lava2
{
	 qer_editorImage	 textures/tr3b/lava/lava2
	 
	 lava

	 diffuseMap		textures/tr3b/lava/lava2
	 normalMap		displacemap(textures/tr3b/lava/lava2_n, invertColor(textures/tr3b/lava/lava2_disp) )
	 specularMap	textures/tr3b/lava/lava1_s
	 /*
	 {
		blend add
		map textures/tr3b/lava/lava1_occ.png
	 }
	 */
}