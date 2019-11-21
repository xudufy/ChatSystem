#include "../common.h"
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

    int numUserIndex = 0;
    int currentChosenUser = 0;
    std::map<int, string> idToNick;
    std::map<string, GtkWidget*> NicktoRow;
    ClientNet * netcore=NULL;
    ClientChatData * netdata=NULL;
    GtkBuilder * builder = NULL;

};

ClientGuiData guiData;

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
gboolean msg_recv_cb(ClientGuiData *guidata);

//these are connect to signals of GUI widgets.
void login_clicked_cb( GtkWidget *widget, ClientGuiData * guidata);
void msg_send_cb(GtkWidget *widget, ClientGuiData * guidata);

//if the user logged out before the tabid is chosen, do nothing (or maybe call user_deleted_cb).
//if normal, change the currentChosenUser and call refresh_msg_window. 
void switch_user_cb(GtkWidget *widget, int *tabid);
void logout_cb(GtkWidget *widget, ClientGuiData * guidata);


int gui_entry(int argc, char *argv[]) {
    GtkBuilder* builder;
    GError* err = NULL;
    gtk_init (&argc, &argv);
    builder = gtk_builder_new();
    if (gtk_builder_add_from_file(builder, "client.glade", &err) == 0) {
        NPNX_LOG("builder", err->message);
        g_clear_error(&err);
        exit(-1);
    }

    //setup netcore and callback.
    guiData.netcore = new ClientNet();
    guiData.netdata = guiData.netcore->getChatDataPointer();
    guiData.builder = builder;


    //set up gui callback.
    GObject *login_window = gtk_builder_get_object(builder, "login_window");
    g_signal_connect(login_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GObject *main_window = gtk_builder_get_object(builder, "main_window");
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GObject *login_button = gtk_builder_get_object(builder, "login_button");
    g_signal_connect(login_button, "clicked", G_CALLBACK(login_clicked_cb), guiData);

    GObject *logout_button = gtk_builder_get_object(builder, "logout_button");
    g_signal_connect(logout_button, "clicked", G_CALLBACK(logout_cb), guiData);

    GObject *send_button = gtk_builder_get_object(builder, "send_button");
    g_signal_connect(send_button, "clicked", G_CALLBACK(msg_send_cb), guiData);

    gtk_widget_show_all(GTK_WIDGET(login_window));
    gtk_widget_show_all(GTK_WIDGET(main_window));
    gtk_main();
    g_object_unref(G_OBJECT(builder));
    gtk_widget_destory(login_window);
    gtk_widget_destory(main_window);
    return 0;
}



int main(int argc, char *argv[]) {
    return gui_entry(argc, argv);
}
