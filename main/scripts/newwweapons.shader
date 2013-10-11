	gfx/weapons/spiderflash
{
  cull none
  entityMergable
  {
    map gfx/weapons/spiderflash.tga
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/flash
{
  cull none
  entityMergable
  {
    map gfx/weapons/flash.tga
    blendFunc add
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/weapons/smoke
{
  cull none
  entityMergable
  {
    map gfx/weapons/smoke.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/grenade/smoke
{
  cull none
  entityMergable
  {
    map gfx/weapons/smoke.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
}
gfx/grenade/puff
{
  //nopicmip
  cull disable
  entityMergable
  //deformVertexes wave 40 sin 0 0.1 0 0.5
  {
    map gfx/weapons/puff.tga
    blendFunc blend
    rgbGen vertex
    alphaGen vertex
  }
  {
    map gfx/weapons/puffstreak.tga
    blendfunc blend
    rgbGen vertex
    alphaGen vertex

    tcMod turb 0 0.05 0 0.5
    tcMod scroll -0.5 0.0
  }
  {
    map gfx/weapons/fire.tga
    blendfunc blend
    rgbGen vertex
    alphaGen vertex
    tcMod turb 0 0.05 0 0.5
    tcMod scroll -1 0
  }
}

models/weapons/lcannon/skin
{
	diffuseMap models/weapons/lcannon/skin_COLOR
	normalMap models/weapons/lcannon/skin_NRM
	specularMap models/weapons/lcannon/skin_SPEC
}

models/weapons/shotgun/shotgun
{
	diffuseMap models/weapons/shotgun/shotgun_COLOR
	normalMap models/weapons/shotgun/shotgun_NRM
	specularMap models/weapons/shotgun/shotgun_SPEC
}

models/weapons/ckit/weapon
{
	diffuseMap models/weapons/ckit/weapon_COLOR
	normalMap models/weapons/ckit/weapon_NRM
	specularMap models/weapons/ckit/weapon_SPEC
}

models/weapons/ckit/demoncore
{
	diffuseMap models/weapons/ckit/demoncore_COLOR
	normalMap models/weapons/ckit/demoncore_NRM
	specularMap models/weapons/ckit/demoncore_SPEC
}

models/weapons/flamer/weaponbody
{
	diffuseMap models/weapons/flamer/weaponbody_COLOR
	normalMap models/weapons/flamer/weaponbody_NRM
	specularMap models/weapons/flamer/weaponbody_SPEC
}

models/weapons/flamer/weapongrip
{
	diffuseMap models/weapons/flamer/weapongrip_COLOR
	normalMap models/weapons/flamer/weapongrip_NRM
	specularMap models/weapons/flamer/weapongrip_SPEC
}

models/weapons/flamer/bluepipe
{
	diffuseMap models/weapons/flamer/bluepipe_COLOR
	normalMap models/weapons/flamer/bluepipe_NRM
	specularMap models/weapons/flamer/bluepipe_SPEC
}

models/weapons/flamer/redpipe
{
	diffuseMap models/weapons/flamer/redpipe_COLOR
	normalMap models/weapons/flamer/redpipe_NRM
	specularMap models/weapons/flamer/redpipe_SPEC
}

models/weapons/lgun/zlasgun
{
	diffuseMap models/weapons/lgun/zlasgun
	normalMap models/weapons/lgun/zlasgun_NRM
	specularMap models/weapons/lgun/zlasgun_SPEC
}

models/weapons/lgun/grill
{
	diffuseMap models/weapons/lgun/grill_COLOR
	normalMap models/weapons/lgun/grill_NRM
	specularMap models/weapons/lgun/grill_SPEC
}

models/weapons/mdriver/zmdriver
{
	diffuseMap models/weapons/mdriver/zmdriver_COLOR
	normalMap models/weapons/mdriver/zmdriver_NRM
	specularMap models/weapons/mdriver/zmdriver_SPEC
}

models/weapons/mdriver/core
{
	diffuseMap models/weapons/mdriver/core_COLOR
	normalMap models/weapons/mdriver/core_NRM
	specularMap models/weapons/mdriver/core_SPEC
}

models/weapons/mdriver/lens
{
	diffuseMap models/weapons/mdriver/lens
	specularMap models/weapons/mdriver/lens_SPEC
}

models/weapons/prifle/zprifle
{
	diffuseMap models/weapons/prifle/zprifle_COLOR
	normalMap models/weapons/prifle/zprifle_NRM
	specularMap models/weapons/prifle/zprifle_SPEC
}

models/weapons/prifle/misc
{
	diffuseMap models/weapons/prifle/misc_COLOR
	normalMap models/weapons/prifle/misc_NRM
	specularMap models/weapons/prifle/misc_SPEC
}

models/weapons/prifle/sight
{
	diffuseMap models/weapons/prifle/sight_COLOR
	normalMap models/weapons/prifle/sight_NRM
	specularMap models/weapons/prifle/sight_SPEC
}

models/weapons/prifle/lense
{
	diffuseMap models/weapons/prifle/lense_COLOR
	specularMap models/weapons/prifle/lense_SPEC
}

models/weapons/rifle/zsight
{
	diffuseMap models/weapons/rifle/zsight_COLOR
	normalMap models/weapons/rifle/zsight_NRM
	specularMap models/weapons/rifle/zsight_SPEC
}

models/weapons/rifle/zrifle
{
	diffuseMap models/weapons/rifle/zrifle_COLOR
	normalMap models/weapons/rifle/zrifle_NRM
	specularMap models/weapons/rifle/zrifle_SPEC
}


