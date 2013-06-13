gfx/weapons/blasterMF_1
{
  cull none
  entityMergable
  {
    map gfx/weapons/blasterMF_1.tga
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/blasterMF_2
{
  cull none
  entityMergable
  {
    map gfx/weapons/blasterMF_2.tga
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/blastertrail
{
  cull none
  entityMergable
  {
    map gfx/weapons/blastertrail.tga
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
blasterbullet
{
  cull disable
  {
    map gfx/weapons/blasterbullet.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    alphaGen vertex
    rgbGen vertex
  }
}
MarkBlasterBullet
{
  polygonOffset
  {
    map gfx/marks/mark_blaster.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen exactVertex
    alphaGen vertex
  }
}

models/weapons/blaster/blasterz
{
    diffuseMap models/weapons/blaster/blasterz_COLOR
    normalMap  models/weapons/blaster/blasterz_NRM
    specularMap models/weapons/blaster/blasterz_SPEC
    glowMap     models/weapons/blaster/blaster_glow
}
