models/players/builder/shell
{
	qer_editorimage models/players/builder/granger_diff.png
	diffuseMap  models/players/builder/granger_diff
	normalMap   models/players/builder/granger_norm
	specularMap models/players/builder/granger_spec
	cull none
}

//  same textures; shell has culling disabled
models/players/builder/skin
{
	qer_editorimage models/players/builder/granger_diff.png
	diffuseMap  models/players/builder/granger_diff
	normalMap   models/players/builder/granger_norm
	{
		stage specularMap
		map models/players/builder/granger_spec
		specularExponentMin 0
		specularExponentMax 128
	}
}

