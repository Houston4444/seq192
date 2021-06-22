// This file is part of seq192
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "package.h"

#include "core/midifile.h"
#include "core/configfile.h"
#include "core/cachefile.h"
#include "core/perform.h"

#include "gui/mainwindow.h"
#include <gtkmm.h>

/* struct for command parsing */
static struct
option long_options[] = {

    {"file",     required_argument, 0, 'f'},
    {"config", 1,0,'c'},
    {"help",     0, 0, 'h'},
    {"osc-port", 1,0,'p'},
    {"jack-transport",0, 0, 'j'},
    {"no-gui",0, 0, 'n'},
    {"version",0, 0, 'v'},
    {0, 0, 0, 0}

};

string config_filename = "";
string global_filename = "";
string last_used_dir = getenv("HOME");

bool global_no_gui = false;
bool global_with_jack_transport = false;

bool global_is_running = true;

char* global_oscport;

user_midi_bus_definition   global_user_midi_bus_definitions[c_maxBuses];
user_instrument_definition global_user_instrument_definitions[c_max_instruments];
user_keymap_definition     global_user_keymap_definitions[c_max_instruments];

int
main (int argc, char *argv[])
{

    for (int i=0; i<c_maxBuses; i++)
    {
        for (int j=0; j<16; j++) {
            global_user_midi_bus_definitions[i].instrument[j] = -1;
            global_user_midi_bus_definitions[i].keymap[j] = -1;
        }
    }

    for (int i=0; i<c_max_instruments; i++)
    {
        for (int j=0; j<128; j++)
            global_user_instrument_definitions[i].controllers_active[j] = false;
    }

    for (int i=0; i<c_max_instruments; i++)
    {
        for (int j=0; j<128; j++)
            global_user_keymap_definitions[i].keys_active[j] = false;
    }

    /* the main performance object */
    perform * p = new perform();

    /* parse parameters */
    int c;
    while (1) {

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "p:f:c:hjnv", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {

            case '?':
            case 'h':

                printf("\n");
                printf("seq192 - live MIDI sequencer\n\n");
                printf("Usage: seq192 [options]\n\n");
                printf("Options:\n");
                printf("  -h, --help              show available options\n");
                printf("  -f, --file <filename>   load midi file on startup\n");
                printf("  -c, --config <filename> load config file on startup\n");
                printf("  -p, --osc-port <port>   osc input port (udp port number or unix socket path)\n");
                printf("  -j, --jack-transport    sync to jack transport\n");
                printf("  -n, --no-gui            enable headless mode\n");
                printf("  -v, --version           show version and exit\n");
                printf("\n");

                return 0;
                break;

            case 'j':
                global_with_jack_transport = true;
                break;

            case 'n':
                global_no_gui = true;
                break;

            case 'f':
                global_filename = string(optarg);
                break;

            case 'c':
                config_filename = string(optarg);
                break;

            case 'p':
                global_oscport = optarg;
                break;

            case 'v':
                printf("%s %s\n", PACKAGE, VERSION);
                return EXIT_SUCCESS;
                break;

            default:
                break;
        }

    }

    // read config file
    string config_path = getenv("XDG_CONFIG_HOME") == NULL ? string(getenv("HOME")) + "/.config" : getenv("XDG_CONFIG_HOME");
    config_path += string("/") + PACKAGE;
    mkdir(config_path.c_str(), 0777);
    string file_path = config_filename == "" ? (config_path + "/config.json") : config_filename;
    ConfigFile config(file_path);
    config.parse();

    // read/touch cache file
    string cache_path = getenv("XDG_CACHE_HOME") == NULL ? string(getenv("HOME")) + "/.cache" : getenv("XDG_CACHE_HOME");
    cache_path += string("/") + PACKAGE;
    mkdir(cache_path.c_str(), 0777);
    file_path = cache_path + "/cache.json";
    std::ifstream infile(file_path);
    if (!infile.good()) {
        std::fstream fs;
        fs.open(file_path, std::ios::out);
        fs.close();
    }
    CacheFile cache(file_path);
    cache.parse();

    p->init();

    p->launch_input_thread();
    p->launch_output_thread();
    p->init_jack();

    if (global_filename != "") {
        /* import that midi file */
        midifile *f = new midifile(global_filename);
        f->parse(p, 0);
        delete f;
    }

    if (global_oscport != 0 && strstr(global_oscport, "/")) {
        // liblo needs a gracefull ctrl+c handler to release unix socket
        signal(SIGINT, [](int param){global_is_running = false;});
    }

    int status = 0;
    if (global_no_gui) {
        while (global_is_running) {
            usleep(1000);
        }
    } else {
        auto application = Gtk::Application::create();
        MainWindow window(p);
        status = application->run(window);
    }

    // write cache file
    cache.write();

    delete p;

    return status;
}
