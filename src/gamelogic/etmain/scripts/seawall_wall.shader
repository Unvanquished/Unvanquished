// *************************************************************
// *************************************************************
// OUT OF DATE - - MAKE FUTURE CHANGES TO "battery_wall.shader"
// *************************************************************
// *************************************************************

// thanks for leaving this legacy stuff now i have to make it 
// backwards compatible because someone might have used it in
// a custom lvl - Eonfge May 2011

textures/seawall_wall/metal_trim1
{
	surfaceparm		metalsteps
	implicitMap 	-
	
	qer_editorimage textures/seawall_wall/metal_trim1
	diffusemap		textures/seawall_wall/metal_trim1
	bumpmap         displacemap( textures/seawall_wall/metal_trim1_norm invertColor(textures/seawall_wall/metal_trim1_disp) )
	pecularmap		textures/seawall_wall/metal_trim1
	
}

textures/seawall_wall/wall02_mid
{
    qer_editorimage textures/seawall_wall/wall02_mid
	diffusemap		textures/seawall_wall/wall02_mid
	bumpmap         displacemap( textures/seawall_wall/wall02_norm, invertColor(textures/seawall_wall/wall02_disp) )
	specularmap		textures/seawall_wall/wall02_spec
	
}

textures/seawall_wall/wall03_mid
{
    qer_editorimage textures/seawall_wall/wall03_mid
	diffusemap		textures/seawall_wall/wall03_mid
	bumpmap         displacemap( textures/seawall_wall/wall03_norm, invertColor(textures/seawall_wall/wall03_disp) )
	specularmap		textures/seawall_wall/wall03_spec
	
}

textures/battery_wall/wall03_top
{
    qer_editorimage textures/seawall_wall/wall03_top
	diffusemap		textures/seawall_wall/wall03_top
	bumpmap         displacemap( textures/seawall_wall/wall03_norm, invertColor(textures/seawall_wall/wall03_disp) )
	specularmap		textures/seawall_wall/wall03_spec
	
}
