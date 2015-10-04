/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "cg_local.h"
#include "rocket/rocket.h"
#include <Rocket/Core/Element.h>
#include <Rocket/Core/ElementInstancer.h>
#include <Rocket/Core/ElementInstancerGeneric.h>
#include <Rocket/Core/Factory.h>
#include <Rocket/Core/ElementText.h>
#include <Rocket/Core/StyleSheetKeywords.h>

static void CG_GetRocketElementColor( vec4_t color )
{
	Rocket_GetProperty( "color", color, sizeof( vec4_t ), ROCKET_COLOR );
}

static void CG_GetRocketElementBGColor( vec4_t bgColor )
{
	Rocket_GetProperty( "background-color", bgColor, sizeof( vec4_t ), ROCKET_COLOR );
}

static void CG_GetRocketElementRect( rectDef_t *rect )
{
	Rocket_GetElementAbsoluteOffset( &rect->x, &rect->y );
	Rocket_GetElementDimensions( &rect->w, &rect->h );

	// Convert from absolute monitor coords to a virtual 640x480 coordinate system
	rect->x = ( rect->x / cgs.glconfig.vidWidth ) * 640;
	rect->y = ( rect->y / cgs.glconfig.vidHeight ) * 480;
	rect->w = ( rect->w / cgs.glconfig.vidWidth ) * 640;
	rect->h = ( rect->h / cgs.glconfig.vidHeight ) * 480;
}

class HudElement : public Rocket::Core::Element
{
public:
	HudElement(const Rocket::Core::String& tag, rocketElementType_t type_, bool replacedElement) :
			Rocket::Core::Element(tag),
			type(type_),
			isReplacedElement(replacedElement) {}

	HudElement(const Rocket::Core::String& tag, rocketElementType_t type_) :
			Rocket::Core::Element(tag),
			type(type_),
			isReplacedElement(false) {}

	void OnUpdate()
	{
		Rocket::Core::Element::OnUpdate();
		if (CG_Rocket_IsCommandAllowed(type))
		{
			DoOnUpdate();
		}
	}

	void OnRender()
	{
		if (CG_Rocket_IsCommandAllowed(type))
		{
			DoOnRender();
			Rocket::Core::Element::OnRender();
		}
	}

	virtual void DoOnRender() {}
	virtual void DoOnUpdate() {}

	bool GetIntrinsicDimensions( Rocket::Core::Vector2f &dimension )
	{
		if ( !isReplacedElement )
		{
			return false;
		}

		const Rocket::Core::Property *property;
		property = GetProperty( "width" );

		// Absolute unit. We can use it as is
		if ( property->unit & Rocket::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.x = property->value.Get<float>();
		}
		else
		{
			Rocket::Core::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.x = ResolveProperty( "width", parent->GetBox().GetSize().x );
			}
		}

		property = GetProperty( "height" );
		if ( property->unit & Rocket::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.y = property->value.Get<float>();
		}
		else
		{
			Rocket::Core::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.y = ResolveProperty( "height", parent->GetBox().GetSize().y );
			}
		}

		// Return the calculated dimensions. If this changes the size of the element, it will result in
		// a 'resize' event which is caught below and will regenerate the geometry.
		dimension = dimensions;
		return true;
	}

	void GetElementRect( rectDef_t& rect )
	{
		auto offset = GetAbsoluteOffset();
		rect.x = offset.x;
		rect.y = offset.y;
		auto size = GetBox().GetSize();
		rect.w = size.x;
		rect.h = size.y;

		// Convert from absolute monitor coords to a virtual 640x480 coordinate system
		rect.x = ( rect.x / cgs.glconfig.vidWidth ) * 640;
		rect.y = ( rect.y / cgs.glconfig.vidHeight ) * 480;
		rect.w = ( rect.w / cgs.glconfig.vidWidth ) * 640;
		rect.h = ( rect.h / cgs.glconfig.vidHeight ) * 480;
	}

	void GetColor( const Rocket::Core::String& property, vec4_t color )
	{
		Rocket::Core::Colourb c = GetProperty<Rocket::Core::Colourb>( property );
		color[0] = c.red, color[1] = c.green, color[2] = c.blue, color[3] = c.alpha;
		Vector4Scale( color, 1 / 255.0f, color);
	}

protected:
	Rocket::Core::Vector2f dimensions;

private:
	rocketElementType_t type;
	bool isReplacedElement;
};

class TextHudElement : public HudElement
{
public:
	TextHudElement( const Rocket::Core::String& tag, rocketElementType_t type ) :
		HudElement( tag, type )
	{
		InitializeTextElement();
	}

	TextHudElement( const Rocket::Core::String& tag, rocketElementType_t type, bool replacedElement ) :
		HudElement( tag, type, replacedElement )
	{
		InitializeTextElement();
	}

	void SetText(const Rocket::Core::String& text )
	{
		textElement->SetText( text );
	}

private:
	void InitializeTextElement()
	{
		textElement = dynamic_cast< Rocket::Core::ElementText* >( Rocket::Core::Factory::InstanceElement(
			this,
			"#text",
			"#text",
			Rocket::Core::XMLAttributes() ) );
		AppendChild( textElement );
	}

	Rocket::Core::ElementText* textElement;

};

class AmmoHudElement : public TextHudElement
{
public:
	AmmoHudElement( const Rocket::Core::String& tag ) :
			TextHudElement( tag, ELEMENT_BOTH ),
			showTotalAmmo( false ),
			value( 0 ),
			valueMarked( 0 ) {}

	void OnAttributeChange( const Rocket::Core::AttributeNameList& changed_attributes )
	{
		TextHudElement::OnAttributeChange( changed_attributes );
		if ( changed_attributes.find( "type" ) != changed_attributes.end() )
		{
			const Rocket::Core::String& type = GetAttribute<Rocket::Core::String>( "type", "" );
			showTotalAmmo = type == "total";
		}
	}

	void DoOnRender()
	{
		bool bp = false;
		weapon_t weapon = BG_PrimaryWeapon( cg.snap->ps.stats );
		switch ( weapon )
		{
			case WP_NONE:
			case WP_BLASTER:
				return;

			case WP_ABUILD:
			case WP_ABUILD2:
			case WP_HBUILD:
				if ( cg.snap->ps.persistant[ PERS_BP ] == value &&
					cg.snap->ps.persistant[ PERS_MARKEDBP ] == valueMarked )
				{
					return;
				}
				value = cg.snap->ps.persistant[ PERS_BP ];
				valueMarked = cg.snap->ps.persistant[ PERS_MARKEDBP ];
				bp = true;
				break;

			default:
				if ( showTotalAmmo )
				{
					int maxAmmo = BG_Weapon( weapon )->maxAmmo;
					if ( value == cg.snap->ps.ammo + ( cg.snap->ps.clips * maxAmmo ) )
					{
						return;
					}
					value = cg.snap->ps.ammo + ( cg.snap->ps.clips * maxAmmo );
				}
				else
				{
					if ( value == cg.snap->ps.ammo )
					{
						return;
					}
					value = cg.snap->ps.ammo;
				}

				break;
		}

		if ( value > 999 )
		{
			value = 999;
		}

		if ( valueMarked > 999 )
		{
			valueMarked = 999;
		}

		if ( !bp )
		{
			SetText( va( "%d", value ) );
		}
		else if ( valueMarked > 0 )
		{
			SetText( va( "%d+%d", value, valueMarked ) );
		}
		else
		{
			SetText( va( "%d", value ) );
		}
	}

private:
    bool showTotalAmmo;
	int value;
	int valueMarked;
};


class ClipsHudElement : public TextHudElement
{
public:
	ClipsHudElement( const Rocket::Core::String& tag ) :
		TextHudElement( tag, ELEMENT_HUMANS ),
		clips( 0 ) {}

	virtual void DoOnRender()
	{
		int           value;
		playerState_t *ps = &cg.snap->ps;

		switch ( BG_PrimaryWeapon( ps->stats ) )
		{
			case WP_NONE:
			case WP_BLASTER:
			case WP_ABUILD:
			case WP_ABUILD2:
			case WP_HBUILD:
				if ( clips != -1 )
				{
					SetText( "" );
				}
				clips = -1;
				return;

			default:
				value = ps->clips;

				if ( value > -1 && value != clips )
				{
					SetText( va( "%d", value ) );
					clips = value;
				}

				break;
		}
	}

private:
	int clips;
};


static const int FPS_FRAMES = 20;

class FpsHudElement : public TextHudElement
{
public:
	FpsHudElement( const Rocket::Core::String& tag )
			: TextHudElement( tag, ELEMENT_ALL ),
			  shouldShowFps( true ),
			  index(0),
			  previous(0) {}

	void DoOnUpdate()
	{
		int        i, total;
		int        fps;
		int        t, frameTime;

		if ( !cg_drawFPS.integer && shouldShowFps )
		{
			shouldShowFps = false;
			SetText( "" );
			return;
		} else if ( !shouldShowFps )
		{
			shouldShowFps = true;
		}

		// don't use serverTime, because that will be drifting to
		// correct for Internet lag changes, timescales, timedemos, etc.
		t = trap_Milliseconds();
		frameTime = t - previous;
		previous = t;

		previousTimes[ index % FPS_FRAMES ] = frameTime;
		index++;

		if ( index > FPS_FRAMES )
		{
			// average multiple frames together to smooth changes out a bit
			total = 0;

			for ( i = 0; i < FPS_FRAMES; i++ )
			{
				total += previousTimes[ i ];
			}

			if ( !total )
			{
				total = 1;
			}

			fps = 1000 * FPS_FRAMES / total;
		}

		else
			fps = 0;

		SetText( va( "%d", fps ) );
	}
private:
	bool shouldShowFps;
	int previousTimes[ FPS_FRAMES ];
	int index;
	int previous;

};

#define CROSSHAIR_INDICATOR_HITFADE 500

class CrosshairIndicatorHudElement : public HudElement
{
public:
	CrosshairIndicatorHudElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_BOTH, true ) {}

	void OnPropertyChange( const Rocket::Core::PropertyNameList& changed_properties )
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( changed_properties.find( "color" ) != changed_properties.end() )
		{
			GetColor( "color", color );
		}
	}

	void DoOnRender()
	{
		rectDef_t    rect;
		float        x, y, w, h, dim;
		qhandle_t    indicator;
		vec4_t       drawColor, baseColor;
		weapon_t     weapon;
		weaponInfo_t *wi;
		bool     onRelevantEntity;

		if ( ( !cg_drawCrosshairHit.integer && !cg_drawCrosshairFriendFoe.integer ) ||
			cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT ||
			cg.snap->ps.pm_type == PM_INTERMISSION ||
			cg.renderingThirdPerson )
		{
			return;
		}

		weapon = BG_GetPlayerWeapon( &cg.snap->ps );
		wi = &cg_weapons[ weapon ];
		indicator = wi->crossHairIndicator;

		if ( !indicator )
		{
			return;
		}

		GetElementRect( rect );

		// set base color (friend/foe detection)
		if ( cg_drawCrosshairFriendFoe.integer >= CROSSHAIR_ALWAYSON ||
			( cg_drawCrosshairFriendFoe.integer >= CROSSHAIR_RANGEDONLY && BG_Weapon( weapon )->longRanged ) )
		{
			if ( cg.crosshairFoe )
			{
				Vector4Copy( colorRed, baseColor );
				baseColor[ 3 ] = color[ 3 ] * 0.75f;
				onRelevantEntity = true;
			}

			else if ( cg.crosshairFriend )
			{
				Vector4Copy( colorGreen, baseColor );
				baseColor[ 3 ] = color[ 3 ] * 0.75f;
				onRelevantEntity = true;
			}

			else
			{
				Vector4Set( baseColor, 1.0f, 1.0f, 1.0f, 0.0f );
				onRelevantEntity = false;
			}
		}

		else
		{
			Vector4Set( baseColor, 1.0f, 1.0f, 1.0f, 0.0f );
			onRelevantEntity = false;
		}

		// add hit color
		if ( cg_drawCrosshairHit.integer && cg.hitTime + CROSSHAIR_INDICATOR_HITFADE > cg.time )
		{
			dim = ( ( cg.hitTime + CROSSHAIR_INDICATOR_HITFADE ) - cg.time ) / ( float )CROSSHAIR_INDICATOR_HITFADE;

			Vector4Lerp( dim, baseColor, colorWhite, drawColor );
		}

		else if ( !onRelevantEntity )
		{
			return;
		}

		else
		{
			Vector4Copy( baseColor, drawColor );
		}

		// set size
		w = h = wi->crossHairSize * cg_crosshairSize.value;
		w *= cgs.aspectScale;

		x = rect.x + ( rect.w / 2 ) - ( w / 2 );
		y = rect.y + ( rect.h / 2 ) - ( h / 2 );

		// draw
		trap_R_SetColor( drawColor );
		CG_DrawPic( x, y, w, h, indicator );
		trap_R_SetColor( nullptr );
	}
