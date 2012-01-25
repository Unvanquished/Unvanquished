

textures/tr3b/parallax/rockwall
{
	qer_editorimage textures/tr3b/parallax/rockwall_d

	{
		stage diffusemap
		map textures/tr3b/parallax/rockwall_d
		//heightBias 0.5 * (sinTable[time] * 0.5 + 0.5)
		depthScale 0.02
	}
	bumpmap displacemap( textures/tr3b/parallax/rockwall_local, invertColor(textures/tr3b/parallax/rockwall_disp) )
	specularmap	textures/tr3b/parallax/rockwall_d
}

textures/tr3b/parallax/rockwall2
{
	qer_editorimage textures/tr3b/parallax/rockwall2_d

	diffusemap textures/tr3b/parallax/rockwall2_d
	bumpmap textures/tr3b/parallax/rockwall2_local
	specularmap	textures/tr3b/parallax/rockwall2_d
}

textures/tr3b/parallax/stonewall
{
	qer_editorimage textures/tr3b/parallax/stonewall_d

	diffusemap textures/tr3b/parallax/stonewall_d
	bumpmap textures/tr3b/parallax/stonewall_local
	specularmap	textures/tr3b/parallax/stonewall_d
}

textures/tr3b/parallax/brick
{
	qer_editorimage textures/tr3b/parallax/brick_d
	
	parallax

	diffusemap textures/tr3b/parallax/brick_d
	bumpmap displacemap( textures/tr3b/parallax/brick_local, invertColor(textures/tr3b/parallax/brick_disp) )
	specularmap	textures/tr3b/parallax/brick_d
}

textures/tr3b/parallax/brick2
{
	qer_editorimage textures/tr3b/parallax/brick2_d

	diffusemap textures/tr3b/parallax/brick2_d
	bumpmap textures/tr3b/parallax/brick2_local
	specularmap	textures/tr3b/parallax/brick2_d
}

textures/tr3b/parallax/roots
{
	qer_editorimage textures/tr3b/parallax/roots_d

	diffusemap textures/tr3b/parallax/roots_d
	bumpmap displacemap( textures/tr3b/parallax/roots_local, invertColor(textures/tr3b/parallax/roots_disp) )
	specularmap	textures/tr3b/parallax/roots_d
}

textures/tr3b/parallax/tile1
{
	qer_editorimage textures/tr3b/parallax/tile1_local

	diffusemap textures/tr3b/parallax/tile1_d
	bumpmap textures/tr3b/parallax/tile1_local
	specularmap	textures/tr3b/parallax/tile1_d
}
