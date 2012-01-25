// awf_props.shader
// 4 total shaders

textures/awf_props/tool_m01
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/awf_props/tool_m01
	diffusemap		textures/awf_props/tool_m01
	bumpmap         displacemap( textures/awf_props/tool_m01_norm, invertColor(textures/awf_props/tool_m01_disp) )
	specularmap		textures/awf_props/tool_m01_spec
}

textures/awf_props/tool_m02
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/awf_props/tool_m02
	diffusemap		textures/awf_props/tool_m02
	bumpmap         displacemap( textures/awf_props/tool_m02_norm, invertColor(textures/awf_props/tool_m02_disp) )
	specularmap		textures/awf_props/tool_m02_spec
}

textures/awf_props/tool_m06
{
	surfaceparm metalsteps
	implicitMap -
	
	qer_editorimage textures/awf_props/tool_m06
	diffusemap		textures/awf_props/tool_m06
	bumpmap         displacemap( textures/awf_props/tool_m06_norm, invertColor(textures/awf_props/tool_m06_disp) )
	specularmap		textures/awf_props/tool_m06_spec
}

textures/awf_props/tool_m07
{
	cull disable
	nomipmaps
	nopicmip
	surfaceparm alphashadow
	surfaceparm metalsteps
	surfaceparm pointlight
	surfaceparm trans
	implicitMask -
	
	qer_editorimage textures/awf_props/tool_m07
	diffusemap		textures/awf_props/tool_m07
	bumpmap         displacemap( textures/awf_props/tool_m07_norm, invertColor(textures/awf_props/tool_m07_disp) )
	specularmap		textures/awf_props/tool_m07_spec
}
