#include "sms_interactive_protocol.h"
#include <assert.h>
#include <string.h>
static void checksum(unsigned char *p) { unsigned s=0; int i; for(i=0;i<16;i++) s+=p[i]; p[16]=(unsigned char)-s; }
int main(void) {
    unsigned char q[7], p[18]={'=',0,0,4,253,4,246,1,65,2,88,0,100,1,45,200,0,13};
    struct sms_readings r;
    assert(!sms_request('Q',q)); assert(q[5]==179 && q[6]==13);
    assert(sms_request('S',q)==-1);
    checksum(p); assert(!sms_parse(p,18,&r));
    assert(r.input==127.7 && r.output==127.0 && r.load==32.1 && r.frequency==60.0);
    assert(r.battery_raw==100 && r.flags==200);
    assert(sms_parse(p,17,&r)==-1);
    p[5]^=1; assert(sms_parse(p,18,&r)==-1); p[5]^=1;
    p[17]=0; assert(sms_parse(p,18,&r)==-1); p[17]=13;
    p[0]='!'; checksum(p); assert(sms_parse(p,18,&r)==-1);
    /* Actual hardware capture: Gamer 2000 VA, firmware 1.4, mains. */
    {
        const unsigned char live_i[] = {0x3a,0x47,0x41,0x4d,0x32,0x30,0x30,0x30,0x42,0x69,0x42,0x69,0x57,0x31,0x2e,0x34,0xef,0x0d};
        const unsigned char live_f[] = {0x3b,0x45,0x42,0x69,0x53,0x31,0x31,0x35,0x30,0x30,0x30,0x20,0x32,0x34,0x36,0x30,0x6f,0x0d};
        const unsigned char live_q[] = {0x3d,0x00,0x00,0x04,0xa7,0x04,0xa6,0x00,0x01,0x02,0x57,0x01,0x22,0x01,0x99,0x28,0x2f,0x0d};
        assert(sms_valid(live_i, sizeof(live_i)));
        assert(sms_valid(live_f, sizeof(live_f)));
        assert(!sms_parse(live_q, sizeof(live_q), &r));
        assert(r.input==119.1 && r.output==119.0 && r.frequency==59.9);
        assert(r.load==0.1 && r.battery_raw==290 && r.temperature==40.9);
        assert(r.flags==0x28);
        {
            const unsigned char battery_q[] = {0x3d,0,0,0,0,0x04,0x7e,0,2,2,0x58,3,0xe8,1,0x9b,0xa8,0xb6,0x0d};
            assert(!sms_parse(battery_q, sizeof(battery_q), &r));
            assert(r.input==0 && r.output==115.0 && r.frequency==60.0);
            assert(r.battery_raw==1000 && r.flags==0xa8);
            {
                const unsigned char restored_q[] = {0x3d,0,0,4,0x63,4,0x62,0,0,2,0x56,3,0xb6,1,0x9c,0x28,0x20,0x0d};
                assert(!sms_parse(restored_q, sizeof(restored_q), &r));
                assert(r.input==112.3 && r.output==112.2 && r.battery_raw==950);
                assert(r.flags==0x28);
            }
        }
    }
    return 0;
}
