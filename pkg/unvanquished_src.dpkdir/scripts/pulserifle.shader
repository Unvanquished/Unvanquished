models/weapons/prifle/lense
{
	{
		map models/weapons/prifle/lense
		blendfunc add
		tcGen environment
	}
}
gfx/weapons/pulserifleimpact
{
  cull none
  entityMergable
  {
    map gfx/weapons/pulserifleimpact
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/spiderflash_p
{
  cull none
  entityMergable
  {
    map gfx/weapons/spiderflash_p
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/flash_p
{
  cull none
  entityMergable
  {
    map gfx/weapons/flash_p
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/pulserifletrail
{
  cull none
  entityMergable
  {
    map gfx/weapons/pulserifletrail
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
MarkPulseRifleBullet
{
  polygonOffset
  {
    map gfx/marks/mark_pulserifle
    //map models/weapons/rifle/lense
    blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
    rgbGen exactVertex
  }
}