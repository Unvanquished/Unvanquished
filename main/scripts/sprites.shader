gfx/sprites/smoke
{
  cull none
  entityMergable
  {
    map gfx/sprites/smoke.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen    vertex
    alphaGen  vertex
  }
}

gfx/sprites/green_acid
{
  nopicmip
  {
    clampmap gfx/sprites/green_acid.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen    vertex
    alphaGen  vertex
  }
}

gfx/sprites/spark
{
  cull none
  {
    map gfx/sprites/spark.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen    vertex
    alphaGen  vertex
  }
}

gfx/sprites/bubble
{
  sort  underwater
  cull none
  entityMergable
  {
    map gfx/sprites/bubble.jpg
    blendFunc GL_ONE GL_ONE
    rgbGen    vertex
    alphaGen  vertex
  }
}

gfx/sprites/poisoncloud
{
  cull none
  entityMergable
  {
    map gfx/sprites/poisoncloud.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen    vertex
    alphaGen  vertex
  }
}

gfx/sprites/acid_1
{
  cull none
  entityMergable
  {
    map gfx/sprites/acid_1.png
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen    vertex
    alphaGen  vertex
  }
}

gfx/sprites/acid_2
{
  cull none
  entityMergable
  {
    map gfx/sprites/acid_2.png
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen    vertex
    alphaGen  vertex
  }
}

gfx/sprites/acid_3
{
  cull none
  entityMergable
  {
    map gfx/sprites/acid_3.png
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen    vertex
    alphaGen  vertex
  }
}


gfx/sprites/chatballoon
{
	{
		map gfx/sprites/chatballoon.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
