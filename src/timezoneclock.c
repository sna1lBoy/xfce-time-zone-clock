#include <string.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include "timezoneclock.h"
#include "dialogs.h"
#include <time.h>

// make and register plugin
static void construct(XfcePanelPlugin *plugin);
XFCE_PANEL_PLUGIN_REGISTER(construct);

// structs for bundling data
typedef struct {
	GtkWidget *button;
	gchar *timeZone;
	gchar *format;
} clockData;

typedef struct {
	GtkWidget *ebox;
	gchar *format;
	gchar *font;
	gchar *additionalTimeZones;
} tooltipData;       

// write current values to disk
void saveSettings(XfcePanelPlugin *plugin, clockPlugin *clock) {

	// get rc using file
	gchar *file = xfce_panel_plugin_save_location(plugin, TRUE);
	if (G_UNLIKELY(file == NULL)) { return; }
	XfceRc *rc = xfce_rc_simple_open(file, FALSE);
	g_free (file);

	// write settings to rc
	if (G_LIKELY(rc != NULL)) {
		if (clock->timeZone) {
			xfce_rc_write_entry(rc, "timeZone", clock->timeZone);
		}
		if (clock->calendar) {
			xfce_rc_write_entry(rc, "calendar", clock->calendar);
		}
		if (clock->format) {
			xfce_rc_write_entry(rc, "format", clock->format);
		}
		if (clock->font) {
			xfce_rc_write_entry(rc, "font", clock->font);
		}
		if (clock->additionalTimeZones) {
			xfce_rc_write_entry(rc, "additionalTimeZones", clock->additionalTimeZones);
		}
		xfce_rc_close(rc);
	}
}

// read values from disk
static void readSettings(clockPlugin *clock) {

	// get rc using file
	gchar *file = xfce_panel_plugin_save_location(clock->plugin, TRUE);
	if (G_UNLIKELY(file == NULL)) { return; }
	XfceRc *rc = xfce_rc_simple_open (file, TRUE);
	g_free (file);

	// read settings from rc
	if (G_LIKELY(rc != NULL)) {
		const gchar *timeZoneValue = xfce_rc_read_entry(rc, "timeZone", NULL);
		clock->timeZone = g_strdup(timeZoneValue);

		const gchar *calendarValue = xfce_rc_read_entry(rc, "calendar", NULL);
		clock->calendar = g_strdup(calendarValue);

		const gchar *formatValue = xfce_rc_read_entry(rc, "format", NULL);
		clock->format = g_strdup(formatValue);

		const gchar *fontValue = xfce_rc_read_entry(rc, "font", NULL);
		clock->font = g_strdup(fontValue);

		const gchar *additionalTimeZonesValue = xfce_rc_read_entry (rc, "additionalTimeZones", NULL);
		clock->additionalTimeZones = g_strdup(additionalTimeZonesValue);

		// close the file and leave
		xfce_rc_close(rc);
		return;
	}

	// if something fucks up just panic and set them all to NULL, we'll deal with them later
	clock->timeZone = NULL;
	clock->calendar = NULL;
	clock->format = NULL;
       	clock->font = NULL;
	clock->additionalTimeZones = NULL;

}

// accessible widgets for future functions
GtkWidget *button = NULL;
GtkWidget *calendar = NULL;

// update main clock
static gboolean updateTime(gpointer userData) {
	clockData *data = (clockData *)userData;

	// initialize variables for time
	time_t rawTime;
	struct tm *timeInfo;
	char timeString[256];
	const gchar *timeZone = data->timeZone;
	
	// apply alternate time zone if it exists
	if (timeZone != NULL && strlen(timeZone) > 0) {
		setenv("TZ", timeZone, 1);
		tzset(); 
	} else {
		unsetenv("TZ");
		tzset();
	}

	// get and format time
	time(&rawTime);
	timeInfo = localtime(&rawTime);
       	if (data->format != NULL && strlen(data->format) > 0) {  
		strftime(timeString, sizeof(timeString), data->format, timeInfo);
	} else {
		strftime(timeString, sizeof(timeString), "%H:%M", timeInfo);
	}

	// set the label with the formatted time
	gtk_button_set_label(GTK_BUTTON(data->button), timeString);
	return TRUE;

}