private:
	vec4_t color;


};

class CrosshairHudElement : public HudElement {
public:
	CrosshairHudElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_BOTH, true ) {
		color[0] = color[1] = color[2] = color[3] = 255;
	}

	void OnPropertyChange( const Rocket::Core::PropertyNameList& changed_properties )
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( changed_properties.find( "color" ) != changed_properties.end() )
		{
			GetColor( "color", color );
		}
	}

	void DoOnRender()
	{
		rectDef_t    rect;
		float        w, h;
		qhandle_t    crosshair;
		float        x, y;
		weaponInfo_t *wi;
		weapon_t     weapon;

		weapon = BG_GetPlayerWeapon( &cg.snap->ps );

		if ( cg_drawCrosshair.integer == CROSSHAIR_ALWAYSOFF )
		{
			return;
		}

		if ( cg_drawCrosshair.integer == CROSSHAIR_RANGEDONLY &&
			!BG_Weapon( weapon )->longRanged )
		{
			return;
		}

		if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
		{
			return;
		}

		if ( cg.renderingThirdPerson )
		{
			return;
		}

		if ( cg.snap->ps.pm_type == PM_INTERMISSION )
		{
			return;
		}

		GetElementRect( rect );

		wi = &cg_weapons[ weapon ];

		w = h = wi->crossHairSize * cg_crosshairSize.value;
		w *= cgs.aspectScale;

		// HACK: This ignores the width/height of the rect (does it?)
		x = rect.x + ( rect.w / 2 ) - ( w / 2 );
		y = rect.y + ( rect.h / 2 ) - ( h / 2 );

		crosshair = wi->crossHair;

		if ( crosshair )
		{
			CG_GetRocketElementColor( color );
			trap_R_SetColor( color );
			CG_DrawPic( x, y, w, h, crosshair );
			trap_R_SetColor( nullptr );
		}
	}

private:
	vec4_t color;
};

#define SPEEDOMETER_NUM_SAMPLES 4096
#define SPEEDOMETER_NUM_DISPLAYED_SAMPLES 160
#define SPEEDOMETER_DRAW_TEXT   0x1
#define SPEEDOMETER_DRAW_GRAPH  0x2
#define SPEEDOMETER_IGNORE_Z    0x4
float speedSamples[ SPEEDOMETER_NUM_SAMPLES ];
int speedSampleTimes[ SPEEDOMETER_NUM_SAMPLES ];
// array indices
int   oldestSpeedSample = 0;
int   maxSpeedSample = 0;
int   maxSpeedSampleInWindow = 0;

/*
===================
CG_AddSpeed

append a speed to the sample history
===================
*/
void CG_AddSpeed()
{
	float  speed;
	vec3_t vel;
	int    windowTime;
	bool newSpeedGteMaxSpeed, newSpeedGteMaxSpeedInWindow;

	VectorCopy( cg.snap->ps.velocity, vel );

	if ( cg_drawSpeed.integer & SPEEDOMETER_IGNORE_Z )
	{
		vel[ 2 ] = 0;
	}

	speed = VectorLength( vel );

	windowTime = cg_maxSpeedTimeWindow.integer;

	if ( windowTime < 0 )
	{
		windowTime = 0;
	}

	else if ( windowTime > SPEEDOMETER_NUM_SAMPLES * 1000 )
	{
		windowTime = SPEEDOMETER_NUM_SAMPLES * 1000;
	}

	if ( ( newSpeedGteMaxSpeed = ( speed >= speedSamples[ maxSpeedSample ] ) ) )
	{
		maxSpeedSample = oldestSpeedSample;
	}

	if ( ( newSpeedGteMaxSpeedInWindow = ( speed >= speedSamples[ maxSpeedSampleInWindow ] ) ) )
	{
		maxSpeedSampleInWindow = oldestSpeedSample;
	}

	speedSamples[ oldestSpeedSample ] = speed;

	speedSampleTimes[ oldestSpeedSample ] = cg.time;

	if ( !newSpeedGteMaxSpeed && maxSpeedSample == oldestSpeedSample )
	{
		// if old max was overwritten find a new one
		int i;

		for ( maxSpeedSample = 0, i = 1; i < SPEEDOMETER_NUM_SAMPLES; i++ )
		{
			if ( speedSamples[ i ] > speedSamples[ maxSpeedSample ] )
			{
				maxSpeedSample = i;
			}
		}
	}

	if ( !newSpeedGteMaxSpeedInWindow && ( maxSpeedSampleInWindow == oldestSpeedSample ||
										   cg.time - speedSampleTimes[ maxSpeedSampleInWindow ] > windowTime ) )
	{
		int i;

		do
		{
			maxSpeedSampleInWindow = ( maxSpeedSampleInWindow + 1 ) % SPEEDOMETER_NUM_SAMPLES;
		}
		while ( cg.time - speedSampleTimes[ maxSpeedSampleInWindow ] > windowTime );

		for ( i = maxSpeedSampleInWindow; ; i = ( i + 1 ) % SPEEDOMETER_NUM_SAMPLES )
		{
			if ( speedSamples[ i ] > speedSamples[ maxSpeedSampleInWindow ] )
			{
				maxSpeedSampleInWindow = i;
			}

			if ( i == oldestSpeedSample )
			{
				break;
			}
		}
	}

	oldestSpeedSample = ( oldestSpeedSample + 1 ) % SPEEDOMETER_NUM_SAMPLES;
}

#define SPEEDOMETER_MIN_RANGE 900
#define SPEED_MED             1000.f
#define SPEED_FAST            1600.f

class SpeedGraphElement : public HudElement
{
public:
	SpeedGraphElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_GAME, true )
	{
		Rocket::Core::XMLAttributes xml;
		maxSpeedElement = dynamic_cast< Rocket::Core::ElementText* >( Rocket::Core::Factory::InstanceElement(
			this,
			"#text",
			"span",
			xml ) );
		maxSpeedElement->SetClass( "speed_max", true );
		currentSpeedElement = dynamic_cast< Rocket::Core::ElementText* >( Rocket::Core::Factory::InstanceElement(
			this,
			"#text",
			"span",
			xml) );
		currentSpeedElement->SetClass( "speed_current", true );
		AppendChild( maxSpeedElement );
		AppendChild( currentSpeedElement );
	}

	void OnPropertyChange( const Rocket::Core::PropertyNameList& changed_properties )
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( changed_properties.find( "color" ) != changed_properties.end() )
		{
			GetColor( "color", color );
		}

		if ( changed_properties.find( "background-color" ) != changed_properties.end() )
		{
			GetColor( "background-color", backColor );
		}
	}

	void DoOnRender()
	{
		int          i;
		float        val, max, top;
		// colour of graph is interpolated between these values
		const vec3_t slow = { 0.0, 0.0, 1.0 };
		const vec3_t medium = { 0.0, 1.0, 0.0 };
		const vec3_t fast = { 1.0, 0.0, 0.0 };
		rectDef_t    rect;

		if ( !cg_drawSpeed.integer )
		{
			if ( shouldDrawSpeed )
			{
				currentSpeedElement->SetText( "" );
				maxSpeedElement->SetText( "" );
				shouldDrawSpeed = false;
			}
			return;
		}
		else if ( !shouldDrawSpeed )
		{
			shouldDrawSpeed = true;
		}


		if ( cg_drawSpeed.integer & SPEEDOMETER_DRAW_GRAPH )
		{
			// grab info from libRocket
			GetElementRect( rect );

			max = speedSamples[ maxSpeedSample ];

			if ( max < SPEEDOMETER_MIN_RANGE )
			{
				max = SPEEDOMETER_MIN_RANGE;
			}

			trap_R_SetColor( backColor );
			CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cgs.media.whiteShader );

			for ( i = 1; i < SPEEDOMETER_NUM_DISPLAYED_SAMPLES; i++ )
			{
				val = speedSamples[( oldestSpeedSample + i + SPEEDOMETER_NUM_SAMPLES -
				SPEEDOMETER_NUM_DISPLAYED_SAMPLES ) % SPEEDOMETER_NUM_SAMPLES ];

				if ( val < SPEED_MED )
				{
					VectorLerpTrem( val / SPEED_MED, slow, medium, color );
				}

				else if ( val < SPEED_FAST )
				{
					VectorLerpTrem( ( val - SPEED_MED ) / ( SPEED_FAST - SPEED_MED ),
									medium, fast, color );
				}

				else
				{
					VectorCopy( fast, color );
				}

				trap_R_SetColor( color );
				top = rect.y + ( 1 - val / max ) * rect.h;
				CG_DrawPic( rect.x + ( i / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES ) * rect.w, top,
							rect.w / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES, val * rect.h / max,
							cgs.media.whiteShader );
			}

			trap_R_SetColor( nullptr );
		}

		if ( cg_drawSpeed.integer & SPEEDOMETER_DRAW_TEXT )
		{
			// Add text to be configured via CSS
			if ( cg.predictedPlayerState.clientNum == cg.clientNum )
			{
				vec3_t vel;
				VectorCopy( cg.predictedPlayerState.velocity, vel );

				if ( cg_drawSpeed.integer & SPEEDOMETER_IGNORE_Z )
				{
					vel[ 2 ] = 0;
				}

				val = VectorLength( vel );
			}

			else
			{
				val = speedSamples[( oldestSpeedSample - 1 + SPEEDOMETER_NUM_SAMPLES ) % SPEEDOMETER_NUM_SAMPLES ];
			}
			// HACK: Put extra spaces to separate the children because setting them to block makes them disappear.
			// TODO: Figure out why setting these two elements to block makes them disappear.
			maxSpeedElement->SetText( va( "%d   ", ( int ) speedSamples[ maxSpeedSampleInWindow ] ) );
			currentSpeedElement->SetText( va( "%d", ( int ) val ) );
		}
	}

private:
	Rocket::Core::ElementText* maxSpeedElement;
	Rocket::Core::ElementText* currentSpeedElement;
	bool shouldDrawSpeed;
	vec4_t color;
	vec4_t backColor;

};

class CreditsValueElement : public TextHudElement
{
public:
	CreditsValueElement( const Rocket::Core::String& tag ) :
			TextHudElement( tag, ELEMENT_HUMANS ),
			credits( -1 ) {}

	void DoOnUpdate()
	{
		playerState_t *ps = &cg.snap->ps;
		int value = ps->persistant[ PERS_CREDIT ];;
		if ( credits != value )
		{
			credits = value;
			SetText( va( "%d", credits ) );
		}
	}

private:
	int credits;
};

