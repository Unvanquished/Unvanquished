gfx/weapons/spiderflash
{
  cull none
  entityMergable
  {
    map gfx/weapons/spiderflash.tga
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/flash
{
  cull none
  entityMergable
  {
    map gfx/weapons/flash.tga
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/smoke
{
  cull none
  entityMergable
  {
    map gfx/weapons/smoke.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/grenade/smoke
{
  cull none
  entityMergable
  {
    map gfx/weapons/smoke.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/grenade/puff
{
  //nopicmip
  cull disable
  entityMergable
  //deformVertexes wave 40 sin 0 0.1 0 0.5
  {
    map gfx/weapons/puff.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
  {
    map gfx/weapons/puffstreak.tga
    blendfunc blend
    rgbGen vertex
    alphaGen vertex

    tcMod turb 0 0.05 0 0.5
    tcMod scroll -0.5 0.0
  }
  {
    map gfx/weapons/fire.tga
    blendfunc blend
    rgbGen vertex
    alphaGen vertex
    tcMod turb 0 0.05 0 0.5
    tcMod scroll -1 0
  }
}