//----------------------------------------------------------------------------
//
//  This file is part of seq24.
//
//  seq24 is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  seq24 is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with seq24; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//-----------------------------------------------------------------------------

#include <cctype>

#include "mainwnd.h"
#include "perform.h"
#include "midifile.h"

#include "xpm/play2.xpm"
#include "xpm/stop.xpm"
#include "xpm/seq24.xpm"
#include "xpm/seq24_32.xpm"

bool is_pattern_playing = false;
bool global_is_modified = false;

mainwnd::mainwnd(perform *a_p)
{
    set_icon(Gdk::Pixbuf::create_from_xpm_data(seq24_32_xpm));

    /* set the performance */
    m_mainperf = a_p;

    /* main window */
    update_window_title();

    m_main_wid = manage( new mainwid( m_mainperf, this ));
    m_main_time = manage( new maintime( ));

    m_menubar = manage(new MenuBar());

    m_menu_file = manage(new Menu());
    m_menubar->items().push_front(MenuElem("_File", *m_menu_file));

    m_menu_help = manage( new Menu());
    m_menubar->items().push_back(MenuElem("_Help", *m_menu_help));

    /* file menu items */
    m_menu_file->items().push_back(MenuElem("_New",
                Gtk::AccelKey("<control>N"),
                mem_fun(*this, &mainwnd::file_new)));
    m_menu_file->items().push_back(MenuElem("_Open...",
                Gtk::AccelKey("<control>O"),
                mem_fun(*this, &mainwnd::file_open)));
    m_menu_file->items().push_back(MenuElem("_Save",
                Gtk::AccelKey("<control>S"),
                mem_fun(*this, &mainwnd::file_save)));
    m_menu_file->items().push_back(MenuElem("Save _as...",
                sigc::bind(mem_fun(*this, &mainwnd::file_save_as), E_MIDI_SEQ24_SESSION, -1, -1)));
    m_menu_file->items().push_back(SeparatorElem());
    m_menu_file->items().push_back(MenuElem("_Import...",
                mem_fun(*this, &mainwnd::file_import_dialog)));
    m_menu_file->items().push_back(MenuElem("Export Screen Set...",
                sigc::bind(mem_fun(*this, &mainwnd::file_save_as), E_MIDI_SEQ24_SCREENSET, 1, -1)));
    m_menu_file->items().push_back(SeparatorElem());
    m_menu_file->items().push_back(MenuElem("E_xit",
                Gtk::AccelKey("<control>Q"),
                mem_fun(*this, &mainwnd::file_exit)));

    /* help menu items */
    m_menu_help->items().push_back(MenuElem("_About...",
                mem_fun(*this, &mainwnd::about_dialog)));


    /* bottom line items */
    HBox *hbox = manage( new HBox( false, 2 ) );

    /* stop button */
    m_button_stop = manage( new Button( ));
    m_button_stop->add(*manage(new Image(
                    Gdk::Pixbuf::create_from_xpm_data( stop_xpm ))));
    m_button_stop->signal_clicked().connect(
            mem_fun(*this, &mainwnd::stop_playing));
    m_button_stop->set_tooltip_text( "Stop playing MIDI sequence" );
    m_button_stop->set_focus_on_click(false);
    hbox->pack_start(*m_button_stop, false, false);

    /* play button */
    m_button_play = manage(new Button() );
    m_button_play->add(*manage(new Image(
                    Gdk::Pixbuf::create_from_xpm_data( play2_xpm ))));
    m_button_play->signal_clicked().connect(
            mem_fun( *this, &mainwnd::start_playing));
            m_button_play->set_tooltip_text( "Play MIDI sequence" );
    m_button_play->set_focus_on_click(false);

    hbox->pack_start(*m_button_play, false, false);

    /* bpm spin button */
    m_adjust_bpm = manage(new Adjustment(m_mainperf->get_bpm(), c_bpm_minimum, c_bpm_maximum, 1));
    m_spinbutton_bpm = manage( new SpinButton( *m_adjust_bpm ));
    m_spinbutton_bpm->set_name( "BPM Edit" );
    m_spinbutton_bpm->set_editable( true );
    m_spinbutton_bpm->set_digits(2);
    m_adjust_bpm->signal_value_changed().connect(
            mem_fun(*this, &mainwnd::adj_callback_bpm ));
    m_spinbutton_bpm->signal_activate().connect(
            mem_fun(*this, &mainwnd::adj_enter_callback_bpm));

    m_spinbutton_bpm->set_tooltip_text( "Adjust beats per minute (BPM) value" );
    hbox->pack_start(*(manage( new Label( "  bpm " ))), false, false, 4);
    hbox->pack_start(*m_spinbutton_bpm, false, false );

    /* sequence set spin button */
    m_adjust_ss = manage( new Adjustment( 0, 0, c_max_sets - 1, 1 ));
    m_spinbutton_ss = manage( new SpinButton( *m_adjust_ss ));
    m_spinbutton_ss->set_name( "Screen Set" );
    m_spinbutton_ss->set_editable( true );
    m_spinbutton_ss->set_wrap( true );
    m_adjust_ss->signal_value_changed().connect(
            mem_fun(*this, &mainwnd::adj_callback_ss ));
    m_spinbutton_ss->signal_activate().connect(
            mem_fun(*this, &mainwnd::adj_enter_callback_ss));
    m_spinbutton_ss->set_tooltip_text( "Select sreen set" );
    hbox->pack_end(*m_spinbutton_ss, false, false );
    hbox->pack_end(*(manage( new Label( "  set " ))), false, false, 4);

    /* screen set name edit line */
    m_entry_notes = manage( new Entry());
    m_entry_notes->set_name( "Screen Name" );
    m_entry_notes->signal_focus_out_event().connect(
            mem_fun(*this, &mainwnd::edit_callback_notepad));
    m_entry_notes->signal_activate().connect(
            mem_fun(*this, &mainwnd::edit_enter_callback_notepad));
    m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                m_mainperf->get_screenset()));
    m_entry_notes->set_tooltip_text( "Enter screen set name" );
    hbox->pack_start( *m_entry_notes, true, true );


    /* top line items */
    HBox *hbox2 = manage( new HBox( false, 0 ) );
    hbox2->pack_start(*manage(new Image(
                    Gdk::Pixbuf::create_from_xpm_data(seq24_xpm))),
            false, false);

    // adjust placement...
    VBox *vbox_b = manage( new VBox() );
    HBox *hbox3 = manage( new HBox( false, 0 ) );
    vbox_b->pack_start( *hbox3, false, false );
    hbox2->pack_end( *vbox_b, false, false );
    hbox3->set_spacing( 10 );

    /* timeline */
    hbox3->pack_start( *m_main_time, false, false );

    // SCROLL MOD
    m_hadjust = (manage(new Gtk::Adjustment(0,0,1,10,100,0)));
    m_vadjust = (manage(new Gtk::Adjustment(0,0,1,10,100,0)));
    m_hscroll = (manage(new Gtk::HScrollbar(*m_hadjust)));
    m_vscroll = (manage(new Gtk::VScrollbar(*m_vadjust)));

    Gtk::Layout * mainwid_wrapper = new Gtk::Layout(*m_hadjust, *m_vadjust);
    mainwid_wrapper->add(*m_main_wid);
    mainwid_wrapper->set_size(c_mainwid_x,c_mainwid_y);

    Gtk::HBox * mainwid_vscroll_wrapper = new Gtk::HBox();
    mainwid_vscroll_wrapper->set_spacing(5);
    mainwid_vscroll_wrapper->pack_start
    (
        *mainwid_wrapper,
        Gtk::PACK_EXPAND_WIDGET
    );

    Gtk::VBox * mainwid_hscroll_wrapper = new Gtk::VBox();
    mainwid_hscroll_wrapper->set_spacing(5);
    mainwid_hscroll_wrapper->pack_start
    (
        *mainwid_vscroll_wrapper, Gtk::PACK_EXPAND_WIDGET
    );
    m_hadjust->signal_changed().connect
    (
        mem_fun(*this, &mainwnd::on_scrollbar_resize)
    );
    m_vadjust->signal_changed().connect
    (
        mem_fun(*this, &mainwnd::on_scrollbar_resize)
    );
    m_main_wid->signal_scroll_event().connect
    (
        mem_fun(*this, &mainwnd::on_scroll_event)
    );
    set_resizable(true);
    // SCROLL MOD


    /* set up a vbox, put the menu in it, and add it */
    VBox *vbox = new VBox();
    vbox->set_border_width( 10 );
    vbox->pack_start(*hbox2, false, false );
    vbox->pack_start(*mainwid_hscroll_wrapper, true, true, 10 );
    vbox->pack_start(*hbox, false, false );


    VBox *ovbox = new VBox();

    ovbox->pack_start(*m_menubar, false, false );
    ovbox->pack_start( *vbox );

    /* add box */
    this->add (*ovbox);

    // SCROLL MOD
    resize(
        c_mainwid_x + 20,
        hbox2->get_allocation().get_height() +
        hbox->get_allocation().get_height() +
        hbox3->get_allocation().get_height() +
        m_menubar->get_allocation().get_height() +
        c_mainwid_y + 40
    );
    mainwid_hscroll_wrapper->pack_start(*m_hscroll, false, false);
    mainwid_vscroll_wrapper->pack_start(*m_vscroll, false, false);
    // SCROLL MOD

    /* show everything */
    show_all();

    // resize again for exact size
    resize(
        c_mainwid_x + 20,
        hbox2->get_allocation().get_height() +
        hbox->get_allocation().get_height() +
        hbox3->get_allocation().get_height() +
        m_menubar->get_allocation().get_height() +
        c_mainwid_y + 40
    );

    add_events( Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK | Gdk::SCROLL_MASK);

    m_timeout_connect = Glib::signal_timeout().connect(
            mem_fun(*this, &mainwnd::timer_callback), 25);

}


