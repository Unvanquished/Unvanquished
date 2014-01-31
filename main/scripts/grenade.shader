// still used?
models/weapons/grenade/grenade_s
{
	{
		map models/weapons/grenade/energy
		rgbGen wave sawtooth 0.3 1 0 0.5
		tcMod scale 2 1
		tcMod scroll 0 1
	}
}

gfx/grenade/flare_01
{
	{
		map gfx/grenade/flare_01
		blendfunc add
	}
}

gfx/weapons/grenade_shockwave_haze
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

models/weapons/grenade/grenade
{
    diffuseMap  models/weapons/grenade/grenade
    normalMap   models/weapons/grenade/grenade_n
    specularMap models/weapons/grenade/grenade_s
}
