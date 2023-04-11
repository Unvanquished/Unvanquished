models/buildables/trapper
{
	qer_editorimage models/buildables/trapper/trapper
	diffuseMap models/buildables/trapper/trapper
	bumpMap models/buildables/trapper/trapper_n
	specularMap models/buildables/trapper/trapper_s
	glowMap models/buildables/trapper/trapper_g
}

models/buildables/humanSpawning
{
	deformVertexes rotgrow 5.0 3.0 2.0
        cull disable
        {
                map models/buildables/telenode/rep_cyl
                blendfunc add
                rgbGen lightingDiffuse
                tcMod scroll 0.2 0
        }
        {
                map models/buildables/telenode/lines2
                blendfunc add
                rgbGen identity
                tcMod scroll 0 0.2
        }
}