mainwnd::~mainwnd()
{

}

// SCROLL MOD
void
mainwnd::on_scrollbar_resize ()
{
    int bar = m_vscroll->get_allocation().get_width() + 5;

    bool h_visible = (m_vscroll->get_visible() ? bar : 0) < m_hadjust->get_upper() - m_hadjust->get_page_size();
    bool v_visible = (m_hscroll->get_visible() ? bar : 0) < m_vadjust->get_upper() - m_vadjust->get_page_size();

    if (m_hscroll->get_visible() != h_visible)
        m_hscroll->set_visible(h_visible);

    if (m_vscroll->get_visible() != v_visible)
        m_vscroll->set_visible(v_visible);
}


bool
mainwnd::on_scroll_event (GdkEventScroll * ev)
{
    if (ev->direction == GDK_SCROLL_LEFT  ||
        ev->direction == GDK_SCROLL_RIGHT || ev->state & GDK_SHIFT_MASK)
    {
        double v = ev->direction == GDK_SCROLL_LEFT ||
                   ev->direction == GDK_SCROLL_UP ?
            m_hadjust->get_value() - m_hadjust->get_step_increment():
            m_hadjust->get_value() + m_hadjust->get_step_increment();

        m_hadjust->clamp_page(v, v + m_hadjust->get_page_size());
    }
    else if (ev->direction == GDK_SCROLL_UP ||
             ev->direction == GDK_SCROLL_DOWN)
    {
        double v = ev->direction == GDK_SCROLL_UP ?
                m_vadjust->get_value() - m_vadjust->get_step_increment():
                m_vadjust->get_value() + m_vadjust->get_step_increment();

        m_vadjust->clamp_page(v, v + m_vadjust->get_page_size());
    }

    return true;
}
// SCROLL MOD

