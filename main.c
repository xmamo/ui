#include <gtk/gtk.h>

static void vertical_paned_notify_position_handler(GObject* vertical_paned, GParamSpec* pspec, gpointer _data) {
    (void)pspec;
    GKeyFile* key_file = _data;

    g_key_file_set_integer(key_file, "vertical_paned", "position", gtk_paned_get_position(GTK_PANED(vertical_paned)));
}

static void horizontal_paned_notify_position_handler(GObject* horizontal_paned, GParamSpec* pspec, gpointer _data) {
    (void)pspec;
    GKeyFile* key_file = _data;

    g_key_file_set_integer(key_file, "horizontal_paned", "position", gtk_paned_get_position(GTK_PANED(horizontal_paned)));
}

static gboolean window_state_event_handler(GtkWidget* window, GdkEvent* event, gpointer _data) {
    GKeyFile* key_file = _data;

    gboolean maximized = (event->window_state.new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
    g_key_file_set_boolean(key_file, "window", "maximized", maximized);

    return GDK_EVENT_PROPAGATE;
}

static void window_size_allocate_handler(GtkWidget* window, GtkAllocation* allocation, gpointer _data) {
    GKeyFile* key_file = _data;
    (void)allocation;

    GError* error = NULL;
    gboolean maximized = g_key_file_get_boolean(key_file, "window", "maximized", &error);

    if (error == NULL) {
        if (!maximized) {
            gint size[2];
            gtk_window_get_size(GTK_WINDOW(window), &size[0], &size[1]);
            g_key_file_set_integer_list(key_file, "window", "size", size, 2);
        }
    } else {
        g_error_free(error);
    }
}

static void window_destroy_handler(GtkWidget* window, gpointer _data) {
    (void)window;
    GKeyFile* key_file = _data;

    g_key_file_save_to_file(key_file, "ui.ini", NULL);
    g_key_file_unref(key_file);
}

static void app_activate_handler(GtkApplication* app, gpointer _data) {
    (void)_data;

    // Build the UI:

    GtkWidget* left_text_view = gtk_text_view_new();
    GtkWidget* right_text_view = gtk_text_view_new();
    GtkWidget* bottom_text_view = gtk_text_view_new();

    GtkWidget* left_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(left_scrolled_window), left_text_view);

    GtkWidget* right_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(right_scrolled_window), right_text_view);

    GtkWidget* bottom_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(bottom_scrolled_window), bottom_text_view);

    GtkWidget* horizontal_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_pack1(GTK_PANED(horizontal_paned), left_scrolled_window, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(horizontal_paned), right_scrolled_window, TRUE, FALSE);

    GtkWidget* vertical_paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_pack1(GTK_PANED(vertical_paned), horizontal_paned, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(vertical_paned), bottom_scrolled_window, FALSE, FALSE);

    GtkWidget* header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);

    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    gtk_container_add(GTK_CONTAINER(window), vertical_paned);

    // Load the UI state:

    GKeyFile* key_file = g_key_file_new();
    g_key_file_load_from_file(key_file, "ui.ini", G_KEY_FILE_KEEP_COMMENTS, NULL);

    {
        gsize length;
        GError* error = NULL;
        gint* size = g_key_file_get_integer_list(key_file, "window", "size", &length, &error);

        if (error == NULL) {
            if (length == 2) {
                gtk_window_set_default_size(GTK_WINDOW(window), size[0], size[1]);
            }

            g_free(size);
        } else {
            g_error_free(error);
        }
    }

    {
        GError* error = NULL;
        gboolean maximized = g_key_file_get_boolean(key_file, "window", "maximized", &error);

        if (error == NULL) {
            if (maximized) {
                gtk_window_maximize(GTK_WINDOW(window));
            }
        } else {
            g_error_free(error);
        }
    }

    {
        GError* error = NULL;
        gint position = g_key_file_get_integer(key_file, "vertical_paned", "position", &error);

        if (error == NULL) {
            gtk_paned_set_position(GTK_PANED(vertical_paned), position);
        } else {
            g_error_free(error);
        }
    }

    {
        GError* error = NULL;
        gint position = g_key_file_get_integer(key_file, "horizontal_paned", "position", &error);

        if (error == NULL) {
            gtk_paned_set_position(GTK_PANED(horizontal_paned), position);
        } else {
            g_error_free(error);
        }
    }

    // Connect the signal handlers to save the UI state:

    g_signal_connect(horizontal_paned, "notify::position", G_CALLBACK(horizontal_paned_notify_position_handler), key_file);
    g_signal_connect(vertical_paned, "notify::position", G_CALLBACK(vertical_paned_notify_position_handler), key_file);
    g_signal_connect(window, "window-state-event", G_CALLBACK(window_state_event_handler), key_file);
    g_signal_connect(window, "size-allocate", G_CALLBACK(window_size_allocate_handler), key_file);
    g_signal_connect(window, "destroy", G_CALLBACK(window_destroy_handler), key_file);

    // Show the UI:

    gtk_widget_grab_focus(left_text_view);
    gtk_widget_show_all(window);
}

int main(int argc, char** argv) {
    GtkApplication* app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate_handler), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
