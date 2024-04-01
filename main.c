#include <gtk/gtk.h>

#define WINDOW_STATE_WITHDRAWN_OR_ICONIFIED_OR_MAXIMIZED_OR_FULLSCREEN \
(GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN)

#define MAIN_WINDOW_TYPE (main_window_get_type())
G_DECLARE_FINAL_TYPE(MainWindow, main_window, MAIN, WINDOW, GtkApplicationWindow)

struct _MainWindow {
    GtkApplicationWindow parent;
    GKeyFile* key_file;
    GtkWidget* vertical_paned;
    GtkWidget* horizontal_paned;
};

#define MAIN_APPLICATION_TYPE (main_application_get_type())
G_DECLARE_FINAL_TYPE(MainApplication, main_application, MAIN, APPLICATION, GtkApplication)

struct _MainApplication {
    GtkApplication parent;
};

// MainWindow:

G_DEFINE_TYPE(MainWindow, main_window, GTK_TYPE_APPLICATION_WINDOW);

static MainWindow* main_window_new(MainApplication* app) {
    return g_object_new(MAIN_WINDOW_TYPE, "application", app, NULL);
}

static void main_window_init(MainWindow* window) {
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

    window->horizontal_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_pack1(GTK_PANED(window->horizontal_paned), left_scrolled_window, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(window->horizontal_paned), right_scrolled_window, TRUE, FALSE);

    window->vertical_paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_pack1(GTK_PANED(window->vertical_paned), window->horizontal_paned, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(window->vertical_paned), bottom_scrolled_window, FALSE, FALSE);

    GtkWidget* header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);

    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    gtk_container_add(GTK_CONTAINER(window), window->vertical_paned);

    gtk_widget_grab_focus(left_text_view);

    // Load the UI state:

    window->key_file = g_key_file_new();
    g_key_file_load_from_file(window->key_file, "ui.ini", G_KEY_FILE_KEEP_COMMENTS, NULL);

    {
        gsize length;
        GError* error = NULL;
        gint* position = g_key_file_get_integer_list(window->key_file, "window", "position", &length, &error);

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
        gint* size = g_key_file_get_integer_list(window->key_file, "window", "size", &length, &error);

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
        gboolean maximized = g_key_file_get_boolean(window->key_file, "window", "maximized", &error);

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
        gint position = g_key_file_get_integer(window->key_file, "vertical_paned", "position", &error);

        if (error == NULL) {
            gtk_paned_set_position(GTK_PANED(window->vertical_paned), position);
        } else {
            g_error_free(error);
        }
    }

    {
        GError* error = NULL;
        gint position = g_key_file_get_integer(window->key_file, "horizontal_paned", "position", &error);

        if (error == NULL) {
            gtk_paned_set_position(GTK_PANED(window->horizontal_paned), position);
        } else {
            g_error_free(error);
        }
    }
}

static gboolean main_window_state_event(GtkWidget* window, GdkEventWindowState* event) {
    if ((event->new_window_state & WINDOW_STATE_WITHDRAWN_OR_ICONIFIED_OR_MAXIMIZED_OR_FULLSCREEN) == 0) {
        gint position[2];
        gtk_window_get_position(GTK_WINDOW(window), &position[0], &position[1]);
        g_key_file_set_integer_list(MAIN_WINDOW(window)->key_file, "window", "position", position, 2);

        gint size[2];
        gtk_window_get_size(GTK_WINDOW(window), &size[0], &size[1]);
        g_key_file_set_integer_list(MAIN_WINDOW(window)->key_file, "window", "size", size, 2);
    }

    return GDK_EVENT_PROPAGATE;
}

static gboolean main_window_delete_event(GtkWidget* window, GdkEventAny* event) {
    (void)event;

    {
        GdkWindowState state = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(window)));
        g_key_file_set_boolean(MAIN_WINDOW(window)->key_file, "window", "maximized", (state & GDK_WINDOW_STATE_MAXIMIZED) != 0);

        if ((state & WINDOW_STATE_WITHDRAWN_OR_ICONIFIED_OR_MAXIMIZED_OR_FULLSCREEN) == 0) {
            gint position[2];
            gtk_window_get_position(GTK_WINDOW(window), &position[0], &position[1]);
            g_key_file_set_integer_list(MAIN_WINDOW(window)->key_file, "window", "position", position, 2);

            gint size[2];
            gtk_window_get_size(GTK_WINDOW(window), &size[0], &size[1]);
            g_key_file_set_integer_list(MAIN_WINDOW(window)->key_file, "window", "size", size, 2);
        }
    }

    {
        gint position = gtk_paned_get_position(GTK_PANED(MAIN_WINDOW(window)->vertical_paned));
        g_key_file_set_integer(MAIN_WINDOW(window)->key_file, "vertical_paned", "position", position);
    }

    {
        gint position = gtk_paned_get_position(GTK_PANED(MAIN_WINDOW(window)->horizontal_paned));
        g_key_file_set_integer(MAIN_WINDOW(window)->key_file, "horizontal_paned", "position", position);
    }

    g_key_file_save_to_file(MAIN_WINDOW(window)->key_file, "ui.ini", NULL);

    return GDK_EVENT_PROPAGATE;
}

static void main_window_dispose(GObject* window) {
    if (MAIN_WINDOW(window)->key_file != NULL) {
        g_key_file_unref(MAIN_WINDOW(window)->key_file);
        MAIN_WINDOW(window)->key_file = NULL;
    }

    G_OBJECT_CLASS(main_window_parent_class)->dispose(window);
}

static void main_window_class_init(MainWindowClass* class) {
    GTK_WIDGET_CLASS(class)->window_state_event = main_window_state_event;
    GTK_WIDGET_CLASS(class)->delete_event = main_window_delete_event;
    G_OBJECT_CLASS(class)->dispose = main_window_dispose;
}

// MainApplication:

G_DEFINE_TYPE(MainApplication, main_application, GTK_TYPE_APPLICATION);

static MainApplication* main_application_new(void) {
    return g_object_new(MAIN_APPLICATION_TYPE, "flags", G_APPLICATION_DEFAULT_FLAGS, NULL);
}

static void main_application_init(MainApplication* app) {}

static void main_application_activate(GApplication* app) {
    MainWindow* window = main_window_new(MAIN_APPLICATION(app));
    gtk_widget_show_all(GTK_WIDGET(window));
}

static void main_application_class_init(MainApplicationClass* class) {
    G_APPLICATION_CLASS(class)->activate = main_application_activate;
}

// Entry point:

int main(int argc, char** argv) {
    MainApplication* app = main_application_new();
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
