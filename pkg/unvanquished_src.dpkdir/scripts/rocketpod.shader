models/buildables/rocketpod/rocketpod
{
	diffuseMap  models/buildables/rocketpod/rocketpod_d.tga
	normalMap   models/buildables/rocketpod/rocketpod_n.tga
	specularMap models/buildables/rocketpod/rocketpod_s.tga
}

models/buildables/rocketpod/rocket
{
	cull                none
	surfaceparm         trans

	{
		map       models/buildables/rocketpod/rocket_d.tga
		stage     diffuseMap
		blend     blend
	}
	normalMap   models/buildables/rocketpod/rocket_n.tga
	specularMap models/buildables/rocketpod/rocket_s.tga
	glowMap     models/buildables/rocketpod/rocket_g.tga
}
