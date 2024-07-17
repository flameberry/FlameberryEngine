#pragma once

#include <vector>
#include <msdf-atlas-gen.h>

namespace Flameberry {

	struct MSDFFontData
	{
		std::vector<msdf_atlas::GlyphGeometry> Glyphs;
		msdf_atlas::FontGeometry FontGeometry;
	};

} // namespace Flameberry
