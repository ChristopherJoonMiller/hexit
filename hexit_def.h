#ifndef __HEXIT_DEF_H__
#define __HEXIT_DEF_H__

#define READ_BUFFER_BYTES		16
#define WORD_SIZE				 2
#define ROW_SIZE				16

#define ASCII_MIN				' '
#define ASCII_MAX				'~'


// Flags for command line switches
#define FLAG(x)					(1<<x)
#define SWITCH_UPPER			FLAG(0)
#define SWITCH_OUTPUT			FLAG(1)
#define SWITCH_EDIT				FLAG(2)
#define SWITCH_SHOW_BYTE_COUNT	FLAG(3)
#define SWITCH_SHOW_ASCII		FLAG(4)
#define SWITCH_COLOR			FLAG(5)
// make sure you don't make more than 32 flags!

#define NIBBLE_SHIFT(x)			((0x3 - x)<<2)			 // 0-3 * 4
#define NIBBLE_MASK(x)			(0xF << NIBBLE_SHIFT(x)) // invert nibble count

#define MORE_SIG_BYTE(x)		(x<<4);
#define LESS_SIG_BYTE(x)		(x>>4);

#define INPUT_KEY_0				0x0
#define INPUT_KEY_1				0x1
#define INPUT_KEY_2				0x2
#define INPUT_KEY_3				0x3
#define INPUT_KEY_4				0x4
#define INPUT_KEY_5				0x5
#define INPUT_KEY_6				0x6
#define INPUT_KEY_7				0x7
#define INPUT_KEY_8				0x8
#define INPUT_KEY_9				0x9
#define INPUT_KEY_A				0xA
#define INPUT_KEY_B				0xB
#define INPUT_KEY_C				0xC
#define INPUT_KEY_D				0xD
#define INPUT_KEY_E				0xE
#define INPUT_KEY_F				0xF

#define COLOR_STANDARD			1
#define COLOR_HIGHLIGHT			2
#define COLOR_EDIT				3
#define COLOR_TITLE				4
#define COLOR_EDITOR			5	// Status line will share background with the Editor
#define COLOR_COMMAND			6

#define ROWS_TITLE				1
#define ROWS_STATUS				1
#define ROWS_COMMAND			2
#define ROWS_EDIT(x)			(x - ROWS_TITLE - ROWS_COMMAND - ROWS_STATUS)

#define HALF_WIDTH(x)			(x>>1)

#define SAFE_DELETE_WINDOW(ptr)		if(ptr) delwin(ptr); ptr=NULL;

typedef unsigned int uint;

#endif