// This is the GTK timer callback, used to draw our current time and bpm
// ondd_events( the main window
bool
mainwnd::timer_callback(  )
{
    long ticks = m_mainperf->get_tick();

    m_main_time->idle_progress( ticks );
    m_main_wid->update_markers( ticks );

    if ( m_adjust_bpm->get_value() != m_mainperf->get_bpm()){
        m_adjust_bpm->set_value( m_mainperf->get_bpm());
    }

    if ( m_adjust_ss->get_value() !=  m_mainperf->get_screenset() )
    {
        m_main_wid->set_screenset(m_mainperf->get_screenset());
        m_adjust_ss->set_value( m_mainperf->get_screenset());
        m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                    m_mainperf->get_screenset()));
    }

    return true;
}


void
mainwnd::start_playing()
{
    m_mainperf->start_playing();
    is_pattern_playing = true;
}


void
mainwnd::stop_playing()
{
    m_mainperf->stop_playing();
    m_main_wid->update_sequences_on_window();
    is_pattern_playing = false;
}

/* callback function */
void mainwnd::file_new()
{
    if (is_save())
        new_file();
}


void mainwnd::new_file()
{
    m_mainperf->clear_all();

    m_main_wid->reset();
    m_entry_notes->set_text( * m_mainperf->get_screen_set_notepad(
                m_mainperf->get_screenset() ));

    global_filename = "";
    update_window_title();
    global_is_modified = false;
}


