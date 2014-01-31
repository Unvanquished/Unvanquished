gfx/weapons/lasgunspark1
{
  cull none
  entityMergable
  {
    map gfx/weapons/lasgunspark1
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
    map gfx/weapons/lasgunspark2
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
    map gfx/weapons/lasgunspark3
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
    map gfx/weapons/lasgunspark4
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
    map gfx/weapons/lasgunspark5
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
    map gfx/weapons/lasgunspark6
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
    map gfx/weapons/spiderflash_lgun
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
    map gfx/weapons/flash_lgun
    blendFunc add
    //rgbGen vertex
    //alphaGen vertex
  }
}
models/weapons/lgun/sight
{

	{
		map models/weapons/rifle/lense
		blendfunc add
		tcGen environment
	}
}
weapons/lgun/sight
{

	{
		map models/weapons/rifle/lense
		blendfunc add
		tcGen environment
	}
}
models/weapons/lgun/heat
{
  {
    map models/weapons/lgun/heat

    blendfunc add
  }
}
models/weapons/lgun/grill
{
	cull disable
	nopicmip
	surfaceparm alphashadow
	{
		map models/weapons/lgun/grill
		alphaFunc GE128
		depthWrite
		rgbGen lightingDiffuse
	}
}