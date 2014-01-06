#include <string.h>
#include <stdlib.h>
#include <gst/gst.h>
#include "gstreamer.h"

static gboolean cb_print_position(GstElement *pipeline)
{
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos, len;
	char format[TIME_FORMAT_LEN];
	char *p;
	gint i;

	if (gst_element_query_position(pipeline, &fmt, &pos)
			&& gst_element_query_duration(pipeline, &fmt, &len)) {
		g_snprintf(format, TIME_FORMAT_LEN - 1,
				"Time:%u:%02u:%02u:%01u\r",
				GST_TIME_ARGS(pos));
		for (i = 0, p = format; i < FIRST_CAT; p++) {
			if (*p == ':')
				i++;
		}
		*--p = '\r';
		*++p = '\0';
		g_print("%s", format);

		g_snprintf(format, TIME_FORMAT_LEN - 1,
				"\t\t\t%u:%02u:%02u:%01u\r",
				GST_TIME_ARGS(len));
		for (i = 0, p = format; i < SECOND_CAT; p++) {
			if (*p == ':')
				i++;
		}
		*--p = '\r';
		*++p = '\0';

		g_print("%s", format);
	}

	return TRUE;
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop *) data;

	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_EOS:
		g_print("End of stream\n");
		g_main_loop_quit(loop);
		break;

	case GST_MESSAGE_ERROR:
	{
		gchar *debug;
		GError *error;

		gst_message_parse_error(msg, &error, &debug);
		g_free(debug);
		g_printerr("ERROR:%s\n", error->message);
		g_error_free(error);
		g_main_loop_quit(loop);
	}
		break;

	default:
		break;
	}

	return TRUE;
}

int gstreamer_play(const char *path)
{
	GMainLoop *loop;
	GstElement *play;
	GstBus *bus;
	char buff[PATH_LEN] = "";

	gst_init(NULL, NULL);
	loop = g_main_loop_new(NULL, FALSE);

	play = gst_element_factory_make("playbin", "play");
	if (!strncmp(path, HTTP_URI, HTTP_LEN)) {
		g_object_set(G_OBJECT(play), "uri", path, NULL);
	}
	else {
		strncpy(buff, FILE_URI, FILE_LEN);
		strcat(buff, realpath(path, NULL));
		g_object_set(G_OBJECT(play), "uri", buff, NULL);
	}

	bus = gst_pipeline_get_bus(GST_PIPELINE(play));
	gst_bus_add_watch(bus, bus_call, loop);
	gst_object_unref(bus);

	gst_element_set_state(play, GST_STATE_PLAYING);
	g_print("Running\n");

	g_timeout_add(200, (GSourceFunc)cb_print_position, play);
	g_main_loop_run(loop);

	g_print("Returned, stopping playback\n");
	gst_element_set_state(play, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(play));

	return 0;
}
