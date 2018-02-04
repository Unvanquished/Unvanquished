gfx/sprites/bubble
{
	sort  underwater
	cull none
	entityMergable
	{
		map gfx/sprites/bubble
		blendFunc GL_ONE GL_ONE
		rgbGen    vertex
		alphaGen  vertex
	}
}

gfx/sprites/chatballoon
{
	{
		map gfx/sprites/chatballoon
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

gfx/sprites/green_acid
{
	nopicmip
	{
		clampmap gfx/sprites/green_acid
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen    vertex
		alphaGen  vertex
	}
}

gfx/sprites/smoke
{
	cull none
	entityMergable
	{
		map gfx/sprites/smoke
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen    vertex
		alphaGen  vertex
	}
}

gfx/sprites/spark
{
	cull none
	{
		map gfx/sprites/spark
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen    vertex
		alphaGen  vertex
	}
}:q
