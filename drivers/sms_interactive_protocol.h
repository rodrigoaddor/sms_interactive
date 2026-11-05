/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SMS_INTERACTIVE_PROTOCOL_H
#define SMS_INTERACTIVE_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#define SMS_FRAME_SIZE 18
struct sms_readings { double last_voltage, input, output, load, frequency, battery_charge, temperature; unsigned flags; };
static int sms_request(unsigned char command, unsigned char out[7])
{
    if (command != 'I' && command != 'F' && command != 'Q') return -1;
    out[0] = command;
    out[1] = out[2] = out[3] = out[4] = 255;
    out[5] = (unsigned char)(4 - command);
    out[6] = 13;
    return 0;
}
static int sms_valid(const unsigned char *p, size_t n)
{
    unsigned sum = 0; size_t i;
    if (n != SMS_FRAME_SIZE || p[17] != 13) return 0;
    for (i = 0; i < 17; i++) sum += p[i];
    return (sum & 255) == 0;
}
static int sms_parse(const unsigned char *p, size_t n, struct sms_readings *r)
{
    double v[7]; size_t i;
    if (!sms_valid(p, n) || (p[0] != '<' && p[0] != '=' && p[0] != '>')) return -1;
    for (i = 0; i < 7; i++) v[i] = (p[1+i*2]*256u + p[2+i*2])/10.0;
    r->last_voltage=v[0]; r->input=v[1]; r->output=v[2]; r->load=v[3];
    r->frequency=v[4]; r->battery_charge=v[5]; r->temperature=v[6]; r->flags=p[15];
    return 0;
}
#endif
