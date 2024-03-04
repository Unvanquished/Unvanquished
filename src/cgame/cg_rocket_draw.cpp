/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "common/FileSystem.h"
#include "cg_local.h"
#include "cg_key_name.h"
#include "rocket/rocket.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/ElementText.h>

Cvar::Cvar<bool> cg_drawPosition("cg_drawPosition", "show position. Requires cg_drawSpeed to be enabled.", Cvar::NONE, false);

static void CG_GetRocketElementColor( Color::Color& color )
{
	Rocket_GetProperty( "color", &color, sizeof(Color::Color), rocketVarType_t::ROCKET_COLOR );
}

static void CG_GetRocketElementBGColor( Color::Color& bgColor )
{
	Rocket_GetProperty( "background-color", &bgColor, sizeof(Color::Color), rocketVarType_t::ROCKET_COLOR );
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

static bool updateLanguage;
void Rocket_UpdateLanguage()
{
	updateLanguage = true;
	if ( menuContext )
		menuContext->Update();
	if ( hudContext )
		hudContext->Update();
	updateLanguage = false;
}

// Standard HTML

class LiElement : public Rml::Element
{
public:
	LiElement(const Rml::String& tag) : Rml::Element(tag) {}

private:
	void OnUpdate() override
	{
		if ( !done )
		{
			SetInnerRML( va( "<li-bullet>• </li-bullet><li-content>%s</li-content>",
				GetInnerRML().c_str() ) );
			done = true;
		}
	}

	bool done = false;
};

// Game-specific RML

class WebElement : public Rml::Element
{
public:
	WebElement(const Rml::String& tag) : Rml::Element(tag) {}

private:
	const std::unordered_map<std::string, std::string> webUrls = {
		{ "bugs", "bugs.unvanquished.net" },
		{ "chat", "unvanquished.net/chat" },
		{ "forums", "forums.unvanquished.net" },
		{ "wiki", "wiki.unvanquished.net" },
	};

	void OnAttributeChange( const Rml::ElementAttributes& changed_attributes ) override
	{
		Rml::Element::OnAttributeChange( changed_attributes );

		if ( changed_attributes.find( "dest" ) != changed_attributes.end() )
		{
			const Rml::String dest = GetAttribute< Rml::String >( "dest",  "" );

			if ( webUrls.find( dest ) != webUrls.end() )
			{
				SetInnerRML( webUrls.at( dest ) );
			}
			else
			{
				Log::Warn( "Unknown \"dest\" attribute in web RML tag" );
				SetInnerRML( "⚠ BUG" );
			}
		}
	}
};

class TranslateElement : public Rml::Element
{
public:
	TranslateElement(const Rml::String& tag) : Rml::Element(tag) {}

	void InitContent(const Rml::String& content)
	{
		originalRML_ = content;
		if ( HasAttribute( "quake" ) )
		{
			flags_ |= RP_QUAKE;
		}

		if ( HasAttribute( "emoticons" ) )
		{
			flags_ |= RP_EMOTICONS;
		}
		FillTranslatedContent();
	}

	// The translate elements needs to be serializable in some cases. For example,
	// for some reason dropdown menus construct copies of their choices from the RML text.
	void GetRML(Rml::String& out) override
	{
		out.append("<translate");
		if (flags_ & RP_QUAKE)
			out.append(" quake");
		if (flags_ & RP_EMOTICONS)
			out.append(" emoticons");
		out.push_back('>');
		out.append(originalRML_);
		out.append("</translate>");
	}

private:
	void FillTranslatedContent()
	{
		const char* translated = Trans_Gettext( originalRML_.c_str() );
		if ( flags_ != 0 )
		{
			SetInnerRML( Rocket_QuakeToRML( translated, flags_ ) );
		}
		else
		{
			SetInnerRML( translated );
		}
	}

	void OnUpdate() override
	{
		if ( updateLanguage )
			FillTranslatedContent();
	}

	Rml::String originalRML_;
	int flags_ = 0;
};

class TranslateNodeHandler : public Rml::XMLNodeHandler
{
	// Copied from XMLNodeHandlerDefault::ElementStart, which is declared in a private header
	// so I shouldn't inherit from it
	Rml::Element* DefaultElementStart(
		Rml::XMLParser* parser, const Rml::String& name, const Rml::XMLAttributes& attributes)
	{
		Rml::Element* parent = parser->GetParseFrame()->element;
		Rml::ElementPtr element = Rml::Factory::InstanceElement(parent, name, name, attributes);
		if (!element)
		{
			Log::Warn("Failed to create element for <translate>");
			return nullptr;
		}
		Rml::Element* result = parent->AppendChild(std::move(element));
		return result;
	}

	Rml::Element* ElementStart(
		Rml::XMLParser* parser, const Rml::String& name, const Rml::XMLAttributes& attributes) override
	{
		// BaseXMLParser checks for CDATAness *after* calling ElementStart, so we can do this.
		parser->RegisterCDATATag("translate");

		return DefaultElementStart(parser, name, attributes);
	}

	bool ElementData(Rml::XMLParser* parser, const Rml::String& data, Rml::XMLDataType type) override
	{
		ASSERT(dynamic_cast<TranslateElement*>(parser->GetParseFrame()->element));
		if (type != Rml::XMLDataType::CData)
			Log::Warn("unexpected argument to TranslateElement ElementData");
		TranslateElement* element = static_cast<TranslateElement*>(parser->GetParseFrame()->element);
		element->InitContent(data);
		return true;
	}

	bool ElementEnd(Rml::XMLParser*, const Rml::String& data) override { return true; }
};

class HudElement : public Rml::Element
{
public:
	HudElement(const Rml::String& tag, rocketElementType_t type_, bool replacedElement) :
			Rml::Element(tag),
			type(type_),
			isReplacedElement(replacedElement) {}

	HudElement(const Rml::String& tag, rocketElementType_t type_) :
			Rml::Element(tag),
			type(type_),
			isReplacedElement(false) {}

	void OnUpdate() override
	{
		Rml::Element::OnUpdate();
		if (CG_Rocket_IsCommandAllowed(type))
		{
			DoOnUpdate();
		}
	}

	void OnRender() override
	{
		if (CG_Rocket_IsCommandAllowed(type))
		{
			DoOnRender();
			Rml::Element::OnRender();
		}
	}

	virtual void DoOnRender() {}
	virtual void DoOnUpdate() {}

	bool GetIntrinsicDimensions( Rml::Vector2f &dimension, float& /*ratio*/ ) override
	{
		if ( !isReplacedElement )
		{
			return false;
		}

		const Rml::Property *property = GetProperty( Rml::PropertyId::Width );

		// Absolute unit. We can use it as is
		if ( property->unit & Rml::Property::ABSOLUTE_UNIT )
		{
			dimensions.x = property->value.Get<float>();
		}
		else
		{
			Rml::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.x = ResolveNumericProperty( property, parent->GetBox().GetSize().x );
			}
		}

		property = GetProperty( Rml::PropertyId::Height );
		if ( property->unit & Rml::Property::ABSOLUTE_UNIT )
		{
			dimensions.y = property->value.Get<float>();
		}
		else
		{
			Rml::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.y = ResolveNumericProperty( property, parent->GetBox().GetSize().y );
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

	void GetColor( const Rml::String& property, Color::Color& color )
	{
		color = Color::Adapt( GetProperty<Rml::Colourb>( property ) );
	}

protected:
	Rml::Vector2f dimensions;

private:
	rocketElementType_t type;
	bool isReplacedElement;
};

class TextHudElement : public HudElement
{
public:
	TextHudElement( const Rml::String& tag, rocketElementType_t type ) :
		HudElement( tag, type )
	{
		InitializeTextElement();
	}

	TextHudElement( const Rml::String& tag, rocketElementType_t type, bool replacedElement ) :
		HudElement( tag, type, replacedElement )
	{
		InitializeTextElement();
	}

	void SetText(const Rml::String& text )
	{
		textElement->SetText( text );
	}

private:
	void InitializeTextElement()
	{
		textElement = dynamic_cast< Rml::ElementText* >( AppendChild( Rml::Factory::InstanceElement(
			this,
			"#text",
			"#text",
			Rml::XMLAttributes() ) ) );
	}

	Rml::ElementText* textElement;

};

class AmmoHudElement : public TextHudElement
{
public:
	AmmoHudElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_BOTH ),
			showTotalAmmo_( false ),
			builder_( false ),
			ammo_( 0 ),
			spentBudget_( 0 ),
			markedBudget_( 0 ),
			totalBudget_( 0 ),
			queuedBudget_( 0 ) {}

	void OnAttributeChange( const Rml::ElementAttributes& changed_attributes ) override
	{
		TextHudElement::OnAttributeChange( changed_attributes );
		if ( changed_attributes.find( "type" ) != changed_attributes.end() )
		{
			const Rml::String& type = GetAttribute<Rml::String>( "type", "" );
			showTotalAmmo_ = type == "total";
		}
	}

	void DoOnUpdate() override
	{
		weapon_t weapon = BG_PrimaryWeapon( cg.snap->ps.stats );

		switch ( weapon )
		{
			case WP_NONE:
			case WP_BLASTER:
				return;

			case WP_ABUILD:
			case WP_ABUILD2:
			case WP_HBUILD:
				if ( builder_ &&
				     spentBudget_  == cg.snap->ps.persistant[ PERS_SPENTBUDGET ] &&
				     markedBudget_ == cg.snap->ps.persistant[ PERS_MARKEDBUDGET ] &&
				     totalBudget_  == cg.snap->ps.persistant[ PERS_TOTALBUDGET ] &&
				     queuedBudget_ == cg.snap->ps.persistant[ PERS_QUEUEDBUDGET ] )
				{
					return;
				}

				spentBudget_  = cg.snap->ps.persistant[ PERS_SPENTBUDGET ];
				markedBudget_ = cg.snap->ps.persistant[ PERS_MARKEDBUDGET ];
				totalBudget_  = cg.snap->ps.persistant[ PERS_TOTALBUDGET ];
				queuedBudget_ = cg.snap->ps.persistant[ PERS_QUEUEDBUDGET ];
				builder_      = true;

				break;

			default:
				if ( showTotalAmmo_ )
				{
					int maxAmmo = BG_Weapon( weapon )->maxAmmo;
					if ( !builder_ &&
					     ammo_ == cg.snap->ps.ammo + ( cg.snap->ps.clips * maxAmmo ) )
					{
						return;
					}

					ammo_ = cg.snap->ps.ammo + ( cg.snap->ps.clips * maxAmmo );
				}
				else
				{
					if ( !builder_ &&
					     ammo_ == cg.snap->ps.ammo )
					{
						return;
					}

					ammo_ = cg.snap->ps.ammo;
				}

				builder_ = false;

				break;
		}

		if ( builder_ )
		{
			int freeBudget = totalBudget_ - (spentBudget_ + queuedBudget_);
			int available  = freeBudget + markedBudget_;

			if ( markedBudget_ != 0 )
			{
				SetText( va( "%d+%d = %d", freeBudget, markedBudget_, available ) );
			}
			else
			{
				SetText( va( "%d", freeBudget ) );
			}
		}
		else
		{
			SetText( va( "%d", ammo_ ) );
		}
	}

private:
	bool showTotalAmmo_;
	bool builder_;
	int  ammo_;
	int  spentBudget_;
	int  markedBudget_;
	int  totalBudget_;
	int  queuedBudget_;
};


class ClipsHudElement : public TextHudElement
{
public:
	ClipsHudElement( const Rml::String& tag ) :
		TextHudElement( tag, ELEMENT_HUMANS ),
		clips_( 0 ) {}

	void DoOnUpdate() override
	{
		int           value;
		playerState_t *ps = &cg.snap->ps;

		if ( BG_Weapon( BG_PrimaryWeapon( ps->stats ) )->infiniteAmmo )
		{
			if ( clips_ != -1 )
			{
				SetText( "" );
			}
			clips_ = -1;
			return;
		}
		value = ps->clips;

		if ( value > -1 && value != clips_ )
		{
			SetText( va( "%d", value ) );
			clips_ = value;
		}
	}

private:
	int clips_;
};

class FpsHudElement : public TextHudElement
{
public:
	FpsHudElement( const Rml::String& tag )
			: TextHudElement( tag, ELEMENT_ALL ),
			  showingFps_( false ),
			  lastFrame_( 0 ),
			  fps_( 0 ) {}

	void DoOnUpdate() override
	{
		// don't use serverTime, because that will be drifting to
		// correct for Internet lag changes, timescales, timedemos, etc.
		int t = trap_Milliseconds();
		Util::UpdateFPSCounter( 1.0f, t - lastFrame_, fps_ );
		lastFrame_ = t;

		if ( cg_drawFPS.Get() )
		{
			SetText( Str::Format("%.0f", fps_) );
			showingFps_ = true;
		}
		else if ( showingFps_ )
		{
			SetText( "" );
			showingFps_ = false;
		}
	}

private:
	bool showingFps_;
	int lastFrame_;
	float fps_;
};

#define CROSSHAIR_INDICATOR_HITFADE 500

class CrosshairIndicatorHudElement : public HudElement
{
public:
	CrosshairIndicatorHudElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_GAME, true ),
			color_( Color::White ) {}

	void OnPropertyChange( const Rml::PropertyIdSet& changed_properties ) override
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( changed_properties.Contains( Rml::PropertyId::Color ) )
		{
			GetColor( "color", color_ );
		}
	}

	void DoOnRender() override
	{
		rectDef_t    rect;
		float        x, y, w, h, dim;
		qhandle_t    indicator;
		Color::Color drawColor, baseColor;
		weapon_t     weapon;
		weaponInfo_t *wi;
		bool     onRelevantEntity;

		if ( ( !cg_drawCrosshairHit.Get() && !cg_drawCrosshairFriendFoe.Get() ) ||
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
		if ( cg_drawCrosshairFriendFoe.Get() >= CROSSHAIR_ALWAYSON ||
			( cg_drawCrosshairFriendFoe.Get() >= CROSSHAIR_RANGEDONLY && BG_Weapon( weapon )->longRanged ) )
		{
			if ( cg.crosshairFoe )
			{
				baseColor = Color::Red;
				baseColor.SetAlpha( color_.Alpha() * 0.75f );
				onRelevantEntity = true;
			}

			else if ( cg.crosshairFriend )
			{
				baseColor = Color::Green;
				baseColor.SetAlpha( color_.Alpha() * 0.75f );
				onRelevantEntity = true;
			}

			else
			{
				baseColor = { 1.0f, 1.0f, 1.0f, 0.0f };
				onRelevantEntity = false;
			}
		}

		else
		{
			baseColor = { 1.0f, 1.0f, 1.0f, 0.0f };
			onRelevantEntity = false;
		}

		// add hit color
		if ( cg_drawCrosshairHit.Get() && cg.hitTime + CROSSHAIR_INDICATOR_HITFADE > cg.time )
		{
			dim = ( ( cg.hitTime + CROSSHAIR_INDICATOR_HITFADE ) - cg.time ) / ( float )CROSSHAIR_INDICATOR_HITFADE;
			drawColor = Color::Blend( baseColor, Color::White, dim );
		}

		else if ( !onRelevantEntity )
		{
			return;
		}

		else
		{
			drawColor = baseColor;
		}

		// set size
		w = h = wi->crossHairSize * cg_crosshairSize.Get();
		w *= cgs.aspectScale;

		x = rect.x + ( rect.w / 2 ) - ( w / 2 );
		y = rect.y + ( rect.h / 2 ) - ( h / 2 );

		// draw
		trap_R_SetColor( drawColor );
		CG_DrawPic( x, y, w, h, indicator );
		trap_R_ClearColor();
	}
private:
	Color::Color color_;
};

