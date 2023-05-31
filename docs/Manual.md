# MisbitFont Assembler Manual (Version 0.2)

## Getting Started
To run this program, simply use the following syntax:
```
misbitfont_assembler <input> -o <output>
```
The input file is simply any text file (no matter what format) that contains commands that helps assemble working MisbitFont files.  The output file is simply a MisbitFont file to create that can be used in different applications.

This is not an assembler to create programs, but instead to assemble MisbitFont files.  You get to produce MisbitFont files in a fashion similar to assembly programming in a nice and straightforward way.  Here's an example on a basic use of this program:

```
; Basic Font File

palette_format 1 ; Create 1-bit fonts.
max_font_size 8x8 ; Specify a max size of 8x8 for all characters
draw_mode binary ; Use binary mode to draw.
spacing_type monospace ; Use Monospace to indicate all characters will use the same max font width.
font_name "Basic Font" ; Completely optional name.
language "Neutral" ; Completely optional, yet can be useful in applications for localization purposes.

draw on ; Start drawing a character, using binary mode.

00011000 ; No need for binary notation when drawing in binary mode.  Remember the most characters you can use is to the max font width or current font width (used only in Variable spacing type.)
00100100
01000010
01000010
01111110
01000010
01000010
01000010

draw off ; Stop drawing this character.

draw on ; Draw another character.

01111100
01000010
01000010
01111100
01000010
01000010
01000010
01111100

draw off ; Stop drawing this character.

draw on ; Draw yet another character.

00111100
01000010
01000000
01000000
01000000
01000000
01000010
00111100

draw off ; Stop drawing this character.
```

Using the `draw on` and `draw off` commands, you can draw characters with ease depending on the draw mode specified.  Some draw modes are very useful depending the on the palette format you're targeting.  One thing to take note is you can draw fewer characters than the max font width or current font width (used only when the Variable spacing is specified).  Draw over the max and it will not be recorded in the output file.  Same if you draw more lines than the max font height; yet you can in fact draw less by calling `draw off` sooner.

All commands are case insensitive similar to how assemblers for programming work.  One of the neatest things about drawing is you do not need to use any notation whatsoever.  You can simply draw the next pixel in certain draw mode and palette format combinations without spacing or you can space them apart.  This makes it resemble text art drawing, but done in a fashion that can produce real results.

## Commands
|Command |Description |Operand |
|--------|------------|--------|
|`current_font_width`|Sets the font width to utilize for drawing.  Only usable when `variable` spacing is used.  Maximum possible font width is 256.  If `0` is specified, it will use the max font width specified in the variable table for that font.|`1 - 256`|
|`draw`|Turns drawing off or on.  While drawing is on, all other commands are disabled with the exception of another `draw` command to turn it off.  Turning off drawing will add the finished font character to the file and increment the font count.|`off`, `on`|
|`draw_mode`|Selects the mode to draw in.|`binary`, `octal`, `decimal`, `hexadecimal`|
|`font_name`|Sets a font name in the resulting MisbitFont file.  Supports UTF-8 encoding with up to 64 bytes worth of space.  This is optional.|`64 Byte UTF-8 String`|
|`language`|Specifies a language in the resulting MisbitFont file.  Supports UTF-8 encoding with up to 64 bytes worth of space.  This is optional.|`64 Byte UTF-8 String`|
|`max_font_size`|Sets the maximum font dimensions possible for all the fonts.  Maximum possible width and height is 256.|`[1-256]x[1-256]`|
|`palette_format`|Selects the palette format for the resulting MisbitFont file and how drawing is handled.  Palette format is represented in bits per pixel.|`1-8`|
|`spacing_type`|Specifies the spacing type to use for the font file.|`monospace`, `variable`|
|`skip`|Inserts a specified number of blank characters.|`1 - 65535`|

## Draw Modes
|Mode |Description |
|-----|------------|
|binary|Uses 0s and 1s (Base-2) for drawing.  Useful for any palette format. (Default)|
|octal|Uses 0-7 digits (Base-8) for drawing.  Very useful for 3-bit and 6-bit palette formats, though can be used in all of them.|
|decimal|Uses 0-9 digits (Base-10) for drawing.  Can be used for any palette format.|
|hexadecimal|Uses 0-F hexadigits (Base-16) for drawing.  Useful for any palette format.|

## Spacing Types
|Type |Description |
|-----|------------|
|monospace|All characters utilize the max font width. (Default)|
|variable|Creates a variable table as well as enable support to specify different widths for different characters.|
