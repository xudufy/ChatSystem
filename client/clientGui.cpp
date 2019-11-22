#include "../common.h"
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <libgen.h>

#include "clientGui.h"
#include "clientChatData.h"
#include "clientNet.h"

using namespace std;

class ClientGuiData {
public:

    ClientGuiData() = default;
    inline ~ClientGuiData() {
        if (netcore) {
            delete netcore;
            netcore=NULL;
        }
    }

    bool loggedin = false;

    ClientNet * netcore=NULL;
    ClientChatData * netdata=NULL;
    GtkBuilder * builder = NULL;

    std::map<GtkListBoxRow*, string> RowtoNick;
    std::map<string, GtkListBoxRow*> NicktoRow;

};

//functions run in mainloop to do common operations
void write_main_window_log(ClientGuiData *guidata);
void refresh_msg_window(ClientGuiData *guidata);

//these are CB used in CB of netcore, which uses gdk_threads_add_idle () add them to run in main loop. 
//these functions must return FALSE to run only once.
gboolean login_ok_cb(ClientGuiData *guidata);
gboolean back_to_login_cb(ClientGuiData *guidata);
gboolean user_added_cb(ClientGuiData *guidata);
gboolean user_deleted_cb(ClientGuiData *guidata);

//whatever the sender is, call refresh_msg_window.
gboolean msg_recv_cb(ClientGuiData *guidata){
    refresh_msg_window(guidata);
    write_main_window_log(guidata);
    return false;
}
gboolean msg_recv_error_cb(ClientGuiData *guidata) {
    write_main_window_log(guidata);
    return false;
}

//these are connect to signals of GUI widgets.
void login_clicked_cb( GtkWidget *widget, ClientGuiData * guidata);
void msg_send_cb(GtkWidget *widget, ClientGuiData * guidata);

//if the user logged out before the tabid is chosen, do nothing (or maybe call user_deleted_cb).
//if normal, change the currentChosenRow and call refresh_msg_window. 
void switch_user_cb(GtkListBox *box, GtkListBoxRow *row, ClientGuiData *guidata);
void logout_cb(GtkWidget *widget, ClientGuiData * guidata);

int gui_entry(int argc, char *argv[]) {
    GtkBuilder* builder;
    GError* err = NULL;
    gtk_init (&argc, &argv);
    builder = gtk_builder_new();

    char buf[4096];
    strncpy(buf,argv[0],4095);
    string exedir = dirname(buf);

    if (gtk_builder_add_from_file(builder, (exedir+"/client.glade").c_str(), &err) == 0) {
        NPNX_LOG("builder", err->message);
        g_clear_error(&err);
        exit(-1);
    }

    ClientGuiData guiData;
    //setup netcore and callback.
    guiData.netcore = new ClientNet();
    guiData.netdata = guiData.netcore->getChatDataPointer();
    guiData.builder = builder;
    ClientGuiData *guiDataP = &guiData;

#define LOCAL_ADDNETCB(A, B) \
    guiData.netcore-> A = [guiDataP](){gdk_threads_add_idle ((GSourceFunc)B, guiDataP);}

    LOCAL_ADDNETCB(connectionErrorCB, back_to_login_cb);
    LOCAL_ADDNETCB(loginOKCB, login_ok_cb);
    LOCAL_ADDNETCB(loginErrorCB, back_to_login_cb);
    LOCAL_ADDNETCB(oneUserAddedCB, user_added_cb);
    LOCAL_ADDNETCB(oneUserDeletedCB, user_deleted_cb);
    LOCAL_ADDNETCB(messageReceivedCB, msg_recv_cb);
    LOCAL_ADDNETCB(messageErrorCB, msg_recv_error_cb);

#undef LOCAL_ADDNETCB
    //set up gui callback.
    GObject *login_window = gtk_builder_get_object(builder, "login_window");
    g_signal_connect(login_window, "delete-event", G_CALLBACK(gtk_main_quit), NULL);

    GObject *main_window = gtk_builder_get_object(builder, "main_window");
    g_signal_connect(main_window, "delete-event", G_CALLBACK(gtk_main_quit), NULL);

    GObject *login_button = gtk_builder_get_object(builder, "login_button");
    g_signal_connect(login_button, "clicked", G_CALLBACK(login_clicked_cb), &guiData);

    GObject *logout_button = gtk_builder_get_object(builder, "logout_button");
    g_signal_connect(logout_button, "clicked", G_CALLBACK(logout_cb), &guiData);

    GObject *send_button = gtk_builder_get_object(builder, "send_button");
    g_signal_connect(send_button, "clicked", G_CALLBACK(msg_send_cb), &guiData);

    GObject *user_list = gtk_builder_get_object(builder, "user_list");
    g_signal_connect(user_list, "row-activated", G_CALLBACK(switch_user_cb), &guiData);


    gtk_widget_show_all(GTK_WIDGET(login_window));
    gtk_widget_show_all(GTK_WIDGET(main_window));
    gtk_window_set_transient_for(GTK_WINDOW(login_window), GTK_WINDOW(main_window));
    gtk_main();
    g_object_unref(G_OBJECT(builder));
    return 0;
}