class CrosshairHudElement : public HudElement {
public:
	CrosshairHudElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_GAME, true ),
			// Use as a sentinel value to denote that no weapon is
			// explicitly set.
			weapon_( WP_NUM_WEAPONS ) {
	}

	void OnAttributeChange( const Rml::ElementAttributes& changed_attributes ) override
	{
		HudElement::OnAttributeChange( changed_attributes );
		// If we are not in game, then don't bother looking up weapon info, since it's not loaded.
		if ( rocketInfo.cstate.connState != connstate_t::CA_ACTIVE )
		{
			return;
		}
		// If we have a weapon attribute, then *always* display the crosshair for that weapon, regardless
		// of team or spectator state.
		auto it = changed_attributes.find( "weapon" );
		if ( it != changed_attributes.end() )
		{
			auto weaponName = it->second.Get<std::string>();
			auto wa = BG_WeaponByName( weaponName.c_str() );
			if ( wa == nullptr || wa->number == WP_NONE )
			{
				Log::Warn( "Invalid forced crosshair weapon: %s", weaponName );
			}
			else
			{
				weapon_ = wa->number;
			}
		}
	}

	void DoOnRender() override
	{
		rectDef_t    rect;
		float        w, h;
		qhandle_t    crosshair;
		float        x, y;
		weaponInfo_t *wi;
		weapon_t     weapon;

		if ( weapon_ == WP_NUM_WEAPONS )
		{
			weapon = BG_GetPlayerWeapon( &cg.snap->ps );
		}
		else
		{
			weapon = weapon_;
		}

		if ( weapon_ == WP_NUM_WEAPONS )
		{
			if ( cg_drawCrosshair.Get() == CROSSHAIR_ALWAYSOFF )
			{
				return;
			}

			if ( cg_drawCrosshair.Get() == CROSSHAIR_RANGEDONLY &&
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
		}

		GetElementRect( rect );

		wi = &cg_weapons[ weapon ];

		w = h = wi->crossHairSize * cg_crosshairSize.Get();
		w *= cgs.aspectScale;

		// HACK: This ignores the width/height of the rect (does it?)
		x = rect.x + ( rect.w / 2 ) - ( w / 2 );
		y = rect.y + ( rect.h / 2 ) - ( h / 2 );

		crosshair = wi->crossHair;

		if ( crosshair )
		{
			Color::Color crosshairColor;

			if ( cg_crosshairStyle.Get() == CROSSHAIR_STYLE_CUSTOM ) {
				crosshairColor.SetRed( cg_crosshairColorRed.Get() );
				crosshairColor.SetGreen( cg_crosshairColorGreen.Get() );
				crosshairColor.SetBlue( cg_crosshairColorBlue.Get() );
				crosshairColor.SetAlpha( cg_crosshairColorAlpha.Get() );
			} else {
				CG_GetRocketElementColor( crosshairColor );
			}

			if ( cg_crosshairOutlineStyle.Get() != CROSSHAIR_OUTLINE_STYLE_NONE ) {
				Color::Color outlineColor(crosshairColor);

				if ( cg_crosshairOutlineStyle.Get() == CROSSHAIR_OUTLINE_STYLE_AUTO ) {
					/* Approximation of sRGB to linear conversion without using gamma expansion?
					   Luma grayscale formula: Y' = 0.299R' + 0.587G' + 0.114B'
					   This might be wrong w. r. t. linear/sRGB colorspace */
					float grayScale = outlineColor.Red() * 0.299f + outlineColor.Green() * 0.587f + outlineColor.Blue() * 0.114f;
					if ( fabsf( 0.5f - grayScale ) < 0.25f ) {
						grayScale += 0.5f * ( grayScale < 0.5f ? 1.0f : -1.0f );
					} else {
						grayScale = 1.0f - grayScale;
					}

					outlineColor.SetRed( grayScale );
					outlineColor.SetGreen( grayScale );
					outlineColor.SetBlue( grayScale );
					outlineColor.SetAlpha( 1.0f );
				} else {
					outlineColor.SetRed( cg_crosshairOutlineColorRed.Get() );
					outlineColor.SetGreen( cg_crosshairOutlineColorGreen.Get() );
					outlineColor.SetBlue( cg_crosshairOutlineColorBlue.Get() );
					outlineColor.SetAlpha( cg_crosshairOutlineColorAlpha.Get() );
				}

				const float scale = cg_crosshairOutlineScale.Get();
				const float outlineHeight = ( wi->crossHairSizeNoBorder * scale + cg_crosshairOutlineOffset.Get() )
										    * ( wi->crossHairSize / wi->crossHairSizeNoBorder ) * cg_crosshairSize.Get();
				const float outlineWidth = outlineHeight * cgs.aspectScale;
				const float outlineX = rect.x + ( rect.w / 2 ) - ( outlineWidth / 2 );
				const float outlineY = rect.y + ( rect.h / 2 ) - ( outlineHeight / 2 );

				trap_R_SetColor( outlineColor );
				CG_DrawPic( outlineX, outlineY, outlineWidth, outlineHeight, crosshair );
			}

			trap_R_SetColor( crosshairColor );
			CG_DrawPic( x, y, w, h, crosshair );
			trap_R_ClearColor();
		}
	}
	private:
	weapon_t weapon_;
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

	if ( cg_drawSpeed.Get() & SPEEDOMETER_IGNORE_Z )
	{
		vel[ 2 ] = 0;
	}

	speed = VectorLength( vel );

	windowTime = cg_maxSpeedTimeWindow.Get();

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
	SpeedGraphElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_GAME, true ),
			shouldDrawSpeed_( false )
	{
		Rml::XMLAttributes xml;
		maxSpeedElement = dynamic_cast< Rml::ElementText* >( AppendChild( Rml::Factory::InstanceElement(
			this,
			"#text",
			"span",
			xml ) ) );
		maxSpeedElement->SetClass( "speed_max", true );
		currentSpeedElement = dynamic_cast< Rml::ElementText* >( AppendChild( Rml::Factory::InstanceElement(
			this,
			"#text",
			"span",
			xml) ) );
		currentSpeedElement->SetClass( "speed_current", true );
	}

	void DoOnRender() override
	{
		int          i;
		float        val, max, top;
		// colour of graph is interpolated between these values
		const Color::Color slow = { 0.0, 0.0, 1.0 };
		const Color::Color medium = { 0.0, 1.0, 0.0 };
		const Color::Color fast = { 1.0, 0.0, 0.0 };
		rectDef_t    rect;

		if ( !cg_drawSpeed.Get() )
		{
			if ( shouldDrawSpeed_ )
			{
				currentSpeedElement->SetText( "" );
				maxSpeedElement->SetText( "" );
				shouldDrawSpeed_ = false;
			}
			return;
		}
		else if ( !shouldDrawSpeed_ )
		{
			shouldDrawSpeed_ = true;
		}


		if ( cg_drawSpeed.Get() & SPEEDOMETER_DRAW_GRAPH )
		{
			// grab info from libRocket
			GetElementRect( rect );

			max = speedSamples[ maxSpeedSample ];

			if ( max < SPEEDOMETER_MIN_RANGE )
			{
				max = SPEEDOMETER_MIN_RANGE;
			}

			for ( i = 1; i < SPEEDOMETER_NUM_DISPLAYED_SAMPLES; i++ )
			{
				val = speedSamples[( oldestSpeedSample + i + SPEEDOMETER_NUM_SAMPLES -
				SPEEDOMETER_NUM_DISPLAYED_SAMPLES ) % SPEEDOMETER_NUM_SAMPLES ];
				Color::Color color;

				if ( val < SPEED_MED )
				{
					color = Color::Blend ( slow, medium, val / SPEED_MED );
				}

				else if ( val < SPEED_FAST )
				{
					color = Color::Blend ( medium, fast, ( val - SPEED_MED ) / ( SPEED_FAST - SPEED_MED ) );
				}

				else
				{
					color = fast;
				}

				trap_R_SetColor( color );
				top = rect.y + ( 1 - val / max ) * rect.h;
				CG_DrawPic( rect.x + ( i / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES ) * rect.w, top,
							rect.w / ( float ) SPEEDOMETER_NUM_DISPLAYED_SAMPLES, val * rect.h / max,
							cgs.media.whiteShader );
			}

			trap_R_ClearColor();
		}
	}

	void DoOnUpdate() override
	{
		if ( cg_drawSpeed.Get() & SPEEDOMETER_DRAW_TEXT )
		{
			float val;

			// Add text to be configured via CSS
			if ( cg.predictedPlayerState.clientNum == cg.clientNum )
			{
				vec3_t vel;
				VectorCopy( cg.predictedPlayerState.velocity, vel );

				if ( cg_drawSpeed.Get() & SPEEDOMETER_IGNORE_Z )
				{
					vel[ 2 ] = 0;
				}

				val = VectorLength( vel );
			}

			else
			{
				val = speedSamples[( oldestSpeedSample - 1 + SPEEDOMETER_NUM_SAMPLES ) % SPEEDOMETER_NUM_SAMPLES ];
			}
			maxSpeedElement->SetText( va( "%d ", ( int ) speedSamples[ maxSpeedSampleInWindow ] ) );
			currentSpeedElement->SetText( va( "%d", ( int ) val ) );
		}
	}