class EvosValueElement : public TextHudElement
{
public:
	EvosValueElement( const Rocket::Core::String& tag ) :
			TextHudElement( tag, ELEMENT_ALIENS ),
			evos( -1 ) {}

	void DoOnUpdate()
	{
		playerState_t *ps = &cg.snap->ps;
		float value = ps->persistant[ PERS_CREDIT ];;

		value /= ( float ) CREDITS_PER_EVO;

		if ( evos != value )
		{
			evos = value;
			SetText( va( "%1.1f", evos ) );
		}
	}

private:
	float evos;
};

class StaminaValueElement : public TextHudElement
{
public:
	StaminaValueElement( const Rocket::Core::String& tag ) :
	TextHudElement( tag, ELEMENT_HUMANS ),
	stamina( -1 ) {}

	void DoOnUpdate()
	{
		playerState_t *ps = &cg.snap->ps;
		float         value = ps->stats[ STAT_STAMINA ];

		if ( stamina != value )
		{
			stamina = value;
			int percent = 100 * ( stamina / ( float ) STAMINA_MAX );
			SetText( va( "%d", percent ) );
		}
	}

private:
	float stamina;
};

class WeaponIconElement : public HudElement
{
public:
	WeaponIconElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_BOTH ),
			weapon( WP_NONE ),
			isNoAmmo( false ) {}

	void DoOnUpdate()
	{
		playerState_t *ps;
		weapon_t      newWeapon;

		ps = &cg.snap->ps;
		newWeapon = BG_GetPlayerWeapon( ps );

		if ( newWeapon != weapon )
		{
			weapon = newWeapon;
			// don't display if dead
			if ( ( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 || weapon == WP_NONE ) && !IsVisible() )
			{
				SetProperty( "display", "none" );
				return;
			}

			if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
			{
				CG_Error( "CG_DrawWeaponIcon: weapon out of range: %d", weapon );
			}

			if ( !cg_weapons[ weapon ].registered )
			{
				Com_Printf( S_WARNING "CG_DrawWeaponIcon: weapon %d (%s) "
				"is not registered\n", weapon, BG_Weapon( weapon )->name );
				SetProperty( "display", "none" );
				return;
			}

			SetInnerRML( va( "<img src='/%s' />", CG_GetShaderNameFromHandle( cg_weapons[ weapon ].weaponIcon ) ) );
			SetProperty( "display", "block" );
		}

		if ( !isNoAmmo && ps->clips == 0 && ps->ammo == 0 && !BG_Weapon( weapon )->infiniteAmmo )
		{
			SetClass( "no_ammo", true );
		}
		else if ( isNoAmmo )
		{
			SetClass( "no_ammo", false );
		}

	}
private:
	int weapon;
	bool isNoAmmo;
};

class WallwalkElement : public HudElement
{
public:
	WallwalkElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_ALIENS ),
			isActive( false ) {}

	void DoOnUpdate()
	{
		bool wallwalking = cg.snap->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING;
		if ( ( wallwalking && !isActive ) || ( !wallwalking && isActive ) )
		{
			SetActive( wallwalking );
		}
	}

private:
	void SetActive( bool active )
	{
		isActive = active;
		SetClass( "active", active );
		SetClass( "inactive", !active );

	}

	bool isActive;
};

class UsableBuildableElement : public HudElement
{
public:
	UsableBuildableElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_HUMANS ),
			display( "block" ) {}

	void OnPropertyChange( const Rocket::Core::PropertyNameList& changed_properties )
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( display.Empty() && changed_properties.find( "display" ) != changed_properties.end() )
		{
			display = GetProperty<Rocket::Core::String>( "display" );
		}
	}

	void DoOnUpdate()
	{
		vec3_t        view, point;
		trace_t       trace;
		entityState_t *es;

		AngleVectors( cg.refdefViewAngles, view, nullptr, nullptr );
		VectorMA( cg.refdef.vieworg, 64, view, point );
		CG_Trace( &trace, cg.refdef.vieworg, nullptr, nullptr,
				  point, cg.predictedPlayerState.clientNum, MASK_SHOT, 0 );

		es = &cg_entities[ trace.entityNum ].currentState;

		if ( es->eType == ET_BUILDABLE && BG_Buildable( es->modelindex )->usable &&
			cg.predictedPlayerState.persistant[ PERS_TEAM ] == BG_Buildable( es->modelindex )->team )
		{
			//hack to prevent showing the usable buildable when you aren't carrying an energy weapon
			if ( ( es->modelindex == BA_H_REACTOR || es->modelindex == BA_H_REPEATER ) &&
				( !BG_Weapon( cg.snap->ps.weapon )->usesEnergy ||
				BG_Weapon( cg.snap->ps.weapon )->infiniteAmmo ) )
			{
				cg.nearUsableBuildable = BA_NONE;
				SetProperty( "display", "none" );
				return;
			}

			if ( IsVisible() )
			{
				return;
			}

			SetProperty( "display", display );
			cg.nearUsableBuildable = es->modelindex;
		}
		else
		{
			if ( IsVisible() )
			{
				// Clear the old image if there was one.
				SetProperty( "display", "none" );
				cg.nearUsableBuildable = BA_NONE;
			}
		}
	}

private:
	Rocket::Core::String display;
};

class LocationElement : public HudElement
{
public:
	LocationElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_GAME ) {}

	void DoOnUpdate()
	{
		Rocket::Core::String newLocation;
		centity_t  *locent;

		if ( cg.intermissionStarted )
		{
			if ( !location.Empty() )
			{
				location = "";
				SetInnerRML( location );
			}
			return;
		}

		locent = CG_GetPlayerLocation();

		if ( locent )
		{
			location = CG_ConfigString( CS_LOCATIONS + locent->currentState.generic1 );
		}
		else
		{
			location = CG_ConfigString( CS_LOCATIONS );
		}

		if ( location != newLocation )
		{
			SetInnerRML( Rocket_QuakeToRML( location.CString(), RP_EMOTICONS ) );
		}
	}

private:
	Rocket::Core::String location;
};

class TimerElement : public TextHudElement
{
public:
	TimerElement( const Rocket::Core::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME ),
			mins_( 0 ),
			seconds_( 0 ),
			tens_( 0 ) {}

	void DoOnUpdate()
	{
		int   mins, seconds, tens;
		int   msec;

		if ( !cg_drawTimer.integer && IsVisible() )
		{
			SetProperty( "display", "none" );
			return;
		}
		else if ( cg_drawTimer.integer && !IsVisible() )
		{
			SetProperty( "display", "block" );
		}

		msec = cg.time - cgs.levelStartTime;

		seconds = msec / 1000;
		mins = seconds / 60;
		seconds -= mins * 60;
		tens = seconds / 10;
		seconds -= tens * 10;

		if ( seconds_ != seconds || mins != mins_ || tens != tens_ )
		{
			SetText( va( "%d:%d%d", mins, tens, seconds ) );
			seconds_ = seconds;
			mins_ = mins;
			tens_ = tens;
		}
	}

private:
	int mins_;
	int seconds_;
	int tens_;

};

#define LAG_SAMPLES 128

typedef struct
{
	int frameSamples[ LAG_SAMPLES ];
	int frameCount;
	int snapshotFlags[ LAG_SAMPLES ];
	int snapshotSamples[ LAG_SAMPLES ];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo()
{
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1 ) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass nullptr for a dropped packet.
==============
*/
#define PING_FRAMES 40
void CG_AddLagometerSnapshotInfo( snapshot_t *snap )
{
	static int previousPings[ PING_FRAMES ];
	static int index;
	int        i;

	// dropped packet
	if ( !snap )
	{
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->snapFlags;
	lagometer.snapshotCount++;

	cg.ping = 0;

	if ( cg.snap )
	{
		previousPings[ index++ ] = cg.snap->ping;
		index = index % PING_FRAMES;

		for ( i = 0; i < PING_FRAMES; i++ )
		{
			cg.ping += previousPings[ i ];
		}

		cg.ping /= PING_FRAMES;
	}
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_Rocket_DrawDisconnect()
{
	float      x, y;
	int        cmdNum;
	usercmd_t  cmd;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );

	// special check for map_restart
	if ( cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time )
	{
		return;
	}

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 )
	{
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader( "gfx/2d/net",
				RSF_DEFAULT ) );
}

#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

class LagometerElement : public TextHudElement
{
public:
	LagometerElement( const Rocket::Core::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME, true ),
			shouldDrawLagometer( true )
	{
		adjustedColor[ 0 ] = adjustedColor[ 1 ] = adjustedColor[ 2 ] = adjustedColor[ 3 ] = 255;
	}

	void OnPropertyChange( const Rocket::Core::PropertyNameList& changed_properties )
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( changed_properties.find( "background-color" ) != changed_properties.end() )
		{
			GetColor( "background-color", adjustedColor );
		}
	}

	void DoOnRender()
	{
		int    a, i;
		float  v;
		float  ax, ay, aw, ah, mid, range;
		int    color;
		float  vscale;
		rectDef_t rect;

		if ( !shouldDrawLagometer )
		{
			return;
		}

		// grab info from libRocket
		GetElementRect( rect );

		trap_R_SetColor( adjustedColor );
		CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cgs.media.whiteShader );
		trap_R_SetColor( nullptr );

		//
		// draw the graph
		//
		ax = rect.x;
		ay = rect.y;
		aw = rect.w;
		ah = rect.h;

		CG_AdjustFrom640( &ax, &ay, &aw, &ah );

		color = -1;
		range = ah / 3;
		mid = ay + range;

		vscale = range / MAX_LAGOMETER_RANGE;

		// draw the frame interpoalte / extrapolate graph
		for ( a = 0; a < aw; a++ )
		{
			i = ( lagometer.frameCount - 1 - a ) & ( LAG_SAMPLES - 1 );
			v = lagometer.frameSamples[ i ];
			v *= vscale;

			if ( v > 0 )
			{
				if ( color != 1 )
				{
					color = 1;
					trap_R_SetColor( g_color_table[ ColorIndex( COLOR_YELLOW ) ] );
				}

				if ( v > range )
				{
					v = range;
				}

				trap_R_DrawStretchPic( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
			}

			else if ( v < 0 )
			{
				if ( color != 2 )
				{
					color = 2;
					trap_R_SetColor( g_color_table[ ColorIndex( COLOR_BLUE ) ] );
				}

				v = -v;

				if ( v > range )
				{
					v = range;
				}

				trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
			}
		}

		// draw the snapshot latency / drop graph
		range = ah / 2;
		vscale = range / MAX_LAGOMETER_PING;

		for ( a = 0; a < aw; a++ )
		{
			i = ( lagometer.snapshotCount - 1 - a ) & ( LAG_SAMPLES - 1 );
			v = lagometer.snapshotSamples[ i ];

			if ( v > 0 )
			{
				if ( lagometer.snapshotFlags[ i ] & SNAPFLAG_RATE_DELAYED )
				{
					if ( color != 5 )
					{
						color = 5; // YELLOW for rate delay
						trap_R_SetColor( g_color_table[ ColorIndex( COLOR_YELLOW ) ] );
					}
				}

				else
				{
					if ( color != 3 )
					{
						color = 3;

						trap_R_SetColor( g_color_table[ ColorIndex( COLOR_GREEN ) ] );
					}
				}

				v = v * vscale;

				if ( v > range )
				{
					v = range;
				}

				trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
			}

			else if ( v < 0 )
			{
				if ( color != 4 )
				{
					color = 4; // RED for dropped snapshots
					trap_R_SetColor( g_color_table[ ColorIndex( COLOR_RED ) ] );
				}

				trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
			}
		}

		trap_R_SetColor( nullptr );
		CG_Rocket_DrawDisconnect();
	}

	void DoOnUpdate()
	{
		const char* ping;

		if ( ( cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION )
			|| !cg_lagometer.integer
			|| cg.demoPlayback )
		{
			if ( shouldDrawLagometer )
			{
				SetText( "" );
				shouldDrawLagometer = false;
			}
			return;
		}
		else if ( !shouldDrawLagometer )
		{
			shouldDrawLagometer = true;
		}

		if ( cg_nopredict.integer || cg.pmoveParams.synchronous )
		{
			ping = "snc";
		}

		else
		{
			ping = va( "%d", cg.ping );
		}

		if ( ping_ != ping )
		{
			SetText( ping );
			ping_ = ping;
		}
	}

private:
	bool shouldDrawLagometer;
	vec4_t adjustedColor;
	Rocket::Core::String ping_;

};

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity()
{
	trace_t       trace;
	vec3_t        start, end;
	team_t        ownTeam, targetTeam;
	entityState_t *targetState;

	if ( cg.snap == nullptr )
	{
		return;
	}

	cg.crosshairFriend = false;
	cg.crosshairFoe    = false;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[ 0 ], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
			  cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY, 0 );

	// ignore special entities
	if ( trace.entityNum > ENTITYNUM_MAX_NORMAL )
	{
		return;
	}

	// ignore targets in fog
	if ( trap_CM_PointContents( trace.endpos, 0 ) & CONTENTS_FOG )
	{
		return;
	}

	ownTeam = ( team_t ) cg.snap->ps.persistant[ PERS_TEAM ];
	targetState = &cg_entities[ trace.entityNum ].currentState;

	if ( trace.entityNum >= MAX_CLIENTS )
	{
		// we have a non-client entity

		// set friend/foe if it's a living buildable
		if ( targetState->eType == ET_BUILDABLE && targetState->generic1 > 0 )
		{
			targetTeam = BG_Buildable( targetState->modelindex )->team;

			if ( targetTeam == ownTeam && ownTeam != TEAM_NONE )
			{
				cg.crosshairFriend = true;
			}

			else if ( targetTeam != TEAM_NONE )
			{
				cg.crosshairFoe = true;
			}
		}

		// set more stuff if requested
		if ( cg_drawEntityInfo.integer && targetState->eType )
		{
			cg.crosshairClientNum = trace.entityNum;
			cg.crosshairClientTime = cg.time;
		}
	}

	else
	{
		// we have a client entitiy
		targetTeam = cgs.clientinfo[ trace.entityNum ].team;

		// only react to living clients
		if ( targetState->generic1 > 0 )
		{
			// set friend/foe
			if ( targetTeam == ownTeam && ownTeam != TEAM_NONE )
			{
				cg.crosshairFriend = true;

				// only set this for friendly clients as it triggers name display
				cg.crosshairClientNum = trace.entityNum;
				cg.crosshairClientTime = cg.time;
			}

			else if ( targetTeam != TEAM_NONE )
			{
				cg.crosshairFoe = true;

				if ( ownTeam == TEAM_NONE )
				{
					// spectating, so show the name
					cg.crosshairClientNum = trace.entityNum;
					cg.crosshairClientTime = cg.time;
				}
			}
		}
	}
}

