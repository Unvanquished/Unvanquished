models/buildables/acid_tube/acidtube
{
	qer_editorimage models/buildables/acid_tube/acidtube
	{
		blend diffusemap
		map models/buildables/acid_tube/acidtube
		alphaFunc GE128
	}
	normalMap models/buildables/acid_tube/acidtube_n
	specularMap models/buildables/acid_tube/acidtube_s
	glowMap models/buildables/acid_tube/acidtube_g
	cull none
}

models/buildables/acid_tube/pod_strands
{
	cull disable
	{
		map models/buildables/eggpod/pod_strands
		rgbGen lightingDiffuse
		alphaFunc GE128
	}
}

