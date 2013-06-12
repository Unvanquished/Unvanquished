gfx/weapons/lasgunspark1
{
  cull none
  entityMergable
  {
    map gfx/weapons/lasgunspark1.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/lasgunspark2
{
  cull none
  entityMergable
  {
    map gfx/weapons/lasgunspark2.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/lasgunspark3
{
  cull none
  entityMergable
  {
    map gfx/weapons/lasgunspark3.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/lasgunspark4
{
  cull none
  entityMergable
  {
    map gfx/weapons/lasgunspark4.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/lasgunspark5
{
  cull none
  entityMergable
  {
    map gfx/weapons/lasgunspark5.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/lasgunspark6
{
  cull none
  entityMergable
  {
    map gfx/weapons/lasgunspark6.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/spiderflash_lgun
{
  cull none
  entityMergable
  {
    map gfx/weapons/spiderflash_lgun.tga
    blendFunc add
    //rgbGen vertex
    //alphaGen vertex
  }
}
gfx/weapons/flash_lgun
{
  cull none
  entityMergable
  {
    map gfx/weapons/flash_lgun.tga
    blendFunc add
    //rgbGen vertex
    //alphaGen vertex
  }
}
models/weapons/lgun/sight
{

	{
		map models/weapons/rifle/lense.tga
		blendfunc add
		tcGen environment
	}
}
weapons/lgun/sight
{

	{
		map models/weapons/rifle/lense.tga
		blendfunc add
		tcGen environment
	}
}
models/weapons/lgun/heat
{
  {
    map models/weapons/lgun/heat.tga

    blendfunc add
  }
}
models/weapons/lgun/grill
{
	cull disable
	nopicmip
	surfaceparm alphashadow
	{
		map models/weapons/lgun/grill.tga
		alphaFunc GE128
		depthWrite
		rgbGen lightingDiffuse
	}
}