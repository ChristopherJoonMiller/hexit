/******************************\

hexit.cpp
140224

HexIt is a command line hex
viewer and editor.

\******************************/

#include "hexit.h"

#include <algorithm>

#define READ_BUFFER_BYTES		16
#define WORD_SIZE				 2

#define ASCII_MIN				' '
#define ASCII_MAX				'~'


// Flags for command line switches
#define FLAG(x)					(1<<x)
#define SWITCH_UPPER			FLAG(0)
#define SWITCH_OUTPUT			FLAG(1)
#define SWITCH_EDIT				FLAG(2)

#define NIBBLE_SHIFT(x)			((0x3 - x)<<2)			 // 0-3 * 4
#define NIBBLE_MASK(x)			(0xF << NIBBLE_SHIFT(x)) // invert nibble count

#define TEXT_COLOR_STANDARD		0
#define TEXT_COLOR_GREEN		1
#define TEXT_COLOR_RED			2
#define TEXT_COLOR_COUNT		3

#define TEXT_COLOR_HIGHLIGHT	TEXT_COLOR_GREEN
#define TEXT_COLOR_EDIT			TEXT_COLOR_RED

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

const char* g_colors[TEXT_COLOR_COUNT] =
{
	"\033[0m",
	"\033[1;32m",
	"\033[1;31m"
};

const char HEX_NIBBLE[0x10] =
{
	'0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'A', 'B',
	'C', 'D', 'E', 'F'
};

uint g_column_pos[0x10] =
{
	 0,  2,  5,  7,
	10, 12, 15, 17,
	20, 22, 25, 27,
	30, 32, 35, 37
};
#define COLUMN_POS(x) (10+g_column_pos[x])

HexIt::HexIt()
:	m_pFile(NULL)
,	m_bRunning(false)
,	m_bBufferDirty(false)
,	m_bPrintUpper(false)
,	m_uHeight(40)
,	m_uWidth(80)
,	m_uFilePos(0)
{

}

HexIt::HexIt(fstream* file)
:	m_bRunning(false)
,	m_bBufferDirty(false)
,	m_bPrintUpper(false)
,	m_uHeight(40)
,	m_uWidth(80)
,	m_uFilePos(0)
{
	m_pFile = file;

	// detect file size!
	if( file )
	{
		file->seekg(0, ios::end);
		m_uFileSize = file->tellg();
		file->seekg(0, ios::beg);
	}
}

HexIt::HexIt(HexIt* h) : m_bPrintUpper(false)
{
	*this = h;
}

bool HexIt::operator=(HexIt* h)
{
	if(h)
	{
		memcpy(this, h, sizeof(HexIt));
		return true;
	}
	return false;
}

void HexIt::print(ostream& output)
{
	if( m_pFile && m_pFile->is_open() )
	{
		m_pFile->seekg(0, ios::beg);

		char c[READ_BUFFER_BYTES+1] = {0}; // grab 16 bytes at a time, add 1 to store terminating null
		uint i;
		uint size = 0;
		uint bytes_read = READ_BUFFER_BYTES;

		// We need to check if we read all the bytes because we're manually
		// adding the file's stop byte to the sequence. If the number of bytes
		// is aligned to 16 byte width, it wouldn't print the stop byte
		for(i = 0; !m_pFile->eof() && bytes_read == READ_BUFFER_BYTES; i++)
		{
			memset(c,0x00,READ_BUFFER_BYTES);	// reset the buffer in case we don't get 16 bytes
			
			if( !m_pFile->eof() ) // keep going!
			{
				size  = m_pFile->tellg();
				m_pFile->read(c,READ_BUFFER_BYTES);

				bytes_read = m_pFile->gcount();		// see how much we actually got
				size += bytes_read;

				if( bytes_read < READ_BUFFER_BYTES )
				{
					c[bytes_read] = 0x0a;
					bytes_read += 1; // fake stop byte
					
				}
			}
			else // This means we read 16 bytes and hit the eof last round
			{
				bytes_read = 1; // fake stop byte
				c[0] = 0x0a;
			}

			renderLine(output, i<<4, c, bytes_read);
			
		}
		output << endl << "bytes: [ 0x" << getCaseFunction() << hex << size << " : " << dec << size << " ]" << endl;	
	}	
}