class CrosshairNamesElement : public HudElement
{
public:
	CrosshairNamesElement( const Rocket::Core::String& tag  ) :
			HudElement( tag, ELEMENT_GAME ) {}

	void DoOnUpdate()
	{
		Rocket::Core::String name;
		float alpha;

		if ( !cg_drawCrosshairNames.integer || cg.renderingThirdPerson )
		{
			Clear();
			return;
		}

		// scan the known entities to see if the crosshair is sighted on one
		CG_ScanForCrosshairEntity();

		// draw the name of the player being looked at
		alpha = CG_FadeAlpha( cg.crosshairClientTime, CROSSHAIR_CLIENT_TIMEOUT );

		if ( cg.crosshairClientTime == cg.time )
		{
			alpha = 1.0f;
		}

		else if ( !alpha )
		{
			Clear();
			return;
		}

		if ( alpha != alpha_ )
		{
			alpha_ = alpha;
			SetProperty( "opacity", va( "%f", alpha ) );
		}

		if ( cg_drawEntityInfo.integer )
		{
			name = va( "(" S_COLOR_CYAN "%s" S_COLOR_WHITE "|" S_COLOR_CYAN "#%d" S_COLOR_WHITE ")",
					   Com_EntityTypeName( cg_entities[cg.crosshairClientNum].currentState.eType ), cg.crosshairClientNum );
		}

		else if ( cg_drawCrosshairNames.integer >= 2 )
		{
			name = va( "%2i: %s", cg.crosshairClientNum, cgs.clientinfo[ cg.crosshairClientNum ].name );
		}

		else
		{
			name = cgs.clientinfo[ cg.crosshairClientNum ].name;
		}

		// add health from overlay info to the crosshair client name
		if ( cg_teamOverlayUserinfo.integer &&
			cg.snap->ps.persistant[ PERS_TEAM ] != TEAM_NONE &&
			cgs.teamInfoReceived &&
			cgs.clientinfo[ cg.crosshairClientNum ].health > 0 )
		{
			name = va( "%s ^7[^%c%d^7]", name.CString(),
					   CG_GetColorCharForHealth( cg.crosshairClientNum ),
					   cgs.clientinfo[ cg.crosshairClientNum ].health );
		}

		if ( name != name_ )
		{
			name_ = name;
			SetInnerRML( Rocket_QuakeToRML( name.CString(), RP_EMOTICONS ) );
		}
	}

private:
	void Clear()
	{
		if ( !name_.Empty() )
		{
			name_ = "";
			SetInnerRML( "" );
		}
	}

	Rocket::Core::String name_;
	float alpha_;
};

class MomentumElement : public TextHudElement
{
public:
	MomentumElement( const Rocket::Core::String& tag ) :
			TextHudElement( tag, ELEMENT_BOTH ),
			momentum_(-1.0f) {}

	void DoOnUpdate()
	{
		float momentum;
		team_t team;

		if ( cg.intermissionStarted )
		{
			Clear();
			return;
		}

		team = ( team_t ) cg.snap->ps.persistant[ PERS_TEAM ];

		if ( team <= TEAM_NONE || team >= NUM_TEAMS )
		{
			Clear();
			return;
		}

		momentum = cg.predictedPlayerState.persistant[ PERS_MOMENTUM ] / 10.0f;
		if ( momentum != momentum_ )
		{
			momentum_ = momentum;
			SetText( va( "%.1f", momentum_) );
		}
	}

private:
	void Clear()
	{
		if (momentum_ != -1.0f)
		{
			momentum_ = -1.0f;
			SetText( "" );
		}
	}

	float momentum_;
};

class LevelshotElement : public HudElement
{
public:
	LevelshotElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_ALL ) {}

	void DoOnUpdate()
	{
		if ( ( rocketInfo.data.mapIndex < 0 || rocketInfo.data.mapIndex >= rocketInfo.data.mapCount ) )
		{
			Clear();
			return;
		}

		if ( mapIndex != rocketInfo.data.mapIndex )
		{
			mapIndex = rocketInfo.data.mapIndex;
			SetInnerRML( va( "<img class='levelshot' src='/meta/%s/%s' />",
							 rocketInfo.data.mapList[ mapIndex ].mapLoadName,
					rocketInfo.data.mapList[ mapIndex ].mapLoadName ) );
		}
	}

private:
	void Clear()
	{
		if ( mapIndex != -1)
		{
			mapIndex = -1;
			SetInnerRML("");
		}
	}

	int mapIndex;

};

class LevelshotLoadingElement : public HudElement
{
public:
	LevelshotLoadingElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_ALL ) {}

	void DoOnUpdate()
	{
		if ( rocketInfo.cstate.connState < CA_LOADING )
		{
			Clear();
			return;
		}

		const char *newMap = Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), "mapname" );
		if ( map != newMap )
		{
			map = newMap;
			SetInnerRML( va( "<img class='levelshot' src='/meta/%s/%s' />", newMap, newMap ) );
		}
	}

private:
	void Clear()
	{
		if ( !map.Empty() )
		{
			map = "";
			SetInnerRML( "" );
		}
	}

	Rocket::Core::String map;
};

#define CENTER_PRINT_DURATION 3000
class CenterPrintElement : public HudElement
{
public:
	CenterPrintElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_GAME ) {}

	void DoOnUpdate()
	{
		if ( !*cg.centerPrint )
		{
			return;
		}

		if ( cg.centerPrintTime + CENTER_PRINT_DURATION < cg.time )
		{
			*cg.centerPrint = '\0';
			SetInnerRML( "" );
			return;
		}

		if ( cg.time == cg.centerPrintTime )
		{
			SetInnerRML( Rocket_QuakeToRML( cg.centerPrint, RP_EMOTICONS ) );
		}

		SetProperty( "opacity", va( "%f", CG_FadeAlpha( cg.centerPrintTime, CENTER_PRINT_DURATION ) ) );
	}
};

class BeaconAgeElement : public TextHudElement
{
public:
	BeaconAgeElement( const Rocket::Core::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME ),
			alpha_(0) {}

	void DoOnUpdate()
	{
		if ( cg.beaconRocket.ageAlpha > 0 )
		{
			if ( age != cg.beaconRocket.age )
			{
				age = cg.beaconRocket.age;
				SetText( age );
			}

			if ( alpha_ != cg.beaconRocket.ageAlpha )
			{
				alpha_ = cg.beaconRocket.ageAlpha;
				SetProperty( "opacity", va( "%f", alpha_ ) );
			}
		}
		else
		{
			Clear();
		}
	}

private:
	void Clear()
	{
		if ( alpha_ != 0 )
		{
			alpha_ = 0;
			SetText( "" );
		}
	}

	float alpha_;
	Rocket::Core::String age;
};

class BeaconDistanceElement : public TextHudElement
{
public:
	BeaconDistanceElement( const Rocket::Core::String& tag ) :
	TextHudElement( tag, ELEMENT_GAME ),
	alpha_(0) {}

	void DoOnUpdate()
	{
		if ( cg.beaconRocket.distanceAlpha > 0 )
		{
			if ( distance != cg.beaconRocket.distance )
			{
				distance = cg.beaconRocket.distance;
				SetText( distance );
			}

			if ( alpha_ != cg.beaconRocket.distanceAlpha )
			{
				alpha_ = cg.beaconRocket.distanceAlpha;
				SetProperty( "opacity", va( "%f", alpha_ ) );
			}
		}
		else
		{
			Clear();
		}
	}

private:
	void Clear()
	{
		if ( alpha_ != 0 )
		{
			alpha_ = 0;
			SetText( "" );
		}
	}

	float alpha_;
	Rocket::Core::String distance;
};

class BeaconInfoElement : public TextHudElement
{
public:
	BeaconInfoElement( const Rocket::Core::String& tag ) :
	TextHudElement( tag, ELEMENT_GAME ),
	alpha_(0) {}

	void DoOnUpdate()
	{
		if ( cg.beaconRocket.infoAlpha > 0 )
		{
			if ( info != cg.beaconRocket.info )
			{
				info = cg.beaconRocket.info;
				SetText( info );
			}

			if ( alpha_ != cg.beaconRocket.infoAlpha )
			{
				alpha_ = cg.beaconRocket.infoAlpha;
				SetProperty( "opacity", va( "%f", alpha_ ) );
			}
		}
		else
		{
			Clear();
		}
	}

private:
	void Clear()
	{
		if ( alpha_ != 0 )
		{
			alpha_ = 0;
			SetText( "" );
		}
	}

