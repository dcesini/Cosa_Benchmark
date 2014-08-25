#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>
// strerror:
#include <string.h>
// usleep:
#include <unistd.h>
// getimeofday:
#include <sys/time.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#ifdef __arm__
#define VNC_USE_GSTREAMER_IMX6
#else
#define VNC_USE_GSTREAMER_X
#endif

#ifdef VNC_USE_GSTREAMER_X
#define VNC_GSTREAMER_DEC "ffdec_h264"
#define VNC_GSTREAMER_SINK "xvimagesink"
#endif

#ifdef VNC_USE_GSTREAMER_IMX6
#define VNC_GSTREAMER_DEC "vpudec"
#define VNC_GSTREAMER_SINK "mfw_v4lsink"
#endif


typedef enum {
    GST_VIDEO_FLIP_METHOD_IDENTITY,
    GST_VIDEO_FLIP_METHOD_90R,
    GST_VIDEO_FLIP_METHOD_180,
    GST_VIDEO_FLIP_METHOD_90L,
    GST_VIDEO_FLIP_METHOD_HORIZ,
    GST_VIDEO_FLIP_METHOD_VERT,
    GST_VIDEO_FLIP_METHOD_TRANS,
    GST_VIDEO_FLIP_METHOD_OTHER
} GstVideoFlipMethod;


// typedef struct {
    GstPipeline *pipeline;
    GstAppSrc *src;
    GstElement *parser;
    GstElement *queue;
    GstElement *decoder;
#ifdef VNC_USE_GSTREAMER_X
    GstElement *ffmpeg;
    GstElement *videoflip;
#endif
    GstElement *imagesink;
    GstElement *fakeimagesink;
    GstElement *osel;
    GstPad *osel_src_show;
    GstPad *osel_src_hide;
    GMainLoop *loop;
    
//     GstPipeline *pipeline;
//     GstAppSrc *src;
//     GstElement *sink;
//     GstElement *parser;
//     GstElement *queue;
//     GstElement *decoder;
//     GstElement *ffmpeg;
//     GstElement *videoflip;
//     GstElement *imagesink;
//     GMainLoop *loop;
//     guint sourceid;
    pthread_t threadid;
    int frameLoadCount;
    FILE *file;
    unsigned char * data;
    int datalen;
    int pos;
// }gst_app_t;

// static gst_app_t gst_app;

#define BUFF_SIZE (11000000)

static int64_t getusTime64()
{
    struct timeval tim;
    gettimeofday(&tim, NULL);
    return (int64_t)tim.tv_sec * 1000000 + (int64_t)tim.tv_usec;
}

int binstrstr (unsigned char * buff1, int len1, unsigned char * buff2, int len2)
{
    int i;
    if (! buff1) return -1;
    if (! buff2) return -1;
    if (len1 == 0) return -1;
    if (len2 == 0) return -1;
    if (len1 < len2) return -1;

    for (i = 0; i <= (len1 - len2); ++ i)
        if (memcmp(buff1 + i, buff2, len2) == 0)
            return i;

    return -1;
}

int64_t getRunningTimeUs()
{
    GstClock *clock = gst_element_get_clock((GstElement*)pipeline);
    if (clock && GST_IS_CLOCK(clock))
    {
        GstClockTime base_time = gst_element_get_base_time((GstElement*)pipeline);
        GstClockTime clock_time = gst_clock_get_time(clock);
//         logger.debug("gst pipeline time: %llu", GST_TIME_AS_USECONDS(clock_time - base_time));
        gst_object_unref(GST_OBJECT (clock));
        clock = NULL;
        return (int64_t)GST_TIME_AS_USECONDS(clock_time) - (int64_t)GST_TIME_AS_USECONDS(base_time);
    }
    else
    {
//         logger.debug("No pipeline clock...");
        return 0;
    }
}

