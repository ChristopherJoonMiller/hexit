/******************************\

hexit.h
140224

HexIt is a command line hex
viewer and editor.

\******************************/


#ifndef __HEXIT_H__
#define __HEXIT_H__

#define VERSION_MAJOR 0
#define VERSION_MINOR 11

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <assert.h>
#include "curses.h"
#include "termkey.h"
#include "hexit_def.h"

using namespace std;

typedef ios_base& (*case_fptr) (ios_base& str);

struct Cursor
{
	uint word;		// start byte count of the word that we're editing (2 bytes), will never be odd
	uint nibble;	// which nibble are we editing? 0-3, most sig nibble is 0, least is 3
	bool editing;	// currently editing
	
	uint backup;	// a backup of a word we're currently editing;
	uint editWord;	// read in 4 nibbles while editing

	Cursor() : word(0), nibble(0), editing(false), backup(0), editWord(0) {};
	void SetPos(uint _word, uint _nibble=0) { word = _word; nibble = _nibble; }
};

class HexIt
{
public:
	HexIt();
	HexIt(char* filename);
	HexIt(HexIt* h);
	~HexIt();
	
	bool operator=(HexIt* h);

	void print(ostream& output); 	 // output to terminal
	void editMode();				 // live editing with ncurses

	void setSwitches(uint switches);

private:
	fstream* m_pFile; // file handle
    char m_inputFilename[128];
    char m_outputFilename[128];
    char m_appVersion[16];

	stringstream m_buffer;	// hold the entire file in memory!
	bool m_bRunning;		// should we quit exit mode?
	bool m_bBufferDirty;	// have we edited the buffer and needs a save

	uint m_uHeight;
	uint m_uWidth;
	uint m_uFilePos;	// how many bytes into the file is the top left corner of the screen? i.e. 16 byte aligned
	uint m_uFileSize;	// how many bytes is the total file (not including stop byte);

    // settings, being greedy with unique bools for each one
    bool m_bShowColor;
    bool m_bPrintUpper;
    bool m_bShowByteCount;
    bool m_bShowASCII;
    uint m_uInsertWord;
    
	Cursor m_cursor;

	// ncurses stuff
	WINDOW* m_wTitleArea;
	WINDOW* m_wEditArea;
	WINDOW* m_wStatusArea;
	WINDOW* m_wCommandArea;

	TermKey* m_tk;

	case_fptr getCaseFunction();

	// render one line of output!
	void renderLine(ostream& output, uint start_byte, char* byte_seq, uint bytes_read);
	void renderScreen();		 						// render the entire screen buffer!
	void textColor(uint byte_pos, char byte_data);	 			// set color of cursor byte to be red
	void setTerminalSize();
	void setCursorPos(uint _word, uint _nibble=0);
	void setCursorPos();
	uint getCursorRow();
	uint getCursorColumn();
	void checkCursorOffscreen();
	void moveCursor(int x, int y);									// this sets up directional input
	void moveNibble(int x);
	uint maxFilePos();												// don't let the user scroll past the end

	// Editing of the file
	void toggleEdit(bool save=true);
	void editKey(uint nibble);										// user input of hex nibbles
	// void insertWord(uint start_byte, uint data);
	// void updateWord(uint start_byte, uint data);
	
	// Input Commands
	void cmdPageDn();
	void cmdCopyByte();
	void cmdFillWord();
	void cmdFindByte();
	void cmdInsertWord();
	void cmdInsertWordAt();
	void cmdOutputFile();
	void cmdCursorWord();
	void cmdCloseFile();
	void cmdPageUp();
	void cmdPasteByte();

	// 
	void editInit();
	void editCleanup();

};

#endif //__HEXIT_H__