private:
	Rml::ElementText* maxSpeedElement;
	Rml::ElementText* currentSpeedElement;
	bool shouldDrawSpeed_;
};

class PositionElement : public TextHudElement
{
public:
	PositionElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME, true ),
			lastpos_(),
			shown_(false)
	{
	}

	void DoOnUpdate() override
	{
		if ( !cg_drawPosition.Get() )
		{
			if ( shown_ )
			{
				SetText( "" );
				shown_ = false;
			}
			return;
		}

		shown_ = true;
		glm::vec3 origin = VEC2GLM( cg.predictedPlayerState.origin );
		if ( lastpos_ != origin )
		{
			SetText( Str::Format( "%0.0f %0.0f %0.0f",
					origin[0], origin[1], origin[2] ) );
			lastpos_ = origin;
		}
	}

	glm::vec3 lastpos_;
	bool shown_;
};

class CreditsValueElement : public TextHudElement
{
public:
	CreditsValueElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_HUMANS ),
			credits_( -1 ) {}

	void DoOnUpdate() override
	{
		playerState_t *ps = &cg.snap->ps;
		int value = ps->persistant[ PERS_CREDIT ];;
		if ( credits_ != value )
		{
			credits_ = value;
			SetText( va( "%d", credits_ ) );
		}
	}

private:
	int credits_;
};

class EvosValueElement : public TextHudElement
{
public:
	EvosValueElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_ALIENS ),
			evos_( -1 ) {}

	void DoOnUpdate() override
	{
		playerState_t *ps = &cg.snap->ps;

		// display the value rounded down
		//
		// we use integer division to avoid rounding errors, those are
		// multiplications and divisions by 10 because humans count in
		// base 10.

		int value = ps->persistant[ PERS_CREDIT ];;
		// value is in tenth of evo points
		value = value * 10 / CREDITS_PER_EVO;

		if ( evos_ != value )
		{
			evos_ = value;
			SetText( va( "%i.%i", value/10, value%10 ) );
		}
	}

private:
	int evos_;
};

class WeaponIconElement : public HudElement
{
public:
	WeaponIconElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_BOTH ),
			weapon_( WP_NONE ),
			isNoAmmo_( false ) {}

	void DoOnUpdate() override
	{
		playerState_t *ps;
		weapon_t      newWeapon;

		ps = &cg.snap->ps;
		newWeapon = BG_GetPlayerWeapon( ps );

		if ( newWeapon != weapon_ )
		{
			weapon_ = newWeapon;
			// don't display if dead
			if ( ( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 || weapon_ == WP_NONE ) && !IsVisible() )
			{
				SetProperty( "display", "none" );
				return;
			}

			if ( weapon_ < WP_NONE || weapon_ >= WP_NUM_WEAPONS )
			{
				Sys::Drop( "CG_DrawWeaponIcon: weapon out of range: %d", weapon_ );
			}

			if ( !cg_weapons[ weapon_ ].registered )
			{
				Log::Warn( "CG_DrawWeaponIcon: weapon %d (%s) "
				"is not registered", weapon_, BG_Weapon( weapon_ )->name );
				SetProperty( "display", "none" );
				return;
			}

			SetInnerRML( va( "<img src='/%s' />", CG_GetShaderNameFromHandle( cg_weapons[ weapon_ ].weaponIcon ) ) );
			SetProperty( "display", "block" );
		}

		if ( !isNoAmmo_ && ps->clips == 0 && ps->ammo == 0 && !BG_Weapon( weapon_ )->infiniteAmmo )
		{
			SetClass( "no_ammo", true );
		}
		else if ( isNoAmmo_ )
		{
			SetClass( "no_ammo", false );
		}

	}
private:
	int weapon_;
	bool isNoAmmo_;
};

class WallwalkElement : public HudElement
{
public:
	WallwalkElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_ALIENS )
	{
		SetActive( false );
	}

	void DoOnUpdate() override
	{
		bool wallwalking = cg.snap->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING;
		if ( wallwalking != isActive_ )
		{
			SetActive( wallwalking );
		}
	}

private:
	void SetActive( bool active )
	{
		isActive_ = active;
		SetClass( "active", active );
		SetClass( "inactive", !active );

	}

	bool isActive_;
};

class StaminaElement : public HudElement
{
public:
	StaminaElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_HUMANS ),
			isActive_( false )
	{
	}

	void DoOnUpdate() override
	{
		bool sprinting = cg.snap->ps.stats[ STAT_STATE ] & SS_SPEEDBOOST;
		if ( sprinting != isActive_ )
		{
			isActive_ = sprinting;
			SetClass( "sprinting", sprinting );
		}
	}

