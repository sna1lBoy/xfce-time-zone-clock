#include <string.h>
#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>
#include "timezoneclock.h"
#include "dialogs.h"

// stripping new lines
char *processNewlines(const char *input) {
    
	// prepare memory
	size_t len = strlen(input);
	char *output = malloc(len + 1); 
	char *ptr = output;
	if (!output) { return NULL; }
	
	// parse string
	while (*input) {
		if (*input == '\\' && *(input + 1) == 'n') {
			*ptr++ = '\n'; 
			input += 2;  
		} else {
			*ptr++ = *input++;
		}
	}
	*ptr = '\0';
	return output;

}

// respond to buttons in config menu
static void configureResponse(GtkWidget *dialog, gint response, clockPlugin *clock) {

	// if "help" pressed
	if (response == GTK_RESPONSE_HELP) {
		g_spawn_command_line_async ("exo-open --launch WebBrowser https://codeberg.org/snailboy/xfce-time-zone-clock", NULL); 
	
	// otherwise prepare to close
	} else {

		// save alternative main time zone
		GtkWidget *timeZoneEntry = g_object_get_data(G_OBJECT(dialog), "timeZoneEntry");
		g_free(clock->timeZone);
		clock->timeZone = g_strdup(gtk_entry_get_text(GTK_ENTRY(timeZoneEntry)));

		// save alternative calendar command
		GtkWidget *calendarEntry = g_object_get_data(G_OBJECT(dialog), "calendarEntry");
		g_free(clock->calendar);
		clock->calendar = g_strdup(gtk_entry_get_text(GTK_ENTRY(calendarEntry)));

		// save format
		GtkWidget *formatEntry = g_object_get_data(G_OBJECT(dialog), "formatEntry");
		const char *entryText = gtk_entry_get_text(GTK_ENTRY(formatEntry));
		char *processedText = processNewlines(entryText);
		g_free(clock->format);
		clock->format = g_strdup(processedText);
		g_free(processedText);

		// save additional time zones
		GtkWidget *additionalTimeZonesEntry = g_object_get_data(G_OBJECT(dialog), "additionalTimeZonesEntry");
		const gchar *entry = gtk_entry_get_text(GTK_ENTRY(additionalTimeZonesEntry));
		g_free(clock->additionalTimeZones);
		clock->additionalTimeZones = g_strdup(entry);

		// clean up
		g_object_set_data(G_OBJECT(clock->plugin), "dialog", NULL); 
		xfce_panel_plugin_unblock_menu(clock->plugin);
		saveSettings(clock->plugin, clock);
		gtk_widget_destroy(dialog);
		system("xfce4-panel -r"); // hack, additionalTimeZones gets corrupted in updateTooltip and i can't fix it [pull requests welcome :)]
	}
}

// weird GTK font shenanigans... still not sure why it needs its own callback unlike the other settings?
static void fontSet(GtkFontButton *button, gpointer data) {

	// get font and save it to settings
	clockPlugin *clock = (clockPlugin *)data;
	gchar *font = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(button));
	g_free(clock->font);
	clock->font = g_strdup(font);
	saveSettings(clock->plugin, clock);

	// update font button
	gtk_button_set_label(GTK_BUTTON(button), font);
	gtk_widget_queue_draw(GTK_WIDGET(button));

	// update clock font
	PangoFontDescription *fontDesc = pango_font_description_from_string(font);
	gtk_widget_override_font(GTK_WIDGET(clock->button), fontDesc);
	pango_font_description_free(fontDesc);

}

// creating the config page
void configure(XfcePanelPlugin *plugin, clockPlugin *clock) {

	// set up dialog
	GtkWidget *dialog = xfce_titled_dialog_new_with_mixed_buttons(_("Sample Plugin"), GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))), GTK_DIALOG_DESTROY_WITH_PARENT, "help-browser-symbolic", _("_Help"), GTK_RESPONSE_HELP, "window-close-symbolic", _("_Close"), GTK_RESPONSE_OK, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name(GTK_WINDOW(dialog), "xfce4-settings");
	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);
	GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(content_area), mainBox);

	// main time zone
	GtkWidget *timeZoneBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkWidget *timeZoneLabel = gtk_label_new("alternate main time zone:");
	GtkWidget *timeZoneEntry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(timeZoneEntry), clock->timeZone);
	gtk_box_pack_start(GTK_BOX(timeZoneBox), timeZoneLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(timeZoneBox), timeZoneEntry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainBox), timeZoneBox, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(dialog), "timeZoneEntry", timeZoneEntry);

	// calendar
	GtkWidget *calendarBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkWidget *calendarLabel = gtk_label_new("alternate calendar command:");
	GtkWidget *calendarEntry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(calendarEntry), clock->calendar);
	gtk_box_pack_start(GTK_BOX(calendarBox), calendarLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(calendarBox), calendarEntry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainBox), calendarBox, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(dialog), "calendarEntry", calendarEntry);

	// format
	GtkWidget *formatBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkWidget *formatLabel = gtk_label_new("date and time format:");
	GtkWidget *formatEntry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(formatEntry), clock->format); 
	gtk_box_pack_start(GTK_BOX(formatBox), formatLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(formatBox), formatEntry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainBox), formatBox, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(dialog), "formatEntry", formatEntry);

	// font
	GtkWidget *fontBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); 
	GtkWidget *fontLabel = gtk_label_new("font:");  
	GtkWidget *fontButton = gtk_font_button_new();
	gtk_font_chooser_set_font(GTK_FONT_CHOOSER(fontButton), clock->font);
	gtk_button_set_label(GTK_BUTTON(fontButton), clock->font);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(fontButton), TRUE);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(fontButton), TRUE);
	g_signal_connect(fontButton, "font-set", G_CALLBACK(fontSet), clock);
	gtk_box_pack_start(GTK_BOX(fontBox), fontLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(fontBox), fontButton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainBox), fontBox, FALSE, FALSE, 0);

	// additional time zones
	GtkWidget *additionalTimeZonesBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkWidget *additionalTimeZonesLabel = gtk_label_new("tooltip time zones:\n(comma delimited, no spaces)");
	GtkWidget *additionalTimeZonesEntry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(additionalTimeZonesEntry), clock->additionalTimeZones); 
	gtk_box_pack_start(GTK_BOX(additionalTimeZonesBox), additionalTimeZonesLabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(additionalTimeZonesBox), additionalTimeZonesEntry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(mainBox), additionalTimeZonesBox, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(dialog), "additionalTimeZonesEntry", additionalTimeZonesEntry);

	// display
	gtk_widget_show_all(dialog);
	g_signal_connect(dialog, "response", G_CALLBACK(configureResponse), clock);

}
