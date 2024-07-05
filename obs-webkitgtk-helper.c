/*
 * obs-webkitgtk. OBS Studio source plugin.
 * Copyright (C) 2020 Florian Zwoch <fzwoch@gmail.com>
 *
 * This file is part of obs-webkitgtk.
 *
 * obs-webkitgtk is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * obs-webkitgtk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obs-webkitgtk. If not, see <http://www.gnu.org/licenses/>.
 */

#include <webkit2/webkit2.h>

#define SECONDS_TIMEOUT 60

static guint timeout_id;

static gboolean timeout(gpointer user_data)
{
	gtk_widget_queue_draw(user_data);

	return TRUE;
}

static gboolean capture(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	size_t res;
	cairo_surface_t *surface = gtk_offscreen_window_get_surface(
		GTK_OFFSCREEN_WINDOW(user_data));

	if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_IMAGE) {
		res = fwrite(cairo_image_surface_get_data(surface),
			     cairo_image_surface_get_stride(surface) *
				     cairo_image_surface_get_height(surface),
			     1, stdout);
	} else {
		GdkPixbuf *pix = gtk_offscreen_window_get_pixbuf(
			GTK_OFFSCREEN_WINDOW(user_data));

		res = fwrite(gdk_pixbuf_read_pixels(pix),
			     gdk_pixbuf_get_rowstride(pix) *
				     gdk_pixbuf_get_height(pix),
			     1, stdout);

		g_object_unref(pix);
	}
	fflush(stdout);

	if (res == 0) {
		gtk_main_quit();
	}

	g_source_remove(timeout_id);
	timeout_id = g_timeout_add_seconds(SECONDS_TIMEOUT, timeout, user_data);

	return TRUE;
}

int main(int argc, char **argv)
{
	if (argc != 5) {
		printf("Usage: %s <width> <height> <url> <css>\n", argv[0]);
		return 0;
	}

	gtk_init(&argc, &argv);

	GtkWidget *window = gtk_offscreen_window_new();
	gtk_window_set_default_size(GTK_WINDOW(window), atoi(argv[1]),
				    atoi(argv[2]));

	GtkCssProvider *css = gtk_css_provider_new();
	gtk_css_provider_load_from_data(
		css, argv[4], -1, NULL);
	gtk_style_context_add_provider(gtk_widget_get_style_context(window),
				       GTK_STYLE_PROVIDER(css),
				       GTK_STYLE_PROVIDER_PRIORITY_USER);
	g_object_unref(css);

	WebKitWebView *webview = WEBKIT_WEB_VIEW(webkit_web_view_new());

	WebKitSettings *settings = webkit_web_view_get_settings(webview);
	webkit_settings_set_hardware_acceleration_policy(
		settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

	GdkRGBA bg = {1.0, 1.0, 0.0, 0.0};
	webkit_web_view_set_background_color(webview, &bg);

	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webview));
	webkit_web_view_load_uri(webview, argv[3]);
	gtk_widget_show_all(window);

	g_signal_connect(window, "damage-event", G_CALLBACK(capture), window);

	timeout_id = g_timeout_add_seconds(SECONDS_TIMEOUT, timeout, window);

	gtk_main();

	return 0;
}
