#include <gtk/gtk.h>

#define WINDOW_STATE_WITHDRAWN_OR_ICONIFIED_OR_MAXIMIZED_OR_FULLSCREEN \
(GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN)

typedef struct WindowDeleteEventHandlerData {
    GKeyFile* key_file;
    GtkWidget* vertical_paned;
    GtkWidget* horizontal_paned;
} WindowDeleteEventHandlerData;

static WindowDeleteEventHandlerData* window_delete_event_handler_data_new(
    GKeyFile* key_file,
    GtkWidget* vertical_paned,
    GtkWidget* horizontal_paned
) {
    WindowDeleteEventHandlerData* data = g_malloc(sizeof(WindowDeleteEventHandlerData));
    data->key_file = g_key_file_ref(key_file);
    data->vertical_paned = vertical_paned;
    data->horizontal_paned = horizontal_paned;
    return data;
}

static void window_delete_event_handler_data_free(WindowDeleteEventHandlerData* data) {
    g_key_file_unref(data->key_file);
    g_free(data);
}

static gboolean window_state_event_handler(GtkWidget* window, GdkEvent* event, gpointer _data) {
    GKeyFile* key_file = _data;

    if ((event->window_state.new_window_state & WINDOW_STATE_WITHDRAWN_OR_ICONIFIED_OR_MAXIMIZED_OR_FULLSCREEN) == 0) {
        gint position[2];
        gtk_window_get_position(GTK_WINDOW(window), &position[0], &position[1]);
        g_key_file_set_integer_list(key_file, "window", "position", position, 2);

        gint size[2];
        gtk_window_get_size(GTK_WINDOW(window), &size[0], &size[1]);
        g_key_file_set_integer_list(key_file, "window", "size", size, 2);
    }

    return GDK_EVENT_PROPAGATE;
}

static void destroy_window_state_event_handler_data(gpointer _data, GClosure* closure) {
    GKeyFile* key_file = _data;
    (void)closure;

    g_key_file_unref(key_file);
}

static gboolean window_delete_event_handler(GtkWidget* window, GdkEvent* event, gpointer _data) {
    (void)event;
    const WindowDeleteEventHandlerData* data = _data;

    {
        GdkWindowState state = gdk_window_get_state(gtk_widget_get_window(window));
        g_key_file_set_boolean(data->key_file, "window", "maximized", (state & GDK_WINDOW_STATE_MAXIMIZED) != 0);

        if ((state & WINDOW_STATE_WITHDRAWN_OR_ICONIFIED_OR_MAXIMIZED_OR_FULLSCREEN) == 0) {
            gint position[2];
            gtk_window_get_position(GTK_WINDOW(window), &position[0], &position[1]);
            g_key_file_set_integer_list(data->key_file, "window", "position", position, 2);

            gint size[2];
            gtk_window_get_size(GTK_WINDOW(window), &size[0], &size[1]);
            g_key_file_set_integer_list(data->key_file, "window", "size", size, 2);
        }
    }

    {
        gint position = gtk_paned_get_position(GTK_PANED(data->vertical_paned));
        g_key_file_set_integer(data->key_file, "vertical_paned", "position", position);
    }

    {
        gint position = gtk_paned_get_position(GTK_PANED(data->horizontal_paned));
        g_key_file_set_integer(data->key_file, "horizontal_paned", "position", position);
    }

    g_key_file_save_to_file(data->key_file, "ui.ini", NULL);

    return GDK_EVENT_PROPAGATE;
}

static void destroy_window_delete_event_handler_data(gpointer _data, GClosure* closure) {
    WindowDeleteEventHandlerData* data = _data;
    (void)closure;

    window_delete_event_handler_data_free(data);
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
        gint* position = g_key_file_get_integer_list(key_file, "window", "position", &length, &error);

        if (error == NULL) {
            if (length == 2) {
                gtk_window_move(GTK_WINDOW(window), position[0], position[1]);
            }

            g_free(position);
        } else {
            g_error_free(error);
        }
    }

    {
        gsize length;
        GError* error = NULL;
        gint* size = g_key_file_get_integer_list(key_file, "window", "size", &length, &error);

        if (error == NULL) {
            if (length == 2) {
                gtk_window_resize(GTK_WINDOW(window), size[0], size[1]);
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

    g_signal_connect_data(
        window,
        "window-state-event",
        G_CALLBACK(window_state_event_handler),
        g_key_file_ref(key_file),
        destroy_window_state_event_handler_data,
        G_CONNECT_DEFAULT
    );

    g_signal_connect_data(
        window,
        "delete-event",
        G_CALLBACK(window_delete_event_handler),
        window_delete_event_handler_data_new(key_file, vertical_paned, horizontal_paned),
        destroy_window_delete_event_handler_data,
        G_CONNECT_DEFAULT
    );

    // Show the UI:

    gtk_widget_grab_focus(left_text_view);
    gtk_widget_show_all(window);

    // Unreference pending objects:

    g_key_file_unref(key_file);
}

int main(int argc, char** argv) {
    GtkApplication* app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate_handler), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
