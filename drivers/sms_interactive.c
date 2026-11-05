/* SPDX-License-Identifier: GPL-2.0-or-later
 * SMS Interactive monitoring driver. Protocol independently reconstructed from
 * PowerView's XML resources and serial bytecode; see docs/protocol.md.
 */
#include "config.h"
#include "main.h"
#include "serial.h"
#include "sms_interactive_protocol.h"
#include <string.h>

upsdrv_info_t upsdrv_info = { "SMS Interactive monitoring driver", "0.1", "sms_interactive contributors", DRV_EXPERIMENTAL, { NULL } };
static int identified;
static int query(unsigned char command, unsigned char reply[18])
{
    unsigned char request[7];
    sms_request(command, request);
    ser_flush_io(upsfd);
    if (ser_send_buf(upsfd, request, sizeof(request)) != sizeof(request)) return -1;
    if (ser_get_buf_len(upsfd, reply, 18, 3, 0) != 18 || !sms_valid(reply, 18)) return -1;
    return 0;
}
static void identify(void)
{
    unsigned char reply[18]; char model[13], firmware[4];
    if (query('I', reply) || (reply[0] != ':' && reply[0] != '?' && reply[0] != ';')) return;
    memcpy(model, reply+1, 12); model[12]=0;
    memcpy(firmware, reply+13, 3); firmware[3]=0;
    dstate_setinfo("ups.model", "%s", model);
    dstate_setinfo("ups.firmware", "%s", firmware);
    identified=1;
}
void upsdrv_initups(void)
{
    upsfd=ser_open(device_path);
    if (ser_set_speed(upsfd, device_path, B2400) < 0 || ser_set_rts(upsfd, 1) < 0 || ser_set_dtr(upsfd, 0) < 0)
        fatalx(EXIT_FAILURE, "Unable to configure serial port; check USB/IP connection");
}
void upsdrv_initinfo(void)
{
    dstate_setinfo("ups.mfr", "SMS");
    identify();
}
void upsdrv_updateinfo(void)
{
    unsigned char reply[18]; struct sms_readings r;
    if (!identified) identify();
    if (query('Q', reply) || sms_parse(reply, 18, &r)) {
        upslogx(LOG_WARNING, "Invalid or missing SMS response; check USB/IP connection");
        dstate_datastale(); return;
    }
    dstate_setinfo("input.voltage", "%.1f", r.input);
    dstate_setinfo("output.voltage", "%.1f", r.output);
    dstate_setinfo("ups.load", "%.1f", r.load);
    dstate_setinfo("output.frequency", "%.1f", r.frequency);
    dstate_setinfo("ups.temperature", "%.1f", r.temperature);
    /* Battery units differ between protocol implementations. Preserve raw data
     * until the actual Gamer firmware's scaling has been established. */
    dstate_setinfo("experimental.sms.battery.raw", "%.0f", r.battery_raw);
    dstate_setinfo("ups.beeper.status", "%s", (r.flags & 1) ? "enabled" : "disabled");
    status_init();
    status_set((r.flags & 128) ? "OB" : "OL");
    if (r.flags & 64) status_set("LB");
    if (r.flags & 32) status_set("BYPASS");
    if (r.flags & 16) status_set("BOOST");
    if (r.flags & 4) status_set("CAL");
    alarm_init();
    if (!(r.flags & 8)) alarm_set("UPS reports a fault");
    if (r.flags & 2) alarm_set("UPS shutdown pending");
    alarm_commit(); status_commit(); dstate_dataok();
}
void upsdrv_shutdown(void)
{
    upslogx(LOG_ERR, "Shutdown is not implemented: SMS Interactive control commands need hardware validation");
    if (handling_upsdrv_shutdown > 0) set_exit_flag(EF_EXIT_FAILURE);
}
void upsdrv_cleanup(void) { ser_close(upsfd, device_path); }
void upsdrv_help(void) {}
void upsdrv_makevartable(void) {}
void upsdrv_tweak_prognames(void) {}