/* callback function */
void mainwnd::file_save()
{
    save_file();
}


/* callback function */
void mainwnd::file_save_as(file_type_e type, int a_sset, int a_seq)
{
    Gtk::FileChooserDialog dialog("Save file as",
                      Gtk::FILE_CHOOSER_ACTION_SAVE);

    switch(type)
    {
        case E_MIDI_SEQ24_SEQUENCE:
            dialog.set_title("Midi export sequence");
            break;
        case E_MIDI_SEQ24_SCREENSET:
            dialog.set_title("Midi export screenset");
            break;
        case E_MIDI_SEQ24_SESSION:
            break;
    }

    if (a_sset != -1) {
        a_sset = m_mainperf->get_screenset();
    }

    dialog.set_transient_for(*this);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    Gtk::FileFilter filter_midi;
    filter_midi.set_name("MIDI files");
    filter_midi.add_pattern("*.midi");
    filter_midi.add_pattern("*.mid");
    dialog.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    dialog.set_current_folder(last_used_dir);
    int result = dialog.run();

    switch (result) {
        case Gtk::RESPONSE_OK:
        {
            std::string fname = dialog.get_filename();
            Gtk::FileFilter* current_filter = dialog.get_filter();

            if ((current_filter != NULL) &&
                    (current_filter->get_name() == "MIDI files")) {

                // check for MIDI file extension; if missing, add .midi
                std::string suffix = fname.substr(
                        fname.find_last_of(".") + 1, std::string::npos);
                toLower(suffix);
                if ((suffix != "midi") && (suffix != "mid"))
                    fname = fname + ".midi";
            }

            if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS)) {
                Gtk::MessageDialog warning(*this,
                        "File already exists!\n"
                        "Do you want to overwrite it?",
                        false,
                        Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
                result = warning.run();

                if (result == Gtk::RESPONSE_NO)
                    return;
            }

            if(type == E_MIDI_SEQ24_SESSION)
            {
                global_filename = fname;
                update_window_title();
                save_file();
            }
            else
            {
                file_export(fname, a_sset, a_seq);
            }
            break;
        }

        default:
            break;
    }
}


void mainwnd::open_file(const Glib::ustring& fn)
{
    bool result;

    m_mainperf->clear_all();

    midifile f(fn);
    result = f.parse(m_mainperf, 0);
    global_is_modified = !result;

    if (!result) {
        Gtk::MessageDialog errdialog(*this,
                "Error reading file: " + fn, false,
                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        errdialog.run();
        return;
    }

    last_used_dir = fn.substr(0, fn.rfind("/") + 1);
    global_filename = fn;
    update_window_title();

    m_main_wid->reset();
    m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                m_mainperf->get_screenset()));
    m_adjust_bpm->set_value( m_mainperf->get_bpm());
}


/*callback function*/
void mainwnd::file_open()
{
    if (is_save())
        choose_file();
}


void mainwnd::choose_file()
{
    Gtk::FileChooserDialog dialog("Open MIDI file",
                      Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*this);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    Gtk::FileFilter filter_midi;
    filter_midi.set_name("MIDI files");
    filter_midi.add_pattern("*.midi");
    filter_midi.add_pattern("*.mid");
    dialog.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    dialog.set_current_folder(last_used_dir);

    int result = dialog.run();

    switch(result) {
        case(Gtk::RESPONSE_OK):
            open_file(dialog.get_filename());

        default:
            break;
    }
}