void HexIt::editMode()
{
	initNCurses();

	int rows, columns;
	getmaxyx(stdscr, rows, columns);

	//cout << "screen dimensions: " << rows << "x" << columns << endl;

	setTerminalSize(rows, columns);

	// we're editing so set running flag!
	m_bRunning = true;

	if( m_pFile && m_pFile->is_open() )
	{
		// initialize edit state
		m_bBufferDirty = false;

		m_pFile->seekg(0, ios::beg);
		m_uFilePos = 0;

	    m_buffer << m_pFile->rdbuf();
	    m_buffer.seekg(0, ios::beg);
		// load the screen up with data!
		while(m_bRunning)
		{
			// reset cursor position
			move(0,0); // render from the top!

			stringstream screenBuffer;
			renderScreen(screenBuffer);

			// output screen to ncurses
			addstr(screenBuffer.str().c_str());

			// set cursor position
			setCursorPos();

			// accept input
			int ch = getch();


			switch(ch)
			{
				case 'q':
					m_bRunning = false;
					break;
				case KEY_DOWN:
					if(!m_cursor.editing)
						moveCursor(0,1<<4);
					break;
				case KEY_UP:
					if(!m_cursor.editing)				
						moveCursor(0,-1<<4);
					break;
				case KEY_LEFT:
					if(m_cursor.editing)
						moveNibble(-1);
					else
						moveCursor(-2,0);
					break;
				case KEY_RIGHT:
					if(m_cursor.editing)
						moveNibble(1);
					else
						moveCursor(2,0);
					break;
				case '\n':
				case KEY_ENTER:
					toggleEdit(true);
					break;
				case 27: // escape
					toggleEdit(false);
					break;

				// Editing Keys
				case '0':
					editKey(0x0);
					break;
				case '1':
					editKey(0x1);
					break;
				case '2':
					editKey(0x2);
					break;
				case '3':
					editKey(0x3);
					break;
				case '4':
					editKey(0x4);
					break;
				case '5':
					editKey(0x5);
					break;
				case '6':
					editKey(0x6);
					break;
				case '7':
					editKey(0x7);
					break;
				case '8':
					editKey(0x8);
					break;
				case '9':
					editKey(0x9);
					break;
				case 'a':
				case 'A':
					editKey(0xA);
					break;
				case 'b':
				case 'B':
					editKey(0xA);
					break;
				case 'c':
				case 'C':
					editKey(0xA);
					break;
				case 'd':
				case 'D':
					editKey(0xA);
					break;
				case 'e':
				case 'E':
					editKey(0xA);
					break;
				case 'f':
				case 'F':
					editKey(0xA);
					break;
			}
		}
	}

	cleanup();
}

case_fptr HexIt::getCaseFunction()
{
	return (m_bPrintUpper ? uppercase : nouppercase );
}

// string HexIt::textColor(uint byte_pos, char byte_data)
// {
// 	stringstream text;
// 	text << hex;

// 	// determine if we're at the cursor byte
// 	if( byte_pos - m_cursor.word <= 1 )
// 	{
// 		if( m_cursor.editing )
// 		{
// 			char first;
// 			char second;

// 			stringstream hexword;
// 			hexword << hex << setw(2) << setfill('0') << (byte_data & 0xFF);
// 			hexword.read(&first,1);
// 			hexword.read(&second,1);
			
// 			// byte pos is == to m_cursor.word, or m_cursor.word+1
// 			if( byte_pos == m_cursor.word ) // editing this byte
// 			{
// 				if(m_cursor.nibble == 0)
// 				{
// 					text << g_colors[TEXT_COLOR_EDIT] << first << g_colors[TEXT_COLOR_HIGHLIGHT] << second;
// 				}
// 				else if(m_cursor.nibble == 1)
// 				{
// 					text << g_colors[TEXT_COLOR_HIGHLIGHT] << first << g_colors[TEXT_COLOR_EDIT] << second;
// 				}
// 				else
// 				{
// 					text << g_colors[TEXT_COLOR_HIGHLIGHT] << first << second;
// 				}
// 			}
// 			else
// 			{
// 				if(m_cursor.nibble == 2)
// 				{
// 					text << g_colors[TEXT_COLOR_EDIT] << first << g_colors[TEXT_COLOR_HIGHLIGHT] << second;
// 				}
// 				else if(m_cursor.nibble == 3)
// 				{
// 					text << g_colors[TEXT_COLOR_EDIT] << first << g_colors[TEXT_COLOR_HIGHLIGHT] << second;
// 				}
// 				else
// 				{
// 					text << g_colors[TEXT_COLOR_HIGHLIGHT] << first << second;
// 				}
// 			}
// 		}
// 		else
// 		{
// 			text << g_colors[TEXT_COLOR_HIGHLIGHT] << setw(2) << setfill('0') << (byte_data & 0xFF) << g_colors[TEXT_COLOR_STANDARD];
// 		}
		
// 	}
// 	else
// 	{
// 		text << g_colors[TEXT_COLOR_STANDARD] << setw(2) << setfill('0') << (byte_data & 0xFF);
// 	}
// 	text << g_colors[TEXT_COLOR_STANDARD];
// 	return text.str();
// }

