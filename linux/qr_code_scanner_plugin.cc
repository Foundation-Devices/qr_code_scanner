#include "include/qr_code_scanner/qr_code_scanner_plugin.h"
#include "include/qr_code_scanner/video_outlet.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>

#include <cstring>
#include <iostream>

#include <zbar.h>

#define QR_CODE_SCANNER_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), qr_code_scanner_plugin_get_type(), \
                              QrCodeScannerPlugin))

struct _QrCodeScannerPlugin {
    GObject parent_instance;
};

G_DEFINE_TYPE(QrCodeScannerPlugin, qr_code_scanner_plugin, g_object_get_type()
)


FlEventChannel *event_channel;
FlMethodChannel *method_channel;
FlTextureRegistrar *texture_registrar;
VideoOutlet *outlet;

static zbar::zbar_processor_t *proc;

static void data_handler(zbar::zbar_image_t *img, const void *userdata) {
    auto video_outlet_private =
            (VideoOutletPrivate *) video_outlet_get_instance_private(outlet);

//    int format = zbar_image_get_format(img);
//    printf("%d", format);

    // Convert to RGBA
    zbar::zbar_image_t* converted = zbar_image_convert(img, zbar_fourcc('R', 'G', 'B', '3'));

    video_outlet_private->buffer = (uint8_t *) zbar_image_get_data(converted);
    video_outlet_private->video_width = zbar_image_get_width(converted);
    video_outlet_private->video_height = zbar_image_get_height(converted);

//    zbar_image_write(converted, "/home/igor/converted");
//    zbar_image_write(img, "/home/igor/img");

    fl_texture_registrar_mark_texture_frame_available(texture_registrar, FL_TEXTURE(outlet));

    const zbar::zbar_symbol_t *sym = zbar::zbar_image_first_symbol(img);
    assert(sym);

    for (; sym; sym = zbar_symbol_next(sym)) {
        if (zbar_symbol_get_count(sym))
            continue;

        zbar::zbar_symbol_type_t type = zbar::zbar_symbol_get_type(sym);
        if (type == zbar::ZBAR_PARTIAL)
            continue;

        g_autoptr(FlValue)
                data = fl_value_new_map();

        // Format: QR
        fl_value_set(data, fl_value_new_string("type"),  fl_value_new_string("QR_CODE"));
        // Type: unknown
        fl_value_set(data, fl_value_new_string("code"), fl_value_new_string(zbar_symbol_get_data(sym)));

        //g_autoptr(GError) error = nullptr;
        fl_method_channel_invoke_method(method_channel, "onRecognizeQR", data, nullptr, nullptr, nullptr);
    }
}

static void stop_scanning() {
    zbar::zbar_processor_destroy(proc);
}

static int start_scanning() {
    proc = zbar::zbar_processor_create(1);
    if (!proc) {
        fprintf(stderr, "ERROR: unable to allocate memory?\n");
        return (1);
    }

    zbar_processor_set_data_handler(proc, data_handler, NULL);

    const char *video_device = "/dev/video0";
    int display = 1;
    unsigned long infmt = 0, outfmt = 0;//0x41424752;

    if (infmt || outfmt) { zbar_processor_force_format(proc, infmt, outfmt); }

    if (zbar_processor_init(proc, video_device, display) ||
        (display && zbar_processor_set_visible(proc, 1))) // TODO: toggle here
        return (zbar_processor_error_spew(proc, 0));

    int active = 1;
    if (zbar_processor_set_active(proc, active))
        return (zbar_processor_error_spew(proc, 0));

    //outlet = FL_TEXTURE(g_object_new(fl_pixel_buffer_texture_get_type(), nullptr));
    outlet = video_outlet_new();
    FL_PIXEL_BUFFER_TEXTURE_GET_CLASS(outlet)->copy_pixels =
            video_outlet_copy_pixels;
    int texture_id = fl_texture_registrar_register_texture(texture_registrar, FL_TEXTURE(outlet));

    auto video_outlet_private =
            (VideoOutletPrivate*)video_outlet_get_instance_private(outlet);
    video_outlet_private->texture_id = texture_id;

    return texture_id;
}

