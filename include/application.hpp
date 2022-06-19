#ifndef _APPLICATION_HPP_
#define _APPLICATION_HPP_

#include <string>
#include <array>
#include <vector>
#include <memory>
#include <cstdint>

namespace MisbitFontAssembler
{
	struct VersionData
	{
		uint16_t major;
		uint16_t minor;
	};

	struct FontSizeData
	{
		uint16_t width;
		uint16_t height;
	};

	struct DrawCoordinates
	{
		uint16_t x;
		uint16_t y;
	};

	struct FontCharacterData
	{
		std::vector<uint8_t> character;
		uint16_t width; // Used only with Variable Spacing.
	};

	enum class TokenType
	{
		None, CurrentFontWidth, Draw, DrawMode, FontName, Language, MaxFontSize, PaletteFormat,
		SpacingType
	};

	enum class ErrorType
	{
		NoError, InvalidToken, MissingOperand, InvalidValue, IllegalToken, UnsupportedPaletteFormat,
		UnsupportedMaxFontSize, StringRequirement
	};

	enum class DrawMode
	{
		Binary,
		Octal,
		Decimal,
		Hexadecimal
	};

	enum class SpacingType
	{
		Monospace,
		Variable
	};

	class Application
	{
		public:
			Application(std::vector<std::string> &&Args);
			~Application();
			void Assemble();
			bool GetExit() const;
			int GetReturnCode() const;
		private:
			size_t current_line_number;
			size_t error_count;
			size_t warning_count;
			std::vector<std::string> Args;
			const std::array<std::string, 8> TokenList = {
				"CURRENT_FONT_WIDTH", "DRAW", "DRAW_MODE", "FONT_NAME", "LANGUAGE",
				"MAX_FONT_SIZE", "PALETTE_FORMAT", "SPACING_TYPE"
			};
			const std::array<std::string, 4> DrawModeList = {
				"BINARY", "OCTAL", "DECIMAL", "HEXADECIMAL"
			};
			const std::array<std::string, 2> SpacingTypeList = {
				"MONOSPACE", "VARIABLE"
			};
			const std::array<std::string, 2> ToggleList = {
				"OFF", "ON"
			};
			const VersionData Version = { 0, 1 };
			DrawMode current_draw_mode;
			uint8_t palette_format;
			FontSizeData current_max_font_size;
			DrawCoordinates current_draw_coordinates;
			uint16_t current_font_width;
			std::string font_name;
			std::string language;
			SpacingType current_spacing_type;
			FontCharacterData CurrentFontCharacter;
			std::vector<FontCharacterData> FontCharacterTable;
			bool draw;
			bool exit;
			int retcode;
	};
}

#endif
