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
