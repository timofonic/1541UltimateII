#ifndef SAMPLER_H
#define SAMPLER_H

//#include "config.h"

#define SAMPLER_BASE        0x5060800

#define VOICES              8
#define BYTES_PER_VOICE     16

// #define VOICE_CONTROL       *((volatile BYTE *) SAMPLER_BASE + 0x00)
// #define VOICE_VOLUME        *((volatile BYTE *) SAMPLER_BASE + 0x01)
// #define VOICE_PAN           *((volatile BYTE *) SAMPLER_BASE + 0x02)
// #define VOICE_START         *((volatile DWORD *)SAMPLER_BASE + 0x04)
// #define VOICE_LENGTH        *((volatile DWORD *)SAMPLER_BASE + 0x08)
// #define VOICE_RATE          *((volatile DWORD *)SAMPLER_BASE + 0x0C)
// #define VOICE_RATE_H        *((volatile WORD *) SAMPLER_BASE + 0x0E)
// #define VOICE_RATE_L        *((volatile WORD *) SAMPLER_BASE + 0x0F)

#define VOICE_CONTROL(x)       *((volatile BYTE *) (SAMPLER_BASE + (x*16) + 0x00))
#define VOICE_VOLUME(x)        *((volatile BYTE *) (SAMPLER_BASE + (x*16) + 0x01))
#define VOICE_PAN(x)           *((volatile BYTE *) (SAMPLER_BASE + (x*16) + 0x02))
#define VOICE_START(x)         *((volatile DWORD *)(SAMPLER_BASE + (x*16) + 0x04))
#define VOICE_LENGTH(x)        *((volatile DWORD *)(SAMPLER_BASE + (x*16) + 0x08))
#define VOICE_RATE(x)          *((volatile DWORD *)(SAMPLER_BASE + (x*16) + 0x0C))
#define VOICE_RATE_H(x)        *((volatile WORD *) (SAMPLER_BASE + (x*16) + 0x0E))
#define VOICE_RATE_L(x)        *((volatile WORD *) (SAMPLER_BASE + (x*16) + 0x0F))

#define VOICE_CTRL_ENABLE   0x01
#define VOICE_CTRL_REPEAT   0x02
#define VOICE_CTRL_IRQ      0x04
#define VOICE_CTRL_8BIT     0x00
#define VOICE_CTRL_16BIT    0x10

#endif
