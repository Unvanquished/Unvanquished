gfx/weapons/glow_particle_1
{
  cull disable
  {
    map gfx/weapons/glow_particle
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/glow_particle_2
{
  cull disable
  {
    map gfx/weapons/glow_particle2
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/massdriver_MF
{
  cull none
  entityMergable
  {
    map gfx/weapons/massdriver_MF
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
