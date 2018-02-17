// displayed in former place of disconnected player
gfx/feedback/bubble
{
	sort underwater
	cull none
	entityMergable
	{
		map gfx/feedback/bubble
		blendFunc GL_ONE GL_ONE
		rgbGen    vertex
		alphaGen  vertex
	}
}

// displayed above player while he is typing
gfx/feedback/chatballoon
{
	{
		map gfx/feedback/chatballoon
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
