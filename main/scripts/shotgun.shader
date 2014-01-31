gfx/weapons/shotgunspark
{
  cull none
  entityMergable
  {
    map gfx/weapons/shotgunspark
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
MarkShotgunBullet
{
  polygonOffset
  {
    map gfx/marks/mark_shotgun
    //map models/weapons/rifle/lense
    blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
    rgbGen exactVertex
  }
}