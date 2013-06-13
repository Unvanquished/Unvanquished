models/weapons/grenade/grenade_s
{
	{
		map models/weapons/grenade/energy.jpg
		rgbGen wave sawtooth 0.3 1 0 0.5
		tcMod scale 2 1
		tcMod scroll 0 1
	}
}

gfx/grenade/flare_01
{
	{
		map gfx/grenade/flare_01.tga
		blendfunc add
	}
}
gfx/weapons/grenade_shockwave_haze
{
  cull none
  entityMergable
  implicitMapGL1 gfx/transparent.png
  {
    stage heathazeMap
    deformMagnitude 5.0
    map gfx/weapons/shockwave_normal.tga
  }
}