// update tooltip clocks
static gboolean updateTooltip(gpointer userData) {
	
	// extract struct data
	tooltipData *data = (tooltipData *)userData;
	gchar *timeZonesCopy = g_strdup(data->additionalTimeZones ? data->additionalTimeZones : "");
	char *format = g_strdup(data->format);
	char *src = data->format, *dst = format;
	while (*src) {
    		if (*src != '\n') {
			*dst++ = *src;
		}
		src++;
	}
	*dst = '\0';

	// if given a list of time zones
	if (data->additionalTimeZones != NULL && strlen(data->additionalTimeZones) > 0) {

		// prepare to parse list
		gchar *result = g_strdup("");
		
		// loop through list
		gchar **tokens = g_strsplit(timeZonesCopy, ",", -1);
		for (gint i = 0; tokens[i] != NULL && i < 10; i++) {
			gchar *token = tokens[i];

			// set time zone to given one
			setenv("TZ", token, 1);
			tzset();
			
			//get time
			time_t rawTime = time(NULL);
			struct tm *timeInfo = localtime(&rawTime);
			char timeString[256];

			// format it to given format if it exists
			if (format != NULL && strlen(format) > 0) {  
				strftime(timeString, sizeof(timeString), format, timeInfo);
			} else {
				strftime(timeString, sizeof(timeString), "%H:%M", timeInfo);
			}

			// add identifier
			gchar *new_result = g_strdup_printf("%s%s (%s)\n", result, timeString, token);
			g_free(result);
			result = new_result;

		}
		
		// clean up and return
		result[strlen(result)-1]='\0';
		gtk_widget_set_tooltip_text(data->ebox, result);
		unsetenv("TZ");
		tzset();
		g_strfreev(tokens);
		g_free(timeZonesCopy);
		g_free(result);
		return TRUE;

	}
	
	// return if no additional time zones
	gtk_widget_set_tooltip_text(data->ebox, "(no additional time zones given)");

	return TRUE;

}

// hide calendar dialog
static gboolean focusOut(GtkWidget *widget, GdkEvent *event, gpointer data) {
	
	gtk_widget_hide(widget); 
	return FALSE;

}

// show calendar
static void onClick(GtkButton *button, gchar *command) {
	
	// run given command if it exists
	if (command != NULL && strlen(command) > 0) {
		system(command);
		return;
	}
	int wx = 0, wy = 0;

	// make a new calendar if needed
	if (calendar == NULL) {

		// prepare dialog
		calendar = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(calendar), "calendar");
 		gtk_window_set_modal(GTK_WINDOW(calendar), TRUE);
		gtk_window_set_decorated(GTK_WINDOW(calendar), FALSE);
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(calendar), TRUE);
		
		// calculate position
		GdkWindow *gdkWindow = gtk_widget_get_window(GTK_WIDGET(button));
		if (gdkWindow) {
			gdk_window_get_root_coords(gdkWindow, 0, 0, &wx, &wy);
		}
        	gtk_window_move(GTK_WINDOW(calendar), wx, wy + 30);

		// make calendar content and show
		GtkWidget *calendarContent = gtk_calendar_new();
		gtk_widget_show(calendarContent);
        	GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(calendar));
        	gtk_box_pack_start(GTK_BOX(vbox), calendarContent, TRUE, TRUE, 0);
		g_signal_connect(calendar, "response", G_CALLBACK(gtk_widget_hide), NULL);
		gtk_widget_show_all(calendar);
		g_signal_connect(calendar, "focus-out-event", G_CALLBACK(focusOut), NULL);

	// hide or show calendar if it exists
	}  else {
		if (gtk_widget_get_visible(calendar)) {
			gtk_widget_set_visible(calendar, FALSE);
        	} else {
 			GdkWindow *gdkWindow = gtk_widget_get_window(GTK_WIDGET(button));
			if (gdkWindow) {
				gdk_window_get_root_coords(gdkWindow, 0, 0, &wx, &wy);
			}
			gtk_window_move(GTK_WINDOW(calendar), wx, wy + 30);
			gtk_widget_set_visible(calendar, TRUE);
		}
	}
}

