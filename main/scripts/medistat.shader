models/buildables/medistat/red_cross
{
  {
    map models/buildables/medistat/red_cross.tga
    blendfunc gl_one_minus_dst_color gl_one
  }
}

models/buildables/medistat/medi_cone
{
  cull none

  {
    map models/buildables/medistat/medi_cone.jpg
    blendfunc add
    rgbGen lightingDiffuse
    tcMod scroll 1 0
  }
  {
    map models/buildables/telenode/sparkles.tga
    blendfunc add
    rgbGen identity
    rgbGen wave noise 0 1 35 1
    tcMod scale 2 2
    tcMod scroll 1 0.5
  }
}