private:
	bool isActive_;
};

class UsableBuildableElement : public HudElement
{
public:
	UsableBuildableElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_HUMANS ) {}

	void DoOnUpdate() override
	{
		vec3_t        view, point;
		trace_t       trace;

		AngleVectors( cg.refdefViewAngles, view, nullptr, nullptr );
		VectorMA( cg.refdef.vieworg, ENTITY_USE_RANGE, view, point );
		CG_Trace( &trace, cg.refdef.vieworg, nullptr, nullptr,
				  point, cg.predictedPlayerState.clientNum, MASK_SHOT, 0 );

		const entityState_t &es = cg_entities[ trace.entityNum ].currentState;

		if ( es.eType == entityType_t::ET_BUILDABLE
				&& es.modelindex == BA_H_ARMOURY
				&& Distance(cg.predictedPlayerState.origin, es.origin) < ENTITY_USE_RANGE - 0.2f // use an epsilon to account for rounding errors
				&& CG_IsOnMyTeam(es) )
		{
			if ( !IsVisible() )
			{
				SetProperty( Rml::PropertyId::Display, Rml::Property( Rml::Style::Display::Block ) );
				cg.nearUsableBuildable = es.modelindex;
			}
		}
		else
		{
			if ( IsVisible() )
			{
				// Clear the old image if there was one.
				SetProperty( Rml::PropertyId::Display, Rml::Property( Rml::Style::Display::None ) );
				cg.nearUsableBuildable = BA_NONE;
			}
		}
	}
};

class LocationElement : public HudElement
{
public:
	LocationElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_GAME ),
			lastlocation_( Util::nullopt ) {}

	void DoOnUpdate() override
	{
		if ( cg.intermissionStarted )
		{
			if ( lastlocation_ )
			{
				SetInnerRML( "" );
				lastlocation_ = {};
			}
			return;
		}

		const centity_t *locent = CG_GetPlayerLocation();

		if ( locent == lastlocation_ )
		{
			return;
		}

		lastlocation_ = locent;
		int cs_index = locent != nullptr
			? CS_LOCATIONS + locent->currentState.generic1
			: CS_LOCATIONS;

		SetInnerRML( Rocket_QuakeToRML( CG_ConfigString( cs_index ), RP_EMOTICONS ) );
	}

private:
	Util::optional<const centity_t *> lastlocation_;
};

class TimerElement : public TextHudElement
{
public:
	TimerElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME ),
			mins_( 0 ),
			seconds_( 0 ),
			tens_( 0 ) {}

	void DoOnUpdate() override
	{
		int   mins, seconds, tens;
		int   msec;

		if ( !cg_drawTimer.Get() && IsVisible() )
		{
			SetProperty( "display", "none" );
			return;
		}
		else if ( cg_drawTimer.Get() && !IsVisible() )
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

struct lagometer_t
{
	int frameSamples[ LAG_SAMPLES ];
	int frameCount;
	int snapshotFlags[ LAG_SAMPLES ];
	int snapshotSamples[ LAG_SAMPLES ];
	int snapshotCount;
};

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
CG_Rocket_DrawDisconnect

Should we draw something different for long lag vs no packets?
==============
*/
static void CG_Rocket_DrawDisconnect()
{
	float      x, y;
	int        cmdNum;
	usercmd_t  cmd;

	// draw the phone jack if we are completely past our buffers
	cmdNum = cg.currentCmdNumber - CMD_BACKUP + 1;
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

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader( "gfx/feedback/net", (RegisterShaderFlags_t) ( RSF_NOMIP ) ) );
}

#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

class LagometerElement : public HudElement
{
public:
	LagometerElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_GAME, true ),
			shouldDrawLagometer_( true ),
			adjustedColor_( 0.0f, 0.0f, 0.0f, 0.0f )
	{
	}

	void OnPropertyChange( const Rml::PropertyIdSet& changed_properties ) override
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( changed_properties.Contains( Rml::PropertyId::BackgroundColor ) )
		{
			GetColor( "background-color", adjustedColor_ );
		}
	}

	void DoOnRender() override
	{
		int    a, i;
		float  v;
		float  ax, ay, aw, ah, mid, range;
		int    color;
		float  vscale;
		rectDef_t rect;

		if ( !shouldDrawLagometer_ )
		{
			return;
		}

		// grab info from libRocket
		GetElementRect( rect );

		trap_R_SetColor( adjustedColor_ );
		CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cgs.media.whiteShader );
		trap_R_ClearColor();

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

		// draw the frame interpolate / extrapolate graph
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
					trap_R_SetColor( Color::Yellow );
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
					trap_R_SetColor( Color::Blue );
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
						trap_R_SetColor( Color::Yellow );
					}
				}

				else
				{
					if ( color != 3 )
					{
						color = 3;

						trap_R_SetColor( Color::Green );
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
					trap_R_SetColor( Color::Red );
				}

				trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
			}
		}

		trap_R_ClearColor();
		CG_Rocket_DrawDisconnect();
	}

private:
	bool shouldDrawLagometer_;
	Color::Color adjustedColor_;
};

class PingElement : public TextHudElement
{
public:
	PingElement( const Rml::String& tag ) :
				 TextHudElement( tag, ELEMENT_GAME, true ),
				 shouldDrawPing_( true )
	{
	}

	void DoOnUpdate() override
	{
		const char* ping;

		if ( ( cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION )
			|| cg.demoPlayback )
		{
			if ( shouldDrawPing_ )
			{
				SetText( "" );
				ping_ = "";
				shouldDrawPing_ = false;
			}
			return;
		}
		else if ( !shouldDrawPing_ )
		{
			shouldDrawPing_ = true;
		}

		if ( cg_nopredict.Get() || cg.pmoveParams.synchronous )
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
	bool shouldDrawPing_;
	Rml::String ping_;
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
	if ( CM_PointContents( trace.endpos, 0 ) & CONTENTS_FOG )
	{
		return;
	}

	entityState_t &targetState = cg_entities[ trace.entityNum ].currentState;

	team_t ownTeam    = CG_MyTeam();
	team_t targetTeam = CG_Team(targetState);

	if ( trace.entityNum >= MAX_CLIENTS )
	{
		// we have a non-client entity

		// set friend/foe if it's a living buildable
		if ( targetState.eType == entityType_t::ET_BUILDABLE
				&& CG_IsAlive(targetState) )
		{
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
		if ( cg_drawEntityInfo.Get() && targetState.eType != entityType_t::ET_GENERAL )
		{
			cg.crosshairEntityNum = trace.entityNum;
			cg.crosshairEntityTime = cg.time;
		}
	}
	else
	{
		// only react to living clients
		if ( !CG_IsAlive(targetState) )
			return;

		// set friend/foe
		if ( targetTeam == ownTeam && ownTeam != TEAM_NONE )
		{
			cg.crosshairFriend = true;

			// only set this for friendly clients as it triggers name display
			cg.crosshairEntityNum = trace.entityNum;
			cg.crosshairEntityTime = cg.time;
		}

		else if ( targetTeam != TEAM_NONE )
		{
			cg.crosshairFoe = true;

			if ( ownTeam == TEAM_NONE )
			{
				// spectating, so show the name
				cg.crosshairEntityNum = trace.entityNum;
				cg.crosshairEntityTime = cg.time;
			}
		}
	}
}

class CrosshairNamesElement : public HudElement
{
public:
	CrosshairNamesElement( const Rml::String& tag  ) :
			HudElement( tag, ELEMENT_GAME ), alpha_( 0.0F ) {}

	void DoOnUpdate() override
	{
		Rml::String name;
		float alpha;

		if ( ( !cg_drawCrosshairNames.Get() && !cg_drawEntityInfo.Get() ) || cg.renderingThirdPerson )
		{
			Clear();
			return;
		}

		// scan the known entities to see if the crosshair is sighted on one
		CG_ScanForCrosshairEntity();

		// draw the name of the player being looked at
		alpha = CG_FadeAlpha( cg.crosshairEntityTime, CROSSHAIR_CLIENT_TIMEOUT );

		if ( cg.crosshairEntityTime == cg.time )
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

		if ( cg_drawEntityInfo.Get() )
		{
			name = va( "(^5%s^7|^5#%d^7)",
					   Com_EntityTypeName( cg_entities[cg.crosshairEntityNum].currentState.eType ), cg.crosshairEntityNum );
		}

		else if ( cg.crosshairEntityNum < MAX_CLIENTS )
		{
			if ( cg_drawCrosshairNames.Get() >= 2 )
			{
				name = va( "%2i: %s", cg.crosshairEntityNum, cgs.clientinfo[ cg.crosshairEntityNum ].name );
			}

			else
			{
				name = cgs.clientinfo[ cg.crosshairEntityNum ].name;
			}
		}

		// add health from overlay info to the crosshair client name
		if ( cg_teamOverlayUserinfo.Get() &&
			CG_MyTeam() != TEAM_NONE &&
			cgs.teamInfoReceived &&
			cg.crosshairEntityNum < MAX_CLIENTS &&
			cgs.clientinfo[ cg.crosshairEntityNum ].health > 0)
		{
			name = va( "%s ^7[^%c%d^7]", name.c_str(),
					   CG_GetColorCharForHealth( cg.crosshairEntityNum ),
					   cgs.clientinfo[ cg.crosshairEntityNum ].health);
		}

		if ( name != name_ )
		{
			name_ = name;
			SetInnerRML( Rocket_QuakeToRML( name.c_str(), RP_EMOTICONS ) );
		}
	}

private:
	void Clear()
	{
		if ( !name_.empty() )
		{
			name_ = "";
			SetInnerRML( "" );
		}
	}

	Rml::String name_;
	float alpha_;
};

class LevelshotElement : public HudElement
{
public:
	LevelshotElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_ALL ), mapIndex_( -1 ) {}

	void DoOnUpdate() override
	{
		if ( rocketInfo.data.mapIndex < 0 ||
		     static_cast<size_t>( rocketInfo.data.mapIndex ) >= rocketInfo.data.mapList.size() )
		{
			Clear();
			return;
		}

		if ( mapIndex_ != rocketInfo.data.mapIndex )
		{
			mapIndex_ = rocketInfo.data.mapIndex;
			std::error_code ignored;
			const std::string& mapName = rocketInfo.data.mapList[ mapIndex_ ].mapLoadName;
			FS::PakPath::LoadPakPrefix(
				*FS::FindPak( "map-" + mapName ), Str::Format( "meta/%s/", mapName ), ignored );
			SetInnerRML( Str::Format( "<img class='levelshot' src='/meta/%s/%s' />", mapName, mapName ) );
		}
	}

private:
	void Clear()
	{
		if ( mapIndex_ != -1)
		{
			mapIndex_ = -1;
			SetInnerRML("");
		}
	}

	int mapIndex_;
};

class LevelshotLoadingElement : public HudElement
{
public:
	LevelshotLoadingElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_ALL ), map_("") {}