int main(int argc, char *argv[]) {
    return gui_entry(argc, argv);
}

#define LOCAL_UNPACK_GUIDATAP() \
    ClientNet * netcore = guidata->netcore; \
    ClientChatData * netdata = guidata->netdata; \
    GtkBuilder * builder = guidata->builder;

//functions run in mainloop to do common operations
void write_main_window_log(ClientGuiData *guidata)
{
    LOCAL_UNPACK_GUIDATAP()
    GtkLabel *notify_label;
    notify_label = GTK_LABEL(gtk_builder_get_object(builder, "notify_label"));
    string loglabel = "";
    {
        lock_guard<mutex> lock(netdata->logsmutex);
        int n = netdata->logs.size();
        for (int i = 5; i>=1; i--) {
            if (n-i>=0) {
                loglabel+=netdata->logs[n-i]+"\n";
            }
        }
    }
    gtk_label_set_label(notify_label, loglabel.c_str());
}

void refresh_msg_window(ClientGuiData *guidata)
{
    LOCAL_UNPACK_GUIDATAP()
    GtkTextView * message_view;
    message_view = GTK_TEXT_VIEW( gtk_builder_get_object(builder, "message_view"));
    
    GtkListBox * user_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "user_list"));
    GList * rowlist = gtk_list_box_get_selected_rows(user_list);

    GtkListBoxRow * currentRow = NULL;
    if (rowlist != NULL) {
        currentRow = GTK_LIST_BOX_ROW(g_list_nth_data(rowlist, 0));
        g_list_free(rowlist);
    }
    if (guidata->RowtoNick.count(currentRow) > 0) {
        string & nick = guidata->RowtoNick[currentRow];
        string msgs = "";
        bool userfound = false;
        {
            lock_guard<mutex> lock(netdata->msgmutex);
            if (netdata->userMessages.count(nick) > 0) {
                for (auto iter: netdata->userMessages[nick]) {
                    msgs+=iter+"\n";
                }
                userfound = true;
            }
        }
        if (userfound) {
            GtkTextBuffer* tb = gtk_text_view_get_buffer(message_view);
            gtk_text_buffer_set_text(tb, msgs.c_str(), msgs.size());
        }
    }
}

//these are CB used in CB of netcore, which uses gdk_threads_add_idle () add them to run in main loop. 
//these functions must return FALSE to run only once.
gboolean login_ok_cb(ClientGuiData *guidata)
{
    LOCAL_UNPACK_GUIDATAP()
    GObject *login_window = gtk_builder_get_object(builder, "login_window");

    GtkLabel *my_nickname_label = GTK_LABEL(gtk_builder_get_object(builder, "my_nickname_label"));
    {
        lock_guard<mutex> lock(netdata->mynamemutex);
        gtk_label_set_label(my_nickname_label, netdata->myNickName.c_str());
    }

    user_added_cb(guidata);
    user_deleted_cb(guidata);

    if (guidata->RowtoNick.size()>0) {
        auto iter = guidata->RowtoNick.begin();
        gtk_widget_activate(GTK_WIDGET(iter->first));
    }

    write_main_window_log(guidata);

    gtk_widget_hide(GTK_WIDGET(login_window));
    
    return false;
}

gboolean back_to_login_cb(ClientGuiData *guidata)
{

    LOCAL_UNPACK_GUIDATAP()
    //set the elements on login window properly.
    GtkEntry * nickname_input = GTK_ENTRY(gtk_builder_get_object(builder, "nickname_input"));    
    gtk_entry_set_text(nickname_input, "");
    
    GtkLabel *login_error_label;
    login_error_label = GTK_LABEL(gtk_builder_get_object(builder, "login_error_label"));
    string loglabel = "";
    {
        lock_guard<mutex> lock(netdata->logsmutex);
        int n = netdata->logs.size();
        if (n>0) {
            loglabel+=netdata->logs.back();
        }
    }
    gtk_label_set_text(login_error_label, loglabel.c_str());

    GtkButton * login_button = GTK_BUTTON(gtk_builder_get_object(builder, "login_button"));
    gtk_button_set_label(login_button, "Login");
    gtk_widget_set_sensitive(GTK_WIDGET(login_button), true);

    GObject *login_window = gtk_builder_get_object(builder, "login_window");
    gtk_widget_show_all(GTK_WIDGET(login_window));

    return false;
}

