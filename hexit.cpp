/******************************\
 
 hexit.cpp
 140224
 
 HexIt is a command line hex
 viewer and editor.
 
 \******************************/

#include "hexit.h"

#include <algorithm>

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
#define COLUMN_POS(x) ((m_bShowByteCount?getByteCountWidth()+3:0)+g_column_pos[x])

HexIt::HexIt()
:	m_pFile(NULL)
,	m_bRunning(false)
,	m_bBufferDirty(false)
,	m_uFilePos(0)
,	m_bShowColor(false)
,	m_bPrintUpper(false)
,	m_bShowByteCount(true)
,	m_bShowASCII(true)
,	m_uInsertWord(0)
,	m_uByteCountWidth(4)
{
	sprintf(m_appVersion, "HexIt v%d.%d", VERSION_MAJOR, VERSION_MINOR);
	m_outputFilename[0] = 0;
}

HexIt::HexIt(char* filename)
:   m_bRunning(false)
,	m_bBufferDirty(false)
,	m_uFilePos(0)
,	m_bShowColor(false)
,	m_bPrintUpper(false)
,	m_bShowByteCount(true)
,	m_bShowASCII(true)
,	m_uInsertWord(0)
,	m_uByteCountWidth(4)
{
	sprintf(m_appVersion, "HexIt v%d.%d", VERSION_MAJOR, VERSION_MINOR);
	m_outputFilename[0] = 0;
	
	m_pFile = new fstream();
	if(m_pFile)
	{
		m_pFile->open(filename);
		
		// detect file size!
		if( m_pFile->is_open() )
		{
			strcpy(m_inputFilename, filename);
			m_pFile->seekg(0, ios::end);
			m_uFileSize = (uint)m_pFile->tellg();
			m_pFile->seekg(0, ios::beg);
		}
	}
}

HexIt::HexIt(HexIt* h) : m_bPrintUpper(false)
{
	*this = h;
}

HexIt::~HexIt()
{
	if( m_pFile )
	{
		m_pFile->close();
		delete m_pFile;
		m_pFile = NULL;
	}
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
				size = (uint)m_pFile->tellg();
				m_pFile->read(c,READ_BUFFER_BYTES);
				
				bytes_read = (uint)m_pFile->gcount();		// see how much we actually got
				size += bytes_read;
				
				// if( bytes_read < READ_BUFFER_BYTES )
				// {
				// 	c[bytes_read] = 0x0a;
				// 	bytes_read += 1; // fake stop byte
					
				// }
			}
			// else // This means we read 16 bytes and hit the eof last round
			// {
			// 	bytes_read = 1; // fake stop byte
			// 	c[0] = 0x0a;
			// }
			
			renderLine(output, i<<4, c, bytes_read);
			
		}
		output << endl << "bytes: [ 0x" << getCaseFunction() << hex << size << " : " << dec << size << " ]" << endl;
	}
}