	float alpha_;
	Rocket::Core::String info;
};

class BeaconNameElement : public HudElement
{
public:
	BeaconNameElement( const Rocket::Core::String& tag ) :
	HudElement( tag, ELEMENT_GAME ),
	alpha_(0) {}

	void DoOnUpdate()
	{
		if ( cg.beaconRocket.nameAlpha > 0 )
		{
			if ( name != cg.beaconRocket.name )
			{
				name = cg.beaconRocket.name;
				SetInnerRML( Rocket_QuakeToRML( name.CString(), RP_EMOTICONS ) );
			}

			if ( alpha_ != cg.beaconRocket.nameAlpha )
			{
				alpha_ = cg.beaconRocket.nameAlpha;
				SetProperty( "opacity", va( "%f", alpha_ ) );
			}
		}
		else
		{
			Clear();
		}
	}

private:
	void Clear()
	{
		if ( alpha_ != 0 )
		{
			alpha_ = 0;
			SetInnerRML( "" );
		}
	}

	float alpha_;
	Rocket::Core::String name;
};

class BeaconIconElement : public HudElement
{
public:
	BeaconIconElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_GAME, true ) {}

	void OnPropertyChange( const Rocket::Core::PropertyNameList& changed_properties )
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( changed_properties.find( "color" ) != changed_properties.end() )
		{
			GetColor( "color", color_ );
		}
	}

	void DoOnRender()
	{
		rectDef_t rect;
		vec4_t color;

		if ( !cg.beaconRocket.icon )
		{
			return;
		}

		GetElementRect( rect );

		Vector4Copy( color_, color );

		color[ 3 ] *= cg.beaconRocket.iconAlpha;

		trap_R_SetColor( color );
		CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cg.beaconRocket.icon );
		trap_R_SetColor( nullptr );
	}

private:
	vec4_t color_;
};


class BeaconOwnerElement : public HudElement
{
public:
	BeaconOwnerElement( const Rocket::Core::String& tag ) :
	HudElement( tag, ELEMENT_GAME ),
	alpha_(0) {}

	void DoOnUpdate()
	{
		if ( cg.beaconRocket.ownerAlpha > 0 )
		{
			if ( owner != cg.beaconRocket.owner )
			{
				owner = cg.beaconRocket.owner;
				SetInnerRML( Rocket_QuakeToRML( owner.CString(), RP_EMOTICONS ) );
			}

			if ( alpha_ != cg.beaconRocket.ownerAlpha )
			{
				alpha_ = cg.beaconRocket.ownerAlpha;
				SetProperty( "opacity", va( "%f", alpha_ ) );
			}
		}
		else
		{
			Clear();
		}
	}

private:
	void Clear()
	{
		if ( alpha_ != 0 )
		{
			alpha_ = 0;
			SetInnerRML( "" );
		}
	}

	float alpha_;
	Rocket::Core::String owner;
};

class PredictedMineEfficiencyElement : public HudElement
{
public:
	PredictedMineEfficiencyElement( const Rocket::Core::String& tag ) :
			HudElement( tag, ELEMENT_BOTH, false ),
			shouldBeVisible( true ),
			display( -1 ),
			pluralSuffix{ { BA_A_LEECH, "es" }, { BA_H_DRILL, "s" } }
	{

	}

	void OnPropertyChange( const Rocket::Core::PropertyNameList& changed_properties )
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( display < 0 && changed_properties.find( "display" ) != changed_properties.end() )
		{
			display = GetProperty<int>( "display" );
		}
	}

	void DoOnUpdate()
	{
		playerState_t  *ps = &cg.snap->ps;
		buildable_t   buildable = ( buildable_t )( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK );

		// If display hasn't been set yet explicitly, assume display is block
		if ( display < 0 )
		{
			display = Rocket::Core::DISPLAY_BLOCK;
		}

		if ( buildable != BA_H_DRILL && buildable != BA_A_LEECH )
		{
			if ( IsVisible() && shouldBeVisible )
			{
				SetProperty("display",
							Rocket::Core::Property(Rocket::Core::DISPLAY_NONE,
												   Rocket::Core::Property::KEYWORD));
				SetInnerRML( "" );
				shouldBeVisible = false;
				// Pick impossible value
				lastDelta = -999;
			}
		}
		else
		{
			if ( !IsVisible() && !shouldBeVisible )
			{
				SetProperty( "display", Rocket::Core::Property( display,
															   Rocket::Core::Property::KEYWORD ) );
				shouldBeVisible = true;
			}
		}
	}

	void DoOnRender()
	{
		if ( shouldBeVisible )
		{
			playerState_t  *ps = &cg.snap->ps;
			buildable_t   buildable = ( buildable_t )( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK );
			const char *msg = nullptr;
			char color;
			int  delta = ps->stats[ STAT_PREDICTION ];

			if ( lastDelta != delta )
			{
				if ( delta < 0 )
				{
					color = COLOR_RED;
					// Error sign
					msg = va( "<span class='material-icon error'>&#xE000;</span> You are losing efficiency. Build the %s%s further apart for more efficiency.", BG_Buildable( buildable )->humanName, pluralSuffix[ buildable ].c_str() );
				}
				else if ( delta < 10 )
				{
					color = COLOR_ORANGE;
					// Warning sign
					msg = va( "<span class='material-icon warning'>&#xE002;</span> Minimal efficency gain. Build the %s%s further apart for more efficiency.", BG_Buildable( buildable )->humanName, pluralSuffix[ buildable ].c_str() );
				}
				else if ( delta < 50 )
				{
					color = COLOR_YELLOW;
					msg = va( "<span class='material-icon warning'>&#xE002;</span> Average efficency gain. Build the %s%s further apart for more efficiency.", BG_Buildable( buildable )->humanName, pluralSuffix[ buildable ].c_str() );
				}
				else
				{
					color = COLOR_GREEN;
				}

				SetInnerRML( va("EFFICIENCY: %s%s%s", CG_Rocket_QuakeToRML( va( "^%c%+d%%", color, delta ) ), msg ? "<br/>" : "", msg ? msg : "" ) );
				lastDelta = delta;
			}
		}
	}
private:
	bool shouldBeVisible;
	int display;
	int lastDelta;
	std::unordered_map<int, std::string> pluralSuffix;
};

void CG_Rocket_DrawPlayerHealth()
{
	static int lastHealth = 0;

	if ( lastHealth != cg.snap->ps.stats[ STAT_HEALTH ] )
	{
		Rocket_SetInnerRML( va( "%d", cg.snap->ps.stats[ STAT_HEALTH ] ), 0 );
	}
}

void CG_Rocket_DrawPlayerHealthCross()
{
	qhandle_t shader;
	vec4_t    color, ref_color;
	float     ref_alpha;
	rectDef_t rect;

	// grab info from libRocket
	CG_GetRocketElementColor( ref_color );
	CG_GetRocketElementRect( &rect );

	// Pick the current icon
	shader = cgs.media.healthCross;

	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_HEALING_8X )
	{
		shader = cgs.media.healthCross3X;
	}

	else if ( cg.snap->ps.stats[ STAT_STATE ] & SS_HEALING_4X )
	{
		if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_ALIENS )
		{
			shader = cgs.media.healthCross2X;
		}

		else
		{
			shader = cgs.media.healthCrossMedkit;
		}
	}

	else if ( cg.snap->ps.stats[ STAT_STATE ] & SS_POISONED )
	{
		shader = cgs.media.healthCrossPoisoned;
	}

	// Pick the alpha value
	Vector4Copy( ref_color, color );

	if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_HUMANS &&
			cg.snap->ps.stats[ STAT_HEALTH ] < 10 )
	{
		color[ 0 ] = 1.0f;
		color[ 1 ] = color[ 2 ] = 0.0f;
	}

	ref_alpha = ref_color[ 3 ];

	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_HEALING_2X )
	{
		ref_alpha = 1.0f;
	}

	// Don't fade from nothing
	if ( !cg.lastHealthCross )
	{
		cg.lastHealthCross = shader;
	}

	// Fade the icon during transition
	if ( cg.lastHealthCross != shader )
	{
		cg.healthCrossFade += cg.frametime / 500.0f;

		if ( cg.healthCrossFade > 1.0f )
		{
			cg.healthCrossFade = 0.0f;
			cg.lastHealthCross = shader;
		}

		else
		{
			// Fading between two icons
			color[ 3 ] = ref_alpha * cg.healthCrossFade;
			trap_R_SetColor( color );
			CG_DrawPic( rect.x, rect.y, rect.w, rect.h, shader );
			color[ 3 ] = ref_alpha * ( 1.0f - cg.healthCrossFade );
			trap_R_SetColor( color );
			CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cg.lastHealthCross );
			trap_R_SetColor( nullptr );
			return;
		}
	}

	// Not fading, draw a single icon
	color[ 3 ] = ref_alpha;
	trap_R_SetColor( color );
	CG_DrawPic( rect.x, rect.y, rect.w, rect.h, shader );
	trap_R_SetColor( nullptr );

}

void CG_Rocket_DrawAlienBarbs()
{
	int numBarbs = cg.snap->ps.ammo;
	char base[ MAX_STRING_CHARS ];
	char rml[ MAX_STRING_CHARS ] = { 0 };

	if ( !numBarbs )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	Com_sprintf( base, sizeof( base ), "<img class='barbs' src='%s' />", CG_Rocket_GetAttribute( "src" ) );

	for ( ; numBarbs > 0; numBarbs-- )
	{
		Q_strcat( rml, sizeof( rml ), base );
	}

	Rocket_SetInnerRML( rml, 0 );
}

/*
============
CG_DrawStack

Helper function: draws a "stack" of <val> boxes in a row or column big
enough to fit <max> boxes. <fill> sets the proportion of space in the stack
with box drawn on it - i.e. lower values will result in thinner boxes.
If the boxes are too thin, or if fill is specified as 0, they will be merged
into a single progress bar.
============
*/

#define LALIGN_TOPLEFT     0
#define LALIGN_CENTER      1
#define LALIGN_BOTTOMRIGHT 2