gboolean user_added_cb(ClientGuiData *guidata)
{
    LOCAL_UNPACK_GUIDATAP()

    vector<string> usersnap;
    usersnap.clear();
    {
        lock_guard<mutex> lock(netdata->msgmutex);
        for (auto iter:netdata->userMessages) {
            usersnap.push_back(iter.first);
        }    
    }

    GtkListBox * user_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "user_list"));
    for (auto realuser: usersnap) {
        if (guidata->NicktoRow.count(realuser)==0) {
            GtkListBoxRow * newrow = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
            GtkLabel * newlabel = GTK_LABEL(gtk_label_new(realuser.c_str()));
            gtk_container_add(GTK_CONTAINER(newrow), GTK_WIDGET(newlabel));
            g_object_set(G_OBJECT(newrow), "width_request", 100, "visible", true, "can_focus", true, NULL);
            g_object_set(G_OBJECT(newlabel), "height_request", 35, "visible", true, "can_focus", false, "margin_top",2, "margin_bottom",2, NULL);
            gtk_label_set_justify(newlabel, GTK_JUSTIFY_CENTER);
            gtk_label_set_line_wrap(newlabel, true);
            gtk_label_set_line_wrap_mode(newlabel, PANGO_WRAP_CHAR);
            gtk_label_set_max_width_chars(newlabel, 14);

            guidata->NicktoRow[realuser] = newrow;
            guidata->RowtoNick[newrow] = realuser;

            gtk_container_add(GTK_CONTAINER(user_list), GTK_WIDGET(newrow));
        }
    }
    gtk_widget_show_all(GTK_WIDGET(user_list));

    write_main_window_log(guidata);

    return false;
}

gboolean user_deleted_cb(ClientGuiData *guidata)
{
    LOCAL_UNPACK_GUIDATAP()

    vector<string> usersnap;
    usersnap.clear();
    for (auto iter:guidata->NicktoRow) {
        usersnap.push_back(iter.first);
    }    

    GtkListBox * user_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "user_list"));
    {
        lock_guard<mutex> lock(netdata->msgmutex);
        for (auto myuser: usersnap) {
            if (netdata->userMessages.count(myuser)==0) {
                GtkListBoxRow * thisrow = guidata->NicktoRow[myuser];
                guidata->RowtoNick.erase(thisrow);
                guidata->NicktoRow.erase(myuser);
                gtk_container_remove(GTK_CONTAINER(user_list), GTK_WIDGET(thisrow));
            }
        }
    }
        
    gtk_widget_show_all(GTK_WIDGET(user_list));

    write_main_window_log(guidata);
    return false;
}

void login_clicked_cb( GtkWidget *widget, ClientGuiData * guidata)
{
    LOCAL_UNPACK_GUIDATAP()
    
    GtkButton * login_button = GTK_BUTTON(widget);
    gtk_widget_set_sensitive(widget, false);
    gtk_button_set_label(login_button, "Pending");

    GtkEntry * nickname_input = 
        GTK_ENTRY(gtk_builder_get_object(builder, "nickname_input"));
    
    string nick(gtk_entry_get_text(nickname_input));
    if (nick.empty()) {
        netcore->writeLog("nickname could not be empty");
        back_to_login_cb(guidata);
    } else {
        netcore->Login(nick);
    }
    
}

void msg_send_cb(GtkWidget *widget, ClientGuiData * guidata)
{
    LOCAL_UNPACK_GUIDATAP()

    GtkListBox * user_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "user_list"));
    GList * rowlist = gtk_list_box_get_selected_rows(user_list);

    GtkListBoxRow * currentRow = NULL;
    if (rowlist != NULL) {
        currentRow = GTK_LIST_BOX_ROW(g_list_nth_data(rowlist, 0));
        g_list_free(rowlist);
    }
    if (currentRow && guidata->RowtoNick.count(currentRow)>0) {
        GtkEntry * message_entry = 
            GTK_ENTRY(gtk_builder_get_object(builder, "message_entry"));

        string line = gtk_entry_get_text(message_entry);
        gtk_entry_set_text(message_entry, "");

        netcore->SendMessageTo(guidata->RowtoNick[currentRow], line);
        refresh_msg_window(guidata);
    } else {
        netcore->writeLog("send: no user selected.");
        write_main_window_log(guidata);
    }
    
}

void switch_user_cb(GtkListBox *box, GtkListBoxRow *row, ClientGuiData *guidata)
{
    refresh_msg_window(guidata);
}

void logout_cb(GtkWidget *widget, ClientGuiData * guidata)
{
    LOCAL_UNPACK_GUIDATAP()

    netcore->Logout();
    back_to_login_cb(guidata);

}

#undef LOCAL_UNPACK_GUIDATAP
