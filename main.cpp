#include "hexit.h"
#include "hexit_def.h"

using namespace std;

#define CHECK_TRUE(x)			(x != '0' && x != 'f')
#define CHECK_ARGC(x)			if( i+1 == argc ) { usage(); return 1; }

// Global Functions!
void usage()
{
	cout << "\nUsage:\n";
	cout << "  hexit [-h][-p][-o output][-a t|f][-b t|f][-c t|f][-u t|f] file\n\n";
	cout << "  -h: display this HELP dialog\n";
	cout << "  -p: PRINT to stdout and exit\n";
	cout << "  -o: supply a filename to OUTPUT ASCII hexadecimal to\n";
	cout << "  -a: show ASCII text true or false\n";
	cout << "  -b: show BYTE count true or false\n";
	cout << "  -c: use COLORED text in the editor true or false\n";
	cout << "                  (if your terminal supports color)\n";
	cout << "  -u: UPPERCASE the hexadecimal output true or false\n";
	cout << endl;
}

// Main Application
int main(int argc, char *argv[])
{
    char a;
    cin >> a; // wait for debugger
    
	uint switches = 0;
    uint default_on = (SWITCH_EDIT | SWITCH_UPPER | SWITCH_SHOW_BYTE_COUNT | SWITCH_SHOW_ASCII | SWITCH_COLOR);
    uint default_off = (SWITCH_OUTPUT);
    switches = default_on;
    switches &= ~(default_off);

	if(argc <= 1) // nothing to work on so exit
	{
		usage();
		return 0;
	}
    
    string output_fn(argv[argc-1]); // default to input filename

	// parse switches
	for(int i = 1; i < (argc - 1); i++)
	{
		if(!strcmp(argv[i],"-h"))
		{
			usage();
			return 0;
		}
		else if(!strcmp(argv[i],"-p"))
		{
			switches &= ~(SWITCH_EDIT);
		}
		else if(!strcmp(argv[i],"-o"))
		{
			CHECK_ARGC(i);
			switches |= SWITCH_OUTPUT;
			switches &= ~(SWITCH_EDIT);
			output_fn.assign(argv[i+1]);
			i++; // skip the output name
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
	}
    
	//cout << "Opening file: " << argv[1] << endl << endl;    
	HexIt h(argv[argc-1]);
    
	h.setSwitches(switches);
	
	// are we outputting?
	if( !(switches & SWITCH_EDIT) ) // if not editing output to a stream
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
	else if( switches & SWITCH_EDIT )
	{
		h.editMode();
	}
    
	return 0;
}
