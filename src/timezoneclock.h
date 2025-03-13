#ifndef timezoneclockH
#define timezoneclockH

G_BEGIN_DECLS

typedef struct {
	XfcePanelPlugin *plugin;

	GtkWidget *ebox;
	GtkWidget *hvbox;
	GtkWidget *label;
	GtkWidget *button;

	gchar *timeZone;
	gchar *calendar;
	gchar *format;
	gchar *font;
	gchar *additionalTimeZones;
} clockPlugin;

void saveSettings(XfcePanelPlugin *plugin, clockPlugin *clock);

G_END_DECLS

#endif
