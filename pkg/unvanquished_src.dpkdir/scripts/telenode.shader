models/buildables/telenode/telenode_dead
{
	diffuseMap	models/buildables/telenode/telenode_d
	normalMap	models/buildables/telenode/telenode_n
	specularMap	models/buildables/telenode/telenode_spec
}

models/buildables/telenode/telenode_full
{
	diffuseMap	models/buildables/telenode/telenode_d
	normalMap	models/buildables/telenode/telenode_n
	specularMap	models/buildables/telenode/telenode_spec
	glowMap		models/buildables/telenode/telenode_glow

	when unpowered models/buildables/telenode/telenode_dead
	when destroyed models/buildables/telenode/telenode_dead
}

models/buildables/telenode/telenode-effect
{
	cull none

	surfaceparm nolightmap

	{
		map   models/buildables/telenode/telenode-effect
		blend blend

		tcMod scroll -1.0 0

		// TODO: Add a comment explaining what this does
		tAlphaZeroClamp
	}
}
