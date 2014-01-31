models/weapons/rifle/zriflelens
{
	{
		map models/weapons/rifle/lense
		blendfunc add
		tcGen environment
	}
}
MarkRifleBullet
{
  polygonOffset
  {
    map gfx/marks/mark_rifle
    //map models/weapons/rifle/lense
    blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
    rgbGen exactVertex
  }
}