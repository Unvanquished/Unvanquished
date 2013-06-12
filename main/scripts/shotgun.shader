gfx/weapons/shotgunspark
{
  cull none
  entityMergable
  {
    map gfx/weapons/shotgunspark.tga
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
MarkShotgunBullet
{
  polygonOffset
  {
    map gfx/marks/mark_shotgun.tga
    //map models/weapons/rifle/lense.tga
    blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
    rgbGen exactVertex
  }
}