bool mainwnd::save_file()
{
    bool result = false;

    if (global_filename == "") {
        file_save_as(E_MIDI_SEQ24_SESSION, -1, -1);
        return true;
    }

    midifile f(global_filename);
    result = f.write(m_mainperf, -1, -1);

    if (!result) {
        Gtk::MessageDialog errdialog(*this,
                "Error writing file.", false,
                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        errdialog.run();
    }
    global_is_modified = !result;
    return result;
}


void mainwnd::file_export(const Glib::ustring& fn, int a_sset, int a_seq)
{
    bool result = false;

    midifile f(fn);

    result = f.write(m_mainperf, a_sset, a_seq);

    if (!result)
    {
        Gtk::MessageDialog errdialog
        (
            *this,
            "Error writing file.",
            false,
            Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK,
            true
        );
        errdialog.run();
    }
}


int mainwnd::query_save_changes()
{
    Glib::ustring query_str;

    if (global_filename == "")
        query_str = "Unnamed file was changed.\nSave changes?";
    else
        query_str = "File '" + global_filename + "' was changed.\n"
                "Save changes?";

    Gtk::MessageDialog dialog(*this, query_str, false,
            Gtk::MESSAGE_QUESTION,
            Gtk::BUTTONS_NONE, true);

    dialog.add_button(Gtk::Stock::YES, Gtk::RESPONSE_YES);
    dialog.add_button(Gtk::Stock::NO, Gtk::RESPONSE_NO);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

    return dialog.run();
}


bool mainwnd::is_save()
{
    bool result = false;

    if (global_is_modified)
    {
        int choice = query_save_changes();
        switch (choice)
        {
        case Gtk::RESPONSE_YES:
            if (save_file())
                result = true;
            break;
        case Gtk::RESPONSE_NO:
            result = true;
            break;
        case Gtk::RESPONSE_CANCEL:
        default:
            break;
        }
    }
    else
        result = true;

    return result;
}

/* convert string to lower case letters */
void
mainwnd::toLower(basic_string<char>& s) {
    for (basic_string<char>::iterator p = s.begin();
            p != s.end(); p++) {
        *p = tolower(*p);
    }
}


void
mainwnd::file_import_dialog()
{
    Gtk::FileChooserDialog dialog("Import MIDI file",
            Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*this);

    Gtk::FileFilter filter_midi;
    filter_midi.set_name("MIDI files");
    filter_midi.add_pattern("*.midi");
    filter_midi.add_pattern("*.mid");
    dialog.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    dialog.set_current_folder(last_used_dir);

    ButtonBox *btnbox = dialog.get_action_area();
    HBox hbox( false, 2 );

    m_adjust_load_offset = manage( new Adjustment( 0, -(c_max_sets - 1),
                c_max_sets - 1, 1 ));
    m_spinbutton_load_offset = manage( new SpinButton( *m_adjust_load_offset ));
    m_spinbutton_load_offset->set_editable( false );
    m_spinbutton_load_offset->set_wrap( true );
    hbox.pack_end(*m_spinbutton_load_offset, false, false );
    hbox.pack_end(*(manage( new Label("Screen Set Offset"))), false, false, 4);

    btnbox->pack_start(hbox, false, false );

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    dialog.show_all_children();

    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
       case(Gtk::RESPONSE_OK):
       {
           try{
               midifile f( dialog.get_filename() );
               f.parse( m_mainperf, (int) m_adjust_load_offset->get_value() );
           }
           catch(...){
               Gtk::MessageDialog errdialog(*this,
                       "Error reading file.", false,
                       Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
                errdialog.run();
           }

           global_filename = std::string(dialog.get_filename());
           update_window_title();
           global_is_modified = true;

           m_main_wid->reset();
           m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                       m_mainperf->get_screenset() ));
           m_adjust_bpm->set_value( m_mainperf->get_bpm() );

           break;
       }

       case(Gtk::RESPONSE_CANCEL):
           break;

       default:
           break;

   }
}

/*callback function*/
void mainwnd::file_exit()
{
    if (is_save()) {
        if (is_pattern_playing)
            stop_playing();
        hide();
    }
}


bool
mainwnd::on_delete_event(GdkEventAny *a_e)
{
    bool result = is_save();
    if (result && is_pattern_playing)
            stop_playing();

    return !result;
}


void
mainwnd::about_dialog()
{
    Gtk::AboutDialog dialog;
    dialog.set_transient_for(*this);
    dialog.set_name(PACKAGE_NAME);
    dialog.set_version(VERSION);
    dialog.set_comments("Interactive MIDI Sequencer\n");

    dialog.set_copyright(
            "(C) 2002 - 2006 Rob C. Buse\n"
            "(C) 2008 - 2009 Seq24team");

    dialog.set_website(
            "http://www.filter24.org/seq24\n"
            "http://edge.launchpad.net/seq24");

    std::list<Glib::ustring> list_authors;
    list_authors.push_back("Rob C. Buse <rcb@filter24.org>");
    list_authors.push_back("Ivan Hernandez <ihernandez@kiusys.com>");
    list_authors.push_back("Guido Scholz <guido.scholz@bayernline.de>");
    list_authors.push_back("Jaakko Sipari <jaakko.sipari@gmail.com>");
    list_authors.push_back("Peter Leigh <pete.leigh@gmail.com>");
    list_authors.push_back("Anthony Green <green@redhat.com>");
    list_authors.push_back("Daniel Ellis <mail@danellis.co.uk>");
    list_authors.push_back("Kevin Meinert <kevin@subatomicglue.com>");
    dialog.set_authors(list_authors);

    std::list<Glib::ustring> list_documenters;
    list_documenters.push_back("Dana Olson <seq24@ubuntustudio.com>");
    dialog.set_documenters(list_documenters);

    dialog.show_all_children();
    dialog.run();
}


