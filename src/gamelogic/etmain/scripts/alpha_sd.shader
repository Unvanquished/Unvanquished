textures/alpha_sd/truss_m06grn
{
	qer_editorimage textures/seawall_wall/truss_m06.tga
	surfaceparm alphashadow
	surfaceparm metalsteps
	nomipmaps
	nopicmip
	cull disable
	{
		map textures/seawall_wall/truss_m06.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

