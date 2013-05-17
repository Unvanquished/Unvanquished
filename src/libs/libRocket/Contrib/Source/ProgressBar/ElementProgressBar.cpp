#include <Rocket/ProgressBar/ElementProgressBar.h>
#include <Rocket/Core/GeometryUtilities.h>
#include <Rocket/Core/ElementDocument.h>
#include <Rocket/Core/ContainerWrapper.h>

namespace Rocket {
namespace ProgressBar {

// Constructs a new ElementProgressBar. This should not be called directly; use the Factory instead.
ElementProgressBar::ElementProgressBar(const Rocket::Core::String& tag) : Core::Element(tag), start_geometry(this), center_geometry(this), end_geometry(this), geometry_dirty(true), is_mask(false), value(0.0f), progressbar_orientation(ProgressBarOrientationLeft)
{
}

ElementProgressBar::~ElementProgressBar()
{
}

// Returns float value between 0 and 1 of the progress bar.
float ElementProgressBar::GetValue() const
{
	return GetAttribute< float >("value", 0.0f);
}

// Sets the current value of the progress bar.
void ElementProgressBar::SetValue(const float value)
{
	SetAttribute("value", value);
}

// Called during the update loop after children are rendered.
void ElementProgressBar::OnUpdate()
{
}

// Called during render after backgrounds, borders, decorators, but before children, are rendered.
void ElementProgressBar::OnRender()
{
	if(geometry_dirty)
		GenerateGeometry();

	start_geometry.Render(GetAbsoluteOffset(Rocket::Core::Box::CONTENT));
	center_geometry.Render(GetAbsoluteOffset(Rocket::Core::Box::CONTENT));
	end_geometry.Render(GetAbsoluteOffset(Rocket::Core::Box::CONTENT));
}

// Called when attributes on the element are changed.
void ElementProgressBar::OnAttributeChange(const Core::AttributeNameList& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	if (changed_attributes.find("value") != changed_attributes.end())
	{
		value = GetAttribute< float >("value", 0.0f);

		if (value < 0)
			value = 0.0f;
		else if (value > 1)
			value = 1.0f;

		geometry_dirty = true;
	}
}

// Called when properties on the element are changed.
void ElementProgressBar::OnPropertyChange(const Core::PropertyNameList& changed_properties)
{
	Element::OnPropertyChange(changed_properties);

	if (changed_properties.find("progress-start-image") != changed_properties.end())
	{
		LoadTexture(0);
	}
	
	if (changed_properties.find("progress-center-image") != changed_properties.end())
	{
		LoadTexture(1);
	}
	
	if (changed_properties.find("progress-end-image") != changed_properties.end())
	{
		LoadTexture(2);
	}

	if (changed_properties.find("orientation") != changed_properties.end())
	{
		Core::String orientation_string = GetProperty< Core::String >("orientation");

		if (orientation_string == "left")
		{
			progressbar_orientation = ProgressBarOrientationLeft;
		}
		else if (orientation_string =="right")
		{
			progressbar_orientation = ProgressBarOrientationRight;
		}
		else if (orientation_string == "top")
		{
			progressbar_orientation = ProgressBarOrientationTop;
		}
		else if (orientation_string == "bottom")
		{
			progressbar_orientation = ProgressBarOrientationBottom;
		}

		geometry_dirty = true;
	}

	if (changed_properties.find("mask") != changed_properties.end())
	{
		is_mask = GetProperty< bool >("mask");

		geometry_dirty = true;
	}
}

// Called when value has changed.
void ElementProgressBar::GenerateGeometry()
{
	const Core::Vector2f complete_extent = GetBox().GetSize(Core::Box::CONTENT);
	Core::Vector2f final_part_position[3];
	Core::Vector2f final_part_size[3];
	float progress_size;
	Core::Vector2f final_center_texcoords[2];

	start_geometry.Release(true);
	center_geometry.Release(true);
	end_geometry.Release(true);

	final_center_texcoords[0] = texcoords[1][0];
	final_center_texcoords[1] = texcoords[1][1];

	switch(progressbar_orientation)
	{
		case ProgressBarOrientationLeft :
		{
			progress_size = value * (complete_extent.x - initial_part_size[0].x - initial_part_size[2].x);

			final_part_size[0].y = final_part_size[1].y = final_part_size[2].y = float(complete_extent.y);
			final_part_size[0].x = float(initial_part_size[0].x);
			final_part_size[2].x = float(initial_part_size[2].x);
			final_part_size[1].x = progress_size;

			final_part_position[0] = Core::Vector2f(0, 0);
			final_part_position[1] = Core::Vector2f(final_part_size[0].x, 0);
			final_part_position[2] = Core::Vector2f(final_part_size[0].x + final_part_size[1].x, 0);

			if (is_mask)
			{
				final_center_texcoords[0].x = texcoords[1][0].x;
				final_center_texcoords[0].y = texcoords[1][0].y;
				final_center_texcoords[1].x = texcoords[1][0].x + value * (texcoords[1][1].x - texcoords[1][0].x);
				final_center_texcoords[1].y = texcoords[1][1].y;
			}
		} break;

		case ProgressBarOrientationRight :
		{
			float
				offset;

			progress_size = value * (complete_extent.x - initial_part_size[0].x - initial_part_size[2].x);
			offset = (complete_extent.x - initial_part_size[0].x - initial_part_size[2].x) - progress_size;

			final_part_size[0].y = final_part_size[1].y = final_part_size[2].y = float(complete_extent.y);
			final_part_size[0].x = float(initial_part_size[0].x);
			final_part_size[2].x = float(initial_part_size[2].x);
			final_part_size[1].x = progress_size;

			final_part_position[0] = Core::Vector2f(offset, 0);
			final_part_position[1] = Core::Vector2f(final_part_size[0].x + offset, 0);
			final_part_position[2] = Core::Vector2f(final_part_size[0].x + final_part_size[1].x + offset, 0);

			if (is_mask)
			{
				final_center_texcoords[0].x = texcoords[1][0].x + (1.0f - value) * (texcoords[1][1].x - texcoords[1][0].x);
				final_center_texcoords[0].y = texcoords[1][0].y;
				final_center_texcoords[1].x = texcoords[1][1].x;
				final_center_texcoords[1].y = texcoords[1][1].y;
			}
		} break;

		case ProgressBarOrientationTop :
		{
			progress_size = value * (complete_extent.y - initial_part_size[0].y - initial_part_size[2].y);

			final_part_size[0].x = final_part_size[1].x = final_part_size[2].x = float(complete_extent.x);
			final_part_size[0].y = float(initial_part_size[0].y);
			final_part_size[2].y = float(initial_part_size[2].y);
			final_part_size[1].y = progress_size;

			final_part_position[0] = Core::Vector2f(0, 0);
			final_part_position[1] = Core::Vector2f(0, final_part_size[0].y);
			final_part_position[2] = Core::Vector2f(0, final_part_size[0].y + final_part_size[1].y);

			if (is_mask)
			{
				final_center_texcoords[0].x = texcoords[1][0].x;
				final_center_texcoords[0].y = texcoords[1][0].y;
				final_center_texcoords[1].x = texcoords[1][1].x;
				final_center_texcoords[1].y = texcoords[1][0].y + value * (texcoords[1][1].y - texcoords[1][0].y);
			}
		} break;

		case ProgressBarOrientationBottom :
		{
			float
				offset;

			progress_size = value * (complete_extent.y - initial_part_size[0].y - initial_part_size[2].y);
			offset = (complete_extent.y - initial_part_size[0].y - initial_part_size[2].y) - progress_size;

			final_part_size[0].x = final_part_size[1].x = final_part_size[2].x = float(complete_extent.x);
			final_part_size[0].y = float(initial_part_size[0].y);
			final_part_size[2].y = float(initial_part_size[2].y);
			final_part_size[1].y = progress_size;

			final_part_position[2] = Core::Vector2f(0, offset);
			final_part_position[1] = Core::Vector2f(0, final_part_size[2].y + offset);
			final_part_position[0] = Core::Vector2f(0, final_part_size[2].y + final_part_size[1].y + offset);

			if (is_mask)
			{
				final_center_texcoords[0].x = texcoords[1][0].x;
				final_center_texcoords[0].y = texcoords[1][0].y + (1.0f - value) * (texcoords[1][1].y - texcoords[1][0].y);
				final_center_texcoords[1].x = texcoords[1][1].x;
				final_center_texcoords[1].y = texcoords[1][1].y;
			}
		} break;
	}

	// Generate start part geometry.
	{
        Rocket::Core::Container::vector< Rocket::Core::Vertex >::Type& vertices = start_geometry.GetVertices();
		Rocket::Core::Container::vector< int >::Type& indices = start_geometry.GetIndices();

		vertices.resize(4);
		indices.resize(6);

		Rocket::Core::GeometryUtilities::GenerateQuad(&vertices[0],
													  &indices[0],
													  final_part_position[0],
													  final_part_size[0],
													  Core::Colourb(255, 255, 255, 255),
													  texcoords[0][0],
													  texcoords[0][1]);
	}

	// Generate center part geometry.
	{
        Rocket::Core::Container::vector< Rocket::Core::Vertex >::Type& vertices = center_geometry.GetVertices();
		Rocket::Core::Container::vector< int >::Type& indices = center_geometry.GetIndices();

		vertices.resize(4);
		indices.resize(6);

		Rocket::Core::GeometryUtilities::GenerateQuad(&vertices[0],
													  &indices[0],
													  final_part_position[1],
													  final_part_size[1],
													  Core::Colourb(255, 255, 255, 255),
													  final_center_texcoords[0],
													  final_center_texcoords[1]);
	}

	// Generate center part geometry.
	{
        Rocket::Core::Container::vector< Rocket::Core::Vertex >::Type& vertices = end_geometry.GetVertices();
		Rocket::Core::Container::vector< int >::Type& indices = end_geometry.GetIndices();

		vertices.resize(4);
		indices.resize(6);

		Rocket::Core::GeometryUtilities::GenerateQuad(&vertices[0],
													  &indices[0],
													  final_part_position[2],
													  final_part_size[2],
													  Core::Colourb(255, 255, 255, 255),
													  texcoords[2][0],
													  texcoords[2][1]);
	}

	geometry_dirty = false;
}

/// Called when source texture has changed.
void ElementProgressBar::LoadTexture(int index)
{
	Core::ElementDocument* document = GetOwnerDocument();
	Core::URL source_url(document == NULL ? "" : document->GetSourceURL());

	switch(index)
	{
		case 0:
		{
			LoadTexture(source_url, 0, "progress-start-image", start_geometry);
			break;
		}

		case 1:
		{
			LoadTexture(source_url, 1, "progress-center-image", center_geometry);
			break;
		}

		case 2:
		{
			LoadTexture(source_url, 2, "progress-end-image", end_geometry);
			break;
		}
	}
}

/// Called when source texture has changed.
void ElementProgressBar::LoadTexture(Core::URL & source_url, int index, const char *property_name, Core::Geometry & geometry)
{
	Core::RenderInterface* render_interface = GetRenderInterface();
	Core::StringList words;
	Core::String source = GetProperty< Core::String >(property_name);
	Core::StringUtilities::ExpandString(words, source, ' ');
	bool it_uses_tex_coords = false;

	if (words.size() == 5)
	{
		it_uses_tex_coords = true;
		source = words[0];
		Core::TypeConverter< Core::String, float >::Convert(words[1], texcoords[index][0].x);
		Core::TypeConverter< Core::String, float >::Convert(words[2], texcoords[index][0].y);
		Core::TypeConverter< Core::String, float >::Convert(words[3], texcoords[index][1].x);
		Core::TypeConverter< Core::String, float >::Convert(words[4], texcoords[index][1].y);
	}

	if (!texture[index].Load(source, source_url.GetPath()))
	{
		geometry.SetTexture(NULL);
		return;
	}

	if (it_uses_tex_coords)
	{
		Core::Vector2i texture_dimensions = texture[index].GetDimensions(render_interface);

		initial_part_size[index].x = texcoords[index][1].x - texcoords[index][0].x;
		initial_part_size[index].y = texcoords[index][1].y - texcoords[index][0].y;

		for (int i = 0; i < 2; i++)
		{
			texcoords[index][i].x /= texture_dimensions.x;
			texcoords[index][i].y /= texture_dimensions.y;
		}
	}
	else
	{
		texcoords[index][0].x = 0.0f;
		texcoords[index][0].y = 0.0f;
		texcoords[index][1].x = 1.0f;
		texcoords[index][1].y = 1.0f;
		initial_part_size[index].x = float(texture[index].GetDimensions(render_interface).x);
		initial_part_size[index].y = float(texture[index].GetDimensions(render_interface).y);
	}

	geometry.SetTexture(&texture[index]);
}

}
}
