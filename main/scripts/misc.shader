gfx/misc/tracer
{
  cull none
  {
    map gfx/sprites/spark.tga
    blendFunc blend
  }
}

gfx/damage/blood
{
  cull disable
  {
    map gfx/damage/blood.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen vertex
    alphaGen vertex
  }
}

gfx/damage/fullscreen_painblend
{
  {
    map gfx/damage/fullscreen_painblend.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen vertex
    alphaGen vertex
    tcMod rotate 90
  }

  {
    map gfx/damage/fullscreen_painblend.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen vertex
    alphaGen vertex
    tcMod rotate -90
  }
}

models/fx/metal_gibs/metal_gibs
{
  {
    map models/fx/metal_gibs/metal_gibs.tga
    rgbGen lightingDiffuse
  }
  {
    map models/fx/metal_gibs/hot_gibs.tga
    blendfunc add
    rgbGen wave sin 0 1 0 0.0625
  }
}

gfx/misc/redbuild
{
  {
    map gfx/misc/redbuild.tga
    blendfunc add
    rgbGen identity
  }
}

gfx/misc/greenbuild
{
  {
    map gfx/misc/greenbuild.tga
    blendfunc add
    rgbGen identity
  }
}

gfx/misc/nopower
{
  {
    map gfx/misc/nopower.tga
    blendfunc add
    rgbGen identity
  }
}