	void DoOnUpdate() override
	{
		if ( rocketInfo.cstate.connState < connstate_t::CA_LOADING )
		{
			Clear();
			return;
		}

		const char *newMap = Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), "mapname" );
		if ( map_ != newMap )
		{
			map_ = newMap;
			SetInnerRML( Str::Format( "<img src='/meta/%s/%s' />", newMap, newMap ) );
		}
	}

private:
	void Clear()
	{
		if ( !map_.empty() )
		{
			map_ = "";
			SetInnerRML( "" );
		}
	}

	Rml::String map_;
};

#define CENTER_PRINT_DURATION 3000
class CenterPrintElement : public HudElement
{
public:
	CenterPrintElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_GAME ) {}

	void DoOnUpdate() override
	{
		if ( !*cg.centerPrint )
		{
			return;
		}

		if ( cg.centerPrintTime + CENTER_PRINT_DURATION < cg.time )
		{
			if ( showTime_ >= 0 )
			{
				showTime_ = -1;
				SetInnerRML( "" );
			}
			return;
		}

		if ( showTime_ != cg.centerPrintTime )
		{
			showTime_ = cg.centerPrintTime;
			SetInnerRML( Rocket_QuakeToRML( cg.centerPrint, RP_EMOTICONS ) );
			SetProperty( Rml::PropertyId::FontSize,
			             Rml::Property( cg.centerPrintSizeFactor, Rml::Property::EM ) );
		}

		SetProperty( "opacity", va( "%f", CG_FadeAlpha( cg.centerPrintTime, CENTER_PRINT_DURATION ) ) );
	}

private:
	int showTime_ = -1;
};