static void CG_DrawStack( rectDef_t *rect, vec4_t color, float fill,
						  int align, float val, int max )
{
	int      i;
	float    each, frac;
	float    nudge;
	float    fmax = max; // we don't want integer division
	bool vertical; // a stack taller than it is wide is drawn vertically
	vec4_t   localColor;

	// so that the vertical and horizontal bars can share code, abstract the
	// longer dimension and the alignment parameter
	int length;


	if ( val <= 0 || max <= 0 )
	{
		return; // nothing to draw
	}

	trap_R_SetColor( color );

	if ( rect->h >= rect->w )
	{
		vertical = true;
		length = rect->h;
	}

	else
	{
		vertical = false;
		length = rect->w;
	}

	// the filled proportion of the length, divided into max bits
	each = fill * length / fmax;

	// if the scaled length of each segment is too small, do a single bar
	if ( each * ( vertical ? cgs.screenYScale : cgs.screenXScale ) < 4.f )
	{
		float loff;

		switch ( align )
		{
			case LALIGN_TOPLEFT:
			default:
				loff = 0;
				break;

			case LALIGN_CENTER:
				loff = length * ( 1 - val / fmax ) / 2;
				break;

			case LALIGN_BOTTOMRIGHT:
				loff = length * ( 1 - val / fmax );
		}

		if ( vertical )
		{
			CG_DrawPic( rect->x, rect->y + loff, rect->w, rect->h * val / fmax,
						cgs.media.whiteShader );
		}

		else
		{
			CG_DrawPic( rect->x + loff, rect->y, rect->w * val / fmax, rect->h,
						cgs.media.whiteShader );
		}

		trap_R_SetColor( nullptr );
		return;
	}

	// the code would normally leave a bit of space after every square, but this
	// leaves a space on the end, too: nudge divides that space among the blocks
	// so that the beginning and end line up perfectly
	if ( fmax > 1 )
	{
		nudge = ( 1 - fill ) / ( fmax - 1 );
	}

	else
	{
		nudge = 0;
	}

	frac = val - ( int ) val;

	for ( i = ( int ) val - 1; i >= 0; i-- )
	{
		float start;

		switch ( align )
		{
			case LALIGN_TOPLEFT:
				start = ( i * ( 1 + nudge ) + frac ) / fmax;
				break;

			case LALIGN_CENTER:

				// TODO (fallthrough for now)
			default:
			case LALIGN_BOTTOMRIGHT:
				start = 1 - ( val - i - ( i + fmax - val ) * nudge ) / fmax;
				break;
		}

		if ( vertical )
		{
			CG_DrawPic( rect->x, rect->y + rect->h * start, rect->w, each,
						cgs.media.whiteShader );
		}

		else
		{
			CG_DrawPic( rect->x + rect->w * start, rect->y, each, rect->h,
						cgs.media.whiteShader );
		}
	}

	// if there is a partial square, draw it dropping off the end of the stack
	if ( frac <= 0.f )
	{
		trap_R_SetColor( nullptr );
		return; // no partial square, we're done here
	}

	Vector4Copy( color, localColor );
	localColor[ 3 ] *= frac;
	trap_R_SetColor( localColor );

	switch ( align )
	{
		case LALIGN_TOPLEFT:
			if ( vertical )
			{
				CG_DrawPic( rect->x, rect->y - rect->h * ( 1 - frac ) / fmax,
							rect->w, each, cgs.media.whiteShader );
			}

			else
			{
				CG_DrawPic( rect->x - rect->w * ( 1 - frac ) / fmax, rect->y,
							each, rect->h, cgs.media.whiteShader );
			}

			break;

		case LALIGN_CENTER:

			// fallthrough
		default:
		case LALIGN_BOTTOMRIGHT:
			if ( vertical )
			{
				CG_DrawPic( rect->x, rect->y + rect->h *
							( 1 + ( ( 1 - fill ) / fmax ) - frac / fmax ),
							rect->w, each, cgs.media.whiteShader );
			}

			else
			{
				CG_DrawPic( rect->x + rect->w *
							( 1 + ( ( 1 - fill ) / fmax ) - frac / fmax ), rect->y,
							each, rect->h, cgs.media.whiteShader );
			}
	}

	trap_R_SetColor( nullptr );
}

static void CG_DrawPlayerAmmoStack()
{
	float         val;
	int           maxVal, align;
	static int    lastws, maxwt, lastval, valdiff;
	playerState_t *ps = &cg.snap->ps;
	weapon_t      primary = BG_PrimaryWeapon( ps->stats );
	vec4_t        localColor, foreColor;
	rectDef_t     rect;
	static char   buf[ 100 ];

	// grab info from libRocket
	CG_GetRocketElementColor( foreColor );
	CG_GetRocketElementRect( &rect );

	maxVal = BG_Weapon( primary )->maxAmmo;

	if ( maxVal <= 0 )
	{
		return; // not an ammo-carrying weapon
	}

	val = ps->ammo;

	// smoothing effects (only if weaponTime etc. apply to primary weapon)
	if ( ps->weapon == primary && ps->weaponTime > 0 &&
			( ps->weaponstate == WEAPON_FIRING ||
			  ps->weaponstate == WEAPON_RELOADING ) )
	{
		// track the weaponTime we're coming down from
		// if weaponstate changed, this value is invalid
		if ( lastws != ps->weaponstate || ps->weaponTime > maxwt )
		{
			maxwt = ps->weaponTime;
			lastws = ps->weaponstate;
		}

		// if reloading, move towards max ammo value
		if ( ps->weaponstate == WEAPON_RELOADING )
		{
			val = maxVal;
		}

		// track size of change in ammo
		if ( lastval != val )
		{
			valdiff = lastval - val; // can be negative
			lastval = val;
		}

		if ( maxwt > 0 )
		{
			float f = ps->weaponTime / ( float ) maxwt;
			// move from last ammo value to current
			val += valdiff * f * f;
		}
	}

	else
	{
		// reset counters
		lastval = val;
		valdiff = 0;
		lastws = ps->weaponstate;
	}

	if ( val == 0 )
	{
		return; // nothing to draw
	}

	if ( val * 3 < maxVal )
	{
		// low on ammo
		// FIXME: don't hardcode this colour
		vec4_t lowAmmoColor = { 1.f, 0.f, 0.f, 0.f };
		// don't lerp alpha
		VectorLerpTrem( ( cg.time & 128 ), foreColor, lowAmmoColor, localColor );
		localColor[ 3 ] = foreColor[ 3 ];
	}

	else
	{
		Vector4Copy( foreColor, localColor );
	}

	Rocket_GetProperty( "text-align", buf, sizeof( buf ), ROCKET_STRING );

	if ( *buf && !Q_stricmp( buf, "right" ) )
	{
		align = LALIGN_BOTTOMRIGHT;
	}

	else if ( *buf && !Q_stricmp( buf, "center" ) )
	{
		align = LALIGN_CENTER;
	}

	else
	{
		align = LALIGN_TOPLEFT;
	}

	CG_DrawStack( &rect, localColor, 0.8, align, val, maxVal );
}

static void CG_DrawPlayerClipsStack()
{
	float         val;
	int           maxVal;
	static int    lastws, maxwt;
	playerState_t *ps = &cg.snap->ps;
	rectDef_t      rect;
	vec4_t         foreColor;

	// grab info from libRocket
	CG_GetRocketElementColor( foreColor );
	CG_GetRocketElementRect( &rect );

	maxVal = BG_Weapon( BG_PrimaryWeapon( ps->stats ) )->maxClips;

	if ( !maxVal )
	{
		return; // not a clips weapon
	}

	val = ps->clips;

	// if reloading, do fancy interpolation effects
	if ( ps->weaponstate == WEAPON_RELOADING )
	{
		float frac;

		// if we just started a reload, note the weaponTime we're coming down from
		if ( lastws != ps->weaponstate || ps->weaponTime > maxwt )
		{
			maxwt = ps->weaponTime;
			lastws = ps->weaponstate;
		}

		// just in case, don't divide by zero
		if ( maxwt != 0 )
		{
			frac = ps->weaponTime / ( float ) maxwt;
			val -= 1 - frac * frac; // speed is proportional to distance from target
		}
	}

	CG_DrawStack( &rect, foreColor, 0.8, LALIGN_TOPLEFT, val, maxVal );
}

void CG_Rocket_DrawMinimap()
{
	if ( cg.minimap.defined )
	{
		vec4_t    foreColor;
		rectDef_t rect;

		// grab info from libRocket
		CG_GetRocketElementColor( foreColor );
		CG_GetRocketElementRect( &rect );

		CG_DrawMinimap( &rect, foreColor );
	}
}

#define FOLLOWING_STRING "following "
#define CHASING_STRING   "chasing "
void CG_Rocket_DrawFollow()
{
	if ( cg.snap && cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		char buffer[ MAX_STRING_CHARS ];

		if ( !cg.chaseFollow )
		{
			Q_strncpyz( buffer, FOLLOWING_STRING, sizeof( buffer ) );
		}

		else
		{
			Q_strncpyz( buffer, CHASING_STRING, sizeof( buffer ) );
		}

		Q_strcat( buffer, sizeof( buffer ), cgs.clientinfo[ cg.snap->ps.clientNum ].name );

		Rocket_SetInnerRML( buffer, RP_EMOTICONS );
	}
	else
	{
		Rocket_SetInnerRML( "", 0 );
	}
}

void CG_Rocket_DrawConnectText()
{
	char       rml[ MAX_STRING_CHARS ];
	const char *s;

	if ( !Q_stricmp( rocketInfo.cstate.servername, "localhost" ) )
	{
		Q_strncpyz( rml, "Starting up…", sizeof( rml ) );
	}

	else
	{
		Q_strncpyz( rml, va( "Connecting to %s <br/>", rocketInfo.cstate.servername ), sizeof( rml ) );
	}

	if ( rocketInfo.cstate.connState < CA_CONNECTED && *rocketInfo.cstate.messageString )
	{
		Q_strcat( rml, sizeof( rml ), "<br />" );
		Q_strcat( rml, sizeof( rml ), rocketInfo.cstate.messageString );
	}

	switch ( rocketInfo.cstate.connState )
	{
		case CA_CONNECTING:
			s = va( _( "Awaiting connection…%i" ), rocketInfo.cstate.connectPacketCount );
			break;

		case CA_CHALLENGING:
			s = va( _( "Awaiting challenge…%i" ), rocketInfo.cstate.connectPacketCount );
			break;

		case CA_CONNECTED:
			s = _( "Awaiting gamestate…" );
		break;

		case CA_LOADING:
			return;

		case CA_PRIMED:
			return;

		default:
			return;
	}

	Q_strcat( rml, sizeof( rml ), s );

	Rocket_SetInnerRML( rml, 0 );
}

void CG_Rocket_DrawClock()
{
	char    *s;
	qtime_t qt;

	if ( !cg_drawClock.integer )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	Com_RealTime( &qt );

	if ( cg_drawClock.integer == 2 )
	{
		s = va( "%02d%s%02d", qt.tm_hour, ( qt.tm_sec % 2 ) ? ":" : " ",
				qt.tm_min );
	}

	else
	{
		const char *pm = "am";
		int  h = qt.tm_hour;

		if ( h == 0 )
		{
			h = 12;
		}

		else if ( h == 12 )
		{
			pm = "pm";
		}

		else if ( h > 12 )
		{
			h -= 12;
			pm = "pm";
		}

		s = va( "%d%s%02d%s", h, ( qt.tm_sec % 2 ) ? ":" : " ", qt.tm_min, pm );
	}

	Rocket_SetInnerRML( s, 0 );
}

void CG_Rocket_DrawTutorial()
{
	if ( !cg_tutorial.integer )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	Rocket_SetInnerRML( CG_TutorialText(), RP_EMOTICONS );
}

void CG_Rocket_DrawStaminaBolt()
{
	bool  activate = cg.snap->ps.stats[ STAT_STATE ] & SS_SPEEDBOOST;
	Rocket_SetClass( "sprint", activate );
	Rocket_SetClass( "walk", !activate );
}

void CG_Rocket_DrawChatType()
{
	static const struct {
		char colour[4]; // ^n
		char prompt[12];
	} sayText[] = {
		{ "",   "" },
		{ "^2", N_("Say: ") },
		{ "^5", N_("Team Say: ") },
		{ "^6", N_("Admin Say: ") },
		{ "",   N_("Command: ") },
	};

	if ( (size_t) cg.sayType < ARRAY_LEN( sayText ) )
	{
		const char *prompt = _( sayText[ cg.sayType ].prompt );

		if ( ui_chatPromptColors.integer )
		{
			Rocket_SetInnerRML( va( "%s%s", sayText[ cg.sayType ].colour, prompt ), RP_QUAKE );
		}
		else
		{
			Rocket_SetInnerRML( prompt, RP_QUAKE );
		}
	}
}

#define MOMENTUM_BAR_MARKWIDTH 0.5f
#define MOMENTUM_BAR_GLOWTIME  2000