void HexIt::editMode()
{
	editInit();
	
	setTerminalSize();

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
		
		// initialize the screen
		renderScreen();
		
		while(m_bRunning)
		{
			// output the screen's text
			renderScreen();
			renderScreen();
			
			// accept input
			TermKeyResult ret = TERMKEY_RES_NONE;
			TermKeyKey key;
			
			// check for input
			ret = termkey_waitkey(m_tk, &key);
			
			// so far the only error I know of is sigwinch
			// we'll see if this turns out to be a problem
			if( ret == TERMKEY_RES_ERROR)
			{
				setTerminalSize();
			}
			
			if( ret != TERMKEY_RES_KEY )
				continue;
			
			if( key.type == TERMKEY_TYPE_KEYSYM) // a symbol key
			{
				switch(key.code.sym)
				{
				case TERMKEY_SYM_DOWN:
					if(!m_cursor.editing)
						moveCursor(0,ROW_SIZE);
					break;
				case TERMKEY_SYM_UP:
					if(!m_cursor.editing)
						moveCursor(0,-ROW_SIZE);
					break;
				case TERMKEY_SYM_LEFT:
					if(m_cursor.editing)
						moveNibble(-1);
					else
						moveCursor(-WORD_SIZE,0);
					break;
				case TERMKEY_SYM_RIGHT:
					if(m_cursor.editing)
						moveNibble(1);
					else
						moveCursor(WORD_SIZE,0);
					break;
				 case TERMKEY_SYM_ENTER:
					 toggleEdit(true);
					 break;
				 case TERMKEY_SYM_ESCAPE:
					if(m_cursor.editing)
						toggleEdit(false);
					break;
				 default:
					break;
				}
			}
			else if( key.type == TERMKEY_TYPE_UNICODE ) // a letter was pressed
			{
				if(key.modifiers & TERMKEY_KEYMOD_CTRL) // COMMAND KEYS
				{
					switch(key.code.codepoint)
					{
						// Commands
						case 'b':
						case 'B':
							cmdPageDn();
							break;

						case 'c':
						case 'C':
							if(!m_cursor.editing)
							{
								cmdCopyWord();
							}
							break;

						case 'f':
							cmdFillWord();
							break;
						case 'F':
							cmdSetCursorWordAt();
							cmdFillWord();
							break;

						case 'i':
							cmdInsertWord();
							break;
						case 'I':
							cmdSetCursorWordAt();
							cmdInsertWord();
							break;

						case 'o':
						case 'O':
							cmdOutputFile();
							break;
						
						case 'w':
						case 'W':
							cmdFindWord();
							break;

						case 'x':
						case 'X':
							m_bRunning = false;
							cmdCloseFile();
							break;

						case 'y':
						case 'Y':
							cmdPageUp();
							break;
						
						case 'v':
						case 'V':
							if(!m_cursor.editing)
							{
								toggleEdit(true);
								cmdPasteWord();
							}
							break;
						default:
							break;
					}
				}
				else
				{
					switch(key.code.codepoint)
					{
						// Editing Keys
						case '0':
							editKey(INPUT_KEY_0);
							break;
						case '1':
							editKey(INPUT_KEY_1);
							break;
						case '2':
							editKey(INPUT_KEY_2);
							break;
						case '3':
							editKey(INPUT_KEY_3);
							break;
						case '4':
							editKey(INPUT_KEY_4);
							break;
						case '5':
							editKey(INPUT_KEY_5);
							break;
						case '6':
							editKey(INPUT_KEY_6);
							break;
						case '7':
							editKey(INPUT_KEY_7);
							break;
						case '8':
							editKey(INPUT_KEY_8);
							break;
						case '9':
							editKey(INPUT_KEY_9);
							break;
						case 'a':
						case 'A':
							editKey(INPUT_KEY_A);
							break;
						case 'b':
						case 'B':
							editKey(INPUT_KEY_B);
							break;
						case 'c':
						case 'C':
							editKey(INPUT_KEY_C);
							break;
						case 'd':
						case 'D':
							editKey(INPUT_KEY_D);
							break;
						case 'e':
						case 'E':
							editKey(INPUT_KEY_E);
							break;
						case 'f':
						case 'F':
							editKey(INPUT_KEY_F);
							break;
						default:
							break;
					}
				}
			}
		}
	}
	
	editCleanup();
}

void HexIt::setSwitches(uint switches)
{
	m_bShowColor     = switches & SWITCH_COLOR;
	m_bPrintUpper 	 = switches & SWITCH_UPPER;
	m_bShowByteCount = switches & SWITCH_SHOW_BYTE_COUNT;
	m_bShowASCII 	 = switches & SWITCH_SHOW_ASCII;
}

case_fptr HexIt::getCaseFunction()
{
	return (m_bPrintUpper ? uppercase : nouppercase );
}

