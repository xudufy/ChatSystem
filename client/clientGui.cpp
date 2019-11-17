#include "../common.h"
#include "clientGui.h"
#include "clientChatData.h"

using namespace std;

class ClientGuiData {
public:
    int numUserIndex;
    int currentChosenUser;
    std::map<int, string> idToNick;
    std::map<string, GtkWidget*> NicktoRow;

};

ClientGuiData clientData;
ClientChatData chatData;

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

    GObject *login_window = gtk_builder_get_object(builder, "login_window");
    g_signal_connect(login_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GObject *main_window = gtk_builder_get_object(builder, "main_window");
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(GTK_WIDGET(login_window));
    gtk_widget_show_all(GTK_WIDGET(main_window));
    gtk_main();
    return 0;
}
int main(int argc, char *argv[]) {
    return gui_entry(argc, argv);
}