static gboolean read_data()
{
    GstBuffer *buffer;
    guint8 *ptr;
//     gint size;
    GstFlowReturn ret;
    
//         ret = gst_app_src_end_of_stream(src);
    int len;
    unsigned char NALstartcode[4] = {'\0', '\0', '\0', '\1'};
    
//     printf("finding %x\n", *((int*) NALstartcode));
    len = binstrstr(data + pos + 4, datalen - pos - 4, NALstartcode, 4) + 4;
    printf("len %d\n", len);
    
    
    ptr = g_malloc(len);
    g_assert(ptr);
    
    g_memmove((void *)ptr, (void *)(data + pos), len);

//     size = fread(ptr, 1, len, file);
// 
//     if(size == 0 || size != len){
//         ret = gst_app_src_end_of_stream(src);
//         g_debug("eos returned %d at %d\n", ret, __LINE__);
//         return FALSE;
//     }

    buffer = gst_buffer_new();
    GST_BUFFER_MALLOCDATA(buffer) = ptr;
    GST_BUFFER_SIZE(buffer) = len;
    GST_BUFFER_DATA(buffer) = GST_BUFFER_MALLOCDATA(buffer);
    
    int64_t timestamp = getRunningTimeUs() * GST_USECOND; 
    printf("timestamp %lld\n", timestamp);
    GST_BUFFER_TIMESTAMP(buffer) = timestamp;
        // not sure what gstreamer does precisely when timestamps are for some time 'in the past', 
        // perhaps resets it's own clock so that the time is _now_?
        // might have to modify timestamps after a laggy frame to avoid the entire stream ending up really far behind

    ret = gst_app_src_push_buffer(src, buffer);

    if(ret !=  GST_FLOW_OK){
        g_debug("push buffer returned %d for %d bytes \n", ret, len);
        return FALSE;
    }

    
    pos += len;
    printf("pos %d\n", pos);
    
   
    return TRUE;
}

static void * threadEntry(void *arg)
{
//     gst_app_t *app = (gst_app_t *)arg;
    
    frameLoadCount = 0;
    
    while (read_data() == TRUE)
    {
//         printf("read_data, %lld\n", getusTime64());
        printf("Press enter to continue...");
        char buf[1000];
        gets(buf);
        
        frameLoadCount++;
//         if (frameLoadCount == 5)
//             g_object_set (G_OBJECT (videoflip), "method", GST_VIDEO_FLIP_METHOD_IDENTITY, NULL);
        
//         if (frameLoadCount == 5)
//             usleep(8000000);
    }
    return NULL;
}

static void start_feed (GstElement * pipeline, guint size, void *app)
{
    if (threadid == 0) {
        g_print ("start feeding\n");
//         sourceid = g_idle_add ((GSourceFunc) read_data, app);
        
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        if (pthread_create(&threadid, &attr, threadEntry, (void *)app))
            printf("Couldn't start thread. %s\n", strerror(errno));
        pthread_attr_destroy(&attr);
    
    }
}

static void stop_feed (GstElement * pipeline, void *app)
{
    if (threadid != 0) {
        g_print ("stop feeding\n");
//         g_source_remove (sourceid);
        
        pthread_join(threadid, NULL);
        
        threadid = 0;
    }
}

// static void on_pad_added(GstElement *element, GstPad *pad)
// {
//     GstCaps *caps;
//     GstStructure *str;
//     gchar *name;
//     GstPad *ffmpegsink;
//     GstPadLinkReturn ret;
// 
//     g_debug("pad added");
// 
//     caps = gst_pad_get_caps(pad);
//     str = gst_caps_get_structure(caps, 0);
// 
//     g_assert(str);
// 
//     name = (gchar*)gst_structure_get_name(str);
// 
//     g_debug("pad name %s", name);
// 
//     if(g_strrstr(name, "video")){
// 
//         ffmpegsink = gst_element_get_pad(gst_app.ffmpeg, "sink");
//         g_assert(ffmpegsink);
//         ret = gst_pad_link(pad, ffmpegsink);
//         g_debug("pad_link returned %d\n", ret);
//         gst_object_unref(ffmpegsink);
//     }
//     gst_caps_unref(caps);
// }

