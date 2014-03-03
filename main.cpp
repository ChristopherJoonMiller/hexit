#include "hexit.h"
#include "hexit_def.h"

using namespace std;

#define CHECK_TRUE(x)			(x != '0' && x != 'f')
#define CHECK_ARGC(x)			if( i+1 == argc ) { usage(); return 1; }

// Global Functions!
void usage()
{
	cout << "\nUsage:\n";
	cout << "  hexit file [-h][-e][-o output][-u t|f][-a t|f][-b t|f][-c t|f]\n\n";
	cout << "  -h: display this help dialog\n";
	cout << "  -e: edit mode instead of outputting to stdout\n";
	cout << "  -o: supply a filename to output ASCII HEX to\n";
	cout << "  -u: uppercase the hexadecimal output true or false\n";
	cout << "  -a: show ascii text true or false\n";
	cout << "  -b: show byte count true or false\n";
	cout << "  -c: use colored text in the editor true or false\n";
	cout << "                  (if your terminal supports color)\n";
	cout << endl;
}

// Main Application
int main(int argc, char *argv[])
{
    // char a;
    // cin >> a; // wait for debugger
    
	uint switches = 0;
    uint default_on = (SWITCH_UPPER | SWITCH_SHOW_BYTE_COUNT | SWITCH_SHOW_ASCII | SWITCH_COLOR);
    uint default_off = (SWITCH_OUTPUT | SWITCH_EDIT );
    switches = default_on;
    switches &= ~(default_off);

	if(argc <= 1)
	{
		
		usage();
		return 0;
	}
    
    string output_fn(argv[1]); // default to input filename

	// parse switches
	for(int i = 1; i < argc; i++)
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
			CHECK_ARGC(i);
			if(CHECK_TRUE(argv[i+1][0]))
			{
				switches |= SWITCH_UPPER;
			}
			else
			{
				switches &= ~(SWITCH_UPPER);
			}
			i++;
		}
		else if(!strcmp(argv[i],"-o"))
		{
			CHECK_ARGC(i);
			switches |= SWITCH_OUTPUT;
			output_fn.assign(argv[i+1]);
			i++; // skip the output name
		}
		else if(!strcmp(argv[i],"-b"))
		{
			CHECK_ARGC(i);
			if(CHECK_TRUE(argv[i+1][0]))
			{
				switches |= SWITCH_SHOW_BYTE_COUNT;
			}
			else
			{
				switches &= ~(SWITCH_SHOW_BYTE_COUNT);
			}
			i++;
		}
		else if(!strcmp(argv[i],"-a"))
		{
			CHECK_ARGC(i);
			if(CHECK_TRUE(argv[i+1][0]))
			{
				switches |= SWITCH_SHOW_ASCII;
			}
			else
			{
				switches &= ~(SWITCH_SHOW_ASCII);
			}
			i++;
		}
		else if(!strcmp(argv[i],"-c"))
		{
			CHECK_ARGC(i);
			if(CHECK_TRUE(argv[i+1][0]))
			{
				switches |= SWITCH_COLOR;
			}
			else
			{
				switches &= ~(SWITCH_COLOR);
			}
			i++;
		}
	}
    
	//cout << "Opening file: " << argv[1] << endl << endl;    
	HexIt h(argv[1]);
    
	// Does the user want uppercase letters?
	h.setSwitches(switches);
	
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