static void CG_Rocket_DrawPlayerMomentumBar()
{
	// data
	rectDef_t     rect;
	vec4_t        foreColor, backColor, lockedColor, unlockedColor;
	playerState_t *ps;
	float         momentum, rawFraction, fraction, glowFraction, glowOffset, borderSize;
	int           threshold;
	team_t        team;
	bool      unlocked;

	momentumThresholdIterator_t unlockableIter = { -1, 0 };

	// display
	vec4_t        color;
	float         x, y, w, h, b, glowStrength;
	bool      vertical;

	CG_GetRocketElementRect( &rect );
	CG_GetRocketElementBGColor( backColor );
	CG_GetRocketElementColor( foreColor );
	Rocket_GetProperty( "border-width", &borderSize, sizeof( borderSize ), ROCKET_FLOAT );
	Rocket_GetProperty( "locked-marker-color", &lockedColor, sizeof( lockedColor ), ROCKET_COLOR );
	Rocket_GetProperty( "unlocked-marker-color", &unlockedColor, sizeof( unlockedColor ), ROCKET_COLOR );


	ps = &cg.predictedPlayerState;

	team = ( team_t ) ps->persistant[ PERS_TEAM ];
	momentum = ps->persistant[ PERS_MOMENTUM ] / 10.0f;

	x = rect.x;
	y = rect.y;
	w = rect.w;
	h = rect.h;
	b = 1.0f;

	vertical = ( h > w );

	// draw border
	CG_DrawRect( x, y, w, h, borderSize, backColor );

	// adjust rect to draw inside border
	x += b;
	y += b;
	w -= 2.0f * b;
	h -= 2.0f * b;

	// draw background
	Vector4Copy( backColor, color );
	color[ 3 ] *= 0.5f;
	CG_FillRect( x, y, w, h, color );

	// draw momentum bar
	fraction = rawFraction = momentum / MOMENTUM_MAX;

	if ( fraction < 0.0f )
	{
		fraction = 0.0f;
	}

	else if ( fraction > 1.0f )
	{
		fraction = 1.0f;
	}

	if ( vertical )
	{
		CG_FillRect( x, y + h * ( 1.0f - fraction ), w, h * fraction, foreColor );
	}

	else
	{
		CG_FillRect( x, y, w * fraction, h, foreColor );
	}

	// draw glow on momentum event
	if ( cg.momentumGainedTime + MOMENTUM_BAR_GLOWTIME > cg.time )
	{
		glowFraction = fabs( cg.momentumGained / MOMENTUM_MAX );
		glowStrength = ( MOMENTUM_BAR_GLOWTIME - ( cg.time - cg.momentumGainedTime ) ) /
					   ( float )MOMENTUM_BAR_GLOWTIME;

		if ( cg.momentumGained > 0.0f )
		{
			glowOffset = vertical ? 0 : glowFraction;
		}

		else
		{
			glowOffset = vertical ? glowFraction : 0;
		}

		CG_SetClipRegion( x, y, w, h );

		color[ 0 ] = 1.0f;
		color[ 1 ] = 1.0f;
		color[ 2 ] = 1.0f;
		color[ 3 ] = 0.5f * glowStrength;

		if ( vertical )
		{
			CG_FillRect( x, y + h * ( 1.0f - ( rawFraction + glowOffset ) ), w, h * glowFraction, color );
		}

		else
		{
			CG_FillRect( x + w * ( rawFraction - glowOffset ), y, w * glowFraction, h, color );
		}

		CG_ClearClipRegion();
	}

	// draw threshold markers
	while ( ( unlockableIter = BG_IterateMomentumThresholds( unlockableIter, team, &threshold, &unlocked ) ),
			( unlockableIter.num >= 0 ) )
	{
		fraction = threshold / MOMENTUM_MAX;

		if ( fraction > 1.0f )
		{
			fraction = 1.0f;
		}

		if ( vertical )
		{
			CG_FillRect( x, y + h * ( 1.0f - fraction ), w, MOMENTUM_BAR_MARKWIDTH, unlocked ? unlockedColor : lockedColor );
		}

		else
		{
			CG_FillRect( x + w * fraction, y, MOMENTUM_BAR_MARKWIDTH, h, unlocked ? unlockedColor : lockedColor );
		}
	}

	trap_R_SetColor( nullptr );

}

void CG_Rocket_DrawMineRate()
{
	float levelRate, rate;
	int efficiency;

	// check if builder
	switch ( BG_GetPlayerWeapon( &cg.snap->ps ) )
	{
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			break;

		default:
			Rocket_SetInnerRML( "", 0 );
			return;
	}

	levelRate  = cg.predictedPlayerState.persistant[ PERS_MINERATE ] / 10.0f;
	efficiency = cg.predictedPlayerState.persistant[ PERS_RGS_EFFICIENCY ];
	rate       = ( ( efficiency / 100.0f ) * levelRate );

	Rocket_SetInnerRML( va( _( "%.1f BP/min (%d%% × %.1f)" ), rate, efficiency, levelRate ), 0 );
}

static INLINE qhandle_t CG_GetUnlockableIcon( int num )
{
	int index = BG_UnlockableTypeIndex( num );

	switch ( BG_UnlockableType( num ) )
	{
		case UNLT_WEAPON:
			return cg_weapons[ index ].weaponIcon;

		case UNLT_UPGRADE:
			return cg_upgrades[ index ].upgradeIcon;

		case UNLT_BUILDABLE:
			return cg_buildables[ index ].buildableIcon;

		case UNLT_CLASS:
			return cg_classes[ index ].classIcon;

        default:
            return 0;
	}
}

static void CG_Rocket_DrawPlayerUnlockedItems()
{
	rectDef_t     rect;
	vec4_t        foreColour, backColour;
	momentumThresholdIterator_t unlockableIter = { -1, 1 }, previousIter;

	// data
	team_t    team;

	// display
	float     x, y, w, h, iw, ih, borderSize;
	bool  vertical;

	int       icons, counts;
	int       count[ 32 ] = { 0 };
	struct
	{
		qhandle_t shader;
		bool  unlocked;
	} icon[ NUM_UNLOCKABLES ]; // more than enough(!)

	CG_GetRocketElementRect( &rect );
	Rocket_GetProperty( "cell-color", backColour, sizeof( vec4_t ), ROCKET_COLOR );
	CG_GetRocketElementColor( foreColour );
	Rocket_GetProperty( "border-width", &borderSize, sizeof( borderSize ), ROCKET_FLOAT );

	team = ( team_t ) cg.predictedPlayerState.persistant[ PERS_TEAM ];

	w = rect.w - 2 * borderSize;
	h = rect.h - 2 * borderSize;

	vertical = ( h > w );

	ih = vertical ? w : h;
	iw = ih * cgs.aspectScale;

	x = rect.x + borderSize;
	y = rect.y + borderSize + ( h - ih ) * vertical;

	icons = counts = 0;

	for ( ;; )
	{
		qhandle_t shader;
		int       threshold;
		bool  unlocked;

		previousIter = unlockableIter;
		unlockableIter = BG_IterateMomentumThresholds( unlockableIter, team, &threshold, &unlocked );

		if ( previousIter.threshold != unlockableIter.threshold && icons )
		{
			count[ counts++ ] = icons;
		}

		// maybe exit the loop?
		if ( unlockableIter.num < 0 )
		{
			break;
		}

		// okay, next icon
		shader = CG_GetUnlockableIcon( unlockableIter.num );

		if ( shader )
		{
			icon[ icons ].shader = shader;
			icon[ icons].unlocked = unlocked;
			++icons;
		}
	}

	{
		float gap;
		int i, j;
		vec4_t unlockedBg, lockedBg;

		Vector4Copy( foreColour, unlockedBg );
		unlockedBg[ 3 ] *= 0.0f;  // No background
		Vector4Copy( backColour, lockedBg );
		lockedBg[ 3 ] *= 0.0f;  // No background

		gap = vertical ? ( h - icons * ih ) : ( w - icons * iw );

		if ( counts > 2 )
		{
			gap /= counts - 1;
		}

		for ( i = 0, j = 0; count[ i ]; ++i )
		{
			if ( vertical )
			{
				float yb = y - count[ i ] * ih - i * gap;
				float hb = ( count[ i ] - j ) * ih;
				CG_DrawRect( x - borderSize, yb - borderSize, iw + 2 * borderSize, hb, borderSize, backColour );
				CG_FillRect( x, yb, iw, hb - 2 * borderSize, icon[ j ].unlocked ? unlockedBg : lockedBg );
			}

			else
			{
				float xb = x + j * iw + i * gap;
				float wb = ( count[ i ] - j ) * iw + 2;
				CG_DrawRect( xb - borderSize, y - borderSize, wb, ih + 2 * borderSize, borderSize, backColour );
				CG_FillRect( xb, y, wb - 2 * borderSize, ih, icon[ j ].unlocked ? unlockedBg : lockedBg );
			}

			j = count[ i ];
		}

		for ( i = 0, j = 0; i < icons; ++i )
		{
			trap_R_SetColor( icon[ i ].unlocked ? foreColour : backColour );

			if ( i == count[ j ] )
			{
				++j;

				if ( vertical )
				{
					y -= gap;
				}

				else
				{
					x += gap;
				}
			}

			CG_DrawPic( x, y, iw, ih, icon[ i ].shader );

			if ( vertical )
			{
				y -= ih;
			}

			else
			{
				x += iw;
			}
		}
	}

	trap_R_SetColor( nullptr );
}

static void CG_Rocket_DrawVote_internal( team_t team )
{
	char   *s;
	int    sec;

	if ( !cgs.voteTime[ team ] )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified[ team ] )
	{
		cgs.voteModified[ team ] = false;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime[ team ] ) ) / 1000;

	if ( sec < 0 )
	{
		sec = 0;
	}

	Rocket::Core::String yeskey = CG_KeyBinding( va( "%svote yes", team == TEAM_NONE ? "" : "team" ), team );
	Rocket::Core::String nokey = CG_KeyBinding( va( "%svote no", team == TEAM_NONE ? "" : "team" ), team );

	s = va( "%sVOTE(%i): %s\n"
			"    Called by: \"%s\"\n"
			"    [%s][<span class='material-icon'>&#xe8dc;</span>]:%i [%s][<span class='material-icon'>&#xe8db;</span>]:%i\n",
			team == TEAM_NONE ? "" : "TEAM", sec, cgs.voteString[ team ],
			cgs.voteCaller[ team ], yeskey.CString(), cgs.voteYes[ team ], nokey.CString(), cgs.voteNo[ team ] );

	Rocket_SetInnerRML( s, 0 );
}

static void CG_Rocket_DrawVote()
{
	CG_Rocket_DrawVote_internal( TEAM_NONE );
}

static void CG_Rocket_DrawTeamVote()
{
	CG_Rocket_DrawVote_internal( ( team_t ) cg.predictedPlayerState.persistant[ PERS_TEAM ] );
}

static void CG_Rocket_DrawSpawnQueuePosition()
{
	int    position;
	const char *s;

	if ( !( cg.snap->ps.pm_flags & PMF_QUEUED ) )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	position = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] >> 8;

	if ( position < 1 )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	if ( position == 1 )
	{
		s = va( _( "You are at the front of the spawn queue" ) );
	}

	else
	{
		s = va( _( "You are at position %d in the spawn queue" ), position );
	}

	Rocket_SetInnerRML( s, 0 );
}

static void CG_Rocket_DrawNumSpawns()
{
	int    position, spawns;
	const char *s;

	if ( !( cg.snap->ps.pm_flags & PMF_QUEUED ) )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	spawns   = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] & 0x000000ff;
	position = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] >> 8;

	if ( position < 1 || cg.intermissionStarted )
	{
		s = "";
	}
	else if ( spawns == 0 )
	{
		s = _( "There are no spawns remaining" );
	}
	else
	{
		s = va( P_( "There is %d spawn remaining", "There are %d spawns remaining", spawns ), spawns );
	}

	Rocket_SetInnerRML( s, 0 );
}