// Called when a method call is received from Flutter.
static void qr_code_scanner_plugin_handle_method_call(
        QrCodeScannerPlugin *self,
        FlMethodCall *method_call) {
    g_autoptr(FlMethodResponse)
            response = nullptr;

    const gchar *method = fl_method_call_get_name(method_call);

    if (strcmp(method, "getPlatformVersion") == 0) {
        struct utsname uname_data = {};
        uname(&uname_data);
        g_autofree
        gchar *version = g_strdup_printf("Linux %s", uname_data.version);
        g_autoptr(FlValue)
                result = fl_value_new_string(version);
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
    } else if (strcmp(method, "state") == 0) {
        g_autoptr(FlValue)
                result = fl_value_new_int(1);
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
    } else if (strcmp(method, "startScan") == 0) {
        int texture_id = start_scanning();

        // Get these from above ^^
        g_autoptr(FlValue)
                torchable = fl_value_new_bool(false);

        g_autoptr(FlValue)
                size = fl_value_new_map();

        fl_value_set(size, fl_value_new_string("width"), fl_value_new_float(640));
        fl_value_set(size, fl_value_new_string("height"), fl_value_new_float(480));

        g_autoptr(FlValue)
                result = fl_value_new_map();

        fl_value_set(result, fl_value_new_string("torchable"), torchable);
        fl_value_set(result, fl_value_new_string("textureId"), fl_value_new_int(texture_id));
        fl_value_set(result, fl_value_new_string("size"), size);

        response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
    } else if (strcmp(method, "pauseCamera") == 0) {
        if (proc != NULL) {
            stop_scanning();
        }
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
    }
    else {
        response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
    }

    fl_method_call_respond(method_call, response, nullptr);
}


static void qr_code_scanner_plugin_dispose(GObject *object) {
    G_OBJECT_CLASS(qr_code_scanner_plugin_parent_class)->dispose(object);
}

static void qr_code_scanner_plugin_class_init(QrCodeScannerPluginClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = qr_code_scanner_plugin_dispose;
}

static void qr_code_scanner_plugin_init(QrCodeScannerPlugin *self) {
}

static void method_call_cb(FlMethodChannel *channel, FlMethodCall *method_call,
                           gpointer user_data) {
    QrCodeScannerPlugin *plugin = QR_CODE_SCANNER_PLUGIN(user_data);
    qr_code_scanner_plugin_handle_method_call(plugin, method_call);
}

static FlMethodErrorResponse *events_listen_cb(FlEventChannel *channel,
                                               FlValue *args,
                                               gpointer user_data) {
    return nullptr;
}

void qr_code_scanner_plugin_register_with_registrar(FlPluginRegistrar *registrar) {
    QrCodeScannerPlugin *plugin = QR_CODE_SCANNER_PLUGIN(
            g_object_new(qr_code_scanner_plugin_get_type(), nullptr));

    texture_registrar = fl_plugin_registrar_get_texture_registrar(registrar);

    g_autoptr(FlStandardMethodCodec)
            codec = fl_standard_method_codec_new();

    method_channel =
            fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                                  "net.touchcapture.qr.flutterqr/qrview_0",
                                  FL_METHOD_CODEC(codec));
    fl_method_channel_set_method_call_handler(method_channel, method_call_cb,
                                              g_object_ref(plugin),
                                              g_object_unref);

    event_channel =
            fl_event_channel_new(fl_plugin_registrar_get_messenger(registrar),
                                 "dev.steenbakker.qr_code_scanner/scanner/event",
                                 FL_METHOD_CODEC(codec));

    fl_event_channel_set_stream_handlers(event_channel, events_listen_cb, nullptr, nullptr, nullptr);

    g_object_unref(plugin);
}