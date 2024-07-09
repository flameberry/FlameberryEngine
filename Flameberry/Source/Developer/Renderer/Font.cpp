#include "Font.h"

#include "Core/Core.h"
#include "Renderer/Texture2D.h"
#include "Renderer/MSDFFontData.h"

namespace Flameberry {

	template <typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
	static Ref<Texture2D> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
		const msdf_atlas::FontGeometry& fontGeometry, uint32_t width, uint32_t height)
	{
		msdf_atlas::GeneratorAttributes attributes;
		attributes.config.overlapSupport = true;
		attributes.scanlinePass = true;

		msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
		generator.setAttributes(attributes);
		generator.setThreadCount(8);
		generator.generate(glyphs.data(), (int)glyphs.size());

		msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

		Texture2DSpecification spec;
		spec.Width = bitmap.width;
		spec.Height = bitmap.height;
		spec.Format = VK_FORMAT_R8G8B8A8_UNORM;
		spec.GenerateMipmaps = false;

		Ref<Texture2D> texture = CreateRef<Texture2D>((const void*)bitmap.pixels, spec);
		return texture;
	}

	Ref<Font> Font::GetDefault()
	{
		static Ref<Font> DefaultFont;

		if (!DefaultFont)
			DefaultFont = CreateRef<Font>(FBY_PROJECT_DIR "Flameberry/Assets/Fonts/opensans/OpenSans-Regular.ttf");

		return DefaultFont;
	}

	Font::Font(const std::filesystem::path& path)
		: m_Data(new MSDFFontData())
	{
		using namespace msdfgen;

		FreetypeHandle* ft = initializeFreetype();
		FBY_ASSERT(ft, "Failed to initialize freetype!");

		const std::string pathStr = path.string();
		FontHandle* font = loadFont(ft, pathStr.c_str());

		if (!font)
		{
			FBY_ERROR("Failed to load font from: {}", path);
			return;
		}

		struct CharsetRange
		{
			uint32_t Start, End;
		};

		const CharsetRange charsetRanges[] = {
			{ 0x20, 0xFF }
		};

		msdf_atlas::Charset charset;

		for (CharsetRange range : charsetRanges)
			for (uint32_t c = range.Start; c < range.End; c++)
				charset.add(c);

		double fontScale = 1.0;
		m_Data->FontGeometry = msdf_atlas::FontGeometry(&m_Data->Glyphs);
		int glyphsLoaded = m_Data->FontGeometry.loadCharset(font, fontScale, charset);
		FBY_INFO("Loaded {} glyphs from font (out of {})", glyphsLoaded, charset.size());

		double emSize = 40.0;

		msdf_atlas::TightAtlasPacker atlasPacker;
		atlasPacker.setPixelRange(2.0);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setScale(emSize);

		int remaining = atlasPacker.pack(m_Data->Glyphs.data(), (int)m_Data->Glyphs.size());
		FBY_ASSERT(remaining == 0);

		int width, height;
		atlasPacker.getDimensions(width, height);
		emSize = atlasPacker.getScale();

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8

		// if MSDF || MTSDF

		uint64_t coloringSeed = 0;
		bool expensiveColoring = false;
		if (expensiveColoring)
		{
			msdf_atlas::Workload([&glyphs = m_Data->Glyphs, &coloringSeed](int i, int threadNo) -> bool {
				unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
				glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
				return true;
			},
				m_Data->Glyphs.size())
				.finish(THREAD_COUNT);
		}
		else
		{
			unsigned long long glyphSeed = coloringSeed;
			for (msdf_atlas::GlyphGeometry& glyph : m_Data->Glyphs)
			{
				glyphSeed *= LCG_MULTIPLIER;
				glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
			}
		}

		m_AtlasTexture = CreateAndCacheAtlas<uint8_t, float, 4, msdf_atlas::mtsdfGenerator>("Test", (float)emSize, m_Data->Glyphs, m_Data->FontGeometry, width, height);

		destroyFont(font);
		deinitializeFreetype(ft);
	}

	Font::~Font()
	{
		delete m_Data;
	}

} // namespace Flameberry