static void CG_Rocket_DrawWarmup()
{
	int   sec = 0;

	if ( !cg.warmupTime )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	sec = ( cg.warmupTime - cg.time ) / 1000;

	if ( sec < 0 )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	Rocket_SetInnerRML( va( "%s", sec ? va( "%d", sec ) : _("FIGHT!") ), 0 );
}

static void CG_Rocket_DrawProgressValue()
{
	const char *src = CG_Rocket_GetAttribute( "src" );
	if ( *src )
	{
		float value = CG_Rocket_ProgressBarValue( src );
		Rocket_SetInnerRML( va( "%d", (int) ( value * 100 ) ), 0 );
	}
}

static void CG_Rocket_DrawLevelName()
{
	Rocket_SetInnerRML( CG_ConfigString( CS_MESSAGE ), RP_QUAKE );
}

static void CG_Rocket_DrawMOTD()
{
	const char *s;
	char       parsed[ MAX_STRING_CHARS ];

	s = CG_ConfigString( CS_MOTD );
	Q_ParseNewlines( parsed, s, sizeof( parsed ) );
	Rocket_SetInnerRML( parsed, RP_EMOTICONS );
}

static void CG_Rocket_DrawHostname()
{
	const char *info;
	info = CG_ConfigString( CS_SERVERINFO );
	Rocket_SetInnerRML( Info_ValueForKey( info, "sv_hostname" ), RP_QUAKE );
}

static void CG_Rocket_DrawDownloadName()
{
	char downloadName[ MAX_STRING_CHARS ];

	trap_Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof( downloadName ) );

	if ( Q_stricmp( downloadName, rocketInfo.downloadName ) )
	{
		Q_strncpyz( rocketInfo.downloadName, downloadName, sizeof( rocketInfo.downloadName ) );
		Rocket_SetInnerRML( rocketInfo.downloadName, RP_QUAKE );
	}
}

static void CG_Rocket_DrawDownloadTime()
{
	float downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );
	float downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	float downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );
	int xferRate;

	if ( !*rocketInfo.downloadName )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	if ( ( rocketInfo.realtime - downloadTime ) / 1000 )
	{
		xferRate = downloadCount / ( ( rocketInfo.realtime - downloadTime ) / 1000 );
	}
	else
	{
		xferRate = 0;
	}

	if ( xferRate && downloadSize )
	{
		char dlTimeBuf[ MAX_STRING_CHARS ];
		int n = downloadSize / xferRate; // estimated time for entire d/l in secs

		// We do it in K (/1024) because we'd overflow around 4MB
		CG_PrintTime( dlTimeBuf, sizeof dlTimeBuf,
				  ( n - ( ( ( downloadCount / 1024 ) * n ) / ( downloadSize / 1024 ) ) ) * 1000 );
		Rocket_SetInnerRML( dlTimeBuf, RP_QUAKE );
	}
	else
	{
		Rocket_SetInnerRML( _( "estimating" ), RP_QUAKE );
	}
}

static void CG_Rocket_DrawDownloadTotalSize()
{
	char totalSizeBuf[ MAX_STRING_CHARS ];
	float downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );

	if ( !*rocketInfo.downloadName )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	CG_ReadableSize( totalSizeBuf,  sizeof totalSizeBuf,  downloadSize );

	Rocket_SetInnerRML( totalSizeBuf, RP_QUAKE );
}

static void CG_Rocket_DrawDownloadCompletedSize()
{
	char dlSizeBuf[ MAX_STRING_CHARS ];
	float downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );

	if ( !*rocketInfo.downloadName )
	{
		Rocket_SetInnerRML( "", 0 );
		return;
	}

	CG_ReadableSize( dlSizeBuf,  sizeof dlSizeBuf,  downloadCount );

	Rocket_SetInnerRML( dlSizeBuf, RP_QUAKE );
}

static void CG_Rocket_DrawDownloadSpeed()
{
	char xferRateBuf[ MAX_STRING_CHARS ];
	float downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	float downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );
	int xferRate;

	if ( !*rocketInfo.downloadName )
	{
		Rocket_SetInnerRML ( "", 0 );
		return;
	}

	if ( ( rocketInfo.realtime - downloadTime ) / 1000 )
	{
		xferRate = downloadCount / ( ( rocketInfo.realtime - downloadTime ) / 1000 );
		CG_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );
		Rocket_SetInnerRML( va( "%s/Sec", xferRateBuf ), RP_QUAKE );
	}
	else
	{
		xferRate = 0;
		Rocket_SetInnerRML( "0 KB/Sec", RP_QUAKE );
	}
}

static void CG_Rocket_HaveJetpck()
{
	bool jetpackInInventory = BG_InventoryContainsUpgrade( UP_JETPACK, cg.snap->ps.stats );
	Rocket_SetClass( "active", jetpackInInventory );
	Rocket_SetClass( "inactive", !jetpackInInventory );
}

typedef struct
{
	const char *name;
	void ( *exec )();
	rocketElementType_t type;
} elementRenderCmd_t;

// THESE MUST BE ALPHABETIZED
static const elementRenderCmd_t elementRenderCmdList[] =
{
	{ "ammo_stack", &CG_DrawPlayerAmmoStack, ELEMENT_HUMANS },
	{ "barbs", &CG_Rocket_DrawAlienBarbs, ELEMENT_ALIENS },
	{ "chattype", &CG_Rocket_DrawChatType, ELEMENT_ALL },
	{ "clip_stack", &CG_DrawPlayerClipsStack, ELEMENT_HUMANS },
	{ "clock", &CG_Rocket_DrawClock, ELEMENT_ALL },
	{ "connecting", &CG_Rocket_DrawConnectText, ELEMENT_ALL },
	{ "downloadCompletedSize", &CG_Rocket_DrawDownloadCompletedSize, ELEMENT_ALL },
	{ "downloadName", &CG_Rocket_DrawDownloadName, ELEMENT_ALL },
	{ "downloadSpeed", &CG_Rocket_DrawDownloadSpeed, ELEMENT_ALL },
	{ "downloadTime", &CG_Rocket_DrawDownloadTime, ELEMENT_ALL },
	{ "downloadTotalSize", &CG_Rocket_DrawDownloadTotalSize, ELEMENT_ALL },
	{ "follow", &CG_Rocket_DrawFollow, ELEMENT_GAME },
	{ "health", &CG_Rocket_DrawPlayerHealth, ELEMENT_BOTH },
	{ "health_cross", &CG_Rocket_DrawPlayerHealthCross, ELEMENT_BOTH },
	{ "hostname", &CG_Rocket_DrawHostname, ELEMENT_ALL },
	{ "inventory", &CG_DrawHumanInventory, ELEMENT_HUMANS },
	{ "itemselect_text", &CG_DrawItemSelectText, ELEMENT_HUMANS },
	{ "jetpack", &CG_Rocket_HaveJetpck, ELEMENT_HUMANS },
	{ "levelname", &CG_Rocket_DrawLevelName, ELEMENT_ALL },
	{ "mine_rate", &CG_Rocket_DrawMineRate, ELEMENT_BOTH },
	{ "minimap", &CG_Rocket_DrawMinimap, ELEMENT_ALL },
	{ "momentum_bar", &CG_Rocket_DrawPlayerMomentumBar, ELEMENT_BOTH },
	{ "motd", &CG_Rocket_DrawMOTD, ELEMENT_ALL },
	{ "numSpawns", &CG_Rocket_DrawNumSpawns, ELEMENT_DEAD },
	{ "progress_value", &CG_Rocket_DrawProgressValue, ELEMENT_ALL },
	{ "spawnPos", &CG_Rocket_DrawSpawnQueuePosition, ELEMENT_DEAD },
	{ "stamina_bolt", &CG_Rocket_DrawStaminaBolt, ELEMENT_HUMANS },
	{ "tutorial", &CG_Rocket_DrawTutorial, ELEMENT_GAME },
	{ "unlocked_items", &CG_Rocket_DrawPlayerUnlockedItems, ELEMENT_BOTH },
	{ "votes", &CG_Rocket_DrawVote, ELEMENT_GAME },
	{ "votes_team", &CG_Rocket_DrawTeamVote, ELEMENT_BOTH },
	{ "warmup_time", &CG_Rocket_DrawWarmup, ELEMENT_GAME },
};

static const size_t elementRenderCmdListCount = ARRAY_LEN( elementRenderCmdList );

static int elementRenderCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( elementRenderCmd_t * ) b )->name );
}

void CG_Rocket_RenderElement( const char *tag )
{
	elementRenderCmd_t *cmd;

	cmd = ( elementRenderCmd_t * ) bsearch( tag, elementRenderCmdList, elementRenderCmdListCount, sizeof( elementRenderCmd_t ), elementRenderCmdCmp );

	if ( cmd && CG_Rocket_IsCommandAllowed( cmd->type ) )
	{
		cmd->exec();
	}
}

#define REGISTER_ELEMENT( tag, clazz ) Rocket::Core::Factory::RegisterElementInstancer( tag, new Rocket::Core::ElementInstancerGeneric< clazz >() )->RemoveReference();
void CG_Rocket_RegisterElements()
{
	for ( unsigned i = 0; i < elementRenderCmdListCount; i++ )
	{
		//Check that the commands are in increasing order so that it can be used by bsearch
		if ( i != 0 && Q_stricmp( elementRenderCmdList[ i - 1 ].name, elementRenderCmdList[ i ].name ) > 0 )
		{
			CG_Printf( "CGame elementRenderCmdList is in the wrong order for %s and %s\n", elementRenderCmdList[i - 1].name, elementRenderCmdList[ i ].name );
		}

		Rocket_RegisterElement( elementRenderCmdList[ i ].name );
	}

	REGISTER_ELEMENT( "ammo", AmmoHudElement )
	REGISTER_ELEMENT( "clips", ClipsHudElement )
	REGISTER_ELEMENT( "fps", FpsHudElement )
	REGISTER_ELEMENT( "crosshair_indicator", CrosshairIndicatorHudElement )
	REGISTER_ELEMENT( "crosshair", CrosshairHudElement )
	REGISTER_ELEMENT( "speedometer", SpeedGraphElement )
	REGISTER_ELEMENT( "credits", CreditsValueElement )
	REGISTER_ELEMENT( "evos", EvosValueElement )
	REGISTER_ELEMENT( "stamina", StaminaValueElement )
	REGISTER_ELEMENT( "weapon_icon", WeaponIconElement )
	REGISTER_ELEMENT( "wallwalk", WallwalkElement )
	REGISTER_ELEMENT( "usable_buildable", UsableBuildableElement )
	REGISTER_ELEMENT( "location", LocationElement )
	REGISTER_ELEMENT( "timer", TimerElement )
	REGISTER_ELEMENT( "lagometer", LagometerElement )
	REGISTER_ELEMENT( "crosshair_name", CrosshairNamesElement )
	REGISTER_ELEMENT( "momentum", MomentumElement )
	REGISTER_ELEMENT( "levelshot", LevelshotElement )
	REGISTER_ELEMENT( "levelshot_loading", LevelshotLoadingElement )
	REGISTER_ELEMENT( "center_print", CenterPrintElement )
	REGISTER_ELEMENT( "beacon_age", BeaconAgeElement )
	REGISTER_ELEMENT( "beacon_distance", BeaconDistanceElement )
	REGISTER_ELEMENT( "beacon_icon", BeaconIconElement )
	REGISTER_ELEMENT( "beacon_info", BeaconInfoElement )
	REGISTER_ELEMENT( "beacon_name", BeaconNameElement )
	REGISTER_ELEMENT( "beacon_owner", BeaconOwnerElement )
	REGISTER_ELEMENT( "predictedMineEfficiency", PredictedMineEfficiencyElement )
}