void
mainwnd::adj_callback_ss( )
{
    m_mainperf->set_screenset( (int) m_adjust_ss->get_value());
    m_main_wid->set_screenset( m_mainperf->get_screenset());
    m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
    m_mainperf->get_screenset()));
}

void
mainwnd::adj_enter_callback_ss( )
{
    m_main_wid->grab_focus();
}

void
mainwnd::adj_callback_bpm( )
{
    m_mainperf->set_bpm(m_adjust_bpm->get_value());
}

void
mainwnd::adj_enter_callback_bpm( )
{
    m_main_wid->grab_focus();
}


bool
mainwnd::edit_callback_notepad( GdkEventFocus *focus )
{
    string text = m_entry_notes->get_text();
    m_mainperf->set_screen_set_notepad( m_mainperf->get_screenset(),
				        &text );
    global_is_modified = true;
    m_main_wid->grab_focus();
    return true;
}

void
mainwnd::edit_enter_callback_notepad( )
{
    GdkEventFocus event = GdkEventFocus();
    this->edit_callback_notepad(&event);
}


bool
mainwnd::on_key_press_event(GdkEventKey* a_ev)
{

    if ( a_ev->type == GDK_KEY_PRESS )
    {

        if ( a_ev->state & GDK_MOD1_MASK ) // alt key
        {
            if ((a_ev->keyval == GDK_f || a_ev->keyval == GDK_F) || // file
                (a_ev->keyval == GDK_h || a_ev->keyval == GDK_H))   // help
            {
                return Gtk::Window::on_key_press_event(a_ev); // return = don't do anything else
            }
        }

        if ( a_ev->state & GDK_CONTROL_MASK ) // ctrl key
        {
            if ((a_ev->keyval == GDK_n || a_ev->keyval == GDK_N) || // new file
                (a_ev->keyval == GDK_o || a_ev->keyval == GDK_O) || // open file
                (a_ev->keyval == GDK_q || a_ev->keyval == GDK_Q) || // quit
                (a_ev->keyval == GDK_s || a_ev->keyval == GDK_S))  // save
            {
                return Gtk::Window::on_key_press_event(a_ev); // return =  don't do anything else
            }
        }

        if (get_focus()->get_name() == "Screen Name")      // if we are on the screen name
            return Gtk::Window::on_key_press_event(a_ev); // return = don't do anything else

        if (get_focus()->get_name() == "BPM Edit")        // if we are on the BPM spin button - allow editing
            return Gtk::Window::on_key_press_event(a_ev); // return = don't do anything else

        if (get_focus()->get_name() == "Screen Set")        // if we are on the Screen Set spin button - allow editing
            return Gtk::Window::on_key_press_event(a_ev); // return = don't do anything else

        // the start/end key may be the same key (i.e. SPACE)
        // allow toggling when the same key is mapped to both
        // triggers (i.e. SPACEBAR)
        bool dont_toggle = m_mainperf->m_key_start != m_mainperf->m_key_stop;

        if ( a_ev->keyval == m_mainperf->m_key_start
                && (dont_toggle || !is_pattern_playing))
        {
            start_playing();
        }
        else if ( a_ev->keyval == m_mainperf->m_key_stop
                && (dont_toggle || is_pattern_playing))
        {
            stop_playing();
        }

    }

    return false;
}


void
mainwnd::update_window_title()
{
    std::string title;

    if (global_filename == "")
        title = ( PACKAGE ) + string( " - [unnamed]" );
    else
        title =
            ( PACKAGE )
            + string( " - [" )
            + Glib::filename_to_utf8(global_filename)
            + string( "]" );

    set_title ( title.c_str());
}