class BeaconAgeElement : public TextHudElement
{
public:
	BeaconAgeElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME ),
			alpha_(0) {}

	void DoOnUpdate() override
	{
		if ( cg.beaconRocket.ageAlpha > 0 )
		{
			if ( age_ != cg.beaconRocket.age )
			{
				age_ = cg.beaconRocket.age;
				SetText( age_ );
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
	Rml::String age_;
};

class BeaconDistanceElement : public TextHudElement
{
public:
	BeaconDistanceElement( const Rml::String& tag ) :
	TextHudElement( tag, ELEMENT_GAME ),
	alpha_(0),
	distance_("") {}

	void DoOnUpdate() override
	{
		if ( cg.beaconRocket.distanceAlpha > 0 )
		{
			if ( distance_ != cg.beaconRocket.distance )
			{
				distance_ = cg.beaconRocket.distance;
				SetText( distance_ );
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
	Rml::String distance_;
};

class BeaconInfoElement : public TextHudElement
{
public:
	BeaconInfoElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME ),
			alpha_(0),
			info_("") {}

	void DoOnUpdate() override
	{
		if ( cg.beaconRocket.infoAlpha > 0 )
		{
			if ( info_ != cg.beaconRocket.info )
			{
				info_ = cg.beaconRocket.info;
				SetText( info_ );
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
	Rml::String info_;
};

class BeaconNameElement : public HudElement
{
public:
	BeaconNameElement( const Rml::String& tag ) :
	HudElement( tag, ELEMENT_GAME ),
	alpha_(0),
	name_("") {}

	void DoOnUpdate() override
	{
		if ( cg.beaconRocket.nameAlpha > 0 )
		{
			if ( name_ != cg.beaconRocket.name )
			{
				name_ = cg.beaconRocket.name;
				SetInnerRML( Rocket_QuakeToRML( name_.c_str(), RP_EMOTICONS ) );
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
	Rml::String name_;
};

class BeaconIconElement : public HudElement
{
public:
	BeaconIconElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_GAME, true ),
			color_(Color::White) {}

	void OnPropertyChange( const Rml::PropertyIdSet& changed_properties ) override
	{
		HudElement::OnPropertyChange( changed_properties );
		if ( changed_properties.Contains( Rml::PropertyId::Color ) )
		{
			GetColor( "color", color_ );
		}
	}

	void DoOnRender() override
	{
		rectDef_t rect;
		Color::Color color;

		if ( !cg.beaconRocket.icon )
		{
			return;
		}

		GetElementRect( rect );

		color = color_;

		color.SetAlpha( color.Alpha() * cg.beaconRocket.iconAlpha );

		trap_R_SetColor( color );
		CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cg.beaconRocket.icon );
		trap_R_ClearColor();
	}

private:
	Color::Color color_;
};


class BeaconOwnerElement : public HudElement
{
public:
	BeaconOwnerElement( const Rml::String& tag ) :
	HudElement( tag, ELEMENT_GAME ),
	alpha_(0),
	owner_("") {}

	void DoOnUpdate() override
	{
		if ( cg.beaconRocket.ownerAlpha > 0 )
		{
			if ( owner_ != cg.beaconRocket.owner )
			{
				owner_ = cg.beaconRocket.owner;
				SetInnerRML( Rocket_QuakeToRML( owner_.c_str(), RP_EMOTICONS ) );
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
	Rml::String owner_;
};

class PredictedMineEfficiencyElement : public HudElement
{
public:
	PredictedMineEfficiencyElement( const Rml::String& tag ) :
			HudElement( tag, ELEMENT_BOTH, false ),
			shouldBeVisible_( false ),
			lastDeltaEfficiencyPct_( -999 ),
			lastDeltaBudget_( -999 ),
			pluralSuffix_{ { BA_A_LEECH, "es" }, { BA_H_DRILL, "s" } }
	{

	}

	void DoOnUpdate() override
	{
		playerState_t  *ps = &cg.snap->ps;
		buildable_t   buildable = ( buildable_t )( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK );

		if ( buildable != BA_H_DRILL && buildable != BA_A_LEECH )
		{
			shouldBeVisible_ = false;
			if ( IsVisible() )
			{
				SetProperty( Rml::PropertyId::Display,
						Rml::Property( Rml::Style::Display::None ) );
				SetInnerRML( "" );

				// Pick impossible value
				lastDeltaEfficiencyPct_ = -999;
				lastDeltaBudget_        = -999;
			}
		}
		else
		{
			shouldBeVisible_ = true;
			if ( !IsVisible() )
			{
				SetProperty( Rml::PropertyId::Display,
						Rml::Property( Rml::Style::Display::Block ) );
			}
		}

		PopulateText();
	}

	void PopulateText()
	{
		if ( shouldBeVisible_ )
		{
			playerState_t *ps = &cg.snap->ps;
			buildable_t buildable = ( buildable_t )( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK );
			const char *msg = nullptr;
			Color::Color color;

			// The efficiency and budget deltas are signed values that are encode as the least and
			// most significant byte of the de-facto short ps->stats[STAT_PREDICTION], respectively.
			// The efficiency delta is a value between -1 and 1, the budget delta is an integer
			// between -128 and 127.
			float deltaEfficiency    = (float)(signed char)(ps->stats[STAT_PREDICTION] & 0xff) / (float)0x7f;
			int   deltaBudget        = (int)(signed char)(ps->stats[STAT_PREDICTION] >> 8);

			int   deltaEfficiencyPct = (int)(deltaEfficiency * 100.0f);

			if ( deltaEfficiencyPct != lastDeltaEfficiencyPct_ ||
			     deltaBudget        != lastDeltaBudget_ )
			{
				if        ( deltaBudget < 0 ) {
					color = Color::Red;
					// Icon U+E000
					msg = va( "<span class='material-icon error'>\xee\x80\x80</span> You are losing build points!"
					          " Build the %s%s further apart for greater efficiency.",
					          BG_Buildable( buildable )->humanName, pluralSuffix_[ buildable ].c_str() );
				} else if ( deltaBudget < cgs.buildPointBudgetPerMiner / 10 ) {
					color = Color::Orange;
					// Icon U+E002
					msg = va( "<span class='material-icon warning'>\xee\x80\x82</span> Minimal build point gain."
					          " Build the %s%s further apart for greater efficiency.",
					          BG_Buildable( buildable )->humanName, pluralSuffix_[ buildable ].c_str() );
				} else if ( deltaBudget < cgs.buildPointBudgetPerMiner / 2 ) {
					color = Color::Yellow;
					// Icon U+E002
					msg = va( "<span class='material-icon warning'>\xee\x80\x82</span> Subpar build point gain."
					          " Build the %s%s further apart for greater efficiency.",
					          BG_Buildable( buildable )->humanName, pluralSuffix_[ buildable ].c_str() );
				} else {
					color = Color::Green;
				}

				char deltaEfficiencyPctStr[64];
				char deltaBudgetStr[64];

				Q_strncpyz(deltaEfficiencyPctStr, CG_Rocket_QuakeToRML(va(
					"%s%+d%%", Color::ToString(color).c_str(), deltaEfficiencyPct
				)), 64);

				Q_strncpyz(deltaBudgetStr, CG_Rocket_QuakeToRML(va(
					"%s%+d", Color::ToString(color).c_str(), deltaBudget
				)), 64);

				SetInnerRML(va(
					"%s EFFICIENCY<br/>%s BUILD POINTS%s%s",
					deltaEfficiencyPctStr, deltaBudgetStr, msg ? "<br/>" : "", msg ? msg : ""
				));

				lastDeltaEfficiencyPct_ = deltaEfficiencyPct;
				lastDeltaBudget_        = deltaBudget;
			}
		}
	}
private:
	bool shouldBeVisible_;
	int  lastDeltaEfficiencyPct_;
	int  lastDeltaBudget_;
	std::unordered_map<int, std::string> pluralSuffix_;
};

class BarbsHudElement : public HudElement
{
public:
	BarbsHudElement ( const Rml::String& tag ) :
	HudElement ( tag, ELEMENT_ALIENS ),
	numBarbs_( 0 ),
	maxBarbs_( BG_Weapon( WP_ALEVEL3_UPG )->maxAmmo ),
	regenerationInterval_ ( 0 ),
	t0_ ( 0 ),
	offset_ ( 0 ) {}

	void OnAttributeChange( const Rml::ElementAttributes& changed_attributes ) override
	{
		HudElement::OnAttributeChange( changed_attributes );
		if ( changed_attributes.find( "src" ) != changed_attributes.end() )
		{
			if ( maxBarbs_ > 0 )
			{
				Rml::String src = GetAttribute<Rml::String>( "src", "" );
				Rml::String base( va("<img class='barbs' src='%s' />", src.c_str() ) );
				Rml::String rml;

				for ( int i = 0; i < maxBarbs_; i++ )
				{
					rml += base;
				}
				SetInnerRML( rml );
			}
			else
			{
				SetInnerRML( "" );
			}
		}
	}

	void DoOnUpdate() override
	{
		int newNumBarbs = cg.snap->ps.ammo;
		int interval = BG_GetBarbRegenerationInterval( cg.snap->ps );

		if ( BG_PrimaryWeapon( cg.snap->ps.stats ) != WP_ALEVEL3_UPG )
		{
			return;
		}

		if ( newNumBarbs < maxBarbs_ )
		{
			// start regenerating barb now
			if ( newNumBarbs > numBarbs_ || ( newNumBarbs < numBarbs_ && numBarbs_ == maxBarbs_ ) )
			{
				t0_ = cg.time;
				offset_ = -M_PI_2; // sin(-pi/2) is minimal
				regenerationInterval_ = interval;
			}
			// change regeneration speed
			else if ( interval != regenerationInterval_ )
			{
				float sinOld = GetSin();
				float cosOld = GetCos();

				// avoid sudden jumps in opacity
				t0_ = cg.time;
				if ( cosOld >= 0.0f )
				{
					offset_ = asinf( sinOld );
				}
				else
				{
					offset_ = M_PI - asinf( sinOld );
				}
				regenerationInterval_ = interval;
			}
		}
		numBarbs_ = newNumBarbs;

		for ( int i = 0; i < GetNumChildren(); i++ )
		{
			Element *barb = GetChild(i);
			if (i < numBarbs_ ) // draw existing barbs
			{
				barb->SetProperty( "opacity", "1.0" );
			}
			else if (i == numBarbs_ ) // draw regenerating barb
			{
				float opacity = GetSin() / 8.0f + ( 1.0f / 8.0f ); // in [0, 0.125]
				barb->SetProperty( "opacity", va( "%f", opacity ) );
			}
			else
			{
				barb->SetProperty( "opacity", "0.0" );
			}
		}
	}

private:

	float GetSin()
	{
		return sinf( GetParam() );
	}

	float GetCos()
	{
		return cosf( GetParam() );
	}

	float GetParam()
	{
		float timeElapsed = ( cg.time - t0_ ) / 1000.0f; // in s
		float frequency = (float)LEVEL3_BOUNCEBALL_REGEN_CREEP
		                / (float)regenerationInterval_; // in Hz
		return offset_ + 2.0f * M_PI * frequency * timeElapsed;
	}

	int numBarbs_;
	int maxBarbs_;
	int regenerationInterval_;

	// t0 and offset are used to make sure that there are no sudden jumps in opacity.
	int t0_;
	float offset_;
};

class SpawnQueueElement : public TextHudElement
{
public:
	SpawnQueueElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME ),
			position_( -1 ) {}

	void DoOnUpdate() override
	{
		if ( !( cg.snap->ps.pm_flags & PMF_QUEUED ) || cg.snap->ps.pm_type == PM_DEAD )
		{
			if ( IsVisible() )
			{
				position_ = -1;
				SetProperty( Rml::PropertyId::Visibility, Rml::Property( Rml::Style::Visibility::Hidden ) );
			}
			return;
		}

		int newPosition = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] >> 8;
		if ( newPosition == position_ )
		{
			return;
		}
		position_ = newPosition;
		if ( position_ == 1 )
		{
			SetText( _( "You are at the front of the spawn queue" ) );
		}
		else
		{
			SetText( va( _( "You are at position %d in the spawn queue" ), position_ ) );
		}

		if ( !IsVisible() )
		{
			SetProperty( Rml::PropertyId::Visibility, Rml::Property( Rml::Style::Visibility::Visible ) );
		}
	}

private:
	int position_;
};

class NumSpawnsElement : public TextHudElement
{
public:
	NumSpawnsElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME ),
			spawns_( -1 ) {}

	void DoOnUpdate() override
	{
		if ( !( cg.snap->ps.pm_flags & PMF_QUEUED ) || cg.snap->ps.pm_type == PM_DEAD )
		{
			if ( IsVisible() )
			{
				spawns_ = -1;
				SetProperty( Rml::PropertyId::Visibility, Rml::Property( Rml::Style::Visibility::Hidden ) );
			}
			return;
		}

		int newSpawns = cg.snap->ps.persistant[ PERS_SPAWNQUEUE ] & 0x000000ff;
		if ( newSpawns == spawns_ )
		{
			return;
		}
		spawns_ = newSpawns;
		if ( spawns_ == 0 )
		{
			SetText( _( "There are no spawns remaining" ) );
		}
		else
		{
			SetText( va( P_( "There is %d spawn remaining", "There are %d spawns remaining", spawns_ ), spawns_ ) );
		}

		if ( !IsVisible() )
		{
			SetProperty( Rml::PropertyId::Visibility, Rml::Property( Rml::Style::Visibility::Visible ) );
		}
	}

private:
	int spawns_;
};

class PlayerCountElement : public TextHudElement
{
public:
	PlayerCountElement( const Rml::String& tag ) :
			TextHudElement( tag, ELEMENT_GAME )
			{}

	void DoOnUpdate() override
	{
		//there is probably a better way to do that, maybe reusing value
		//cached in some struct?
		std::string teamname = GetAttribute< Rml::String >( "team", "" );

		team_t team = BG_PlayableTeamFromString( teamname.c_str() );

		SetText( va( "%d", cg.teamPlayerCount[ team ] ) );
	}
};

static void CG_Rocket_DrawPlayerHealth()
{
	static int lastHealth = 0;

	if ( lastHealth != cg.snap->ps.stats[ STAT_HEALTH ] )
	{
		Rocket_SetInnerRMLRaw( va( "%d", cg.snap->ps.stats[ STAT_HEALTH ] ) );
	}
}

static void CG_Rocket_DrawPlayerHealthCross()
{
	qhandle_t shader;
	Color::Color ref_color;
	float     ref_alpha;
	rectDef_t rect;
	Color::Color color;
	int stats = cg.snap->ps.stats[ STAT_STATE ];

	// grab info from libRocket
	CG_GetRocketElementColor( ref_color );
	CG_GetRocketElementRect( &rect );

	// Pick the current icon
	shader = cgs.media.healthCross;

	if ( stats & SS_HEALING_8X )
	{
		shader = cgs.media.healthCross4X;
	}

	else if ( stats & SS_HEALING_4X )
	{
		if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_ALIENS )
		{
			shader = cgs.media.healthCross3X;
		}
		else
		{
			shader = cgs.media.healthCrossMedkit;
		}
	}

	else if ( stats & SS_HEALING_2X )
	{
		if ( CG_MyTeam() == TEAM_ALIENS )
		{
			shader = cgs.media.healthCross2X;
		}
		else
		{
			shader = cgs.media.healthCross3X;
		}
	}

	if ( stats & SS_POISONED )
	{
		shader = cgs.media.healthCrossPoisoned;
	}

	// Pick the alpha value
	color = ref_color;

	if ( CG_MyTeam() == TEAM_HUMANS &&
			cg.snap->ps.stats[ STAT_HEALTH ] < 10 )
	{
		color = Color::Red;
	}

	ref_alpha = ref_color.Alpha();

	if ( stats & SS_HEALING_2X )
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
			color.SetAlpha( ref_alpha * cg.healthCrossFade );
			trap_R_SetColor( color );
			CG_DrawPic( rect.x, rect.y, rect.w, rect.h, shader );
			color.SetAlpha( ref_alpha * ( 1.0f - cg.healthCrossFade ) );
			trap_R_SetColor( color );
			CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cg.lastHealthCross );
			trap_R_ClearColor();
			return;
		}
	}

	// Not fading, draw a single icon
	color.SetAlpha( ref_alpha );
	trap_R_SetColor( color );
	CG_DrawPic( rect.x, rect.y, rect.w, rect.h, shader );
	trap_R_ClearColor();

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

static void CG_DrawStack( rectDef_t *rect, const Color::Color& color, float fill,
						  int align, float val, int max )
{
	int      i;
	float    each, frac;
	float    nudge;
	float    fmax = max; // we don't want integer division
	bool vertical; // a stack taller than it is wide is drawn vertically

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

		trap_R_ClearColor();
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
		trap_R_ClearColor();
		return; // no partial square, we're done here
	}

	Color::Color localColor = color;
	localColor.SetAlpha( localColor.Alpha() * frac );
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

	trap_R_ClearColor();
}

static void CG_DrawPlayerAmmoStack()
{
	float         val;
	int           maxVal, align;
	static int    lastws, maxwt, lastval, valdiff;
	playerState_t *ps = &cg.snap->ps;
	weapon_t      primary = BG_PrimaryWeapon( ps->stats );
	Color::Color  localColor, foreColor;
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
		Color::Color lowAmmoColor = { 1.f, 0.f, 0.f, foreColor.Alpha() };
		localColor = Color::Blend( foreColor, lowAmmoColor, (cg.time & 128) / 255.0f );
	}

	else
	{
		localColor = foreColor;
	}

	Rocket_GetProperty( "text-align", buf, sizeof( buf ), rocketVarType_t::ROCKET_STRING );

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
	Color::Color   foreColor;

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

static void CG_Rocket_DrawMinimap()
{
	if ( cg.minimap.defined )
	{
		Color::Color foreColor;
		rectDef_t    rect;

		// grab info from libRocket
		CG_GetRocketElementColor( foreColor );
		CG_GetRocketElementRect( &rect );

		CG_DrawMinimap( &rect, foreColor );
	}
}

#define FOLLOWING_STRING "following ^7"
#define CHASING_STRING   "chasing ^7"
static void CG_Rocket_DrawFollow()
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
		Rocket_SetInnerRMLRaw( "" );
	}
}

static void CG_Rocket_DrawConnectText()
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

	if ( rocketInfo.cstate.connState < connstate_t::CA_CONNECTED && *rocketInfo.cstate.messageString )
	{
		Q_strcat( rml, sizeof( rml ), "<br />" );
		Q_strcat( rml, sizeof( rml ), rocketInfo.cstate.messageString );
	}

	switch ( rocketInfo.cstate.connState )
	{
		case connstate_t::CA_CONNECTING:
			s = va( _( "Awaiting connection…%i" ), rocketInfo.cstate.connectPacketCount );
			break;

		case connstate_t::CA_CHALLENGING:
			s = va( _( "Awaiting challenge…%i" ), rocketInfo.cstate.connectPacketCount );
			break;

		case connstate_t::CA_CONNECTED:
			s = _( "Awaiting gamestate…" );
		break;

		case connstate_t::CA_LOADING:
			return;

		case connstate_t::CA_PRIMED:
			return;

		default:
			return;
	}

	Q_strcat( rml, sizeof( rml ), s );

	Rocket_SetInnerRMLRaw( rml );
}

static void CG_Rocket_DrawClock()
{
	char    *s;
	qtime_t qt;

	if ( !cg_drawClock.Get() )
	{
		Rocket_SetInnerRMLRaw( "" );
		return;
	}

	Com_RealTime( &qt );

	if ( cg_drawClock.Get() == 2 )
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

	Rocket_SetInnerRMLRaw( s );
}

static void CG_Rocket_DrawTutorial()
{
	if ( !cg_tutorial.Get() )
	{
		Rocket_SetInnerRMLRaw( "" );
		return;
	}

	Rocket_SetInnerRML( CG_TutorialText(), RP_EMOTICONS | RP_QUAKE );
}

static void CG_Rocket_DrawChatType()
{
	static const struct {
		char colour[4]; // ^n
		char prompt[12];
	} sayText[] = {
		{ "",   "" },
		{ "^2", N_("Say:") " " },
		{ "^5", N_("Team Say:") " " },
		{ "^6", N_("Admin Say:") " " },
		{ "",   N_("Command:") " " },
	};

	if ( (size_t) cg.sayType < ARRAY_LEN( sayText ) )
	{
		const char *prompt = _( sayText[ cg.sayType ].prompt );

		if ( ui_chatPromptColors.Get() )
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
	Color::Color  foreColor, backColor, lockedColor, unlockedColor;
	float         rawFraction, fraction, glowFraction, glowOffset, borderSize;
	int           threshold;
	bool      unlocked;

	momentumThresholdIterator_t unlockableIter = { -1, 0 };

	// display
	Color::Color  color;
	float         x, y, w, h, b, glowStrength;
	bool      vertical;

	CG_GetRocketElementRect( &rect );
	CG_GetRocketElementBGColor( backColor );
	CG_GetRocketElementColor( foreColor );
	Rocket_GetProperty( "momentum-border-width", &borderSize, sizeof( borderSize ), rocketVarType_t::ROCKET_FLOAT );
	Rocket_GetProperty( "locked-marker-color", &lockedColor, sizeof(Color::Color), rocketVarType_t::ROCKET_COLOR );
	Rocket_GetProperty( "unlocked-marker-color", &unlockedColor, sizeof(Color::Color), rocketVarType_t::ROCKET_COLOR );


	team_t team = CG_MyTeam();
	float momentum = cg.predictedPlayerState.persistant[ PERS_MOMENTUM ] / 10.0f;

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
	color = backColor;
	color.SetAlpha( color.Alpha() * 0.5f );
	CG_FillRect( x, y, w, h, color );

	// draw momentum bar
	fraction = Math::Clamp( rawFraction = momentum / MOMENTUM_MAX, 0.0f, 1.0f );

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

		color = Color::Color( 1.0f, 1.0f, 1.0f, 0.5f * glowStrength );

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

	trap_R_ClearColor();
}

static void CG_Rocket_DrawMineRate()
{
	int totalBudget  = cg.snap->ps.persistant[ PERS_TOTALBUDGET ];
	int queuedBudget = cg.snap->ps.persistant[ PERS_QUEUEDBUDGET ];

	if (queuedBudget != 0) {
		float matchTime = (float)(cg.time - cgs.levelStartTime);
		float rate = cgs.buildPointRecoveryInitialRate /
		             std::pow(2.0f, matchTime / (60000.0f * cgs.buildPointRecoveryRateHalfLife));
		Rocket_SetInnerRMLRaw( va( "Recovering %d / %d BP @ %.1f BP/min.",
		                       queuedBudget, totalBudget, rate) );
	} else {
		Rocket_SetInnerRMLRaw( va( "The full budget of %d BP is available.",
		                       totalBudget) );
	}
}

static qhandle_t CG_GetUnlockableIcon( int num )
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
	Color::Color  foreColour, backColour;
	momentumThresholdIterator_t unlockableIter = { -1, 1 }, previousIter;

	team_t team = CG_MyTeam();

	// display
	float x, y, w, h, iw, ih, borderSize;
	bool  vertical;

	int   icons, counts;
	int   count[ 32 ] = { 0 };
	struct
	{
		qhandle_t shader;
		bool  unlocked;
	} icon[ NUM_UNLOCKABLES ]; // more than enough(!)

	CG_GetRocketElementRect( &rect );
	Rocket_GetProperty( "cell-color", &backColour, sizeof(Color::Color), rocketVarType_t::ROCKET_COLOR );
	CG_GetRocketElementColor( foreColour );
	Rocket_GetProperty( "momentum-border-width", &borderSize, sizeof( borderSize ), rocketVarType_t::ROCKET_FLOAT );

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
		Color::Color unlockedBg, lockedBg;

		unlockedBg = foreColour;
		unlockedBg.SetAlpha( 0.0f );  // No background
		lockedBg = backColour;
		lockedBg.SetAlpha( 0.0f );  // No background

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

	trap_R_ClearColor();
}

static void CG_Rocket_DrawVote_internal( team_t team )
{
	int    sec;

	if ( !cgs.voteTime[ team ] )
	{
		Rocket_SetInnerRMLRaw( "" );
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified[ team ] )
	{
		cgs.voteModified[ team ] = false;
		trap_S_StartLocalSound( cgs.media.talkSound, soundChannel_t::CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime[ team ] ) ) / 1000;

	if ( sec < 0 )
	{
		sec = 0;
	}

	int bindTeam = CG_CurrentBindTeam();
	std::string yeskey = CG_EscapeHTMLText( CG_KeyBinding( va( "%svote yes", team == TEAM_NONE ? "" : "team" ), bindTeam ) );
	std::string nokey = CG_EscapeHTMLText( CG_KeyBinding( va( "%svote no", team == TEAM_NONE ? "" : "team" ), bindTeam ) );
	std::string voteString = CG_EscapeHTMLText( cgs.voteString[ team ] );
	int timerColor = ( sec > 10 ? 7 : 1 );
	int timerColor2 = ( sec > 10 ? 3 : 7 );

	std::string s = Str::Format( "^3%sVOTE: ^7%s\n"
			"^7Called by: %s\n"
			"^7[^2%s^7] [check]: %i [^1%s^7] [cross]: %i\n"
			"^%iEnds in ^%i%i ^%iseconds\n",
			team == TEAM_NONE ? "" : "TEAM", voteString,
			cgs.voteCaller[ team ], yeskey, cgs.voteYes[ team ], nokey, cgs.voteNo[ team ],
			timerColor, timerColor2, sec, timerColor );

	Rocket_SetInnerRML( s.c_str(), RP_EMOTICONS );
}

static void CG_Rocket_DrawVersion()
{
	Rocket_SetInnerRML( PRODUCT_VERSION, 0 );
}

static void CG_Rocket_DrawVote()
{
	CG_Rocket_DrawVote_internal( TEAM_NONE );
}

static void CG_Rocket_DrawTeamVote()
{
	CG_Rocket_DrawVote_internal( CG_MyTeam() );
}

static void CG_Rocket_DrawWarmup()
{
	int   sec = 0;

	if ( !cg.warmupTime )
	{
		Rocket_SetInnerRMLRaw( "" );
		return;
	}

	sec = ( cg.warmupTime - cg.time ) / 1000;

	if ( sec < 0 )
	{
		Rocket_SetInnerRMLRaw( "" );
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
		Rocket_SetInnerRMLRaw( va( "%d%%", (int) ( value * 100 ) ) );
	}
}

static void CG_Rocket_DrawLoadingText()
{
	Rocket_SetInnerRML( cg.loadingText.c_str(), 0 );
}

static void CG_Rocket_DrawLevelAuthors()
{
	Rocket_SetInnerRML( cg.mapAuthors.c_str(), RP_QUAKE | RP_EMOTICONS );
}

static void CG_Rocket_DrawLevelName()
{
	Rocket_SetInnerRML( cg.mapLongName.c_str(), RP_QUAKE | RP_EMOTICONS );
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
	Rocket_SetInnerRML( Info_ValueForKey( info, "sv_hostname" ), RP_QUAKE | RP_EMOTICONS );
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
		Rocket_SetInnerRMLRaw( "" );
		return;
	}

	float downloadTimeDelta = ( rocketInfo.realtime - downloadTime ) / 1000;

	if ( downloadTimeDelta != 0.0F )
	{
		xferRate = downloadCount / downloadTimeDelta;
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
		Rocket_SetInnerRMLRaw( "" );
		return;
	}

	CG_ReadableSize( totalSizeBuf,  sizeof totalSizeBuf,  downloadSize );

	Rocket_SetInnerRML( totalSizeBuf, RP_QUAKE );
}

