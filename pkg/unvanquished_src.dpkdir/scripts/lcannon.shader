gfx/weapons/lucy_fire1
{
  cull none
  entityMergable
  {
    map gfx/weapons/lucy_fire1
    blendFunc add
    //rgbGen vertex
    //alphaGen vertex
  }
}
gfx/weapons/lucy_swirl
{
  cull none
  entityMergable
  {
    map gfx/weapons/lucy_swirl
    blendFunc add
    //rgbGen vertex
    //alphaGen vertex
  }
}
gfx/weapons/lcannonmissile
{
  cull disable
  {
    animmap 24 gfx/weapons/primary_1 gfx/weapons/primary_2 gfx/weapons/primary_3 gfx/weapons/primary_4
    blendFunc filter
  }
}
gfx/lcannon/ripple
{
  cull disable
  entityMergable
  {
    map gfx/lcannon/ripple
    blendFunc filter
  }
}
lucybullet
{
  cull disable
  {
    map gfx/weapons/lucybullet
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    alphaGen vertex
    rgbGen vertex
  }
}
gfx/weapons/lucytrail
{
  cull none
  entityMergable
  {
    map gfx/weapons/lucytrail
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/luci_shockwave_haze
{
  cull none
  entityMergable
  implicitMapGL1 gfx/transparent
  {
    stage heathazeMap
    deformMagnitude 5.0
    map gfx/weapons/shockwave_normal
  }
}
gfx/weapons/luci_shockwave_haze_small
{
  cull none
  entityMergable
  implicitMapGL1 gfx/transparent
  {
    stage heathazeMap
    deformMagnitude 3.0
    map gfx/weapons/shockwave_normal
  }
}
