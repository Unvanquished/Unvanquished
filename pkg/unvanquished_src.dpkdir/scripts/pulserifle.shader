models/weapons/prifle/lense
{
	{
		map models/weapons/prifle/lense.tga
		blendfunc add
		tcGen environment
	}
}
gfx/weapons/pulserifleimpact
{
  cull none
  entityMergable
  {
    map gfx/weapons/pulserifleimpact.tga
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
    map gfx/weapons/spiderflash_p.tga
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
    map gfx/weapons/flash_p.tga
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
    map gfx/weapons/pulserifletrail.tga
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
MarkPulseRifleBullet
{
  polygonOffset
  {
    map gfx/marks/mark_pulserifle.jpg
    //map models/weapons/rifle/lense.tga
    blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
    rgbGen exactVertex
  }
}