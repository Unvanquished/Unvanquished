models/buildables/telenode/telenode_dead
{
	diffuseMap	models/buildables/telenode/telenode_d
	normalMap	models/buildables/telenode/telenode_n
	specularMap	models/buildables/telenode/telenode_spec
}

models/buildables/telenode/telenode_full
{
	qer_editorimage models/buildables/telenode/telenode_d

	diffuseMap	models/buildables/telenode/telenode_d
	normalMap	models/buildables/telenode/telenode_n
	{
		stage	specularMap
		map	models/buildables/telenode/telenode_spec
		specularExponentMin 8
		specularExponentMax 85
	}
	glowMap		models/buildables/telenode/telenode_glow

	when destroyed	models/buildables/telenode/telenode_dead
}

models/buildables/telenode/telenode-effect_dead
{
	cull none

	{
		map models/buildables/telenode/telenode-effect
		blendfunc blend
		tcMod scroll -0.997 0
		tAlphaZeroClamp
	}
}

models/buildables/telenode/telenode-effect
{
	cull none

	{
		map models/buildables/telenode/telenode-effect
		blendfunc GL_SRC_ALPHA GL_ONE
		tcMod scroll -0.997 0
		tAlphaZeroClamp
	}

	when destroyed	models/buildables/telenode/telenode-effect_dead
}