// main plugin function
static clockPlugin *new(XfcePanelPlugin *plugin) {

	// prepare plugin
	GtkOrientation orientation;
	clockPlugin *clock = g_slice_new0(clockPlugin);
	clock->plugin = plugin;
	readSettings(clock);
	orientation = xfce_panel_plugin_get_orientation(plugin);

	// set up container
	clock->ebox = gtk_event_box_new();
	gtk_widget_show(clock->ebox);
	clock->hvbox = gtk_box_new(orientation, 2);
	gtk_widget_show(clock->hvbox);
	gtk_container_add(GTK_CONTAINER(clock->ebox), clock->hvbox);

	// add clock
  	clock->button = gtk_button_new_with_label("");
	PangoFontDescription *fontDesc  = pango_font_description_from_string(clock->font);
	gtk_widget_override_font(GTK_WIDGET(clock->button), fontDesc);
	pango_font_description_free(fontDesc);
  	gtk_widget_show(clock->button);
  	gtk_box_pack_start(GTK_BOX(clock->hvbox), clock->button, FALSE, FALSE, 0);
  	xfce_panel_plugin_add_action_widget(plugin, clock->ebox);
	GtkWidget *label = gtk_label_new("00:00");
       	gtk_container_add(GTK_CONTAINER(button), label); 	

	// add calendar to button
	g_signal_connect(clock->button, "clicked", G_CALLBACK(onClick), clock->calendar);

	// start main clock upater
	clockData *data = g_new(clockData, 1);
	data->button = clock->button;
	data->timeZone = clock->timeZone;
	data->format = clock->format;
	g_timeout_add(100, updateTime, data);

	tooltipData *info = g_new(tooltipData, 1);
	info->ebox = clock->ebox;
	info->format = clock->format;
	info->additionalTimeZones = clock->additionalTimeZones;
  	g_timeout_add(510, updateTooltip, info);

	return clock;
}

// free allocated resources
static void freePlugin(XfcePanelPlugin *plugin, clockPlugin *clock) {

	GtkWidget *dialog = g_object_get_data(G_OBJECT(plugin), "dialog");
	if (G_UNLIKELY (dialog != NULL)) {
		gtk_widget_destroy(dialog);
	}
	gtk_widget_destroy (clock->hvbox);

	if (G_LIKELY(clock->timeZone != NULL)) {
		g_free(clock->timeZone);
	}
	if (G_LIKELY(clock->calendar != NULL)) {
		g_free(clock->calendar);
	}
	if (G_LIKELY(clock->format != NULL)) {
		g_free(clock->format);
	}
	if (G_LIKELY(clock->font != NULL)) {
		g_free(clock->font);
	}
	if (G_LIKELY(clock->additionalTimeZones != NULL)) {
		g_free(clock->additionalTimeZones);
	}

	g_slice_free(clockPlugin, clock);

}

// handling orientation changes
static void orientationChanged(XfcePanelPlugin *plugin, GtkOrientation orientation, clockPlugin *clock) {
  
	gtk_orientable_set_orientation(GTK_ORIENTABLE(clock->hvbox), orientation);

}

// handling size changes
static gboolean sizeChanged(XfcePanelPlugin *plugin, gint size, clockPlugin *clock) {

	GtkOrientation orientation = xfce_panel_plugin_get_orientation(plugin);
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		gtk_widget_set_size_request(GTK_WIDGET(plugin), -1, size);
	} else {
		gtk_widget_set_size_request(GTK_WIDGET(plugin), size, -1);
	}
	return TRUE;

}

// ...and ship it!
static void construct(XfcePanelPlugin *plugin) {

	clockPlugin *clock = new(plugin);
	gtk_container_add(GTK_CONTAINER(plugin), clock->ebox);
	xfce_panel_plugin_add_action_widget(plugin, clock->ebox);
	g_signal_connect(G_OBJECT(plugin), "free-data", G_CALLBACK(freePlugin), clock);
	g_signal_connect(G_OBJECT(plugin), "save", G_CALLBACK(saveSettings), clock);
	g_signal_connect(G_OBJECT(plugin), "size-changed", G_CALLBACK(sizeChanged), clock);
	g_signal_connect(G_OBJECT(plugin), "orientation-changed", G_CALLBACK(orientationChanged), clock);
	xfce_panel_plugin_menu_show_configure(plugin);
	g_signal_connect(G_OBJECT(plugin), "configure-plugin", G_CALLBACK(configure), clock);

}