void HexIt::textColor(uint byte_pos, char byte_data)
{
	stringstream text;
	text << hex;

	if( !m_bShowColor )
	{
		text << getCaseFunction() << setw(2) << setfill('0') << (byte_data & 0xFF);
		// ncurses
		waddstr(m_wEditArea, text.str().c_str());
		return;
	}

	// determine if we're at the cursor byte
	if( (byte_pos & 0xFFFFFFFE) == m_cursor.word )
	{
		if( m_cursor.editing )
		{
			text << getCaseFunction() << setw(2) << setfill('0') << (byte_data & 0xFF);
			// ncurses
			wattron(m_wEditArea, COLOR_PAIR(COLOR_EDIT));
			waddstr(m_wEditArea, text.str().c_str());					
			wattroff(m_wEditArea, COLOR_PAIR(COLOR_EDIT));
		}
		else
		{
			text << getCaseFunction() << setw(2) << setfill('0') << (byte_data & 0xFF);
			// ncurses
			wattron(m_wEditArea, COLOR_PAIR(COLOR_HIGHLIGHT));
			waddstr(m_wEditArea, text.str().c_str());
			wattroff(m_wEditArea, COLOR_PAIR(COLOR_HIGHLIGHT));
		}

	}
	else
	{
		text << getCaseFunction() << setw(2) << setfill('0') << (byte_data & 0xFF);
		// ncurses
		wattron(m_wEditArea, COLOR_PAIR(COLOR_EDITOR));
		waddstr(m_wEditArea, text.str().c_str());
		wattroff(m_wEditArea, COLOR_PAIR(COLOR_EDITOR));
	}
}

void HexIt::setTerminalSize()
{
	// we have 4 window areas, they all span the entire window's columns
	// Title Area
	// Editing area
	// Status Area
	// Command Area
	SAFE_DELETE_WINDOW(m_wTitleArea);
	SAFE_DELETE_WINDOW(m_wEditArea);
	SAFE_DELETE_WINDOW(m_wStatusArea);
	SAFE_DELETE_WINDOW(m_wCommandArea);

	// you need to delete subwindows first to get an
	// accurate measurement from stdscr, don't ask me why
	int rows, columns;
	getmaxyx(stdscr, rows, columns);

	m_uWidth = (uint)columns;
	m_uHeight = ROWS_EDIT((uint)rows);
	
	m_wTitleArea 	= newwin(ROWS_TITLE,		m_uWidth, 0,			0);
	m_wEditArea 	= newwin(m_uHeight,			m_uWidth, 1,			0);
	m_wStatusArea	= newwin(ROWS_STATUS,		m_uWidth, m_uHeight+1,	0);
	m_wCommandArea	= newwin(ROWS_COMMAND,		m_uWidth, m_uHeight+2,	0);
	
	wclear(m_wTitleArea);
	wclear(m_wEditArea);
	wclear(m_wStatusArea);
	wclear(m_wCommandArea);

	wbkgd(m_wTitleArea,		COLOR_PAIR(COLOR_TITLE));
	wbkgd(m_wEditArea,		COLOR_PAIR(COLOR_EDITOR));
	wbkgd(m_wStatusArea,	COLOR_PAIR(COLOR_EDITOR));
	wbkgd(m_wCommandArea,	COLOR_PAIR(COLOR_COMMAND));
}

void HexIt::renderStatus(const string& message, uint y, uint x)
{
	wclear(m_wStatusArea);
	wmove(m_wStatusArea,y,x);

	if( message.length() )
	{
		waddstr(m_wStatusArea, message.c_str() );
	}
}

