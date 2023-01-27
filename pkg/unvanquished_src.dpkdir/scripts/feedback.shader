// displayed in former place of disconnected player
gfx/feedback/bubble
{
	sort underwater
	cull none
	entityMergable
	nopicmip
	{
		map gfx/feedback/bubble
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		alphaGen vertex
	}
}

// displayed above player while he is typing
gfx/feedback/chatballoon
{
	nopicmip
	{
		map gfx/feedback/chatballoon
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

// blinked on top of lagometer when connection is interrupted
gfx/feedback/net
{
	nopicmip
	{
		map gfx/feedback/net
	}
}

// font for given damage on impact
gfx/feedback/damage/font
{
	nopicmip
	{
		map gfx/feedback/damage/font
		blendfunc blend
		rgbGen vertex
	}
}

// fullscreen vignetting effect when player is suffering
gfx/feedback/painblend
{
	nopicmip
	{
		map gfx/feedback/painblend
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
		tcMod rotate 90
	}
	{
		map gfx/feedback/painblend
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
		tcMod rotate -90
	}
}