static gboolean bus_callback(GstBus *bus, GstMessage *message, gpointer *ptr)
{
//     gst_app_t *app = (gst_app_t*)ptr;

    switch(GST_MESSAGE_TYPE(message)){

    case GST_MESSAGE_ERROR:{
        gchar *debug;
        GError *err;

        gst_message_parse_error(message, &err, &debug);
        g_print("Error %s\n", err->message);
        g_error_free(err);
        g_free(debug);
        g_main_loop_quit(loop);
    }
    break;

    case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        g_main_loop_quit(loop);
        break;

    default:
//         g_print("got message %s\n", gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
        break;
    }

    return TRUE;
}

// gst-launch-0.10 filesrc location=out.h264 ! h264parse ! ffdec_h264 ! ffmpegcolorspace ! videoflip method=1 ! imagesink
// g_object_set (G_OBJECT (source), "location", filename, NULL);
// g_object_set (G_OBJECT (avidemux), "name", "demux");

int main(int argc, char *argv[])
{
//     gst_app_t *app = &gst_app;
    GstBus *bus;

    if(argc != 2){
        printf("File name not specified\n");
        return 1;
    }
    
    file = fopen(argv[1], "r");
    g_assert(file);
    
    data = malloc(BUFF_SIZE);
    if (!data) return 1;
    datalen = fread(data, 1, BUFF_SIZE, file);
    printf("datalen %d\n", datalen);
    if (datalen <= 0) return 1;
    pos = 0;
    
    
    threadid = 0;

    gst_init(NULL, NULL);

    pipeline = (GstPipeline*)gst_pipeline_new("mypipeline");
    bus = gst_pipeline_get_bus(pipeline);
    gst_bus_add_watch(bus, (GstBusFunc)bus_callback, NULL);
    gst_object_unref(bus);

    src = (GstAppSrc*)gst_element_factory_make("appsrc", "mysrc");
    parser = gst_element_factory_make("h264parse", "myparser");
    queue = gst_element_factory_make("queue", "myqueue");
    decoder = gst_element_factory_make(VNC_GSTREAMER_DEC, "mydecoder");
#ifdef VNC_USE_GSTREAMER_X
    ffmpeg = gst_element_factory_make("ffmpegcolorspace", "myffmpeg");
    videoflip = gst_element_factory_make("videoflip", "myvideoflip");
    g_assert(ffmpeg);
    g_assert(videoflip);
#endif
    osel = gst_element_factory_make ("output-selector", "myoutputselector");
    imagesink = gst_element_factory_make(VNC_GSTREAMER_SINK, "mysink");
//     fakeimagesink = gst_element_factory_make("fakesink", "myfakesink");

    fakeimagesink = gst_element_factory_make("filesink", "myfakesink");
    g_object_set (G_OBJECT (fakeimagesink), "location", "/dev/null", NULL);
    
    g_assert(src);
    g_assert(parser);
    g_assert(queue);
    g_assert(decoder);
    g_assert(osel);
    g_assert(imagesink);
    g_assert(fakeimagesink);
    
    g_object_set (G_OBJECT (src), "is-live", TRUE, NULL);
    g_object_set (G_OBJECT (src), "block", TRUE, NULL);
    gst_app_src_set_stream_type(src, GST_APP_STREAM_TYPE_STREAM);
    gst_app_src_set_max_bytes(src, 16*1024*1024);
//     logger.info("Gstreamer max queue bytes:%llu", gst_app_src_get_max_bytes(src));
    
    g_signal_connect(src, "need-data", G_CALLBACK(start_feed), NULL);
    
//     g_signal_connect(src, "need-data", G_CALLBACK(start_feed_callback), this);
//     g_signal_connect(src, "enough-data", G_CALLBACK(stop_feed_callback), this);
//     g_signal_connect(decoder, "pad-added", G_CALLBACK(on_pad_added), decoder);

    g_object_set (G_OBJECT (queue), "max-size-time", 0, NULL); /* No limit */
    g_object_set (G_OBJECT (queue), "max-size-buffers", 0, NULL); /* No limit */
    
#ifdef VNC_USE_GSTREAMER_IMX6
        g_object_set (G_OBJECT (imagesink), "axis-left", 0, NULL);
        g_object_set (G_OBJECT (imagesink), "axis-top", 0, NULL);
        g_object_set (G_OBJECT (imagesink), "disp-width", 1024, NULL);
        g_object_set (G_OBJECT (imagesink), "disp-height", 768, NULL);
#endif
#ifdef VNC_USE_GSTREAMER_X
    g_object_set (G_OBJECT (imagesink), "force-aspect-ratio", TRUE, NULL);
#endif
#ifdef VNC_USE_GSTREAMER_IMX6
    g_object_set (G_OBJECT (decoder), "low-latency", TRUE, NULL);
    g_object_set (G_OBJECT (decoder), "frame-plus", 1, NULL);
    g_object_set (G_OBJECT (decoder), "framedrop", FALSE, NULL);
    g_object_set (G_OBJECT (imagesink), "device", "/dev/video17", NULL); /* v4lsink */
#endif
    g_object_set (G_OBJECT (imagesink), "max-lateness", (gint64) 10*1000*1000*1000 /*10s*/, NULL);
    
    g_object_set (G_OBJECT (imagesink), "sync", FALSE, "async", FALSE, NULL);
    g_object_set (G_OBJECT (fakeimagesink), "sync", FALSE, "async", FALSE, NULL);
    
//     g_object_set (G_OBJECT (osel), "resend-latest", TRUE, NULL);
    
    gst_bin_add_many(GST_BIN(pipeline), (GstElement*)src, 
                     parser, 
                     queue, 
                     decoder, 
#ifdef VNC_USE_GSTREAMER_X
                     ffmpeg, 
                     videoflip, 
#endif
                     imagesink, 
                     fakeimagesink,
                     osel,
                     NULL);

    /// Could use gst_element_link_many instead of gst_element_link
    
    if(!gst_element_link((GstElement*)src, parser)){
        printf("failed to link src and parser\n");
    }
    
//     if(!gst_element_link(parser, decoder)){
//         printf("failed to link parser and decoder");
//     }
    
    if(!gst_element_link(parser, queue)){
        printf("failed to link parser and queue\n");
    }
    
    if(!gst_element_link(queue, decoder)){
        printf("failed to link queue and decoder\n");
    }
    
#ifdef VNC_USE_GSTREAMER_X
    if(!gst_element_link(decoder, ffmpeg)){
        printf("failed to link decoder and ffmpeg\n");
    }

    if(!gst_element_link(ffmpeg, videoflip)){
        printf("failed to link ffmpeg and videoflip\n");
    }

    if(!gst_element_link(videoflip, osel)){
        printf("failed to link videoflip and osel\n");
    }
#endif
#ifdef VNC_USE_GSTREAMER_IMX6
    if(!gst_element_link(decoder, osel)){
        printf("failed to link decoder and osel\n");
    }
#endif
    
    GstPad *sinkpad_show, *sinkpad_hide;
    /* link output 1 */
    osel_src_show = gst_element_get_request_pad (osel, "src%d");
    sinkpad_show = gst_element_get_static_pad (imagesink, "sink");
    if (gst_pad_link (osel_src_show, sinkpad_show) != GST_PAD_LINK_OK) {
        printf("linking output 1 failed\n");
    }
    gst_object_unref (sinkpad_show);

    /* link output 2 */
    osel_src_hide = gst_element_get_request_pad (osel, "src%d");
    sinkpad_hide = gst_element_get_static_pad (fakeimagesink, "sink");
    if (gst_pad_link (osel_src_hide, sinkpad_hide) != GST_PAD_LINK_OK) {
        printf("linking output 2 failed\n");
    }
    gst_object_unref (sinkpad_hide);

    g_object_set (G_OBJECT (osel), "active-pad", osel_src_show, NULL);
    
    /// Start pipeline
    if (!gst_element_set_state((GstElement*)pipeline, GST_STATE_PLAYING)){
        printf("set state GST_STATE_PLAYING failed\n");
    }

    loop = g_main_loop_new(NULL, FALSE);
    printf("Running main loop\n");
    g_main_loop_run(loop);


    if (!gst_element_set_state((GstElement*)pipeline, GST_STATE_NULL)){
        g_warning("set state GST_STATE_NULL failed\n");
    }

    free(data);
    
    return 0;
}