void HexIt::renderCommand(const string& message, uint y, uint x)
{
	wclear(m_wCommandArea);
	wmove(m_wCommandArea,y,x);

	if( message.length() )
	{
		waddstr(m_wCommandArea, message.c_str() );
	}
}
void HexIt::renderLine(ostream& output, uint start_byte, char* byte_seq, uint bytes_read)
{
	// put hex byte count to start the row
	// todo, predictively set the width of the row header
	output << hex << getCaseFunction() << setw(7) << setfill('0') << start_byte << ": ";
	
	// output each byte
	for(uint j = 0; j < (uint)READ_BUFFER_BYTES; j++)
	{
		if(!(j&1)) // every even byte index put a seperator
		{
			output << " ";
		}
		
		// each byte will print 2 (padded) hex chars
		if(j < bytes_read)
		{
			char toWrite = byte_seq[j];

			// test if we're editing this word, if so use the edit word instead
			// remove the last 2 bits from j so that both bytes we're editing are replaced
			if( m_cursor.editing && 
				m_cursor.word == ( start_byte + (j & (0xFFFC) ) )
			  )
			{
				// pull out individual byte
				toWrite = ((m_cursor.editWord >> ((1-(j&1))*8)) & 0xFF);
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

void HexIt::renderScreen()
{
	wbkgd(m_wTitleArea,		COLOR_PAIR(COLOR_TITLE));
	wbkgd(m_wEditArea,		COLOR_PAIR(COLOR_EDITOR));
	wbkgd(m_wStatusArea,	COLOR_PAIR(COLOR_EDITOR));
	wbkgd(m_wCommandArea,	COLOR_PAIR(COLOR_COMMAND));

	// Render Title Area
	wmove(m_wTitleArea, 0, 2);
	// App Name
	waddch(m_wTitleArea,' ');
	waddstr(m_wTitleArea, m_appVersion);
	// File Name
	uint centered = HALF_WIDTH(m_uWidth) - HALF_WIDTH((uint)(strlen(m_inputFilename) + 6)); // + 6 for "File: "
	wmove(m_wTitleArea, 0, centered);
	waddstr(m_wTitleArea, "File: ");
	waddstr(m_wTitleArea, m_inputFilename);
	
	// Render all output
	stringstream output;

	// Modified?
	if(m_bBufferDirty)
	{
		wmove(m_wTitleArea, 0, m_uWidth - 11);
		waddstr(m_wTitleArea, "Modified");
	}

	
	// Render Edit Area
	char c[READ_BUFFER_BYTES+1] = {0}; // grab 16 bytes at a time, add 1 to store terminating null
	uint bytes_read = READ_BUFFER_BYTES;

	//for( uint byte = m_uFilePos; byte <= m_uFileSize; byte += 16)
	for( uint line = 0; line < m_uHeight; line++)
	{
		wmove(m_wEditArea, line, 0);

		uint byte = m_uFilePos + line * 16;
		
		if( byte < m_uFileSize )
		{
			memset(c,0x00,READ_BUFFER_BYTES);	// reset the buffer in case we don't get 16 bytes
			
			m_buffer.seekg(byte, ios::beg);
			
			if(byte+READ_BUFFER_BYTES >= m_uFileSize)
			{
				m_buffer.read(c,m_uFileSize - byte);
			}
			else
			{
				m_buffer.read(c,READ_BUFFER_BYTES);
			}
			
			bytes_read = (uint)m_buffer.gcount();		// see how much we actually got
			
			// if( bytes_read < READ_BUFFER_BYTES )
			// {
			//     c[bytes_read] = 0x0a;
			//     bytes_read += 1; // fake stop byte
			// }
			
			//renderLine(output, byte, &c[0], bytes_read);
			stringstream output;
			

			if(m_bShowByteCount) 
			{
				wattron(m_wEditArea, COLOR_PAIR(COLOR_EDITOR));
				output << hex << getCaseFunction() << setw(getByteCountWidth()) << setfill('0') << byte << ": ";
				// ncurses
				waddstr(m_wEditArea, output.str().c_str());
				output.str("");
				wattroff(m_wEditArea, COLOR_PAIR(COLOR_EDITOR));
			}

			// output each byte
		
			for(uint j = 0; j < (uint)READ_BUFFER_BYTES; j++)
			{
				if(!(j&1)) // every even byte index put a seperator
				{
					if(!(!m_bShowByteCount && j==0))
					{
						// ncurses
						wattron(m_wEditArea, COLOR_PAIR(COLOR_EDITOR));
						waddch(m_wEditArea, ' ');
						wattroff(m_wEditArea, COLOR_PAIR(COLOR_EDITOR));
					}
				}
				
				// each byte will print 2 (padded) hex chars
				if(j < bytes_read)
				{
					char toWrite = c[j];
					
					// test if we're editing this word, if so use the edit word instead
					// remove the last bit from j so that both bytes we're editing are replaced
					if( m_cursor.editing &&
						m_cursor.word == (byte + (j & (0xFFFE))) )
					{
						// pull out individual byte
						//toWrite = ((char*)&m_cursor.editWord)[j & 0x3];
						toWrite = ((m_cursor.editWord >> ((1-(j&1))*8)) & 0xFF);
					}
					textColor(byte+j, toWrite);
				}
				else
				{
					// ncurses
					waddch(m_wEditArea, ' '); waddch(m_wEditArea, ' ');
					c[j] = ' ';		// blank if not read
				}
				
				// adjust ascii for control chars or anything out of bounds
				if(c[j] < ASCII_MIN || c[j] > ASCII_MAX)
				{
					c[j] = '.';
				}
			}
			
			if( m_bShowASCII )
			{
				wattron(m_wEditArea, COLOR_PAIR(COLOR_EDITOR));
				output << "  ; " << c << " ;"; // output ascii straight onto the end!
				output << endl;

				// ncurses
				waddstr(m_wEditArea, output.str().c_str());
				output.str("");
				wattroff(m_wEditArea, COLOR_PAIR(COLOR_EDITOR));
			}	
		}
		else
		{
			// ncurses
			waddch(m_wEditArea, '\n');
		}
	}

	setCursorPos();
	
	// Render Status Area
	output << "BYTE " << getCaseFunction() << hex << setw(getByteCountWidth()) << setfill('0') << m_cursor.word;
	renderStatus(output.str(),0, m_uWidth - getByteCountWidth() - 6 );

	// Render Command Area
	output.str("");

	output << "^C Copy Word    ^Y Prev Page   ^F Fill Word   ^I Insert Word" << endl;
	output << "^V Paste Word   ^B Next Page   ^W Where Is    ^O Write Out     ^X Exit";
	
	renderCommand(output.str(),0,0);
	output << "Please Enter a Word Position: \n[Default: " << getCaseFunction() << hex << m_cursor.word << "] ";

	// update to screen
	wrefresh(m_wTitleArea);
	wrefresh(m_wStatusArea);
	wrefresh(m_wCommandArea);
	wrefresh(m_wEditArea); // refresh edit last so the cursor goes there
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
	
	wmove(m_wEditArea, row, col);
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
	while(m_cursor.word >= m_uFilePos + HEIGHT_TO_PAGE_SIZE(m_uHeight) || m_cursor.word > m_uFileSize)
	{
		m_uFilePos = min( m_uFilePos + 0x10, maxFilePos());
	}
	while(m_cursor.word < m_uFilePos)
	{
		m_uFilePos = (m_uFilePos >= 0x10 ? m_uFilePos - 0x10 : 0);
	}
}

void HexIt::moveNibble(int x)
{
	m_cursor.nibble = (m_cursor.nibble + x) & 0x3;
}

uint HexIt::maxFilePos()
{
	// if the fileSize is less than one page of hex, don't ever advance the file pos
	if( m_uFileSize < HEIGHT_TO_PAGE_SIZE(m_uHeight) )
	{
		return 0;
	}
	
	// otherwise the max file pos is the end of the file's page start.
	
	// take the file size and clear the least significant 4 bits to get
	// the start byte of the the last line's 16 byte sequence
	uint start_byte = m_uFileSize & ~(0xF);
	
	// subtract one page of hex (not including the final line)
	start_byte -= HEIGHT_TO_PAGE_SIZE((m_uHeight-1));

	return start_byte;
}

void HexIt::updateByteCountWidth()
{
	// see how many hex digits are required to show the file position
	stringstream width;
	width << hex << m_uFileSize;
	m_uByteCountWidth = (uint)width.str().length();
}

uint HexIt::getByteCountWidth()
{
	
	return m_uByteCountWidth;
}

void HexIt::moveCursor(int x, int y)
{
	int newX = m_cursor.word + x;
	int newY = m_cursor.word + y;
	
	// don't allow someone to scroll past the end
	if( (x < 0 && newX >= 0) ||
		(x > 0 && newX < m_uFileSize) )
	{
		m_cursor.word = newX;
	}
	
	if( (y < 0 && newY >= 0) ||
		(y > 0 && newY < m_uFileSize) )
	{
		m_cursor.word = newY;
	}
	else if( y == -HEIGHT_TO_PAGE_SIZE(m_uHeight) && newY < 0 )
	{
		m_cursor.word = 0;
	}
	else if( y == HEIGHT_TO_PAGE_SIZE(m_uHeight) && newY > m_uFileSize )
	{
		// override a page down past the end and just go to the last word
		newY = m_uFileSize & ~(0xF);
		while( newY + 2 < m_uFileSize )
			newY += 2;
		m_cursor.word = newY;
	}

	// first adjust file pos
	checkCursorOffscreen();
	
	// then adjust cursor on screen or not
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
			std::stringbuf *pbuf = m_buffer.rdbuf();
			if( pbuf->pubseekpos(m_cursor.word) == m_cursor.word )
			{
				// if we pressed escape or something reset the word
				uint16_t restore = ( save ? m_cursor.editWord : m_cursor.backup ) & 0xFFFF;
				// because we're putting chars and not 16 bytes at once we need to reverse
				// the order we write them to maintain endianness
				pbuf->sputc(((char*)&restore)[1]);
				pbuf->sputc(((char*)&restore)[0]);
			}
			
			// if we're not saving we restored the stream, so it's not an edit
			// but don't clear the dirty flag when not editing, only on save!
			if(save)
				m_bBufferDirty = true;
		}
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
	// addch(HEX_NIBBLE[nibble]);
}

void HexIt::cmdPageUp()
{
	moveCursor(0,-HEIGHT_TO_PAGE_SIZE(m_uHeight));
}

void HexIt::cmdPageDn()
{
	moveCursor(0,HEIGHT_TO_PAGE_SIZE(m_uHeight));
}

void HexIt::cmdCopyWord()
{
	m_buffer.seekg(m_cursor.word, ios::beg);
	m_buffer.read(m_uCopyWord,WORD_SIZE); // read 4 nibbles, or 2 bytes
}

void HexIt::cmdFindWord()
{

}

void HexIt::cmdFillWord()
{

}

void HexIt::cmdInsertWord()
{
	updateByteCountWidth(); // just in case
}

void HexIt::cmdSetCursorWordAt()
{
	// ask user for position
	stringstream output;
	output << "Please Enter a Word Position: \n[Default: " << getCaseFunction() << hex << m_cursor.word << "] ";
	renderCommand(output.str());

	//
}

void HexIt::cmdOutputFile()
{

}

void HexIt::cmdCursorWord()
{

}

void HexIt::cmdCloseFile()
{

}

void HexIt::cmdPasteWord()
{
	// enter "edit mode" so that we store a backup of the cursor location
	toggleEdit(true);

	// now assign the editWord to be the copy byte

}


void HexIt::ncursesInit()
{
	initscr();
	raw();
	noecho();
	keypad(stdscr, TRUE);

	// debug getch to wait for debugger!
	//getch();

	// if we don't have color ability then it's off regardless
	if(!has_colors())
		m_bShowColor = false;

	// this should be defaulted to on now
	if(m_bShowColor)
	{
		start_color();

		/*
			ncurses colors
				COLOR_BLACK   0
				COLOR_RED     1
				COLOR_GREEN   2
				COLOR_YELLOW  3
				COLOR_BLUE    4
				COLOR_MAGENTA 5
				COLOR_CYAN    6
				COLOR_WHITE   7
		*/
		// init your color pairs! FG, BG
		init_pair(COLOR_STANDARD,		COLOR_WHITE,		COLOR_BLACK);
		init_pair(COLOR_HIGHLIGHT,		COLOR_BLACK,		COLOR_WHITE);
		init_pair(COLOR_EDIT,			COLOR_RED,			COLOR_WHITE);

		init_pair(COLOR_TITLE,			COLOR_BLUE,			COLOR_WHITE);
		init_pair(COLOR_EDITOR,			COLOR_WHITE,		COLOR_BLUE);
		init_pair(COLOR_COMMAND,		COLOR_BLUE,		COLOR_WHITE);
	}
}

void HexIt::termkeyInit()
{
	// let's init termkey now
	m_tk = termkey_new(0, 0);
	termkey_set_flags(m_tk, TERMKEY_FLAG_EINTR); // need this to respond to sigwinch
}

void HexIt::editInit()
{
	updateByteCountWidth();
	termkeyInit();
	ncursesInit();
}

void HexIt::termkeyCleanup()
{
	// delete termkey object!
	if( m_tk )
	{
		termkey_destroy(m_tk);
		m_tk = NULL;
	}
}

void HexIt::ncursesCleanup()
{
	wclear(m_wTitleArea);
	wclear(m_wEditArea);
	wclear(m_wStatusArea);
	wclear(m_wCommandArea);
	
	SAFE_DELETE_WINDOW(m_wTitleArea);
	SAFE_DELETE_WINDOW(m_wEditArea);
	SAFE_DELETE_WINDOW(m_wStatusArea)
	SAFE_DELETE_WINDOW(m_wCommandArea);
	
	clear();
	refresh();
	endwin();
}

void HexIt::editCleanup()
{
	ncursesCleanup();
	termkeyCleanup();
}
/////