void HexIt::renderLine(ostream& output, uint start_byte, char* byte_seq, uint bytes_read)
{
	// put hex byte count to start the row
	// todo, predictively set the width of the row header
	output << hex << getCaseFunction() << setw(7) << setfill('0') << start_byte << ": ";

	// output each byte
	for(int j = 0; j < (uint)READ_BUFFER_BYTES; j++)
	{
		if(!(j&1)) // every even byte index put a seperator
		{
			output << " ";
		}

		// each byte will print 2 (padded) hex chars
		if(j < bytes_read)
		{
			char toWrite = byte_seq[j];

			// test if we're editing this byte, if so use the edit word instead
			// 11 10 01 00 account for the nibble bytes
			if( m_cursor.editing && (m_cursor.word + (m_cursor.nibble >> 1) == start_byte + j) )
			{
				// pull out individual bytes
				toWrite = ((char*)&m_cursor.editWord)[j & 0x3];
			}
			output << hex << getCaseFunction() << setw(2) << setfill('0') << (toWrite & 0xFF);
		}
		else
		{
			output << "  ";			// blank if not read
			byte_seq[j] = ' ';		// blank if not read
		}

		// adjust ascii for control chars or anything out of bounds
		if(byte_seq[j] < ASCII_MIN || byte_seq[j] > ASCII_MAX)
		{
			byte_seq[j] = '.';
		}
	}
	
	output << "  ; " << byte_seq << " ;"; // output ascii straight onto the end!
	output << endl;
}

void HexIt::renderScreen(ostream& output)
{
	char c[READ_BUFFER_BYTES+1] = {0}; // grab 16 bytes at a time, add 1 to store terminating null
	uint bytes_read = READ_BUFFER_BYTES;

	//for( uint byte = m_uFilePos; byte <= m_uFileSize; byte += 16)
	for( uint line = 0; line < m_uHeight; line++)
	{
		uint byte = m_uFilePos + line * 16;

		if( byte < m_uFileSize )
		{
			m_buffer.seekg(byte, ios::beg);
			memset(c,0x00,READ_BUFFER_BYTES);	// reset the buffer in case we don't get 16 bytes
			
			if( !m_buffer.eof() ) // keep going!
			{
				m_buffer.read(c,READ_BUFFER_BYTES);
				bytes_read = m_buffer.gcount();		// see how much we actually got

				if( bytes_read < READ_BUFFER_BYTES )
				{
					c[bytes_read] = 0x0a;
					bytes_read += 1; // fake stop byte
				}
			}
			else // This means we read 16 bytes and hit the eof last round
			{
				bytes_read = 1; // fake stop byte
				c[0] = 0x0a;
			}
			renderLine(output, byte, &c[0], bytes_read);
		}
		else
		{
			output << endl;
		}

		
	}
}

void HexIt::setCursorPos(uint _word, uint _nibble)
{
	m_cursor.word = _word;
	m_cursor.nibble = _nibble;
}

// no parameters means set it with ncurses to the window
void HexIt::setCursorPos()
{
	// calculate the row and column by comparing m_uFilePos and m_cursor.word
	assert(m_cursor.word >= m_uFilePos);

	uint nibble = (m_cursor.editing ? m_cursor.nibble : 0);
	uint row = getCursorRow();
	uint col = COLUMN_POS(getCursorColumn()) + nibble;

	move(row, col);
	//move(0,0);
}

uint HexIt::getCursorRow()
{
	
	uint offset = m_cursor.word - m_uFilePos;
	uint row = offset >> 4; // how many rows of bytes before the cursor.
	
	return row;
}

uint HexIt::getCursorColumn()
{

	uint offset = m_cursor.word - m_uFilePos;
	uint column = offset & 0xF;
	
	return column;
}

void HexIt::checkCursorOffscreen()
{
	while(m_cursor.word > m_uFilePos + (m_uHeight<<4))
	{
		m_uFilePos += 16;
	}
	while(m_cursor.word < m_uFilePos)
	{
		m_uFilePos -= 16;
	}
}

void HexIt::moveNibble(int x)
{
	m_cursor.nibble = (m_cursor.nibble + x) & 0x3;
}

void HexIt::moveCursor(int x, int y)
{
	int newX = m_cursor.word + x;
	int newY = m_cursor.word + y;
	if(x < 0)
	{
		if( newX >= 0 )
		{
			m_cursor.word += x;
		}
	}
	else if (x > 0)
	{
		if( newX > m_uFileSize )
		{
			// increase file size now!
		}
		m_cursor.word += x;
	}

	if(y < 0)
	{
		if( newY >= 0 )
		{
			m_cursor.word += y;
		}
	}
	else if (y > 0)
	{
		if( newY > m_uFileSize )
		{
			// increase file size now!
		}
		m_cursor.word += y;
	}
	checkCursorOffscreen();
	setCursorPos();
}