static void CG_Rocket_DrawDownloadSpeed()
{
	char xferRateBuf[ MAX_STRING_CHARS ];
	float downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	float downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );
	int xferRate;

	if ( !*rocketInfo.downloadName )
	{
		Rocket_SetInnerRMLRaw ( "" );
		return;
	}

	float downloadTimeDelta = ( rocketInfo.realtime - downloadTime ) / 1000;

	if ( downloadTimeDelta != 0.0F )
	{
		xferRate = downloadCount / downloadTimeDelta;
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

struct elementRenderCmd_t
{
	const char *name;
	void ( *update )(); // Modifies the RML or element properties
	void ( *render )(); // Does rendering outside of the HTML engine
	rocketElementType_t type;
};

// This kind of element is a hack left over from when VMs could only use C code
// and the HTML engine ran outside the VMs. Do not add more of them! Instead,
// define a class extending Element (example: CrosshairNamesElement).
// THESE MUST BE ALPHABETIZED
static const elementRenderCmd_t elementRenderCmdList[] =
{
	{ "ammo_stack", nullptr, &CG_DrawPlayerAmmoStack, ELEMENT_HUMANS },
	{ "chattype", &CG_Rocket_DrawChatType, nullptr, ELEMENT_ALL },
	{ "clip_stack", nullptr, &CG_DrawPlayerClipsStack, ELEMENT_HUMANS },
	{ "clock", &CG_Rocket_DrawClock, nullptr, ELEMENT_ALL },
	{ "connecting", &CG_Rocket_DrawConnectText, nullptr, ELEMENT_ALL },
	{ "downloadName", &CG_Rocket_DrawDownloadName, nullptr, ELEMENT_ALL },
	{ "downloadSpeed", &CG_Rocket_DrawDownloadSpeed, nullptr, ELEMENT_ALL },
	{ "downloadTime", &CG_Rocket_DrawDownloadTime, nullptr, ELEMENT_ALL },
	{ "downloadTotalSize", &CG_Rocket_DrawDownloadTotalSize, nullptr, ELEMENT_ALL },
	{ "follow", &CG_Rocket_DrawFollow, nullptr, ELEMENT_GAME },
	{ "health", &CG_Rocket_DrawPlayerHealth, nullptr, ELEMENT_BOTH },
	{ "health_cross", nullptr, &CG_Rocket_DrawPlayerHealthCross, ELEMENT_BOTH },
	{ "hostname", &CG_Rocket_DrawHostname, nullptr, ELEMENT_ALL },
	{ "inventory", &CG_DrawHumanInventory, nullptr, ELEMENT_HUMANS },
	{ "jetpack", &CG_Rocket_HaveJetpck, nullptr, ELEMENT_HUMANS },
	{ "levelauthors", &CG_Rocket_DrawLevelAuthors, nullptr, ELEMENT_ALL },
	{ "levelname", &CG_Rocket_DrawLevelName, nullptr, ELEMENT_ALL },
	{ "loadingText", &CG_Rocket_DrawLoadingText, nullptr, ELEMENT_ALL },
	{ "mine_rate", &CG_Rocket_DrawMineRate, nullptr, ELEMENT_BOTH },
	{ "minimap", nullptr, &CG_Rocket_DrawMinimap, ELEMENT_ALL },
	{ "momentum_bar", nullptr, &CG_Rocket_DrawPlayerMomentumBar, ELEMENT_BOTH },
	{ "motd", &CG_Rocket_DrawMOTD, nullptr, ELEMENT_ALL },
	{ "progress_value", &CG_Rocket_DrawProgressValue, nullptr, ELEMENT_ALL },
	{ "tutorial", &CG_Rocket_DrawTutorial, nullptr, ELEMENT_GAME },
	{ "unlocked_items", nullptr, &CG_Rocket_DrawPlayerUnlockedItems, ELEMENT_BOTH },
	{ "version", &CG_Rocket_DrawVersion, nullptr, ELEMENT_ALL },
	{ "votes", &CG_Rocket_DrawVote, nullptr, ELEMENT_GAME },
	{ "votes_team", &CG_Rocket_DrawTeamVote, nullptr, ELEMENT_BOTH },
	{ "warmup_time", &CG_Rocket_DrawWarmup, nullptr, ELEMENT_GAME },
};

static const size_t elementRenderCmdListCount = ARRAY_LEN( elementRenderCmdList );

static int elementRenderCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( elementRenderCmd_t * ) b )->name );
}

