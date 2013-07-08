gfx/blaster/orange_particle
{
  cull disable
  {
    map gfx/blaster/orange_particle.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    alphaGen vertex
    rgbGen vertex
  }
}

gfx/mdriver/green_particle
{
  cull disable
  {
    map gfx/mdriver/green_particle.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen vertex
    alphaGen vertex
  }
}

gfx/mdriver/trail
{
  nomipmaps
  cull disable
  {
    map gfx/mdriver/trail.tga
    blendFunc blend
  }
}

gfx/psaw/blue_particle
{
  cull disable
  {
    map gfx/psaw/blue_particle.jpg
    blendFunc GL_ONE GL_ONE
    alphaGen vertex
    rgbGen vertex
  }
}

gfx/rifle/verysmallrock
{
  cull disable
  {
    map gfx/rifle/verysmallrock.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    alphaGen vertex
    rgbGen vertex
  }
}

gfx/prifle/red_blob
{
  cull disable
  {
    map gfx/prifle/red_blob.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    alphaGen vertex
  }
}

gfx/prifle/red_streak
{
  nomipmaps
  cull disable
  {
    map gfx/prifle/red_streak.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    alphaGen vertex
  }
}

gfx/lcannon/primary
{
  cull disable
  {
    animmap 24 gfx/lcannon/primary_1.jpg gfx/lcannon/primary_2.jpg gfx/lcannon/primary_3.jpg gfx/lcannon/primary_4.jpg
		blendFunc GL_ONE GL_ONE
  }
}

gfx/lasgun/purple_particle
{
  cull disable
  {
    map gfx/lasgun/purple_particle.tga
		blendFunc GL_ONE GL_ONE
  }
}

