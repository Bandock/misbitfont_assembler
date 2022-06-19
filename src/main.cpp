#include "../include/application.hpp"
#include <cstring>
#include <fstream>
#include <sstream>
#include <regex>
#include <msbtfont/msbtfont.h>
#include <fmt/core.h>

MisbitFontAssembler::Application::Application(std::vector<std::string> &&Args) : current_line_number(1), error_count(0), warning_count(0), Args(Args), current_draw_mode(MisbitFontAssembler::DrawMode::Binary), palette_format(1), current_max_font_size { 1, 1 }, current_draw_coordinates { 0, 0 }, current_font_width(0), font_name(""), language(""), current_spacing_type(SpacingType::Monospace), draw(false), exit(false), retcode(0)
{
	fmt::print("MisbitFont Assembler V{}.{}\n", Version.major, Version.minor);
	fmt::print("By Joshua Moss\n\n");
	if (Args.size() == 0)
	{
		fmt::print("Format:  misbitfont_assembler [input] -o [output]\n");
		exit = true;
	}
}

MisbitFontAssembler::Application::~Application()
{
}

void MisbitFontAssembler::Application::Assemble()
{
	std::ifstream input_file(Args[0]);
	if (!input_file.is_open())
	{
		fmt::print("Unable to open '{}'.\n", Args[0]);
		exit = true;
		retcode = -1;
		return;
	}
	bool output_switch = false;
	std::ofstream output_file;
	for (auto &i : Args)
	{
		if (output_switch)
		{
			if (i == Args[0])
			{
				fmt::print("Do not specify the output file as the input file.\n");
				exit = true;
				retcode = -1;
				return;
			}
			output_file.open(i, std::ios::binary);
			fmt::print("Attempting to assemble {} to {}...\n", Args[0], i);
			break;
		}
		if (i == "-o")
		{
			output_switch = true;
		}
	}
	if (!output_switch)
	{
		fmt::print("You need to specify an output file.\n");
		exit = true;
		retcode = -1;
		return;
	}
	msbtfont_header header;
	msbtfont_header_descriptor header_descriptor;
	memset(&header_descriptor, 0, sizeof(msbtfont_header_descriptor));
	while (!input_file.fail())
	{
		std::array<char, 4096> line_data;
		std::string token = "";
		std::string u_token = "";
		bool error = false;
		bool comment = false;
		bool string_mode = false;
		bool draw_pixel = false;
		ErrorType error_type = ErrorType::NoError;
		TokenType token_type = TokenType::None;
		input_file.getline(line_data.data(), line_data.size(), '\n');
		size_t characters_read = input_file.gcount();
		auto ProcessUInt8 = [&token, &error, &error_type](bool hex_support, bool bin_support)
		{
			std::regex hex("0xa-fA-F0-9]{1,}");
			std::regex bin("0b[0-1]{1,8}");
			std::regex dec("[0-9]{1,}");
			std::smatch match;
			uint16_t value = 0;
			if (hex_support && std::regex_search(token, match, hex))
			{
				if (match.prefix().str().size() > 0 || match.suffix().str().size() > 0)
				{
					error = true;
					error_type = ErrorType::InvalidValue;
					return static_cast<uint8_t>(0);
				}
				std::istringstream hex_str(match.str());
				hex_str >> std::hex >> value;
			}
			else if (bin_support && std::regex_search(token, match, bin))
			{
				if (match.prefix().str().size() > 0 || match.suffix().str().size() > 0)
				{
					error = true;
					error_type = ErrorType::InvalidValue;
					return static_cast<uint8_t>(0);
				}
				for (size_t c = 0; c < match.str().size() - 2; ++c)
				{
					if (match.str()[2 + c] == '1')
					{
						value |= (0x80 >> (7 - ((match.str().size() - 3) - c)));
					}
				}
			}
			else if (std::regex_search(token, match, dec))
			{
				if (match.prefix().str().size() > 0 || match.suffix().str().size() > 0)
				{
					error = true;
					error_type = ErrorType::InvalidValue;
					return static_cast<uint8_t>(0);
				}
				std::istringstream dec_str(match.str());
				dec_str >> value;
			}
			else
			{
				error = true;
				error_type = ErrorType::InvalidValue;
				return static_cast<uint8_t>(0);
			}
			return static_cast<uint8_t>(value & 0xFF);
		};
		auto ProcessUInt16 = [&token, &error, &error_type](bool hex_support, bool bin_support)
		{
			std::regex hex("0x[a-fA-F0-9]{1,}");
			std::regex bin("0b[0-1]{1,16}");
			std::regex dec("[0-9]{1,}");
			std::smatch match;
			uint16_t value = 0;
			if (hex_support && std::regex_search(token, match, hex))
			{
				if (match.prefix().str().size() > 0 || match.suffix().str().size() > 0)
				{
					error = true;
					error_type = ErrorType::InvalidValue;
					return static_cast<uint16_t>(0);
				}
				std::istringstream hex_str(match.str());
				hex_str >> std::hex >> value;
			}
			else if (bin_support && std::regex_search(token, match, bin))
			{
				if (match.prefix().str().size() > 0 || match.suffix().str().size() > 0)
				{
					error = true;
					error_type = ErrorType::InvalidValue;
					return static_cast<uint16_t>(0);
				}
				for (size_t c = 0; c < match.str().size() - 2; ++c)
				{
					if (match.str()[2 + c] == '1')
					{
						value |= (0x8000 >> (15 - ((match.str().size() - 3) - c)));
					}
				}
			}
			else if (std::regex_search(token, match, dec))
			{
				if (match.prefix().str().size() > 0 || match.suffix().str().size() > 0)
				{
					error = true;
					error_type = ErrorType::InvalidValue;
					return static_cast<uint16_t>(0);
				}
				std::istringstream dec_str(match.str());
				dec_str >> value;
			}
			else
			{
				error = true;
				error_type = ErrorType::InvalidValue;
			}
			return value;
		};
		auto ProcessFontSize = [this, &token, &error, &error_type]()
		{
			FontSizeData size = { 0, 0 };
			std::string current_value = "";
			bool set_height = false;
			for (size_t c = 0; c < token.size(); ++c)
			{
				if (token[c] == 'x')
				{
					if(!set_height)
					{
						set_height = true;
						current_value += ' ';
						continue;
					}
					else
					{
						error = true;
						error_type = ErrorType::InvalidValue;
						return size;
					}
				}
				current_value += token[c];
			}
			std::istringstream sizestr(current_value);
			sizestr >> size.width >> size.height;
			return size;
		};
		for (size_t i = 0; i < characters_read; ++i)
		{
			auto IssueWarning = [this, &token, &i](std::string message)
			{
				++warning_count;
				fmt::print("Warning at {}:{} : {}\n", current_line_number, i - token.size(), message);
			};
			auto DrawPixel = [this, &token, &IssueWarning](unsigned char pixel)
			{
				if (current_draw_coordinates.y < current_max_font_size.height)
				{
					uint16_t character_font_width = (current_spacing_type == SpacingType::Variable) ? CurrentFontCharacter.width : current_max_font_size.width;
					if (!character_font_width)
					{
						character_font_width = current_max_font_size.width;
					}
					if (current_draw_coordinates.x < character_font_width)
					{
						size_t c_offset = (((current_draw_coordinates.y * current_max_font_size.width) + current_draw_coordinates.x) * palette_format) / 8;
						uint8_t bit_offset = (((current_draw_coordinates.y * current_max_font_size.width) + current_draw_coordinates.x) * palette_format) % 8;
						if (palette_format < 8)
						{
							pixel <<= (8 - palette_format);
						}
						CurrentFontCharacter.character[c_offset] |= (pixel >> bit_offset);
						uint8_t bit_offset_s = bit_offset;
						size_t bits_left = palette_format;
						bit_offset += bits_left;
						if (bit_offset > 7)
						{
							bits_left -= (bit_offset - bit_offset_s);
							bit_offset -= 8;
							++c_offset;
							if (bits_left > 0)
							{
								CurrentFontCharacter.character[c_offset] |= ((pixel >> (8 - bits_left)) << (8 - palette_format));
							}
						}
						++current_draw_coordinates.x;
					}
					else
					{
						IssueWarning("Drawing out of bounds on the x-axis.  Skipping pixel.");
					}
				}
				else
				{
					IssueWarning("Drawing out of bounds on the y-axis.  Skipping pixel.");
				}
				token = "";
			};
			if (!draw)
			{
				switch (line_data[i])
				{
					case ';':
					{
						if (!string_mode)
						{
							if (!comment)
							{
								comment = true;
							}
						}
						else
						{
							token += line_data[i];
						}
						break;
					}
					case '"':
					{
						if (!comment)
						{
							if (!string_mode)
							{
								switch (token_type)
								{
									case TokenType::FontName:
									case TokenType::Language:
									{
										string_mode = true;
										break;
									}
								}
							}
							else
							{
								size_t token_len = token.size();
								switch (token_type)
								{
									case TokenType::FontName:
									{
										if (token_len > 64)
										{
											IssueWarning("Font Name specified takes up more than 64 bytes.  Upon assembly, it will be truncated.");
										}
										font_name = std::move(token);
										string_mode = false;
										break;
									}
									case TokenType::Language:
									{
										if (token_len > 64)
										{
											IssueWarning("Language specified takes up more than 64 bytes.  Upon assembly, it will be truncated.");
										}
										language = std::move(token);
										string_mode = false;
										break;
									}
								}
							}
						}
						break;
					}
					case ' ':
					{
						if (!string_mode)
						{
							if (token.size() > 0 && !comment)
							{
								if (token_type == TokenType::None)
								{
									bool valid_token = false;
									for (size_t c = 0; c < token.size(); ++c)
									{
										u_token += toupper(static_cast<unsigned char>(token[c]));
									}
									for (auto t : TokenList)
									{
										if (u_token == t)
										{
											valid_token = true;
											if (t == "CURRENT_FONT_WIDTH")
											{
												token_type = TokenType::CurrentFontWidth;
											}
											else if (t == "DRAW")
											{
												token_type = TokenType::Draw;
											}
											else if (t == "DRAW_MODE")
											{
												token_type = TokenType::DrawMode;
											}
											else if (t == "FONT_NAME")
											{
												token_type = TokenType::FontName;
											}
											else if (t == "LANGUAGE")
											{
												token_type = TokenType::Language;
											}
											else if (t == "MAX_FONT_SIZE")
											{
												token_type = TokenType::MaxFontSize;
											}
											else if (t == "PALETTE_FORMAT")
											{
												token_type = TokenType::PaletteFormat;
											}
											else if (t == "SPACING_TYPE")
											{
												token_type = TokenType::SpacingType;
											}
											token = "";
											u_token = "";
											break;
										}
									}
									if (!valid_token)
									{
										error = true;
										error_type = ErrorType::InvalidToken;
									}
									break;
								}
							}
						}
						else
						{
							token += line_data[i];
						}
						break;
					}
					case '\0':
					{
						if (token.size() > 0)
						{
							bool valid_token = false;
							for (size_t c = 0; c < token.size(); ++c)
							{
								u_token += toupper(static_cast<unsigned char>(token[c]));
							}
							switch (token_type)
							{
								case TokenType::None:
								{
									for (auto t : TokenList)
									{
										if (u_token == t)
										{
											valid_token = true;
											if (t == "CURRENT_FONT_WIDTH")
											{
												token_type = TokenType::CurrentFontWidth;
											}
											else if (t == "DRAW")
											{
												token_type = TokenType::Draw;
											}
											else if (t == "DRAW_MODE")
											{
												token_type = TokenType::DrawMode;
											}
											else if (t == "FONT_NAME")
											{
												token_type = TokenType::FontName;
											}
											else if (t == "LANGUAGE")
											{
												token_type = TokenType::Language;
											}
											else if (t == "MAX_FONT_SIZE")
											{
												token_type = TokenType::MaxFontSize;
											}
											else if (t == "PALETTE_FORMAT")
											{
												token_type = TokenType::PaletteFormat;
											}
											else if (t == "SPACING_TYPE")
											{
												token_type = TokenType::SpacingType;
											}
											error = true;
											error_type = ErrorType::MissingOperand;
											break;
										}
									}
									if (!valid_token)
									{
										error = true;
										error_type = ErrorType::InvalidToken;
									}
									break;
								}
								case TokenType::CurrentFontWidth:
								{
									uint16_t current_font_width = ProcessUInt16(true, false);
									if (error)
									{
										break;
									}
									if (current_spacing_type == SpacingType::Variable)
									{
										if (current_font_width > current_max_font_size.width)
										{
											IssueWarning("Current font width must not be greater than the max font width.  This statement has no effect.");
										}
										else
										{
											this->current_font_width = current_font_width;
										}
									}
									else
									{
										IssueWarning("Setting the current font width is unsupported in 'monospace' mode.  No changes were made as a result.");
									}
									break;
								}
								case TokenType::Draw:
								{
									bool valid_token = false;
									for (auto t : ToggleList)
									{
										if (u_token == t)
										{
											valid_token = true;
											if (t == "OFF")
											{
												IssueWarning("Drawing is already off.  This statement has no effect.");
											}
											else if (t == "ON")
											{
												draw = true;
												size_t font_character_data_size = current_max_font_size.width * current_max_font_size.height * (palette_format + 1) / 8;
												if ((current_max_font_size.width * current_max_font_size.height * (palette_format + 1)) % 8 != 0)
												{
													++font_character_data_size;
												}
												CurrentFontCharacter.character.resize(font_character_data_size);
												if (current_spacing_type == SpacingType::Variable)
												{
													CurrentFontCharacter.width = current_font_width;
												}
											}
											break;
										}
									}
									if (!valid_token)
									{
										error = true;
										error_type = ErrorType::InvalidToken;
									}
									break;
								}
								case TokenType::DrawMode:
								{
									bool valid_token = false;
									for (auto d : DrawModeList)
									{
										if (u_token == d)
										{
											valid_token = true;
											if (d == "BINARY")
											{
												current_draw_mode = DrawMode::Binary;
											}
											else if (d == "OCTAL")
											{
												current_draw_mode = DrawMode::Octal;
											}
											else if (d == "DECIMAL")
											{
												current_draw_mode = DrawMode::Decimal;
											}
											else if (d == "HEXADECIMAL")
											{
												current_draw_mode = DrawMode::Hexadecimal;
											}
											break;
										}
									}
									if (!valid_token)
									{
										error = true;
										error_type = ErrorType::InvalidToken;
									}
									break;
								}
								case TokenType::FontName:
								{
									break;
								}
								case TokenType::Language:
								{
									break;
								}
								case TokenType::MaxFontSize:
								{
									FontSizeData size = ProcessFontSize();
									if (error)
									{
										break;
									}
									if ((size.width >= 1 && size.width <= 256) && (size.height >= 1 && size.height <= 256))
									{
										current_max_font_size = size;
									}
									else
									{
										error = true;
										error_type = ErrorType::UnsupportedMaxFontSize;
									}
									break;
								}
								case TokenType::PaletteFormat:
								{
									uint8_t palette_format = ProcessUInt8(false, false);
									if (error)
									{
										break;
									}
									if (FontCharacterTable.size() == 0)
									{
										if (palette_format >= 1 && palette_format <= 8)
										{
											fmt::print("Setting Palette Format to {}.\n", palette_format);
											this->palette_format = palette_format;
										}
										else
										{
											error = true;
											error_type = ErrorType::UnsupportedPaletteFormat;
										}
									}
									else
									{
										IssueWarning("You can only specify the palette format if nothing has been drawn.  This statement has no effect.");
									}
									break;
								}
								case TokenType::SpacingType:
								{
									if (FontCharacterTable.size() == 0)
									{
										bool valid_token = false;
										for (auto s : SpacingTypeList)
										{
											if (u_token == s)
											{
												valid_token = true;
												if (s == "MONOSPACE")
												{
													current_spacing_type = SpacingType::Monospace;
												}
												else if (s == "VARIABLE")
												{
													current_spacing_type = SpacingType::Variable;
												}
												break;
											}
										}
										if (!valid_token)
										{
											error = true;
											error_type = ErrorType::InvalidToken;
										}
									}
									else
									{
										IssueWarning("You can only specify the spacing type format if nothing has been drawn.  This statement has no effect.");
									}
									break;
								}
							}
						}
						break;
					}
					default:
					{
						if (!comment)
						{
							if (token.size() == 0)
							{
								switch (token_type)
								{
									case TokenType::CurrentFontWidth:
									case TokenType::PaletteFormat:
									case TokenType::MaxFontSize:
									{
										if (!isdigit(static_cast<unsigned char>(line_data[i])))
										{
											error = true;
											break;
										}
										else if (isspace(static_cast<unsigned char>(line_data[i])))
										{
											break;
										}
										token += line_data[i];
										break;
									}
									case TokenType::FontName:
									case TokenType::Language:
									{
										if (!string_mode)
										{
											error = true;
											error_type = ErrorType::StringRequirement;
											break;
										}
										token += line_data[i];
										break;
									}
									default:
									{
										if (isdigit(static_cast<unsigned char>(line_data[i])))
										{
											error = true;
											break;
										}
										else if (isspace(static_cast<unsigned char>(line_data[i])))
										{
											break;
										}
										token += line_data[i];
										break;
									}
								}
							}
							else
							{
								token += line_data[i];
							}
						}
						break;
					}
				}
			}
			else
			{
				auto ProcessPixel = [this, &token, &IssueWarning]()
				{
					uint8_t pixel = 0;
					switch (current_draw_mode)
					{
						case DrawMode::Binary:
						{
							for (size_t c = 0; c < token.size(); ++c)
							{
								if (token[c] == '1')
								{
									pixel |= (0x01 << ((token.size() - 1) - c));
								}
								else if (token[c] != '0')
								{
									pixel = 0;
									IssueWarning("Unsupported value (must be 0s or 1s in binary drawing mode).  This pixel will be zeroed.");
									break;
								}
							}
							break;
						}
						case DrawMode::Octal:
						{
							uint8_t max_value = (0xFF >> (8 - palette_format));
							switch (palette_format)
							{
								case 1:
								case 2:
								{
									if (token[0] >= '0' && token[0] <= '7')
									{
										pixel = static_cast<uint8_t>(token[0] - '0');
										if (pixel > max_value)
										{
											pixel &= (0xFF >> (8 - palette_format));
											IssueWarning("Value is beyond the maximum limit for the palette format used while in octal drawing mode.  This pixel will be truncated to fit.");
										}
									}
									else
									{
										pixel = 0;
										IssueWarning("Unsupported value (must be 0-7s in octal drawing).  This pixel will be zeroed.");
									}
									break;
								}
								case 3:
								{
									if (token[0] >= '0' && token[0] <= '7')
									{
										pixel = static_cast<uint8_t>(token[0] - '0');
									}
									else
									{
										pixel = 0;
										IssueWarning("Unsupported value (must be 0-7s in octal drawing).  This pixel will be zeroed.");
									}
									break;
								}
								case 4:
								case 5:
								{
									for (size_t c = 0; c < token.size(); ++c)
									{
										if (token[c] >= '0' && token[c] <= '7')
										{
											pixel |= (static_cast<uint8_t>(token[c] - '0') << (3 * ((token.size() - 1) - c)));
										}
										else
										{
											pixel = 0;
											IssueWarning("Unsupported value (must be 0-7s in octal drawing mode).  This pixel will be zeroed.");
											break;
										}
									}
									if (pixel > max_value)
									{
										pixel &= (0xFF >> (8 - palette_format));
										IssueWarning("Value is beyond the maximum limit for the palette format used while in octal drawing mode.  This pixel will be truncated to fit.");
									}
									break;
								}
								case 6:
								{
									for (size_t c = 0; c < token.size(); ++c)
									{
										if (token[c] >= '0' && token[c] <= '7')
										{
											pixel |= (static_cast<uint8_t>(token[c] - '0') << (3 * ((token.size() - 1) - c)));
										}
										else
										{
											pixel = 0;
											IssueWarning("Unsupported value (must be 0-7s in octal drawing mode).  This pixel will be zeroed.");
										}
									}
									break;
								}
								case 7:
								case 8:
								{
									bool overflow = false;
									for (size_t c = 0; c < token.size(); ++c)
									{
										if (token[c] >= '0' && token[c] <= '7')
										{
											uint8_t digit = static_cast<uint8_t>(token[c] - '0');
											if (c == 0 && digit > 3 && token.size() == 3)
											{
												overflow = true;
											}
											pixel |= static_cast<uint8_t>(digit << (3 * ((token.size() - 1) - c)));
										}
										else
										{
											pixel = 0;
											if (overflow)
											{
												overflow = false;
											}
											IssueWarning("Unsupported value (must be 0-7s in octal drawing mode).  This pixel will be zeroed.");
											break;
										}
									}
									if (pixel > max_value)
									{
										if (!overflow)
										{
											overflow = true;
										}
										pixel &= (0xFF >> (8 - palette_format));
									}
									if (overflow)
									{
										IssueWarning("Value is beyond the maximum limit for the palette format used while in octal drawing mode.  This pixel will be truncated to fit.");
									}
									break;
								}
							}
							break;
						}
						case DrawMode::Decimal:
						{
							uint8_t max_value = (0xFF >> (8 - palette_format));
							switch (palette_format)
							{
								case 1:
								case 2:
								case 3:
								{
									if (token[0] >= '0' && token[0] <= '9')
									{
										pixel = static_cast<uint8_t>(token[0] - '0');
										if (pixel > max_value)
										{
											pixel &= (0xFF >> (8 - palette_format));
											IssueWarning("Value is beyond the maximum limit for the palette format used while in decimal drawing mode.  This pixel will be truncated to fit.");
										}
									}
									else
									{
										pixel = 0;
										IssueWarning("Unsupported value (must be 0-9s in decimal drawing).  This pixel will be zeroed.");
									}
									break;
								}
								case 4:
								case 5:
								case 6:
								{
									uint8_t digit = 1;
									for (size_t c = 1; c < token.size(); ++c)
									{
										digit *= 10;
									}
									for (size_t c = 0; c < token.size(); ++c)
									{
										if (token[c] >= '0' && token[c] <= '9')
										{
											pixel += (static_cast<uint8_t>(token[c] - '0') * digit);
											digit /= 10;
										}
										else
										{
											pixel = 0;
											IssueWarning("Unsupported value (must be 0-9s in decimal drawing mode).  This pixel will be zeroed.");
											break;
										}
									}
									if (pixel > max_value)
									{
										pixel &= (0xFF >> (8 - palette_format));
										IssueWarning("Value is beyond the maximum limit for the palette format used while in decimal drawing mode.  This pixel will be truncated to fit.");
									}
									break;
								}
								case 7:
								case 8:
								{
									bool overflow = false;
									uint8_t digit = 1;
									for (size_t c = 1; c < token.size(); ++c)
									{
										digit *= 10;
									}
									for (size_t c = 0; c < token.size(); ++c)
									{
										if (token[c] >= '0' && token[c] <= '9')
										{
											uint8_t tmp = pixel;
											pixel += (static_cast<uint8_t>(token[c] - '0') * digit);
											digit /= 10;
											if (tmp > pixel && !overflow)
											{
												overflow = true;
											}
										}
										else
										{
											pixel = 0;
											if (overflow)
											{
												overflow = false;
											}
											IssueWarning("Unsupported value (must be 0-9s in decimal drawing mode).  This pixel will be zeroed.");
											break;
										}
									}
									if (pixel > max_value)
									{
										if (!overflow)
										{
											overflow = true;
										}
										pixel &= (0xFF >> (8 - palette_format));
									}
									if (overflow)
									{
										IssueWarning("Value is beyond the maximum limit for the palette format used while in decimal drawing mode.  This pixel will be truncated to fit.");
									}
									break;
								}
							}
							break;
						}
						case DrawMode::Hexadecimal:
						{
							uint8_t max_value = (0xFF >> (8 - palette_format));
							switch (palette_format)
							{
								case 1:
								case 2:
								case 3:
								{
									uint8_t current_char = static_cast<uint8_t>(toupper(static_cast<unsigned char>(token[0])));
									if (current_char >= '0' && current_char <= '9')
									{
										pixel = static_cast<uint8_t>(current_char - '0');
										if (pixel > max_value)
										{
											pixel &= (0xFF >> (8 - palette_format));
											IssueWarning("Value is beyond the maximum limit for the palette format used while in hexadecimal drawing mode.  This pixel will be truncated to fit.");
										}
									}
									else if (current_char >= 'A' && current_char <= 'F')
									{
										pixel = 0xA + static_cast<uint8_t>(current_char - 'A');
										if (pixel > max_value)
										{
											pixel &= (0xFF >> (8 - palette_format));
											IssueWarning("Value is beyond the maximum limit for the palette format used while in hexadecimal drawing mode.  This pixel will be truncated to fit.");
										}
									}
									else
									{
										pixel = 0;
										IssueWarning("Unsupported value (must be 0-Fs in hexadecimal drawing).  This pixel will be zeroed.");
									}
									break;
								}
								case 4:
								{
									uint8_t current_char = static_cast<uint8_t>(toupper(static_cast<unsigned char>(token[0])));
									if (current_char >= '0' && current_char <= '9')
									{
										pixel = static_cast<uint8_t>(current_char - '0');
									}
									else if (current_char >= 'A' && current_char <= 'F')
									{
										pixel = 0xA + static_cast<uint8_t>(current_char - 'A');
									}
									else
									{
										pixel = 0;
										IssueWarning("Unsupported value (must be 0-Fs in hexadecimal drawing).  This pixel will be zeroed.");
									}
									break;
								}
								case 5:
								case 6:
								case 7:
								{
									for (size_t c = 0; c < token.size(); ++c)
									{
										uint8_t current_char = static_cast<uint8_t>(toupper(static_cast<unsigned char>(token[c])));
										if (current_char >= '0' && current_char <= '9')
										{
											pixel |= (static_cast<uint8_t>(current_char - '0') << ((token.size() - 1 - c) << 2));
										}
										else if (current_char >= 'A' && current_char <= 'F')
										{
											pixel |= ((0xA + static_cast<uint8_t>(current_char - 'A')) << ((token.size() - 1 - c) << 2));
										}
										else
										{
											pixel = 0;
											IssueWarning("Unsupported value (must be 0-Fs in hexadecimal drawing).  This will be zeroed.");
											break;
										}
									}
									if (pixel > max_value)
									{
										pixel &= (0xFF >> (8 - palette_format));
										IssueWarning("Value is beyond the maximum limit for the palette format used while in hexadecimal drawing mode.  This pixel will be truncated to fit.");
									}
									break;
								}
								case 8:
								{
									for (size_t c = 0; c < token.size(); ++c)
									{
										uint8_t current_char = static_cast<uint8_t>(toupper(static_cast<unsigned char>(token[c])));
										if (current_char >= '0' && current_char <= '9')
										{
											pixel |= (static_cast<uint8_t>(current_char - '0') << ((token.size() - 1 - c) << 2));
										}
										else if (current_char >= 'A' && current_char <= 'F')
										{
											pixel |= ((0xA + static_cast<uint8_t>(current_char - 'A')) << ((token.size() - 1 - c) << 2));
										}
										else
										{
											pixel = 0;
											IssueWarning("Unsupported value (must be 0-Fs in hexadecimal drawing).  This will be zeroed.");
											break;
										}
									}
									break;
								}
							}
							break;
						}
					}
					return pixel;
				};
				switch (line_data[i])
				{
					case ';':
					{
						if (!comment)
						{
							comment = true;
						}
						break;
					}
					case ' ':
					{
						if (!comment)
						{
							if (token.size() > 0)
							{
								if (token_type == TokenType::None)
								{
									if (current_draw_mode == DrawMode::Hexadecimal && token.size() == 1)
									{
										draw_pixel = true;
									}
									if (!draw_pixel)
									{
										for (size_t c = 0; c < token.size(); ++c)
										{
											u_token += toupper(static_cast<unsigned char>(token[c]));
										}
										bool valid_token = false;
										bool legal_token = false;
										for (auto t : TokenList)
										{
											if (u_token == t)
											{
												valid_token = true;
												if (t == "DRAW")
												{
													legal_token = true;
													token_type = TokenType::Draw;
												}
												break;
											}
										}
										if (!valid_token)
										{
											error = true;
											error_type = ErrorType::InvalidToken;
											break;
										}
										else if (!legal_token)
										{
											error = true;
											error_type = ErrorType::IllegalToken;
											break;
										}
										token = "";
										u_token = "";
									}
									else
									{
										uint8_t pixel = ProcessPixel();
										DrawPixel(pixel);
									}
								}
							}
						}
						break;
					}
					case '\0':
					{
						if (token.size() > 0)
						{
							if (current_draw_mode == DrawMode::Hexadecimal && token_type == TokenType::None && token.size() == 1)
							{
								draw_pixel = true;
							}
							if (!draw_pixel)
							{
								bool valid_token = false;
								bool legal_token = false;
								for (size_t c = 0; c < token.size(); ++c)
								{
									u_token += toupper(static_cast<unsigned char>(token[c]));
								}
								switch (token_type)
								{
									case TokenType::None:
									{
										for (auto t : TokenList)
										{
											if (u_token == t)
											{
												valid_token = true;
												if (t == "DRAW")
												{
													legal_token = true;
													token_type = TokenType::Draw;
													error = true;
													error_type = ErrorType::MissingOperand;
													break;
												}
												break;
											}
										}
										if (!valid_token)
										{
											error = true;
											error_type = ErrorType::InvalidToken;
										}
										else if (!legal_token)
										{
											error = true;
											error_type = ErrorType::IllegalToken;
										}
										break;
									}
									case TokenType::Draw:
									{
										bool valid_token = false;
										for (auto t : ToggleList)
										{
											if (u_token == t)
											{
												valid_token = true;
												if (t == "OFF")
												{
													draw = false;
													current_draw_coordinates = { 0, 0 };
													FontCharacterTable.push_back(std::move(CurrentFontCharacter));	
												}
												break;
											}
										}
										if (!valid_token)
										{
											error = true;
											error_type = ErrorType::InvalidToken;
										}
										break;
									}
								}
							}
							else
							{
								uint8_t pixel = ProcessPixel();
								DrawPixel(pixel);
								current_draw_coordinates.x = 0;
								if (current_draw_coordinates.y < current_max_font_size.height)
								{
									++current_draw_coordinates.y;
								}
							}
						}
						else
						{
							if (draw_pixel)
							{
								current_draw_coordinates.x = 0;
								if (current_draw_coordinates.y < current_max_font_size.height)
								{
									++current_draw_coordinates.y;
								}
							}
						}
						break;
					}
					default:
					{
						if (!comment)
						{
							if (token.size() == 0)
							{
								if (token_type == TokenType::None)
								{
									if (isdigit(static_cast<unsigned char>(line_data[i])))
									{
										draw_pixel = true;
									}
									else if (isspace(static_cast<unsigned char>(line_data[i])))
									{
										break;
									}
									token += line_data[i];
								}
								else if (token_type == TokenType::Draw)
								{
									if (isspace(static_cast<unsigned char>(line_data[i])))
									{
										break;
									}
									token += line_data[i];
								}
							}
							else
							{
								if (current_draw_mode == DrawMode::Hexadecimal && token_type == TokenType::None && !draw_pixel)
								{
									uint8_t current_char = static_cast<uint8_t>(toupper(static_cast<uint8_t>(line_data[i])));
									if (token.size() == 1 && (isdigit(current_char) || (current_char >= 'A' && current_char <= 'F')))
									{
										draw_pixel = true;
									}
								}
								if (isspace(static_cast<unsigned char>(line_data[i])))
								{
									break;
								}
								else if (draw_pixel)
								{
									bool ready_to_draw = false;
									uint8_t pixel = 0;
									switch (current_draw_mode)
									{
										case DrawMode::Binary:
										{
											if (token.size() == palette_format)
											{
												ready_to_draw = true;
												for (size_t c = 0; c < palette_format; ++c)
												{
													if (token[c] == '1')
													{
														pixel |= (palette_format >> c);
													}
													else if (token[c] != '0')
													{
														pixel = 0;
														IssueWarning("Unsupported value (must be 0s or 1s in binary drawing mode).  This pixel will be zeroed.");
														break;
													}
												}
											}
											break;
										}
										case DrawMode::Octal:
										{
											uint8_t max_value = (0xFF >> (8 - palette_format));
											switch (palette_format)
											{
												case 1:
												case 2:
												{
													if (token.size() == 1)
													{
														ready_to_draw = true;
														if (token[0] >= '0' && token[0] <= '7')
														{
															pixel = static_cast<uint8_t>(token[0] - '0');
															if (pixel > max_value)
															{
																pixel &= (0xFF >> (8 - palette_format));
																IssueWarning("Value is beyond the maximum limit for the palette format used while in octal drawing mode.  This pixel will be truncated to fit.");
															}
														}
														else
														{
															pixel = 0;
															IssueWarning("Unsupported value (must be 0-7s in octal drawing mode).  This pixel will be zeroed.");
														}
													}
													break;
												}
												case 3:
												{
													if (token.size() == 1)
													{
														ready_to_draw = true;
														if (token[0] >= '0' && token[0] <= '7')
														{
															pixel = static_cast<uint8_t>(token[0] - '0');
														}
														else
														{
															pixel = 0;
															IssueWarning("Unsupported value (must be 0-7s in octal drawing mode).  This pixel will be zeroed.");
														}
													}
													break;
												}
												case 4:
												case 5:
												{
													if (token.size() == 2)
													{
														ready_to_draw = true;
														for (size_t c = 0; c < 2; ++c)
														{
															if (token[c] >= '0' && token[c] <= '7')
															{
																pixel |= (static_cast<uint8_t>(token[c] - '0') << (3 * (1 - c)));
															}
															else
															{
																pixel = 0;
																IssueWarning("Unsupported value (must be 0-7s in octal drawing mode).  This pixel will be zeroed.");
																break;
															}
														}
														if (pixel > max_value)
														{
															pixel &= (0xFF >> (8 - palette_format));
															IssueWarning("Value is beyond the maximum limit for the palette format used while in octal drawing mode.  This pixel will be truncated to fit.");
														}
													}
													break;
												}
												case 6:
												{
													if (token.size() == 2)
													{
														ready_to_draw = true;
														for (size_t c = 0; c < 2; ++c)
														{
															if (token[c] >= '0' && token[c] <= '7')
															{
																pixel |= (static_cast<uint8_t>(token[c] - '0') << (3 * (1 - c)));
															}
															else
															{
																pixel = 0;
																IssueWarning("Unsupported value (must be 0-7s in octal drawing mode).  This pixel will be zeroed.");
																break;
															}
														}
													}
													break;
												}
												case 7:
												case 8:
												{
													bool overflow = false;
													if (token.size() == 3)
													{
														ready_to_draw = true;
														for (size_t c = 0; c < 3; ++c)
														{
															if (token[c] >= '0' && token[c] <= '7')
															{
																uint8_t digit = static_cast<uint8_t>(token[c] - '0');
																if (c == 0 && digit > 3)
																{
																	overflow = true;
																}
																pixel |= static_cast<uint8_t>(digit << (3 * (2 - c)));
															}
															else
															{
																pixel = 0;
																if (overflow)
																{
																	overflow = false;
																}
																IssueWarning("Unsupported value (must be 0-7s in octal drawing mode).  This pixel will be zeroed.");
																break;
															}
														}
														if (pixel > max_value)
														{
															if (!overflow)
															{
																overflow = true;
															}
															pixel &= (0xFF >> (8 - palette_format));
														}
														if (overflow)
														{
															IssueWarning("Value is beyond the maximum limit for the palette format used while in octal drawing mode.  This pixel will be truncated to fit.");
														}
													}
													break;
												}
											}
											break;
										}
										case DrawMode::Decimal:
										{
											uint8_t max_value = (0xFF >> (8 - palette_format));
											switch (palette_format)
											{
												case 1:
												case 2:
												case 3:
												{
													if (token.size() == 1)
													{
														ready_to_draw = true;
														if (token[0] >= '0' && token[0] <= '9')
														{
															pixel = static_cast<uint8_t>(token[0] - '0');
															if (pixel > max_value)
															{
																pixel &= (0xFF >> (8 - palette_format));
																IssueWarning("Value is beyond the maximum limit for the palette format used while in decimal drawing mode.  This pixel will be truncated to fit.");
															}
														}
														else
														{
															pixel = 0;
															IssueWarning("Unsupported value (must be 0-9s in decimal drawing mode).  This pixel will be zeroed.");
														}
													}
													break;
												}
												case 4:
												case 5:
												case 6:
												{
													if (token.size() == 2)
													{
														uint8_t digit = 10;
														ready_to_draw = true;
														for (size_t c = 0; c < 2; ++c)
														{
															if (token[c] >= '0' && token[c] <= '9')
															{
																pixel += (static_cast<uint8_t>(token[c] - '0') * digit);
																digit /= 10;
															}
															else
															{
																pixel = 0;
																IssueWarning("Unsupported value (must be 0-9s in decimal drawing mode).  This pixel will be zeroed.");
																break;
															}
														}
														if (pixel > max_value)
														{
															pixel &= (0xFF >> (8 - palette_format));
															IssueWarning("Value is beyond the maximum limit for the palette format used while in decimal drawing mode.  This pixel will be truncated to fit.");
														}
													}
													break;
												}
												case 7:
												case 8:
												{
													bool overflow = false;
													if (token.size() == 3)
													{
														uint8_t digit = 100;
														ready_to_draw = true;
														for (size_t c = 0; c < 3; ++c)
														{
															if (token[c] >= '0' && token[c] <= '9')
															{
																uint8_t tmp = pixel;
																pixel += (static_cast<uint8_t>(token[c] - '0') * digit);
																digit /= 10;
																if (tmp > pixel && !overflow)
																{
																	overflow = true;
																}
															}
															else
															{
																pixel = 0;
																if (overflow)
																{
																	overflow = false;
																}
																IssueWarning("Unsupported value (must be 0-9s in decimal drawing mode).  This pixel will be zeroed.");
																break;
															}
														}
														if (pixel > max_value)
														{
															if (!overflow)
															{
																overflow = true;
															}
															pixel &= (0xFF >> (8 - palette_format));
														}
														if (overflow)
														{
															IssueWarning("Value is beyond the maximum limit for the palette format used while in decimal drawing mode.  This pixel will be truncated to fit.");
														}
													}
													break;
												}
											}
											break;
										}
										case DrawMode::Hexadecimal:
										{
											uint8_t max_value = (0xFF >> (8 - palette_format));
											switch (palette_format)
											{
												case 1:
												case 2:
												case 3:
												{
													if (token.size() == 1)
													{
														ready_to_draw = true;
														uint8_t current_char = static_cast<uint8_t>(toupper(static_cast<unsigned char>(token[0] - '0')));
														if (current_char >= '0' && current_char <= '9')
														{
															pixel = static_cast<uint8_t>(current_char - '0');
															if (pixel > max_value)
															{
																pixel &= (0xFF >> (8 - palette_format));
																IssueWarning("Value is beyond the maximum limit for the palette format used while in hexadecimal drawing mode.  This pixel will be truncated to fit.");
															}
														}
														else if (current_char >= 'A' && current_char <= 'F')
														{
															pixel = 0xA + static_cast<uint8_t>(current_char - 'A');
															if (pixel > max_value)
															{
																pixel &= (0xFF >> (8 - palette_format));
																IssueWarning("Value is beyond the maximum limit for the palette format used while in hexadecimal drawing mode.  This pixel will be truncated to fit.");
															}
														}
														else
														{
															pixel = 0;
															IssueWarning("Unsupported value (must be 0-Fs in hexadecimal drawing mode).  This pixel will be zeroed.");
														}
													}
													break;
												}
												case 4:
												{
													if (token.size() == 1)
													{
														ready_to_draw = true;
														uint8_t current_char = static_cast<uint8_t>(toupper(static_cast<unsigned char>(token[0])));
														if (current_char >= '0' && current_char <= '9')
														{
															pixel = static_cast<uint8_t>(current_char - '0');
														}
														else if (current_char >= 'A' && current_char <= 'F')
														{
															pixel = 0xA + static_cast<uint8_t>(current_char - 'A');
														}
														else
														{
															pixel = 0;
															IssueWarning("Unsupported value (must be 0-Fs in hexadecimal drawing mode).  This pixel will be zeroed.");
														}
													}
													break;
												}
												case 5:
												case 6:
												case 7:
												{
													if (token.size() == 2)
													{
														ready_to_draw = true;
														for (size_t c = 0; c < 2; ++c)
														{
															uint8_t current_char = static_cast<uint8_t>(toupper(static_cast<unsigned char>(token[c])));
															if (current_char >= '0' && current_char <= '9')
															{
																pixel |= (static_cast<uint8_t>(current_char - '0') << ((1 - c) << 2)); 
															}
															else if (current_char >= 'A' && current_char <= 'F')
															{
																pixel |= ((0xA + static_cast<uint8_t>(current_char - 'A')) << ((1 - c) << 2));
															}
															else
															{
																pixel = 0;
																IssueWarning("Unsupported value (must be 0-Fs in hexadecimal drawing mode).  This pixel will be zeroed.");
																break;
															}
														}
														if (pixel > max_value)
														{
															pixel &= (0xFF >> (8 - palette_format));
															IssueWarning("Value is beyond the maximum limit for the palette format used while in hexadecimal drawing mode.  This pixel will be truncated to fit.");
														}
													}
													break;
												}
												case 8:
												{
													if (token.size() == 2)
													{
														ready_to_draw = true;
														for (size_t c = 0; c < 2; ++c)
														{
															uint8_t current_char = static_cast<uint8_t>(toupper(static_cast<unsigned char>(token[c])));
															if (current_char >= '0' && current_char <= '9')
															{
																pixel |= (static_cast<uint8_t>(current_char - '0') << ((1 - c) << 2));
															}
															else if (current_char >= 'A' && current_char <= 'F')
															{
																pixel |= ((0xA + static_cast<uint8_t>(current_char - 'A')) << ((1 - c) << 2));
															}
															else
															{
																pixel = 0;
																IssueWarning("Unsupported value (must be 0-Fs in hexadecimal drawing mode).  This pixel will be zeroed.");
																break;
															}
														}
													}
													break;
												}
											}
											break;
										}
									}
									if (ready_to_draw)
									{
										DrawPixel(pixel);
									}
								}
								token += line_data[i];
							}
						}
						break;
					}
				}
			}
			if (error)
			{
				++error_count;
				fmt::print("Error at {}:{} : ", current_line_number, i - token.size());
				switch (error_type)
				{
					case ErrorType::InvalidToken:
					{
						fmt::print("Invalid Token '{}'\n", reinterpret_cast<const char *>(token.c_str()));
						break;
					}
					case ErrorType::MissingOperand:
					{
						fmt::print("Missing Operand for ");
						switch (token_type)
						{
							case TokenType::CurrentFontWidth:
							{
								fmt::print("CURRENT_FONT_WIDTH\n");
								break;
							}
							case TokenType::Draw:
							{
								fmt::print("DRAW\n");
								break;
							}
							case TokenType::DrawMode:
							{
								fmt::print("DRAW_MODE\n");
								break;
							}
							case TokenType::FontName:
							{
								fmt::print("FONT_NAME\n");
								break;
							}
							case TokenType::Language:
							{
								fmt::print("LANGUAGE\n");
								break;
							}
							case TokenType::MaxFontSize:
							{
								fmt::print("MAX_FONT_SIZE\n");
								break;
							}
							case TokenType::PaletteFormat:
							{
								fmt::print("PALETTE_FORMAT\n");
								break;
							}
							case TokenType::SpacingType:
							{
								fmt::print("SPACING_TYPE\n");
								break;
							}
						}
						break;
					}
					case ErrorType::InvalidValue:
					{
						fmt::print("Invalid Value\n");
						break;
					}
					case ErrorType::IllegalToken:
					{
						fmt::print("Illegal Token being used when drawing.\n");
						break;
					}
					case ErrorType::UnsupportedPaletteFormat:
					{
						fmt::print("Palette Format being specified is unsupported (must be between 1 and 8).\n");
						break;
					}
					case ErrorType::UnsupportedMaxFontSize:
					{
						fmt::print("Max Font Size being specified is unsupported (both width and height must be between 1 and 256).\n");
						break;
					}
					case ErrorType::StringRequirement:
					{
						switch (token_type)
						{
							case TokenType::FontName:
							{
								fmt::print("FONT_NAME ");
								break;
							}
							case TokenType::Language:
							{
								fmt::print("LANGUAGE ");
								break;
							}
						}
						fmt::print("must be stored as a string.\n");
						break;
					}
					default:
					{
						fmt::print("Unknown Error\n");
						break;
					}
				}
				break;
			}
		}
		++current_line_number;
	}
	if (error_count == 0)
	{
		header_descriptor.palette_format = palette_format - 1;
		header_descriptor.max_font_width = static_cast<uint8_t>(current_max_font_size.width - 1);
		header_descriptor.max_font_height = static_cast<uint8_t>(current_max_font_size.height - 1);
		if (current_spacing_type == SpacingType::Variable)
		{
			header_descriptor.flags |= 0x01;
		}
		header_descriptor.font_character_count = static_cast<uint32_t>(FontCharacterTable.size());
		size_t font_name_len = font_name.size();
		if (font_name_len > 0)
		{
			if (font_name_len > 64)
			{
				font_name_len = 64;
			}
			memcpy(header_descriptor.font_name, font_name.c_str(), font_name_len);
		}
		size_t language_len = language.size();
		if (language_len > 0)
		{
			if (language_len > 64)
			{
				language_len = 64;
			}
			memcpy(header_descriptor.language, language.c_str(), language_len);
		}
		msbtfont_create_header(&header, &header_descriptor);
		msbtfont_filedata font_filedata;
		msbtfont_create_filedata(&header, &font_filedata);
		for (uint32_t i = 0; i < FontCharacterTable.size(); ++i)
		{
			if (current_spacing_type == SpacingType::Variable && FontCharacterTable[i].width != 0)
			{
				font_filedata.variable_table[i] = FontCharacterTable[i].width - 1;
			}
			msbtfont_store_font_character_data(&header, &font_filedata, FontCharacterTable[i].character.data(), i);
		}
		output_file.write(reinterpret_cast<char *>(&header), sizeof(header));
		if (current_spacing_type == SpacingType::Variable)
		{
			output_file.write(reinterpret_cast<char *>(font_filedata.variable_table), FontCharacterTable.size());
		}
		size_t font_data_size = (current_max_font_size.width * current_max_font_size.height * palette_format * FontCharacterTable.size()) / 8;
		if ((current_max_font_size.width * current_max_font_size.height * palette_format * FontCharacterTable.size()) % 8)
		{
			++font_data_size;
		}
		output_file.write(reinterpret_cast<char *>(font_filedata.font_data), font_data_size);
		msbtfont_delete_filedata(&font_filedata);
		fmt::print("Assembly successful!\n");
		fmt::print("{} character{} {} assembled in total.\n", FontCharacterTable.size(), (FontCharacterTable.size() != 1) ? "s" : "", (FontCharacterTable.size() != 1) ? "were" : "was");
	}
	fmt::print("There {} {} error{} and {} warning{}.\n", ((error_count != 1) ? "were" : "was"), error_count, ((error_count != 1) ? "s" : ""), warning_count, ((warning_count != 1) ? "s" : ""));
}

bool MisbitFontAssembler::Application::GetExit() const
{
	return exit;
}

int MisbitFontAssembler::Application::GetReturnCode() const
{
	return retcode;
}

int main(int argc, char *argv[])
{
	std::vector<std::string> Args;
	for (int i = 1; i < argc; ++i)
	{
		Args.push_back(argv[i]);
	}
	MisbitFontAssembler::Application MainApp(std::move(Args));
	if (!MainApp.GetExit())
	{
		MainApp.Assemble();
	}
	return MainApp.GetReturnCode();
}