void HexIt::toggleEdit(bool save)
{
	if(!m_cursor.editing)
	{
		// initiating an edit, save the current word;
		m_cursor.backup = 0;

		// reset to the first nibble
		m_cursor.nibble = 0;

		// because bytes are read one at a time into "chars"
		// don't read the bytes directly into m_cursor.backup
		char read_buf[WORD_SIZE];

		m_buffer.seekg(m_cursor.word, ios::beg);
		m_buffer.read(read_buf,WORD_SIZE); // read 4 nibbles, or 2 bytes

		// store the two bytes back into a word inside the lower 16 bytes in m_cursor.backup
		// throw away the 16 most sig bits.
		m_cursor.backup = (((read_buf[0] & 0xFF) << 8) | (read_buf[1] & 0xFF)) & 0xFFFF;
		m_buffer.seekg(m_uFilePos);

		// initialize the edit word to be the same as what's in the edit buffer
		m_cursor.editWord = m_cursor.backup;

	}
	else // we are ending an edit
	{
		// don't bother if we didn't change anything
		if( m_cursor.backup != m_cursor.editWord )
		{
			m_buffer.seekg(m_cursor.word, ios::beg);
			wchar_t restore = ( save ? m_cursor.editWord : m_cursor.backup ) & 0xFFFF;
			m_buffer.write((char*)&restore,WORD_SIZE);
		}
		// if we pressed escape or something reset the word
		

	}
	m_cursor.editing = !m_cursor.editing;
}

void HexIt::editKey(uint nibble)
{
	// are we already editing?
	if( !m_cursor.editing )
	{
		toggleEdit(false);
	}

	// to put the new nibble in place, first clear all bits
	uint mask = NIBBLE_MASK(m_cursor.nibble);
	m_cursor.editWord &= ~mask;

	// then |= with the nibble itself.
	uint shift = NIBBLE_SHIFT(m_cursor.nibble);
	m_cursor.editWord |= (nibble << shift);

	// advance the nibble pointer
	m_cursor.nibble++;
	m_cursor.nibble &= 0x3; // % 4

	// print the last input nibble to the dirty screen
	addch(HEX_NIBBLE[nibble]);
}

void HexIt::initNCurses()
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
}

void HexIt::cleanup()
{
	//clear the screen
	move(0,0);
	for(uint y = 0; y < m_uHeight; y++)
		for(uint x = 0; x < m_uWidth; x++)
			addch(' ');
		addch('\n');
	endwin();
}

// Global Functions!
void usage()
{
	cout << "\nUsage:\n";
	cout << "  hexit file [-h][-e][-o output]\n\n";
	cout << "  -h: display this help dialog\n";
	cout << "  -e: edit mode instead of outputting to stdout\n";
	cout << "  -o: supply a filename to output ASCII HEX to\n";
	cout << "  -u: uppercase the hexadecimal output\n";
	cout << endl;
}

// Main Application
int main(int argc, char *argv[])
{
	

	uint switches = 0;
	string output_fn;

	cout << endl << "HexIt" << " v0.1" << endl << endl;

	if(argc <= 1)
	{
		
		usage();
		return 0;
	}

	// parse switches
	for(int i = 2; i < argc; i++)
	{
		if(!strcmp(argv[i],"-e"))
		{
			switches |= SWITCH_EDIT;
		}
		else if(!strcmp(argv[i],"-h"))
		{
			usage();
			return 0;
		}
		else if(!strcmp(argv[i],"-u"))
		{
			switches |= SWITCH_UPPER;
		}
		else if(!strcmp(argv[i],"-o"))
		{
			if( i+1 == argc )
			{
				usage();
				return 1;
			}
			switches |= SWITCH_OUTPUT;
			output_fn.assign(argv[i+1]);
			i++; // skip the output name
		}
		
	}

	cout << "Opening file: " << argv[1] << endl << endl;

	// open the file as read/write
	fstream file;
	file.open(argv[1]);

	HexIt h(&file);

	// Does the user want uppercase letters?
	h.setPrintCase(switches & SWITCH_UPPER);
	
	// are we editing
	if( switches & SWITCH_EDIT )
	{
		h.editMode();

		// check if we need to save anything
	}
	else // output to a stream
	{
		if( switches & SWITCH_OUTPUT )		// file
		{
			cout << "Outputting file: " << output_fn << endl << endl;
			ofstream output_stream;
			output_stream.open(output_fn.c_str());
			h.print(output_stream);
			output_stream.close();
		}
		else								// stdout
		{
			h.print(cout);	
		}
	}

	return 0;
}

