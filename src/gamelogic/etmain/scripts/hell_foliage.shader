// Frozen Grass foliage is based on the wavey green grass foliage by Rayban @ raybanb@gmail.com
// Textures by Avoc
// nedim@husic.dk
//
// waving frozen foliage
//

models/hell/frozen_weed2
{
	nopicmip
	qer_alphafunc greater 0.5
	cull disable

	// distanceCull <inner> <outer> <alpha threshold>
	distanceCull 512 1792 0.49
	sort seethrough
	surfaceparm pointlight
	surfaceparm trans
	surfaceparm nomarks
	nopicmip
	deformVertexes wave 15 sin 0 1 0 0.25
	{
		map models/hell/frozen_weed2.tga
		alphaFunc GE128
		rgbGen exactVertex
		alphaGen vertex
	}
}

models/hell/frozen_weed
{
	nopicmip
	qer_alphafunc greater 0.5
	cull disable

	// distanceCull <inner> <outer> <alpha threshold>
	distanceCull 512 1536 0.49
	sort seethrough
	surfaceparm pointlight
	surfaceparm trans
	surfaceparm nomarks
	nopicmip
	deformVertexes wave 15 sin 0 1 0 0.25
	{
		map models/hell/frozen_weed.tga
		alphaFunc GE128
		rgbGen exactVertex
		alphaGen vertex
	}
}

//
// NEW FOLIAGE BY AVOC
// FLOWER MODEL BY SAGE
//