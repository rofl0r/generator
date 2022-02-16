#ifndef SN76496_H
#define SN76496_H

#define MAX_76496 1

struct SN76496
{
  int SampleRate;
  unsigned int UpdateStep;
  int VolTable[16];             /* volume table */
  int Register[8];              /* registers */
  int LastRegister;             /* last register written */
  int Volume[4];                /* volume of voice 0-2 and noise */
  unsigned int RNG;             /* noise generator */
  int NoiseFB;                  /* noise feedback mask */
  int Period[4];
  int Count[4];
  int Output[4];
};

extern struct SN76496 sn[MAX_76496];

int SN76496Init(int chip, int clock, int gain, int sample_rate);
void SN76496Write(int chip, int data);
void SN76496Update(int chip, uint16 *buffer, int length);

#endif
