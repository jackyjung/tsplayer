#include <gst/gst.h>
#include <glib.h>

test3
test
test1
test3
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
	GMainLoop *loop = (GMainLoop *)data;
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
				g_printerr("Error: %s\n", error->message);
				g_error_free(error);
				g_main_loop_quit(loop);
				break;
			}
		default :
			break;
	}
	return TRUE;
}

static void on_pad_added (GstElement *element, GstPad *pad, gpointer data) {
	GstPad *sinkpad;
	GstElement *decoder = (GstElement *) data;

	g_print("Dynamic pad created, linking demuxer/decoder\n");
	sinkpad = gst_element_get_static_pad(decoder, "sink");
	gst_pad_link(pad, sinkpad);
	gst_object_unref(sinkpad);
}

int main(int argc, char *argv[]) {
	GMainLoop *loop;
	GstElement *pipeline, *source, *demuxer, *vdecoder, *adecoder, *aconv, *vsink, *asink;
	GstElement *dtcpip;
	//GstElement *sink;
	GstBus *bus;
	guint bus_watch_id;

	gst_init (&argc, &argv);

	loop = g_main_loop_new(NULL, FALSE);
	if (argc != 3) {
		g_printerr("Usage : %s <mpeg2ts filename> <program number>\n", argv[0]);
		return -1;
	}

	pipeline = gst_pipeline_new("ts-player");
	source = gst_element_factory_make("filesrc","file-source");
	demuxer = gst_element_factory_make("mpegtsdemux", "mpeg2ts-demuxer");
	dtcpip  = gst_element_factory_make("vldtcpip", "dtcpip-filter");
	vdecoder = gst_element_factory_make("mpeg2dec", "mpeg2-video-decoder");
	vsink = gst_element_factory_make("autovideosink", "video-output");
	adecoder = gst_element_factory_make("a25dec", "mpeg2-audio-decoder");
	aconv = gst_element_factory_make("audioconvert", "converter");
	asink = gst_element_factory_make("autoaudiosink", "audio-output");
	//sink = gst_element_factory_make("alsasink", "av-sink");

	if (!pipeline || !source || !demuxer || !vdecoder || !asink) {
		g_printerr("One element could not be created. Exiting\n");
		return -1;
	}

	g_object_set(G_OBJECT(source), "location", argv[1], NULL);
	g_object_set(G_OBJECT(demuxer), "program-number", atoi(argv[2]), NULL);
#if 1
	gst_bin_add_many(GST_BIN(pipeline), source, dtcpip, demuxer, vdecoder, vsink, NULL);
	gst_bin_add_many(GST_BIN(pipeline), adecoder, aconv, asink, NULL);
	gst_element_link(source , dtcpip);
	gst_element_link(dtcpip ,demuxer);
	//gst_element_link(demuxer, vdecoder);
	gst_element_link(vdecoder, vsink);
	//gst_element_link(demuxer, adecoder);
	//gst_element_link(adecoder, asink);
	gst_element_link_many(adecoder, aconv, asink, NULL);
#else 
	gst_bin_add_many(GST_BIN(pipeline), source ,demuxer, decoder, sink, NULL);
	gst_element_link(source ,demuxer);
	gst_element_link_many(decoder, sink, NULL);
#endif
	g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), vdecoder);
	g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), adecoder);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	g_print("Running...\n");
	g_main_loop_run(loop);

	g_print("Returned, stopping playback\n");
	gst_element_set_state (pipeline, GST_STATE_NULL);

	g_print("Deleting pipeline\n");
	gst_object_unref(GST_OBJECT(pipeline));
	g_source_remove(bus_watch_id);
	g_main_loop_unref(loop);
	return 0;

}
