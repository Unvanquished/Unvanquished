textures/props_sd/basketsand
{
	
	qer_editorimage 	textures/props_sd/basketsand
	diffusemap			textures/props_sd/basketsand
	bumpmap        		displacemap( textures/props_sd/basketsand_norm, invertColor(textures/props_sd/basketsand_disp) )
	specularmap			textures/props_sd/basketsand_spec
}

textures/props_sd/board_cl01m
{
	surfaceparm woodsteps
	implicitMap -
	
	qer_editorimage 	textures/props_sd/board_cl01m
	diffusemap			textures/props_sd/board_cl01m
	bumpmap        		displacemap( textures/props_sd/board_cl01m_norm, invertColor(textures/props_sd/board_cl01m_disp) )
	specularmap			textures/props_sd/board_cl01m_spec
}

textures/props_sd/board_cl02m
{
	surfaceparm woodsteps
	implicitMap -
	
	qer_editorimage 	textures/props_sd/board_cl02m
	diffusemap			textures/props_sd/board_cl02m
	bumpmap        		displacemap( textures/props_sd/board_cl02m_norm, invertColor(textures/props_sd/board_cl02m_disp) )
	specularmap			textures/props_sd/board_cl02m_spec
}

textures/props_sd/s_all
{
	surfaceparm metalsteps
	surfaceparm trans
	implicitMap -
	
	qer_editorimage 	textures/props_sd/s_all
	diffusemap			textures/props_sd/s_all
	bumpmap        		displacemap( textures/props_sd/s_all_norm, invertColor(textures/props_sd/s_all_disp) )
	specularmap			textures/props_sd/s_all_spec
}

textures/props_sd/s_ammo01
{
	surfaceparm metalsteps
	surfaceparm trans
	implicitMap -
	
	qer_editorimage 	textures/props_sd/s_ammo01
	diffusemap			textures/props_sd/s_ammo01
	bumpmap        		displacemap( textures/props_sd/s_all_norm, invertColor(textures/props_sd/s_all_disp) )
	specularmap			textures/props_sd/s_all_spec
}

textures/props_sd/s_casemate01
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage 	textures/props_sd/s_casemate01
	diffusemap			textures/props_sd/s_casemate01
	bumpmap        		displacemap( textures/props_sd/s_all_norm, invertColor(textures/props_sd/s_all_disp) )
	specularmap			textures/props_sd/s_all_spec
}

textures/props_sd/s_casemate02
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage 	textures/props_sd/s_casemate02
	diffusemap			textures/props_sd/s_casemate02
	bumpmap        		displacemap( textures/props_sd/s_all_norm, invertColor(textures/props_sd/s_all_disp) )
	specularmap			textures/props_sd/s_all_spec
}

textures/props_sd/s_generator01
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage 	textures/props_sd/s_generator01
	diffusemap			textures/props_sd/s_generator01
	bumpmap        		displacemap( textures/props_sd/s_all_norm, invertColor(textures/props_sd/s_all_disp) )
	specularmap			textures/props_sd/s_all_spec
}

textures/props_sd/s_gun01
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage 	textures/props_sd/s_generator01
	diffusemap			textures/props_sd/s_generator01
	bumpmap        		displacemap( textures/props_sd/s_all_norm, invertColor(textures/props_sd/s_all_disp) )
	specularmap			textures/props_sd/s_all_spec
}

textures/props_sd/sign_radar
{
	surfaceparm woodsteps
	implicitMap -
	
	qer_editorimage 	textures/props_sd/sign_radar
	diffusemap			textures/props_sd/sign_radar
	bumpmap        		displacemap( textures/props_sd/sign_radar_norm, invertColor(textures/props_sd/sign_radar_disp) )
	specularmap			textures/props_sd/sign_radar_spec
}

textures/mp_colditz/tool_m07
{
	qer_editorimage textures/awf_props/tool_m07.tga
	cull disable
	nomipmaps
	nopicmip
	implicitMask textures/awf_props/tool_m07.tga
}

textures/props_sd/barrel_m01_rednwhite
{
	surfaceparm metalsteps
	implicitMap -
}

textures/props_sd/wires
{
	cull disable
	nomipmaps
	nopicmip
	surfaceparm alphashadow
	surfaceparm metalsteps
	surfaceparm pointlight
	surfaceparm trans
	surfaceparm nonsolid
	
	{
		map textures/props_sd/wires
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

textures/props_sd/wires01
{

	cull disable
	nomipmaps
	nopicmip
	surfaceparm alphashadow
	surfaceparm metalsteps
	surfaceparm pointlight
	surfaceparm trans
	surfaceparm nonsolid
	
	{
		map textures/props_sd/wires01
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}