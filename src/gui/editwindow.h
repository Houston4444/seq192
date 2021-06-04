#ifndef SEQ24_EDITWINDOW
#define SEQ24_EDITWINDOW

#include <gtkmm.h>

#include "../core/globals.h"

#include "styles.h"
#include "mainwindow.h"
#include "sequencebutton.h"
#include "pianoroll.h"
#include "eventroll.h"
#include "pianokeys.h"
#include "dataroll.h"

using namespace Gtk;

class MainWindow;
class EditWindow : public Window {

    public:

        EditWindow(perform * p, MainWindow * m, int seqnum, sequence * seq);
        ~EditWindow();

    private:

        perform            *m_perform;
        sequence           *m_sequence;
        MainWindow         *m_mainwindow;
        int                 m_seqnum;

        // components
        PianoKeys           m_pianokeys;
        EventRoll           m_eventroll;
        PianoRoll           m_pianoroll;
        DataRoll            m_dataroll;

        // layout
        VBox                m_vbox;
        Grid                m_grid;
        HBox                m_hbox;
        VBox                m_left_vbox;
        VBox                m_right_vbox;
        ScrolledWindow      m_hscroll_wrapper;
        VBox                m_hscroll_vbox;
        ScrolledWindow      m_pianokeys_scroller;
        ScrolledWindow      m_pianoroll_scroller;
        Scrollbar           m_vscrollbar;
        Scrollbar           m_hscrollbar;
        Label               m_dummy1;
        Label               m_dummy2;

        // menu
        MenuBar             m_menu;
        MenuItem            m_menu_edit;
        Menu                m_submenu_edit;
        MenuItem            m_menu_edit_cut;
        MenuItem            m_menu_edit_copy;
        MenuItem            m_menu_edit_paste;
        MenuItem            m_menu_edit_delete;
        MenuItem            m_menu_edit_selectall;
        MenuItem            m_menu_edit_unselect;
        MenuItem            m_menu_edit_close;
        SeparatorMenuItem   m_menu_separator1;
        SeparatorMenuItem   m_menu_separator2;


        bool timer_callback();
        bool scroll_callback(GdkEventScroll* event);

        bool on_delete_event(GdkEventAny *event);
};

#endif