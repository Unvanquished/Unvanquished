/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "precompiled.h"
#include <Rocket/Core/FontFaceHandle.h>
#include <algorithm>
#include <Rocket/Core.h>
#include "FontFaceLayer.h"
#include "TextureLayout.h"

namespace Rocket {
namespace Core {

class FontEffectSort
{
public:
	bool operator()(const FontEffect* lhs, const FontEffect* rhs)
	{
		return lhs->GetZIndex() < rhs->GetZIndex();
	}
};

FontFaceHandle::FontFaceHandle()
{
	size = 0;
	average_advance = 0;
	x_height = 0;
	line_height = 0;
	baseline = 0;

	underline_position = 0;
	underline_thickness = 0;

	ft_face = NULL;
	backup_face = NULL;

	base_layer = NULL;

	memset(&fonts_generated, 0, sizeof(fonts_generated));
}

FontFaceHandle::~FontFaceHandle()
{
	for (FontGlyphMap::iterator i = glyphs.begin(); i != glyphs.end(); ++i)
		delete[] i->second.bitmap_data;

	for (FontLayerMap::iterator i = layers.begin(); i != layers.end(); ++i)
		delete i->second;
}

// Initialises the handle so it is able to render text.
bool FontFaceHandle::Initialise(FT_Face ft_face, const String& _charset, int _size)
{
	size = _size;

	raw_charset = _charset;
	if (!UnicodeRange::BuildList(charset, raw_charset))
	{
		Log::Message(Log::LT_ERROR, "Invalid font charset '%s'.", raw_charset.CString());
		return false;
	}

	// Set the character size on the font face.
	FT_Error error = FT_Set_Char_Size(ft_face, 0, size << 6, 0, 0);
	if (error != 0)
	{
		Log::Message(Log::LT_ERROR, "Unable to set the character size '%d' on the font face '%s %s'.", size, ft_face->family_name, ft_face->style_name);
		return false;
	}

	this->ft_face = ft_face;
	this->backup_face = (FT_Face)FontDatabase::GetBackupFace();

	if (this->backup_face)
	{
		FT_Error error = FT_Set_Char_Size(this->backup_face, 0, size << 6, 0, 0);
		if (error != 0)
		{
			Log::Message(Log::LT_ERROR, "Unable to set the character size '%d' on the font face '%s %s'.", size, this->backup_face->family_name, this->backup_face->style_name);
			return false;
		}
	}

	// Construct the list of the characters specified by the charset.
	for (size_t i = 0; i < charset.size(); ++i)
		BuildGlyphMap(charset[i]);

	// Generate the metrics for the handle.
	GenerateMetrics();

	// Generate the default layer and layer configuration.
	base_layer = GenerateLayer(NULL);
	layer_configurations.push_back(LayerConfiguration());
	layer_configurations.back().push_back(base_layer);


	return true;
}

// Returns the point size of this font face.
int FontFaceHandle::GetSize() const
{
	return size;
}

// Returns the average advance of all glyphs in this font face.
int FontFaceHandle::GetCharacterWidth() const
{
	return average_advance;
}

// Returns the pixel height of a lower-case x in this font face.
int FontFaceHandle::GetXHeight() const
{
	return x_height;
}

// Returns the default height between this font face's baselines.
int FontFaceHandle::GetLineHeight() const
{
	return line_height;
}

// Returns the font's baseline.
int FontFaceHandle::GetBaseline() const
{
	return baseline;
}

// Returns the font's glyphs.
const FontGlyphMap& FontFaceHandle::GetGlyphs() const
{
	return glyphs;
}

// Returns the width a string will take up if rendered with this handle.
int FontFaceHandle::GetStringWidth(const WString& string, word prior_character)
{
	int width = 0;
	bool update = false;

	for (size_t i = 0; i < string.Length(); i++)
	{
		word character_code = string[i];

		FontGlyphMap::const_iterator iterator = glyphs.find(character_code);
		if (iterator == glyphs.end())
		{
			word chunk = Math::RoundDown(character_code / 256);

			if (!(fonts_generated[ chunk / 8 ] & (1 << (chunk % 8))))
			{
				UnicodeRange range(chunk * 256, (chunk * 256) + 255);
				fonts_generated[ chunk / 8 ] |= (1 << (chunk % 8));
				if (BuildGlyphMap(range))
				{
					update = true;
				}
			}

			iterator = glyphs.find(character_code);
			if (iterator == glyphs.end())
				continue;
		}

		// Adjust the cursor for the kerning between this character and the previous one.
		if (prior_character != 0)
			width += GetKerning(prior_character, string[i]);
		// Adjust the cursor for this character's advance.
		width += iterator->second.advance;

		prior_character = character_code;
	}

	if ( update )
	{
		GenerateMetrics();
		int num = 0;
		for (size_t j = 0; j < layer_configurations.size(); ++j)
		{
			for (size_t k = 0; k < layer_configurations[j].size(); ++k)
			{
				layer_configurations[j][k]->AddNewGlyphs();
				num++;
			}
		}
	}


	return width;
}

// Generates, if required, the layer configuration for a given array of font effects.
int FontFaceHandle::GenerateLayerConfiguration(FontEffectMap& font_effects)
{
	if (font_effects.empty())
		return 0;

	// Prepare a list of effects, sorted by z-index.
	FontEffectList sorted_effects;
	for (FontEffectMap::const_iterator i = font_effects.begin(); i != font_effects.end(); ++i)
		sorted_effects.push_back(i->second);

	std::sort(sorted_effects.begin(), sorted_effects.end(), FontEffectSort());

	// Check each existing configuration for a match with this arrangement of effects.
	int configuration_index = 1;
	for (; configuration_index < (int) layer_configurations.size(); ++configuration_index)
	{
		const LayerConfiguration& configuration = layer_configurations[configuration_index];

		// Check the size is correct. For a math, there should be one layer in the configuration
		// plus an extra for the base layer.
		if (configuration.size() != sorted_effects.size() + 1)
			continue;

		// Check through each layer, checking it was created by the same effect as the one we're
		// checking.
		size_t effect_index = 0;
		for (size_t i = 0; i < configuration.size(); ++i)
		{
			// Skip the base layer ...
			if (configuration[i]->GetFontEffect() == NULL)
				continue;

			// If the ith layer's effect doesn't match the equivalent effect, then this
			// configuration can't match.
			if (configuration[i]->GetFontEffect() != sorted_effects[effect_index])
				break;

			// Check the next one ...
			++effect_index;
		}

		if (effect_index == sorted_effects.size())
			return configuration_index;
	}

	// No match, so we have to generate a new layer configuration.
	layer_configurations.push_back(LayerConfiguration());
	LayerConfiguration& layer_configuration = layer_configurations.back();

	bool added_base_layer = false;

	for (size_t i = 0; i < sorted_effects.size(); ++i)
	{
		if (!added_base_layer &&
			sorted_effects[i]->GetZIndex() >= 0)
		{
			layer_configuration.push_back(base_layer);
			added_base_layer = true;
		}

		layer_configuration.push_back(GenerateLayer(sorted_effects[i]));
	}

	// Add the base layer now if we still haven't added it.
	if (!added_base_layer)
		layer_configuration.push_back(base_layer);

	return (int) (layer_configurations.size() - 1);
}

// Generates the texture data for a layer (for the texture database).
bool FontFaceHandle::GenerateLayerTexture( const Rocket::Core::byte *&texture_data, Rocket::Core::Vector2i &texture_dimensions, Rocket::Core::FontEffect *layer_id, int layout_id, int texture_id )
{
	FontLayerMap::iterator layer_iterator = layers.find(layer_id);
	if (layer_iterator == layers.end())
		return false;

	return layer_iterator->second->GenerateTexture(texture_data, texture_dimensions, layout_id, texture_id );
}

// Generates the geometry required to render a single line of text.
int FontFaceHandle::GenerateString(GeometryList& geometry, const WString& string, const Vector2f& position, const Colourb& colour, int layer_configuration_index) const
{
	int geometry_index = 0;
	int line_width = 0;

	ROCKET_ASSERT(layer_configuration_index >= 0);
	ROCKET_ASSERT(layer_configuration_index < (int) layer_configurations.size());

	// Fetch the requested configuration and generate the geometry for each one.
	const LayerConfiguration& layer_configuration = layer_configurations[layer_configuration_index];
	for (size_t i = 0; i < layer_configuration.size(); ++i)
	{
		FontFaceLayer* layer = layer_configuration[i];

		Colourb layer_colour;
		if (layer == base_layer)
			layer_colour = colour;
		else
		{
			layer_colour = layer->GetColour();
			layer_colour.alpha *= colour.alpha / 255.0f;
		}

		// Resize the geometry list if required.
		if ((int) geometry.size() < geometry_index + layer->GetNumTextures())
			geometry.resize(geometry_index + layer->GetNumTextures());

		// Bind the textures to the geometries.
		for (int i = 0; i < layer->GetNumTextures(); ++i)
			geometry[geometry_index + i].SetTexture(layer->GetTexture(i));

		line_width = 0;
		word prior_character = 0;

		const word* string_iterator = string.CString();
		const word* string_end = string.CString() + string.Length();

		for (; string_iterator != string_end; string_iterator++)
		{
			FontGlyphMap::const_iterator iterator = glyphs.find(*string_iterator);
			if (iterator == glyphs.end())
				continue;

			// Adjust the cursor for the kerning between this character and the previous one.
			if (prior_character != 0)
				line_width += GetKerning(prior_character, *string_iterator);

			layer->GenerateGeometry(&geometry[geometry_index], *string_iterator, Vector2f(position.x + line_width, position.y), layer_colour);

			line_width += iterator->second.advance;
			prior_character = *string_iterator;
		}

		geometry_index += layer->GetNumTextures();
	}

	// Cull any excess geometry from a previous generation.
	geometry.resize(geometry_index);

	return line_width;
}

// Generates the geometry required to render a line above, below or through a line of text.
void FontFaceHandle::GenerateLine(Geometry* geometry, const Vector2f& position, int width, Font::Line height, const Colourb& colour) const
{
	std::vector< Vertex >& line_vertices = geometry->GetVertices();
	std::vector< int >& line_indices = geometry->GetIndices();

	float offset;
	switch (height)
	{
		case Font::UNDERLINE:			offset = -underline_position; break;
		case Font::OVERLINE:			// where to place? offset = -line_height - underline_position; break;
		case Font::STRIKE_THROUGH:		// where to place? offset = -line_height * 0.5f; break;
		default:						return;
	}

	line_vertices.resize(line_vertices.size() + 4);
	line_indices.resize(line_indices.size() + 6);
	GeometryUtilities::GenerateQuad(&line_vertices[0] + (line_vertices.size() - 4), &line_indices[0] + (line_indices.size() - 6), Vector2f(position.x, position.y + offset), Vector2f((float) width, underline_thickness), colour, (int)line_vertices.size() - 4);
}

// Returns the font face's raw charset (the charset range as a string).
const String& FontFaceHandle::GetRawCharset() const
{
	return raw_charset;
}

// Returns the font face's charset.
const UnicodeRangeList& FontFaceHandle::GetCharset() const
{
	return charset;
}

// Destroys the handle.
void FontFaceHandle::OnReferenceDeactivate()
{
	delete this;
}

void FontFaceHandle::GenerateMetrics()
{
	line_height = ft_face->size->metrics.height >> 6;
	baseline = line_height - (ft_face->size->metrics.ascender >> 6);

	underline_position = FT_MulFix(ft_face->underline_position, ft_face->size->metrics.y_scale) / float(1 << 6);
	underline_thickness = FT_MulFix(ft_face->underline_thickness, ft_face->size->metrics.y_scale) / float(1 << 6);
	underline_thickness = Math::Max(underline_thickness, 1.0f);

	average_advance = 0;
	for (FontGlyphMap::iterator i = glyphs.begin(); i != glyphs.end(); ++i)
		average_advance += i->second.advance;

	// Bring the total advance down to the average advance, but scaled up 10%, just to be on the safe side.
	average_advance = Math::RealToInteger((float) average_advance / (glyphs.size() * 0.9f));

	// Determine the x-height of this font face.
	word x = (word) 'x';
	int index = FT_Get_Char_Index(ft_face, x);
	if (FT_Load_Glyph(ft_face, index, 0) == 0)
		x_height = ft_face->glyph->metrics.height >> 6;
	else
		x_height = 0;
}

bool FontFaceHandle::BuildGlyphMap(const UnicodeRange& unicode_range)
{
	bool success = false;
	for (size_t character_code = (Math::Max< unsigned int >(unicode_range.min_codepoint, 32)); character_code <= unicode_range.max_codepoint; ++character_code)
	{
		if (glyphs.find(character_code) != glyphs.end())
			continue;

		int index = FT_Get_Char_Index(ft_face, character_code);
		if (index != 0)
		{
			FT_Error error = FT_Load_Glyph(ft_face, index, 0);
			if (error != 0)
			{
				Log::Message(Log::LT_WARNING, "Unable to load glyph for character '%u' on the font face '%s %s'; error code: %d.", character_code, ft_face->family_name, ft_face->style_name, error);
				continue;
			}

			error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
			if (error != 0)
			{
				Log::Message(Log::LT_WARNING, "Unable to render glyph for character '%u' on the font face '%s %s'; error code: %d.", character_code, ft_face->family_name, ft_face->style_name, error);
				continue;
			}

			if (!success)
				success = true;

			FontGlyph glyph;
			glyph.character = character_code;
			BuildGlyph(glyph, ft_face->glyph);
			glyphs[character_code] = glyph;
		}
		else if ( backup_face && ( index = FT_Get_Char_Index(backup_face, character_code) ) )
		{
			FT_Error error = FT_Load_Glyph(backup_face, index, 0);
			if (error != 0)
			{
				Log::Message(Log::LT_WARNING, "Unable to load glyph for character '%u' on the font face '%s %s'; error code: %d.", character_code, backup_face->family_name, backup_face->style_name, error);
				continue;
			}

			error = FT_Render_Glyph(backup_face->glyph, FT_RENDER_MODE_NORMAL);
			if (error != 0)
			{
				Log::Message(Log::LT_WARNING, "Unable to render glyph for character '%u' on the font face '%s %s'; error code: %d.", character_code, backup_face->family_name, backup_face->style_name, error);
				continue;
			}

			if (!success)
				success = true;

			FontGlyph glyph;
			glyph.character = character_code;
			BuildGlyph(glyph, backup_face->glyph);
			glyphs[character_code] = glyph;
		}
	}

	return success;
}

void FontFaceHandle::BuildGlyph(FontGlyph& glyph, FT_GlyphSlot ft_glyph)
{
	// Set the glyph's dimensions.
	glyph.dimensions.x = ft_glyph->metrics.width >> 6;
	glyph.dimensions.y = ft_glyph->metrics.height >> 6;

	// Set the glyph's bearing.
	glyph.bearing.x = ft_glyph->metrics.horiBearingX >> 6;
	glyph.bearing.y = ft_glyph->metrics.horiBearingY >> 6;

	// Set the glyph's advance.
	glyph.advance = ft_glyph->metrics.horiAdvance >> 6;

	// Set the glyph's bitmap dimensions.
	glyph.bitmap_dimensions.x = ft_glyph->bitmap.width;
	glyph.bitmap_dimensions.y = ft_glyph->bitmap.rows;

	// Copy the glyph's bitmap data from the FreeType glyph handle to our glyph handle.
	if (glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y != 0)
	{
		// Check the pixel mode is supported.
		if (ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_MONO &&
			ft_glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
		{
			glyph.bitmap_data = NULL;
			Log::Message(Log::LT_WARNING, "Unable to render glyph on the font face '%s %s'; unsupported pixel mode (%d).", ft_glyph->face->family_name, ft_glyph->face->style_name, ft_glyph->bitmap.pixel_mode);
		}
		else
		{
			glyph.bitmap_data = new byte[glyph.bitmap_dimensions.x * glyph.bitmap_dimensions.y];

			byte* source_bitmap = ft_glyph->bitmap.buffer;
			byte* destination_bitmap = glyph.bitmap_data;

			// Copy the bitmap data into the newly-allocated space on our glyph.
			switch (ft_glyph->bitmap.pixel_mode)
			{
				// Unpack 1-bit data into 8-bit.
				case FT_PIXEL_MODE_MONO:
				{
					for (int i = 0; i < glyph.bitmap_dimensions.y; ++i)
					{
						int mask = 0x80;
						byte* source_byte = source_bitmap;
						for (int j = 0; j < glyph.bitmap_dimensions.x; ++j)
						{
							if ((*source_byte & mask) == mask)
								destination_bitmap[j] = 255;
							else
								destination_bitmap[j] = 0;

							mask >>= 1;
							if (mask <= 0)
							{
								mask = 0x80;
								++source_byte;
							}
						}

						destination_bitmap += glyph.bitmap_dimensions.x;
						source_bitmap += ft_glyph->bitmap.pitch;
					}
				}
				break;

				// Directly copy 8-bit data.
				case FT_PIXEL_MODE_GRAY:
				{
					for (int i = 0; i < glyph.bitmap_dimensions.y; ++i)
					{
						memcpy(destination_bitmap, source_bitmap, glyph.bitmap_dimensions.x);
						destination_bitmap += glyph.bitmap_dimensions.x;
						source_bitmap += ft_glyph->bitmap.pitch;
					}
				}
				break;
			}
		}
	}
	else
		glyph.bitmap_data = NULL;
}

int FontFaceHandle::GetKerning(word lhs, word rhs) const
{
	if (!FT_HAS_KERNING(ft_face))
		return 0;

	FT_Vector ft_kerning;

	FT_Error ft_error = FT_Get_Kerning(ft_face,
		FT_Get_Char_Index(ft_face, lhs), FT_Get_Char_Index(ft_face, rhs),
		FT_KERNING_DEFAULT, &ft_kerning);

	if (ft_error != 0)
		return 0;

	int kerning = ft_kerning.x >> 6;
	return kerning;
}

// Generates (or shares) a layer derived from a font effect.
FontFaceLayer* FontFaceHandle::GenerateLayer(FontEffect* font_effect)
{
	// See if this effect has been instanced before, as part of a different configuration.
	FontLayerMap::iterator i = layers.find(font_effect);
	if (i != layers.end())
		return i->second;

	FontFaceLayer* layer = new FontFaceLayer();
	layers[font_effect] = layer;

	if (font_effect == NULL)
	{
		layer->Initialise(this);
	}
	else
	{
		// Determine which, if any, layer the new layer should copy its geometry and textures from.
		FontFaceLayer* clone = NULL;
		bool deep_clone = true;
		String generation_key;

		if (!font_effect->HasUniqueTexture())
		{
			clone = base_layer;
			deep_clone = false;
		}
		else
		{
			generation_key = font_effect->GetName() + ";" + font_effect->GetGenerationKey();
			FontLayerCache::iterator cache_iterator = layer_cache.find(generation_key);
			if (cache_iterator != layer_cache.end())
				clone = cache_iterator->second;
		}

		// Create a new layer.
		layer->Initialise(this, font_effect, clone, deep_clone);

		// Cache the layer in the layer cache if it generated its own textures (ie, didn't clone).
		if (clone == NULL)
			layer_cache[generation_key] = layer;
	}

	return layer;
}

}
}
