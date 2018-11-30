// $Id: main.cpp,v 1.9 2016-01-14 16:16:52-08 - - $

#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <utility>

using namespace std;

#include "commands.h"
#include "debug.h"
#include "file_sys.h"
#include "util.h"

// scan_options
//    Options analysis:  The only option is -Dflags.

void scan_options(int argc, char** argv)
{
    opterr = 0;
    for (;;) {
        int option = getopt(argc, argv, "@:");
        if (option == EOF)
            break;
        std::cout << option << endl;
        switch (option) {
        case '@':
            debugflags::setflags(optarg);
            break;
        default:
            complain() << "-" << static_cast<char>(option)
                       << ": invalid option" << endl;
            break;
        }
    }
    if (optind < argc) {
        complain() << "operands not permitted" << endl;
    }
}

// main -
//    Main program which loops reading commands until end of file.

int main(int argc, char** argv)
{
    execname(argv[0]);
    cout << boolalpha; // Print false or true instead of 0 or 1.
    cerr << boolalpha;
    cout << argv[0] << " build " << __DATE__ << " " << __TIME__ 
        << endl;
    scan_options(argc, argv);
    bool need_echo = want_echo();
    inode_state state;
    try {
        for (;;) {
            try {

                cout << state.prompt();
                string line;
                getline(cin, line);
                if (cin.eof()) {
                    if (need_echo)
                        cout << "^D";
                    cout << endl;
                    DEBUGF('y', "EOF");
                    break;
                }
                if (need_echo)
                    cout << line << endl;

                wordvec words = split(line, " \t");
                DEBUGF('y', "words = " << words);

                if (!words.empty() && words.at(0)[0] != '#') {
                    command_fn fn = find_command_fn(words.at(0));
                    fn(state, words);
                }

            } catch (command_error& error) {

                complain() << error.what() << endl;
            }
        }
    } catch (ysh_exit&) {
        // This catch intentionally left blank.
    }

    return exit_status_message();
}