void CG_Rocket_UpdateElement( const char *tag )
{
	auto *cmd = ( elementRenderCmd_t * ) bsearch( tag, elementRenderCmdList, elementRenderCmdListCount, sizeof( elementRenderCmd_t ), elementRenderCmdCmp );

	if ( cmd && cmd->update && CG_Rocket_IsCommandAllowed( cmd->type ) )
	{
		cmd->update();
	}
}

void CG_Rocket_RenderElement( const char *tag )
{
	auto *cmd = ( elementRenderCmd_t * ) bsearch( tag, elementRenderCmdList, elementRenderCmdListCount, sizeof( elementRenderCmd_t ), elementRenderCmdCmp );

	if ( cmd && cmd->render && CG_Rocket_IsCommandAllowed( cmd->type ) )
	{
		cmd->render();
	}
}

void CG_Rocket_RegisterElements()
{
	Rml::XMLParser::RegisterNodeHandler("translate", Rml::MakeShared<TranslateNodeHandler>());

	for ( unsigned i = 0; i < elementRenderCmdListCount; i++ )
	{
		//Check that the commands are in increasing order so that it can be used by bsearch
		if ( i != 0 && Q_stricmp( elementRenderCmdList[ i - 1 ].name, elementRenderCmdList[ i ].name ) > 0 )
		{
			Log::Warn( "CGame elementRenderCmdList is in the wrong order for %s and %s", elementRenderCmdList[i - 1].name, elementRenderCmdList[ i ].name );
		}

		Rocket_RegisterElement( elementRenderCmdList[ i ].name );
	}

	// Standard HTML
	RegisterElement<LiElement>( "li" );

	// Game-specific RML
	RegisterElement<AmmoHudElement>( "ammo" );
	RegisterElement<ClipsHudElement>( "clips" );
	RegisterElement<FpsHudElement>( "fps" );
	RegisterElement<CrosshairIndicatorHudElement>( "crosshair_indicator" );
	RegisterElement<CrosshairHudElement>( "crosshair" );
	RegisterElement<SpeedGraphElement>( "speedometer" );
	RegisterElement<PositionElement>( "position_indicator" );
	RegisterElement<CreditsValueElement>( "credits" );
	RegisterElement<EvosValueElement>( "evos" );
	RegisterElement<WeaponIconElement>( "weapon_icon" );
	RegisterElement<WallwalkElement>( "wallwalk" );
	RegisterElement<StaminaElement>( "stamina" );
	RegisterElement<UsableBuildableElement>( "usable_buildable" );
	RegisterElement<LocationElement>( "location" );
	RegisterElement<TimerElement>( "timer" );
	RegisterElement<LagometerElement>( "lagometer" );
	RegisterElement<PingElement>( "ping" );
	RegisterElement<CrosshairNamesElement>( "crosshair_name" );
	RegisterElement<LevelshotElement>( "levelshot" );
	RegisterElement<LevelshotLoadingElement>( "levelshot_loading" );
	RegisterElement<CenterPrintElement>( "center_print" );
	RegisterElement<BeaconAgeElement>( "beacon_age" );
	RegisterElement<BeaconDistanceElement>( "beacon_distance" );
	RegisterElement<BeaconIconElement>( "beacon_icon" );
	RegisterElement<BeaconInfoElement>( "beacon_info" );
	RegisterElement<BeaconNameElement>( "beacon_name" );
	RegisterElement<BeaconOwnerElement>( "beacon_owner" );
	RegisterElement<PredictedMineEfficiencyElement>( "predictedMineEfficiency" );
	RegisterElement<BarbsHudElement>( "barbs" );
	RegisterElement<WebElement>( "web" );
	RegisterElement<TranslateElement>( "translate" );
	RegisterElement<SpawnQueueElement>( "spawnPos" );
	RegisterElement<NumSpawnsElement>( "numSpawns" );
	RegisterElement<PlayerCountElement>( "playerCount" );
}
