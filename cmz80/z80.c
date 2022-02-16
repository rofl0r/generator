/* z80stb.c
 * Zilog Z80 processor emulator in C. Modified to be API compliant with Neil
 * Bradley's MZ80 (for ported projects)
 * Ported to 'C' by Jeff Mitchell; original copyright follows:
 *
 * Abstract:
 *   Internal declarations for Zilog Z80 emulator.
 *
 * Revisions:
 *   18-Apr-97 EAM Created
 *   ??-???-97 EAM Released in MageX 0.5
 *   26-Jul-97 EAM Fixed Time Pilot slowdown bug and
 *      Gyruss hiscore bugs (opcode 0xC3)
 *
 * Original copyright (C) 1997 Edward Massey
 */

#include <generator.h>

#define FALSE 0
#define TRUE 1

#ifdef WORDS_BIGENDIAN
#define HOST_TO_LE16(y) ( (((y)>>8)&0x00FF) + (((y)<<8)&0xFF00) )
#else
#define HOST_TO_LE16(y) (y)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cmz80.h"
#include "z80stb.h"

/* ***************************************************************************
 * Edward's Internals (so to speak)
 * ***************************************************************************
 */

#define _ASSERT assert

union { /* WARN: Endianness! */
  WORD m_regAF;
  struct {
#ifdef WORDS_BIGENDIAN
    BYTE m_regA;
    BYTE m_regF;
#else
    BYTE m_regF;
    BYTE m_regA;
#endif
  } regAFs;
} regAF;
#define m_regAF regAF.m_regAF
#define m_regF regAF.regAFs.m_regF
#define m_regA regAF.regAFs.m_regA

union { /* WARN: Endianness! */
  WORD m_regBC;
  struct {
#ifdef WORDS_BIGENDIAN
    BYTE m_regB;
    BYTE m_regC;
#else
    BYTE m_regC;
    BYTE m_regB;
#endif
  } regBCs;
} regBC;
#define m_regBC regBC.m_regBC
#define m_regB regBC.regBCs.m_regB
#define m_regC regBC.regBCs.m_regC

union { /* WARN: Endianness! */
  WORD m_regDE;
  struct {
#ifdef WORDS_BIGENDIAN
    BYTE m_regD;
    BYTE m_regE;
#else
    BYTE m_regE;
    BYTE m_regD;
#endif
  } regDEs;
} regDE;
#define m_regDE regDE.m_regDE
#define m_regD regDE.regDEs.m_regD
#define m_regE regDE.regDEs.m_regE

union { /* WARN: Endianness! */
  WORD m_regHL;
  struct {
#ifdef WORDS_BIGENDIAN
    BYTE m_regH;
    BYTE m_regL;
#else
    BYTE m_regL;
    BYTE m_regH;
#endif
  } regHLs;
} regHL;
#define m_regHL regHL.m_regHL
#define m_regH regHL.regHLs.m_regH
#define m_regL regHL.regHLs.m_regL

union { /* WARN: Endianness! */
  WORD m_regIX;
  struct {
#ifdef WORDS_BIGENDIAN
    BYTE m_regIXh;
    BYTE m_regIXl;
#else
    BYTE m_regIXl;
    BYTE m_regIXh;
#endif
  } regIXs;
} regIX;
#define m_regIX regIX.m_regIX
#define m_regIXl regIX.regIXs.m_regIXl
#define m_regIXh regIX.regIXs.m_regIXh

union {
  WORD m_regIY;
  struct {
#ifdef WORDS_BIGENDIAN
    BYTE m_regIYh;
    BYTE m_regIYl;
#else
    BYTE m_regIYl;
    BYTE m_regIYh;
#endif
  } regIYs;
} regIY;
#define m_regIY regIY.m_regIY
#define m_regIYl regIY.regIYs.m_regIYl
#define m_regIYh regIY.regIYs.m_regIYh

const BYTE *m_rgbOpcode; // takes place of the PC register
LPBYTE m_rgbStack; // takes place of the SP register
LPBYTE m_rgbMemory; // direct access to memory buffer (RAM)
const BYTE *m_rgbOpcodeBase; // "base" pointer for m_rgbOpcode
LPBYTE m_rgbStackBase;
static int cCycles = 0;

BYTE m_regR; // consider:  should be int for performance?
BYTE m_regI; // consider:  combine into

// TODO:  combine into a single int...
int m_iff1, m_iff2;
BOOL m_fHalt;
int m_nIM;
BOOL m_fPendingInterrupt = FALSE;

WORD m_regAF2;
static WORD m_regBC2;
static WORD m_regDE2;
static WORD m_regHL2;
WORD z80intAddr = 0x38;
WORD z80nmiAddr = 0x66;

/* ***************************************************************************
 * Pre-calculated math tables and junk (sucked from Marat and Edward)
 * ***************************************************************************
 */
void InitTables (void);

BYTE ZSTable[256];
BYTE ZSPTable[256];
BYTE rgfAdd[256][256];
BYTE rgfSub[256][256];
BYTE rgfAddc[256][256][2];
BYTE rgfSubc[256][256][2];
BYTE rgfInc[256];
BYTE rgfDec[256];
BYTE rgfBit[256][8];
static UINT32 dwElapsedTicks = 0;

/* #include "z80daa.h" */
#include "z80stbd.h"

/* ***************************************************************************
 * Retrocade guts stuff
 * ***************************************************************************
 */
static struct MemoryReadByte *z80MemoryRead = NULL;
static struct MemoryWriteByte *z80MemoryWrite = NULL;
static struct z80PortRead *z80IoRead = NULL;
static struct z80PortWrite *z80IoWrite = NULL;
WORD z80pc = 0;

/* ***************************************************************************
 * Retrocade interface follows
 * ***************************************************************************
 */

/* reset registers, interrupts, memory read/write memory handlers,
 * port read and writing handlers, etc.
 */
void mz80reset(void) {

  InitTables();

  m_fPendingInterrupt = FALSE;
  m_rgbOpcodeBase = m_rgbMemory;
  /* m_rgbMemory = */
  m_rgbStackBase = m_rgbMemory;

  m_regAF = 0;
  m_regBC = 0;
  m_regDE = 0;
  m_regHL = 0;
  m_regAF2 = 0;
  m_regBC2 = 0;
  m_regDE2 = 0;
  m_regHL2 = 0;
  m_regR = rand();
  m_regI = 0;
  m_fHalt = FALSE;
  m_nIM = 0;
  m_regIX = 0xffff;		// Yes, this intentional, and the way a Z80 comes
  m_regIY = 0xffff;		// up in power-on state!
  m_iff1 = 0;
  m_iff2 = 0;
	z80intAddr = 0x38;
	z80nmiAddr = 0x66;

  SetPC(0x0000);
  SetSP(0x0000);

#if 0
  fprintf ( stderr, "Reset\n" );
#endif
}

/* UINT32 mz80int(UINT32 a) {
 * return ( Irq ( (BYTE) ( a & 0xFF ) ) );
 * }
 */

/* UINT32 mz80nmi(void) {
 * return ( Nmi() );
 * }
 */

/* ***************************************************************************
 * Edward Memory Handlers (guesses)
 * ***************************************************************************
 */

#define ImmedByte() *m_rgbOpcode++

/* static */ UINT16 ImmedWord ( void ) {
  WORD w = HOST_TO_LE16(*(WORD*)m_rgbOpcode);
  m_rgbOpcode += 2;
  return w; // attn: little endian specific
}

#define GetPC() ( (WORD)(m_rgbOpcode - m_rgbOpcodeBase) )

void SetPC ( WORD wAddr ) {
  m_rgbOpcode = &m_rgbOpcodeBase[wAddr];
}

void AdjustPC ( signed char cb ) {
  m_rgbOpcode += cb;
}

void Push(WORD wArg) {
  MemWriteByte(--m_rgbStack - m_rgbStackBase, HIBYTE(wArg));
  MemWriteByte(--m_rgbStack - m_rgbStackBase, LOBYTE(wArg));
}

WORD Pop() {
	unsigned short wAddr = 0;

	wAddr = MemReadByte(m_rgbStack++ - m_rgbStackBase);
	wAddr |= (MemReadByte(m_rgbStack++ - m_rgbStackBase) << 8);

	return(wAddr);
}

WORD GetSP() {
  return (WORD)(m_rgbStack - m_rgbStackBase);
}

void SetSP(WORD wAddr) {
  m_rgbStack = &m_rgbStackBase[wAddr];
}

/* static */ UINT8 mz80GetMemory ( UINT16 addr ) {
  struct MemoryReadByte *mr = z80MemoryRead;

  z80pc = GetPC(); /* for some of the platforms */

  while ( mr -> lowAddr != 0xffffffff ) {

	 if (addr >= mr->lowAddr && addr <= mr->highAddr) {
		z80pc = GetPC();
		return ( mr -> memoryCall ( addr, mr ) );
	 }

	 ++mr;

  }

  return ( m_rgbMemory [ addr ] );
}

/* static */ void mz80PutMemory ( unsigned short int addr, unsigned char byte ) {
  struct MemoryWriteByte *mr = z80MemoryWrite;

  while ( mr->lowAddr != 0xffffffff ) {
	 if (addr >= mr->lowAddr && addr <= mr->highAddr) {
		z80pc = GetPC();
		mr->memoryCall(addr, byte, mr);
		return;
	 }
	 mr++;
  }

  m_rgbMemory [ addr ] = byte;

}

BYTE In(BYTE bPort) {
  BYTE bVal;
  // BYTE bVal = m_rgpfnPortRead[bPort](bPort);

  struct z80PortRead *mr = z80IoRead;

  z80pc = GetPC(); /* for some of the platforms */

  while ( mr->lowIoAddr != 0xffff ) {
    if ( bPort >= mr->lowIoAddr && bPort <= mr->highIoAddr) {
      bVal = mr->IOCall ( bPort, mr );
      break;
    }
    mr++;
  }

  m_regF = (m_regF & C_FLAG) | ZSPTable[bVal];

  return bVal;
}

BYTE InRaw (BYTE bPort) {
  BYTE bVal;

  struct z80PortRead *mr = z80IoRead;

  z80pc = GetPC(); /* for some of the platforms */

  while ( mr->lowIoAddr != 0xffff ) {
    if ( bPort >= mr->lowIoAddr && bPort <= mr->highIoAddr) {
      bVal = mr->IOCall ( bPort, mr );
      break;
    }
    mr++;
  }

  // m_regF = (m_regF & C_FLAG) | ZSPTable[bVal];
  return bVal;
}

void Out(BYTE bPort, BYTE bVal) {
  // m_rgpfnPortWrite[bPort](bPort, bVal);
  struct z80PortWrite *mr = z80IoWrite;

  z80pc = GetPC(); /* for some of the platforms */

  while ( mr->lowIoAddr != 0xffff ) {
    if (bPort >= mr->lowIoAddr && bPort <= mr->highIoAddr) {
      mr->IOCall ( bPort, bVal, mr );
      return;
    }
    mr++;
  }

}

/* static */ void MemWriteWord ( UINT16 wAddr, UINT16 wVal ) {
  MemWriteByte ( wAddr++, LOBYTE ( wVal ) );
  MemWriteByte ( wAddr,   HIBYTE ( wVal ) );
}

/* static */ /* inline */ void MemWriteByte ( UINT16 wAddr, UINT8 bVal ) {
  mz80PutMemory ( wAddr, bVal );
}

/* static */ /* inline */ UINT8 MemReadByte ( UINT16 wAddr ) {
  return ( mz80GetMemory ( wAddr ) );
}

/* static */ /* inline */ UINT16 MemReadWord ( UINT16 wAddr ) {
  UINT8 op1;
  UINT8 op2;

	op1 = MemReadByte(wAddr ++);
	op2 = MemReadByte(wAddr);
  return ( MAKEWORD ( op1, op2 ) );
}

/* ***************************************************************************
 * Retrocade guts stuff (again) (these guys are defined after macros)
 * ***************************************************************************
 */

unsigned long int mz80GetContextSize(void) {
  return ( sizeof ( CONTEXTMZ80 ) );
}

/* set memory base, etc. */
void mz80GetContext ( void *receiver ) {
  CONTEXTMZ80 *c = receiver;
  c -> z80MemRead = z80MemoryRead;
  c -> z80MemWrite = z80MemoryWrite;
  c -> z80Base = (UINT8 *) m_rgbOpcodeBase;
  c -> z80IoRead = z80IoRead;
  c -> z80IoWrite = z80IoWrite;
  c -> z80af = m_regAF;
  c -> z80bc = m_regBC;
  c -> z80de = m_regDE;
  c -> z80hl = m_regHL;
  c -> z80afprime = m_regAF2;
  c -> z80bcprime = m_regBC2;
  c -> z80deprime = m_regDE2;
  c -> z80hlprime = m_regHL2;
  c -> z80ix = m_regIX;
  c -> z80iy = m_regIY;
  c -> z80sp = GetSP();
  c -> z80pc = GetPC();
  c -> z80i = m_regI;
  c -> z80r = m_regR;
	c -> z80intAddr = z80intAddr;
	c -> z80nmiAddr = z80nmiAddr;
	c -> z80inInterrupt = m_iff1;
	c -> z80interruptMode = m_nIM;
	c -> z80interruptState = m_iff2;
	c -> z80halted = m_fHalt;
	
#if 0
  fprintf ( stderr, "GetContext\n" );
#endif
}

void mz80SetContext( void *sender ) {
  CONTEXTMZ80 *c = sender;
  z80MemoryRead = c -> z80MemRead;
  z80MemoryWrite = c -> z80MemWrite;
  m_rgbMemory = c -> z80Base;
  z80IoRead = c -> z80IoRead;
  z80IoWrite = c -> z80IoWrite;
  m_regAF = c -> z80af;
  m_regBC = c -> z80bc;
  m_regDE = c -> z80de;
  m_regHL = c -> z80hl;
  m_regAF2 = c -> z80afprime;
  m_regBC2 = c -> z80bcprime;
  m_regDE2 = c -> z80deprime;
  m_regHL2 = c -> z80hlprime;
  m_regIX = c -> z80ix;
  m_regIY = c -> z80iy;
  m_regI = c -> z80i;
  m_regR = c -> z80r;
  z80intAddr = c -> z80intAddr;
  z80nmiAddr = c -> z80nmiAddr;
  m_rgbOpcodeBase = m_rgbMemory;

	m_iff1 = c->z80inInterrupt;
	m_iff2 = c->z80interruptState;
	m_nIM = c->z80interruptMode;
	m_fHalt = c->z80halted;

  SetPC ( c -> z80pc );
  SetSP ( c -> z80sp );

#if 0
  fprintf ( stderr, "SetContext\n" );
#endif
}

/* ***************************************************************************
 * Z80 Please-inline-my-ass opcodes
 * ***************************************************************************
 */

/* inline */ int Ret0(int f)
{
	if (f)
	{
		SetPC(Pop());
		return 6;
	}
	else
	{
		return 0;
	}
}

/* inline */ int Ret1(int f)
{
	if (f)
	{
		return 0;
	}
	else
	{
		SetPC(Pop());
		return 6;
	}
}

/* inline */ WORD Adc_2 (WORD wArg1, WORD wArg2)
{
#ifdef ARITH_TABLES
	int q  = wArg1 + wArg2 + (m_regF & C_FLAG);
	m_regF = (((wArg1^q^wArg2)&0x1000)>>8)|((q>>16)&1)|((q&0x8000)>>8)|((q&65535)?0:Z_FLAG)|(((wArg2^wArg1^0x8000)&(wArg2^q)&0x8000)>>13);
	return q;
#else
	int q  = wArg1 + wArg2 + (m_regF & C_FLAG);
	m_regF = (((wArg1^q^wArg2)&0x1000)>>8)|((q>>16)&1)|((q&0x8000)>>8)|((q&65535)?0:Z_FLAG)|(((wArg2^wArg1^0x8000)&(wArg2^q)&0x8000)>>13);
	return q;
#endif
}

/* inline */ WORD Sbc_2 (WORD wArg1, WORD wArg2)
{
#ifdef ARITH_TABLES
	int q   = wArg1 - wArg2 - (m_regF & C_FLAG);

	m_regF  = (((wArg1^q^wArg2)&0x1000)>>8)|((q>>16)&1)|((q&0x8000)>>8)|
		((q&65535)?0:Z_FLAG)|(((wArg1^wArg2)&(wArg1^q)&0x8000)>>13)|N_FLAG;

	return q;
#else
	int q   = wArg1 - wArg2 - (m_regF & C_FLAG);

	m_regF  = (((wArg1^q^wArg2)&0x1000)>>8)|((q>>16)&1)|((q&0x8000)>>8)|
		((q&65535)?0:Z_FLAG)|(((wArg1^wArg2)&(wArg1^q)&0x8000)>>13)|N_FLAG;

	return q;
#endif
}

/* inline */ WORD Add_2 (WORD wArg1, WORD wArg2)
{
#ifdef ARITH_TABLES
	int q  = wArg1 + wArg2;
	m_regF = (m_regF&(S_FLAG|Z_FLAG|V_FLAG))|(((wArg1^q^wArg2)&0x1000)>>8)|((q>>16)&1);
	return q;
#else
	int q  = wArg1 + wArg2;
	m_regF = (m_regF&(S_FLAG|Z_FLAG|V_FLAG))|(((wArg1^q^wArg2)&0x1000)>>8)|((q>>16)&1);
	return q;
#endif
}

/* inline */ void Add_1 (BYTE bArg)
{
#ifdef ARITH_TABLES
	int q = m_regA + bArg;
	m_regF = rgfAdd[m_regA][bArg];
	m_regA = q;
#else
	int q = m_regA + bArg;
	m_regF=ZSTable[q&255]|((q&256)>>8)| ((m_regA^q^bArg)&H_FLAG)| (((bArg^m_regA^0x80)&(bArg^q)&0x80)>>5);
	m_regA=q;
#endif
}

/* inline */ void Adc_1 (BYTE bArg)
{
#ifdef ARITH_TABLES
	int q  = m_regA + bArg + (m_regF & C_FLAG);
	m_regF = rgfAddc[m_regA][bArg][m_regF & C_FLAG];
	m_regA = q;
#else
	int q  = m_regA + bArg + (m_regF & C_FLAG);
	m_regF = ZSTable[q & 255]|((q & 256)>>8)|((m_regA^q^bArg)&H_FLAG)|(((bArg^m_regA^0x80)&(bArg^q)&0x80)>>5);
	m_regA = q;
#endif
}

/* inline */ void Sub_1 (BYTE bArg)
{
#ifdef ARITH_TABLES
	int q  = m_regA - bArg;
	m_regF = rgfSub[m_regA][bArg];
	m_regA = q;
#else
	int q  = m_regA - bArg;
	m_regF = ZSTable[q&255]|((q&256)>>8)|N_FLAG|((m_regA^q^bArg)&H_FLAG)|(((bArg^m_regA)&(m_regA^q)&0x80)>>5);
	m_regA = q;
#endif
}

/* inline */ void Sbc_1 (BYTE bArg)
{
#ifdef ARITH_TABLES
	int q = m_regA - bArg - (m_regF & C_FLAG);
	m_regF = rgfSubc[m_regA][bArg][m_regF & C_FLAG];
	m_regA = q;
#else
	int q = m_regA - bArg - (m_regF & C_FLAG);
	m_regF=ZSTable[q&255]|((q&256)>>8)|N_FLAG|((m_regA^q^bArg)&H_FLAG)|(((bArg^m_regA)&(bArg^q)&0x80)>>5);
	m_regA=q;
#endif
}

/* inline */ void And (BYTE bArg)
{
	m_regF = ZSPTable[m_regA &= bArg] | H_FLAG;
}

/* inline */ void Or(BYTE bArg)
{
	m_regF = ZSPTable[m_regA |= bArg];
}

/* inline */ void Xor(BYTE bArg)
{
	m_regF = ZSPTable[m_regA ^= bArg];
}

/* inline */ void Cp(BYTE bArg)
{
#ifdef ARITH_TABLES
	int q = m_regA - bArg;
	m_regF = rgfSub[m_regA][bArg];
#else
	int q = m_regA - bArg;
	m_regF = ZSTable[q&255]|((q&256)>>8)|N_FLAG|((m_regA^q^bArg)&H_FLAG)|(((bArg^m_regA)&(bArg^q)&0x80)>>5);
#endif
}

/* inline */ static BYTE Inc(BYTE bArg)
{
#ifdef ARITH_TABLES
	bArg++;
	m_regF = (m_regF & C_FLAG) | rgfInc[bArg];
	return bArg;
#else
	bArg++;
	m_regF = (m_regF&C_FLAG)|ZSTable[bArg]|((bArg==0x80)?V_FLAG:0)|((bArg&0x0F)?0:H_FLAG);
	return bArg;
#endif
}

/* inline */ static BYTE Dec(BYTE bArg)
{
#ifdef ARITH_TABLES
	m_regF = (m_regF & C_FLAG) | rgfDec[bArg--];
	return bArg;
#else
	m_regF=(m_regF&C_FLAG)|N_FLAG|
		((bArg==0x80)?V_FLAG:0)|((bArg&0x0F)?0:H_FLAG);

	m_regF|=ZSTable[--bArg];

	return bArg;
#endif
}

// shouldn't even be members...

/* inline */ BYTE Set(BYTE bArg, int nBit)
{
	return (bArg | (1 << nBit));
}

/* inline */ BYTE Res(BYTE bArg, int nBit)
{
	return (bArg & ~(1 << nBit));
}

/* inline */ void Bit(BYTE bArg, int nBit)
{
#ifdef ARITH_TABLES
	m_regF = (m_regF & C_FLAG) | rgfBit[bArg][nBit];
#else
	m_regF = (m_regF & C_FLAG) | H_FLAG | ((bArg&(1<< nBit))? ((nBit==7)?S_FLAG:0):Z_FLAG);
#endif
}

/* inline */ BYTE Rlc(BYTE bArg)
{
	int q  = bArg >> 7;
	bArg   = (bArg << 1) | q;
	m_regF = ZSPTable[bArg] | q;
	return bArg;
}

/* inline */ BYTE Rrc(BYTE bArg)
{
	int q = bArg & 1;
	bArg = (bArg >> 1) | (q << 7);
	m_regF = ZSPTable[bArg]|q;
	return bArg;
}

/* inline */ BYTE Rl(BYTE bArg)
{
	int q=bArg>>7;
	bArg=(bArg<<1)|(m_regF&1);
	m_regF=ZSPTable[bArg]|q;
	return bArg;
}

/* inline */ BYTE Rr(BYTE bArg)
{
	int q = bArg&1;
	bArg=(bArg>>1)|(m_regF<<7);
	m_regF=ZSPTable[bArg]|q;
	return bArg;
}

/* inline */ BYTE Sll(BYTE bArg)
{
	int q = bArg>>7;
	bArg=(bArg<<1)|1;
	m_regF=ZSPTable[bArg]|q;
	return bArg;
}

/* inline */ BYTE Srl(BYTE bArg)
{
	int q = bArg&1;
	bArg>>=1;
	m_regF=ZSPTable[bArg]|q;
	return bArg;
}

/* inline */ BYTE Sla(BYTE bArg)
{
	int q = bArg>>7;
	bArg<<=1;
	m_regF=ZSPTable[bArg]|q;
	return bArg;
}

/* inline */ BYTE Sra(BYTE bArg)
{
	int q = bArg&1;
	bArg = (bArg>>1)|(bArg&0x80);
	m_regF = ZSPTable[bArg]|q;
	return bArg;
}

/* ***************************************************************************
 * Edwards Z80 Utilities
 * ***************************************************************************
 */

UINT16 swap_variable;
#define swap(b1,b2) swap_variable = b1; b1 = b2; b2 = swap_variable;
/* inline void swap ( WORD &b1, WORD &b2 ) {
 * WORD b = b1;
 * b1 = b2;
 * b2 = b;
 * }
 */

/* inline */ void Rlca() {
  m_regA = (m_regA << 1) | ((m_regA & 0x80) >> 7);
  m_regF = (m_regF & 0xEC) | (m_regA & C_FLAG);
}

/* inline */ void Rrca() {
  m_regF=(m_regF&0xEC)|(m_regA&0x01);
  m_regA=(m_regA>>1)|(m_regA<<7);
}

/* inline */ void Rla() {
  int i  = m_regF&C_FLAG;
  m_regF = (m_regF&0xEC)|((m_regA&0x80)>>7);
  m_regA = (m_regA<<1)|i;
}

/* inline */ void Rra() {
  int i = m_regF&C_FLAG;
  m_regF=(m_regF&0xEC)|(m_regA&0x01);
  m_regA=(m_regA>>1)|(i<<7);
}

/* ***************************************************************************
 * Edwards Z80 Good-bits follow
 * ***************************************************************************
 */

void InitTables (void) {
  int i;
  DWORD dw;

	//
	//
	//
	
  for (i=0; i<256; ++i)
    {
      BYTE zs = 0;
      int p=0;
      BYTE bArg;
      int j;
		
      // zs |= i ? 0 : Z_FLAG;
      if (i==0)
	zs|=Z_FLAG;
		
      // zs |= i & S_FLAG;
      if (i&0x80)
	zs|=S_FLAG;
		
      if (i&1) ++p;
      if (i&2) ++p;
      if (i&4) ++p;
      if (i&8) ++p;
      if (i&16) ++p;
      if (i&32) ++p;
      if (i&64) ++p;
      if (i&128) ++p;
		
      //		PTable[i]  = (p&1) ? 0:V_FLAG;
      ZSTable[i] = zs;
      ZSPTable[i]= zs | ((p&1) ? 0:V_FLAG);

      rgfInc[i] = ZSTable[i] | ((i == 0x80) ? V_FLAG : 0) | ((i & 0x0F) ? 0 : H_FLAG);

      //		rgfDec[i] = N_FLAG | ((i == 0x80) ? V_FLAG : 0) | ((i & 0x0F) ? 0 : H_FLAG) | ZSTable[(BYTE)i - 1];

      bArg = i;
      rgfDec[i] = N_FLAG|((bArg==0x80)?V_FLAG:0)|((bArg&0x0F)?0:H_FLAG);
      rgfDec[i]|=ZSTable[--bArg];

      for ( j = 0; j < 8; j++)
	rgfBit[i][j] = H_FLAG | ((i & (1 << j))? ((j == 7) ? S_FLAG : 0) : Z_FLAG);
    }

  //
  //  Calculate flag tables
  //
	
  for ( dw = 0; dw <= 256 * 256; dw++)
    {
      BYTE bLo = LOBYTE(LOWORD(dw));
      BYTE bHi = HIBYTE(LOWORD(dw));
		
      const BYTE regA = bLo;
      const BYTE bArg = bHi;

      int nAcc;
      BYTE regF;
      int c;

      //
      //  ADD
      //
		
      nAcc = regA + bArg;
      regF = 0;
		
      regF |= ZSTable[nAcc&255];
      regF |= ((nAcc&256)>>8);
      regF |= ((regA^nAcc^bArg)&H_FLAG);
      regF |= (((bArg^regA^0x80)&(bArg^nAcc)&0x80)>>5);

      rgfAdd[regA][bArg] = regF;

      //
      //  SUB
      //
		
      nAcc  = regA - bArg;
      regF = ZSTable[nAcc&255]|((nAcc&256)>>8)|N_FLAG|((regA^nAcc^bArg)&H_FLAG)|(((bArg^regA)&(regA^nAcc)&0x80)>>5);

      rgfSub[regA][bArg] = regF;

      //
      //
      //

      for ( c = 0; c <= 1; c++)
	{
	  //
	  //  ADC
	  //

	  nAcc = regA + bArg + c;
	  regF = 0;
			
	  regF |= ZSTable[nAcc&255];
	  regF |= ((nAcc&256)>>8);
	  regF |= ((regA^nAcc^bArg)&H_FLAG);
	  regF |= (((bArg^regA^0x80)&(bArg^nAcc)&0x80)>>5);

	  rgfAddc[regA][bArg][c] = regF;

	  //
	  //  SBC
	  //

	  nAcc = regA - bArg - c;
	  regF = ZSTable[nAcc&255]|((nAcc&256)>>8)|N_FLAG|((regA^nAcc^bArg)&H_FLAG)|(((bArg^regA)&(bArg^nAcc)&0x80)>>5);

	  rgfSubc[regA][bArg][c] = regF;
	}
    }
}

/* ***************************************************************************
 * Opcodes
 * ***************************************************************************
 */

/*---------------------------------------------------------------------------

	Exec

	Executes the specified number of t-states.  Returns the actual number of
	t-states executed.  If a halt instruction is encountered, the execution
	loop exits.  If the processor is currently in a halted state this
	function immediately returns 0.

---------------------------------------------------------------------------*/

unsigned long int mz80exec ( unsigned long int cCyclesArg ) {
	UINT32 origElapsedTicks=dwElapsedTicks;
	//
	//
	//
	
//	if (m_fHalt)
//		return 0;

	//
	//  Save t-state count for return value calculation
	//
	
	cCycles = cCyclesArg;

	//
	//
	//
	

#ifndef TRACE_Z80
	while (cCycles > 0) 
#endif
	{
#ifdef _DEBUG
		static int cins;
		
		if ((cins % 1000) == 0 && GetAsyncKeyState('X') < 0)
			_ASSERT(FALSE);

		_ASSERT(cins != _instrap);

		if (cins >= _rgcins[0] && cins <= _rgcins[0] + _rgcins[1] * _rgcins[2] && ((cins - _rgcins[0]) % _rgcins[2]) == 0)
		{
			if (cins == _rgcins[0])
			{
				TRACE("instr    PC   AF   BC   DE   HL   IX   IY   SP   cycl dasm        \n");
				TRACE("-------- ---- ---- ---- ---- ---- ---- ---- ---- ---- ------------\n");
			}

			TCHAR szBuf[128];

			Z80_Dasm(&m_rgbMemory[GetPC()], szBuf, GetPC());

			TRACE("%08X %04X %04X %04X %04X %04X %04X %04X %04X %04X %s\n",
				cins, GetPC(), m_regAF, m_regBC, m_regDE, m_regHL, m_regIX, m_regIY, GetSP(), cCycles, szBuf);
		}

		cins++;
#endif
		const BYTE bOpcode = ImmedByte();

#ifdef _DEBUG
		m_rgcOpcode[0][bOpcode]++;
#endif

		m_regR++;

#if 0
		/* debug crap for RC */
		{
		  char szBuf [ 128 ];
		  Z80_Dasm(&m_rgbMemory[GetPC()], szBuf, GetPC());

		  fprintf ( stderr,
			    "%04X %04X %04X %04X %04X %04X %04X %04X %04X %s\n",
			    GetPC(), m_regAF, m_regBC, m_regDE, m_regHL,
			    m_regIX, m_regIY, GetSP(), cCycles, szBuf );
		}

		// fprintf ( stderr, "%d\n", bOpcode );
#endif
		z80pc = GetPC(); /* for some of the platforms */

		dwElapsedTicks = origElapsedTicks + (cCyclesArg - cCycles);

		switch (bOpcode)
		{
			case 0x00: // nop     
				cCycles -= 4;
				break;
			
			case 0x01: // ld_bc_word
				cCycles -= 10;
				m_regBC = ImmedWord();
				break;
			
			case 0x02: // ld_xbc_a   
				cCycles -= 7;
				MemWriteByte(m_regBC, m_regA);
				break;
			
			case 0x03: // inc_bc    
				cCycles -= 6;
				m_regBC++;
				break;
			
			case 0x04: // inc_b   
				cCycles -= 4;
				m_regB = Inc(m_regB);
				break;
			
			case 0x05: // dec_b   
				cCycles -= 4;
				m_regB = Dec(m_regB);
				break;
			
			case 0x06: // ld_b_byte  
				cCycles -= 7;
				m_regB = ImmedByte();
				break;
			
			case 0x07: // rlca    
				cCycles -= 4;
				Rlca();
				break;

			case 0x08: // ex_af_af
				cCycles -= 4;
				swap(m_regAF, m_regAF2);
				break;
			
			case 0x09: // add_hl_bc 
				cCycles -= 11;
				m_regHL = Add_2(m_regHL, m_regBC);
				break;
			
			case 0x0A: // ld_a_xbc   
				cCycles -= 7;
				m_regA = MemReadByte(m_regBC);
				break;
			
			case 0x0B: // dec_bc    
				cCycles -= 6;
				m_regBC--;
				break;
			
			case 0x0C: // inc_c   
				cCycles -= 4;
				m_regC = Inc(m_regC);
				break;
			
			case 0x0D: // dec_c   
				cCycles -= 4;
				m_regC = Dec(m_regC);
				break;
			
			case 0x0E: // ld_c_byte  
				cCycles -= 7;
				m_regC = ImmedByte();
				break;
			
			case 0x0F: // rrca    
				cCycles -= 4;
				Rrca();
				break;

			case 0x10: // djnz
				cCycles -= 8;
				cCycles -= Jr0(--m_regB);
				break;
			
			case 0x11: // ld_de_word
				cCycles -= 10;
				m_regDE = ImmedWord();
				break;
			
			case 0x12: // ld_xde_a   
				cCycles -= 7;
				MemWriteByte(m_regDE, m_regA);
				break;
			
			case 0x13: // inc_de    
				cCycles -= 6;
				m_regDE++;
				break;
			
			case 0x14: // inc_d   
				cCycles -= 4;
				m_regD = Inc(m_regD);
				break;
			
			case 0x15: // dec_d   
				cCycles -= 4;
				m_regD = Dec(m_regD);
				break;
			
			case 0x16: // ld_d_byte  
				cCycles -= 7;
				m_regD = ImmedByte();
				break;
			
			case 0x17: // rla     
				cCycles -= 4;
				Rla();
				break;

			case 0x18: // jr
			{
#if 0
				cCycles -= Jr0(TRUE);
#else
				// speedup technique adopted from Nicola's code in MAME to speedup
				// Galaga (the sound CPU spins with "loop: jr loop" waiting for
				// an interrupt).  This technique is generic enough that is shouldn't
				// adversely effect other applications.
				
				signed char offset = (signed char)ImmedByte();
				cCycles -= 7;
				cCycles -= 5;

				AdjustPC(offset);

				if (offset == -2)
				{
//					TRACE(__FILE__ ":jr (infinite loop) at %04X\n", GetPC());
				  goto z80ExecBottom;
				  /* return (cCyclesArg - cCycles); */
				}
#endif
				break;
			}
			
			case 0x19: // add_hl_de 
				cCycles -= 11;
				m_regHL = Add_2(m_regHL, m_regDE);
				break;
			
			case 0x1A: // ld_a_xde   
				cCycles -= 7;
				m_regA = MemReadByte(m_regDE);
				break;
			
			case 0x1B: // dec_de    
				cCycles -= 6;
				m_regDE--;
				break;
			
			case 0x1C: // inc_e   
				cCycles -= 4;
				m_regE = Inc(m_regE); 
				break;
			
			case 0x1D: // dec_e   
				cCycles -= 4;
				m_regE = Dec(m_regE);
				break;
			
			case 0x1E: // ld_e_byte  
				cCycles -= 7;
				m_regE = ImmedByte();
				break;
			
			case 0x1F: // rra     
				cCycles -= 4;
				Rra();
				break;

			case 0x20: // jr_nz   
				cCycles -= 7;
				cCycles -= Jr1(m_regF & Z_FLAG);
				break;
			
			case 0x21: // ld_hl_word
				cCycles -= 10;
				m_regHL = ImmedWord();
				break;
			
			case 0x22: // ld_xword_hl
				cCycles -= 16;
				MemWriteWord(ImmedWord(), m_regHL);
				break;
			
			case 0x23: // inc_hl    
				cCycles -= 6;
				m_regHL++;
				break;
			
			case 0x24: // inc_h   
				cCycles -= 4;
				m_regH = Inc(m_regH);
				break;
			
			case 0x25: // dec_h   
				cCycles -= 4;
				m_regH = Dec(m_regH);
				break;
			
			case 0x26: // ld_h_byte  
				cCycles -= 7;
				m_regH = ImmedByte();
				break;
			
			case 0x27: // daa
			{
				int i = m_regA;
				cCycles -= 4;
				if (m_regF&C_FLAG) i|=256;
				if (m_regF&H_FLAG) i|=512;
				if (m_regF&N_FLAG) i|=1024;
				m_regAF = DAATable[i];
				break;
			}

			case 0x28: // jr_z    
				cCycles -= 7;
				cCycles -= Jr0(m_regF & Z_FLAG);
				break;
			
			case 0x29: // add_hl_hl
				cCycles -= 11;
				m_regHL = Add_2(m_regHL, m_regHL);
				break;
			
			case 0x2A: // ld_hl_xword
				cCycles -= 16;
				m_regHL = MemReadWord(ImmedWord());
				break;
			
			case 0x2B: // dec_hl    
				cCycles -= 6;
				m_regHL--;
				break;
			
			case 0x2C: // inc_l   
				cCycles -= 4;
				m_regL = Inc(m_regL);
				break;
			
			case 0x2D: // dec_l   
				cCycles -= 4;
				m_regL = Dec(m_regL);
				break;
			
			case 0x2E: // ld_l_byte  
				cCycles -= 7;
				m_regL = ImmedByte();
				break;
			
			case 0x2F: // cpl     
				cCycles -= 4;
				m_regA ^= 0xFF;
				m_regF |= (H_FLAG | N_FLAG);
				break;

			case 0x30: // jr_nc   
				cCycles -= 7;
				cCycles -= Jr1(m_regF & C_FLAG);
				break;
			
			case 0x31: // ld_sp_word
				cCycles -= 10;
				SetSP(ImmedWord());
				break;
			
			case 0x32: // ld_xbyte_a
				cCycles -= 13;
				MemWriteByte(ImmedWord(), m_regA);
				break;
			
			case 0x33: // inc_sp
				cCycles -= 6;
				SetSP(GetSP() + 1);
				break;
			
			case 0x34: // inc_xhl
				cCycles -= 11;
				MemWriteByte(m_regHL, Inc(MemReadByte(m_regHL)));
				break;
			
			case 0x35: // dec_xhl
				cCycles -= 11;
				MemWriteByte(m_regHL, Dec(MemReadByte(m_regHL)));
				break;
			
			case 0x36: // ld_xhl_byte
				cCycles -= 10;
				MemWriteByte(m_regHL, ImmedByte());
				break;
			
			case 0x37: // scf
				cCycles -= 4;
				m_regF = (m_regF & 0xEC) | C_FLAG;
				break;

			case 0x38: // jr_c
				cCycles -= 7;
				cCycles -= Jr0(m_regF & C_FLAG);
				break;
			
			case 0x39: // add_hl_sp
				cCycles -= 11;
				m_regHL = Add_2(m_regHL, GetSP());
				break;
			
			case 0x3A: // ld_a_xbyte 
				cCycles -= 13;
				m_regA = MemReadByte(ImmedWord());
				break;
			
			case 0x3B: // dec_sp
				cCycles -= 6;
				SetSP(GetSP() - 1);
				break;
			
			case 0x3C: // inc_a
				cCycles -= 4;
				m_regA = Inc(m_regA);
				break;
			
			case 0x3D: // dec_a
				cCycles -= 4;
				m_regA = Dec(m_regA);
				break;
			
			case 0x3E: // ld_a_byte
				cCycles -= 7;
				m_regA = ImmedByte();
				break;
			
			case 0x3F: // ccf
				cCycles -= 4;
				m_regF =((m_regF & 0xED)|((m_regF & 1)<<4))^1;
				break;

			case 0x40: // ld_b_b  
				cCycles -= 4;
				m_regB = m_regB;
				break;
			
			case 0x41: // ld_b_c
				cCycles -= 4;
				m_regB = m_regC;
				break;
			
			case 0x42: // ld_b_d
				cCycles -= 4;
				m_regB = m_regD;
				break;
			
			case 0x43: // ld_b_e
				cCycles -= 4;
				m_regB = m_regE;
				break;
			
			case 0x44: // ld_b_h  
				cCycles -= 4;
				m_regB = m_regH;
				break;
			
			case 0x45: // ld_b_l  
				cCycles -= 4;
				m_regB = m_regL;
				break;
			
			case 0x46: // ld_b_xhl   
				cCycles -= 7;
				m_regB = MemReadByte(m_regHL);
				break;
			
			case 0x47: // ld_b_a  
				cCycles -= 4;
				m_regB = m_regA;
				break;

			case 0x48: // ld_c_b  
				cCycles -= 4;
				m_regC = m_regB;
				break;
			
			case 0x49: // ld_c_c    
				cCycles -= 4;
				m_regC = m_regC;
				break;
			
			case 0x4A: // ld_c_d     
				cCycles -= 4;
				m_regC = m_regD;
				break;
			
			case 0x4B: // ld_c_e    
				cCycles -= 4;
				m_regC = m_regE;
				break;
			
			case 0x4C: // ld_c_h  
				cCycles -= 4;
				m_regC = m_regH;
				break;
			
			case 0x4D: // ld_c_l  
				cCycles -= 4;
				m_regC = m_regL;
				break;
			
			case 0x4E: // ld_c_xhl   
				cCycles -= 7;
				m_regC = MemReadByte(m_regHL);
				break;
			
			case 0x4F: // ld_c_a  
				cCycles -= 4;
				m_regC = m_regA;
				break;

			case 0x50: // ld_d_b  
				cCycles -= 4;
				m_regD = m_regB;
				break;
			
			case 0x51: // ld_d_c    
				cCycles -= 4;
				m_regD = m_regC;
				break;
			
			case 0x52: // ld_d_d     
				cCycles -= 4;
				m_regD = m_regD;
				break;
			
			case 0x53: // ld_d_e    
				cCycles -= 4;
				m_regD = m_regE;
				break;
			
			case 0x54: // ld_d_h  
				cCycles -= 4;
				m_regD = m_regH;
				break;
			
			case 0x55: // ld_d_l  
				cCycles -= 4;
				m_regD = m_regL;
				break;
			
			case 0x56: // ld_d_xhl   
				cCycles -= 7;
				m_regD = MemReadByte(m_regHL);
				break;
			
			case 0x57: // ld_d_a  
				cCycles -= 4;
				m_regD = m_regA;
				break;

			case 0x58: // ld_e_b  
				cCycles -= 4;
				m_regE = m_regB;
				break;
			
			case 0x59: // ld_e_c    
				cCycles -= 4;
				m_regE = m_regC;
				break;
			
			case 0x5A: // ld_e_d     
				cCycles -= 4;
				m_regE = m_regD;
				break;
			
			case 0x5B: // ld_e_e    
				cCycles -= 4;
				m_regE = m_regE;
				break;
			
			case 0x5C: // ld_e_h  
				cCycles -= 4;
				m_regE = m_regH;
				break;
			
			case 0x5D: // ld_e_l  
				cCycles -= 4;
				m_regE = m_regL;
				break;
			
			case 0x5E: // ld_e_xhl   
				cCycles -= 7;
				m_regE = MemReadByte(m_regHL);
				break;
			
			case 0x5F: // ld_e_a  
				cCycles -= 4;
				m_regE = m_regA;
				break;

			case 0x60: // ld_h_b  
				cCycles -= 4;
				m_regH = m_regB;
				break;
			
			case 0x61: // ld_h_c    
				cCycles -= 4;
				m_regH = m_regC;
				break;
			
			case 0x62: // ld_h_d     
				cCycles -= 4;
				m_regH = m_regD;
				break;
			
			case 0x63: // ld_h_e    
				cCycles -= 4;
				m_regH = m_regE;
				break;
			
			case 0x64: // ld_h_h  
				cCycles -= 4;
				m_regH = m_regH;
				break;
			
			case 0x65: // ld_h_l  
				cCycles -= 4;
				m_regH = m_regL;
				break;
			
			case 0x66: // ld_h_xhl   
				cCycles -= 7;
				m_regH = MemReadByte(m_regHL);
				break;
			
			case 0x67: // ld_h_a  
				cCycles -= 4;
				m_regH = m_regA;
				break;

			case 0x68: // ld_l_b  
				cCycles -= 4;
				m_regL = m_regB;
				break;
			
			case 0x69: // ld_l_c    
				cCycles -= 4;
				m_regL = m_regC;
				break;
			
			case 0x6A: // ld_l_d     
				cCycles -= 4;
				m_regL = m_regD;
				break;
			
			case 0x6B: // ld_l_e    
				cCycles -= 4;
				m_regL = m_regE;
				break;
			
			case 0x6C: // ld_l_h  
				cCycles -= 4;
				m_regL = m_regH;
				break;
			
			case 0x6D: // ld_l_l  
				cCycles -= 4;
				m_regL = m_regL;
				break;
			
			case 0x6E: // ld_l_xhl   
				cCycles -= 7;
				m_regL = MemReadByte(m_regHL);
				break;
			
			case 0x6F: // ld_l_a  
				cCycles -= 4;
				m_regL = m_regA;
				break;

			case 0x70: // ld_xhl_b
				cCycles -= 7;
				MemWriteByte(m_regHL, m_regB);
				break;
			
			case 0x71: // ld_xhl_c  
				cCycles -= 7;
				MemWriteByte(m_regHL, m_regC);
				break;
			
			case 0x72: // ld_xhl_d   
				cCycles -= 7;
				MemWriteByte(m_regHL, m_regD);
				break;
			
			case 0x73: // ld_xhl_e  
				cCycles -= 7;
				MemWriteByte(m_regHL, m_regE);
				break;
			
			case 0x74: // ld_xhl_h
				cCycles -= 7;
				MemWriteByte(m_regHL, m_regH);
				break;
			
			case 0x75: // ld_xhl_l
				cCycles -= 7;
				MemWriteByte(m_regHL, m_regL);
				break;
			
			case 0x76: // halt
				cCycles -= 4;
				SetPC(GetPC() - 1);
				m_fHalt = TRUE;
				goto z80ExecBottom;
				/* return (cCyclesArg - cCycles); */
			
			case 0x77: // ld_xhl_a
				cCycles -= 7;
				MemWriteByte(m_regHL, m_regA);
				break;

			case 0x78: // ld_a_b  
				cCycles -= 4;
				m_regA = m_regB;
				break;
			
			case 0x79: // ld_a_c    
				cCycles -= 4;
				m_regA = m_regC;
				break;
			
			case 0x7A: // ld_a_d     
				cCycles -= 4;
				m_regA = m_regD;
				break;
			
			case 0x7B: // ld_a_e    
				cCycles -= 4;
				m_regA = m_regE;
				break;
			
			case 0x7C: // ld_a_h  
				cCycles -= 4;
				m_regA = m_regH;
				break;
			
			case 0x7D: // ld_a_l  
				cCycles -= 4;
				m_regA = m_regL;
				break;
			
			case 0x7E: // ld_a_xhl   
				cCycles -= 7;
				m_regA = MemReadByte(m_regHL);
				break;
			
			case 0x7F: // ld_a_a  
				cCycles -= 4;
				m_regA = m_regA;
				break;

			case 0x80: // add_a_b 
				cCycles -= 4;
				Add_1(m_regB);
				break;
			
			case 0x81: // add_a_c   
				cCycles -= 4;
				Add_1(m_regC);
				break;
			
			case 0x82: // add_a_d    
				cCycles -= 4;
				Add_1(m_regD);
				break;
			
			case 0x83: // add_a_e   
				cCycles -= 4;
				Add_1(m_regE);
				break;
			
			case 0x84: // add_a_h 
				cCycles -= 4;
				Add_1(m_regH);
				break;
			
			case 0x85: // add_a_l 
				cCycles -= 4;
				Add_1(m_regL);
				break;
			
			case 0x86: // add_a_xhl  
				cCycles -= 7;
				Add_1(MemReadByte(m_regHL));
				break;
			
			case 0x87: // add_a_a 
				cCycles -= 4;
				Add_1(m_regA);
				break;

			case 0x88: // adc_a_b 
				cCycles -= 4;
				Adc_1(m_regB);
				break;
			
			case 0x89: // adc_a_c   
				cCycles -= 4;
				Adc_1(m_regC);
				break;
			
			case 0x8A: // adc_a_d    
				cCycles -= 4;
				Adc_1(m_regD);
				break;
			
			case 0x8B: // adc_a_e   
				cCycles -= 4;
				Adc_1(m_regE);
				break;
			
			case 0x8C: // adc_a_h 
				cCycles -= 4;
				Adc_1(m_regH);
				break;
			
			case 0x8D: // adc_a_l 
				cCycles -= 4;
				Adc_1(m_regL);
				break;
			
			case 0x8E: // adc_a_xhl  
				cCycles -= 7;
				Adc_1(MemReadByte(m_regHL));
				break;
			
			case 0x8F: // adc_a_a 
				cCycles -= 4;
				Adc_1(m_regA);
				break;

			case 0x90: // sub_b   
				cCycles -= 4;
				Sub_1(m_regB);
				break;
			
			case 0x91: // sub_c     
				cCycles -= 4;
				Sub_1(m_regC);
				break;
			
			case 0x92: // sub_d      
				cCycles -= 4;
				Sub_1(m_regD);
				break;
			
			case 0x93: // sub_e     
				cCycles -= 4;
				Sub_1(m_regE);
				break;
			
			case 0x94: // sub_h   
				cCycles -= 4;
				Sub_1(m_regH);
				break;
			
			case 0x95: // sub_l   
				cCycles -= 4;
				Sub_1(m_regL);
				break;
			
			case 0x96: // sub_xhl    
				cCycles -= 7;
				Sub_1(MemReadByte(m_regHL));
				break;
			
			case 0x97: // sub_a   
				cCycles -= 4;
				Sub_1(m_regA);
				break;

			case 0x98: // sbc_a_b 
				cCycles -= 4;
				Sbc_1(m_regB);
				break;
			
			case 0x99: // sbc_a_c   
				cCycles -= 4;
				Sbc_1(m_regC);
				break;
			
			case 0x9A: // sbc_a_d    
				cCycles -= 4;
				Sbc_1(m_regD);
				break;
			
			case 0x9B: // sbc_a_e   
				cCycles -= 4;
				Sbc_1(m_regE);
				break;
			
			case 0x9C: // sbc_a_h 
				cCycles -= 4;
				Sbc_1(m_regH);
				break;
			
			case 0x9D: // sbc_a_l 
				cCycles -= 4;
				Sbc_1(m_regL);
				break;
			
			case 0x9E: // sbc_a_xhl  
				cCycles -= 7;
				Sbc_1(MemReadByte(m_regHL));
				break;
			
			case 0x9F: // sbc_a_a 
				cCycles -= 4;
				Sbc_1(m_regA);
				break;

			case 0xA0: // and_b   
				cCycles -= 4;
				And(m_regB);
				break;
			
			case 0xA1: // and_c     
				cCycles -= 4;
				And(m_regC);
				break;
			
			case 0xA2: // and_d      
				cCycles -= 4;
				And(m_regD);
				break;
			
			case 0xA3: // and_e     
				cCycles -= 4;
				And(m_regE);
				break;
			
			case 0xA4: // and_h   
				cCycles -= 4;
				And(m_regH);
				break;
			
			case 0xA5: // and_l   
				cCycles -= 4;
				And(m_regL);
				break;
			
			case 0xA6: // and_xhl    
				cCycles -= 7;
				And(MemReadByte(m_regHL));
				break;
			
			case 0xA7: // and_a   
				cCycles -= 4;
				And(m_regA);
				break;

			case 0xA8: // xor_b   
				cCycles -= 4;
				Xor(m_regB);
				break;
			
			case 0xA9: // xor_c     
				cCycles -= 4;
				Xor(m_regC);
				break;
			
			case 0xAA: // xor_d      
				cCycles -= 4;
				Xor(m_regD);
				break;
			
			case 0xAB: // xor_e     
				cCycles -= 4;
				Xor(m_regE);
				break;
			
			case 0xAC: // xor_h   
				cCycles -= 4;
				Xor(m_regH);
				break;
			
			case 0xAD: // xor_l   
				cCycles -= 4;
				Xor(m_regL);
				break;
			
			case 0xAE: // xor_xhl    
				cCycles -= 7;
				Xor(MemReadByte(m_regHL));
				break;
			
			case 0xAF: // xor_a   
				cCycles -= 4;
				Xor(m_regA);
				break;

			case 0xB0: // or_b    
				cCycles -= 4;
				Or(m_regB);
				break;
			
			case 0xB1: // or_c      
				cCycles -= 4;
				Or(m_regC);
				break;
			
			case 0xB2: // or_d       
				cCycles -= 4;
				Or(m_regD);
				break;
			
			case 0xB3: // or_e      
				cCycles -= 4;
				Or(m_regE);
				break;
			
			case 0xB4: // or_h    
				cCycles -= 4;
				Or(m_regH);
				break;
			
			case 0xB5: // or_l    
				cCycles -= 4;
				Or(m_regL);
				break;
			
			case 0xB6: // or_xhl     
				cCycles -= 7;
				Or(MemReadByte(m_regHL));
				break;
			
			case 0xB7: // or_a    
				cCycles -= 4;
				Or(m_regA);
				break;

			case 0xB8: // cp_b    
				cCycles -= 4;
				Cp(m_regB);
				break;
			
			case 0xB9: // cp_c      
				cCycles -= 4;
				Cp(m_regC);
				break;
			
			case 0xBA: // cp_d       
				cCycles -= 4;
				Cp(m_regD);
				break;
			
			case 0xBB: // cp_e      
				cCycles -= 4;
				Cp(m_regE);
				break;
			
			case 0xBC: // cp_h    
				cCycles -= 4;
				Cp(m_regH);
				break;
			
			case 0xBD: // cp_l    
				cCycles -= 4;
				Cp(m_regL);
				break;
			
			case 0xBE: // cp_xhl     
				cCycles -= 7;
				Cp(MemReadByte(m_regHL));
				break;
			
			case 0xBF: // cp_a    
				cCycles -= 4;
				Cp(m_regA);
				break;

			case 0xC0: // ret_nz 
				cCycles -= 5;
				cCycles -= Ret1(m_regF & Z_FLAG);
				break;
			
			case 0xC1: // pop_bc    
				cCycles -= 10;
				m_regBC = Pop();
				break;
			
			case 0xC2: // jp_nz      
				cCycles -= 10;
				cCycles -= Jp1(m_regF & Z_FLAG);
				break;
			
			case 0xC3: // jp
				cCycles -= 10;
#if 0
				Jp0(TRUE);
#else
				{
					// speedup hack for Galaga adopted from MAME...
					
					WORD w = ImmedWord();
					WORD wPc = GetPC();
					
					SetPC(w);

					// removed since Galaga never hit it (maybe 1942 will), and it
					// caused the slowdown in Time Pilot and Gyruss hiscore bug
					// it isn't correct anyway since "0085 jp 0088" causes is to return (Gyruss)
					// shouldn't it be if (w == wPC - 3)???
					// jul-26-97 eam

/*
					if (w == wPc)
					{
	//					TRACE(__FILE__ ":jp (infinite loop) at %04X\n", GetPC());
						return (cCyclesArg - cCycles);
					}
					else */
					if (w == wPc - 6) // attn: very Galaga specific!!!! should be done in MageX by patching ROMs with HALTs (once I figure out how to fool the ROM check at bootup)
					{
						if (*m_rgbOpcode == 0x31)
						{
		//					TRACE(__FILE__ ":jp (ld sp,#xxxx) at %04X\n", GetPC());
						  goto z80ExecBottom;
						  /* return (cCyclesArg - cCycles); */
						}
					}
				}
#endif
				break;
			
			case 0xC4: // call_nz 
				cCycles -= 10;
				cCycles -= Call1(m_regF & Z_FLAG);
				break;
			
			case 0xC5: // push_bc
				cCycles -= 11;
				Push(m_regBC);
				break;
			
			case 0xC6: // add_a_byte 
				cCycles -= 7;
				Add_1(ImmedByte());
				break;
			
			case 0xC7: // rst_00  
				cCycles -= 11;
				Rst(0x00);
				break;

			case 0xC8: // ret_z
				cCycles -= 5;
				cCycles -= Ret0(m_regF & Z_FLAG);
				break;
			
			case 0xC9: // ret
				cCycles -= 4;
				cCycles -= Ret0(TRUE);
				break;
			
			case 0xCA: // jp_z
				cCycles -= 10;
				cCycles -= Jp0(m_regF & Z_FLAG);
				break;
			
			case 0xCB: // cb
				cCycles -= HandleCB();
				break;
			
			case 0xCC: // call_z
				cCycles -= 10;
				cCycles -= Call0(m_regF & Z_FLAG);
				break;
			
			case 0xCD: // call
				cCycles -= 10;
				cCycles -= Call0(TRUE);
				break;
			
			case 0xCE: // adc_a_byte
				cCycles -= 7;
				Adc_1(ImmedByte());
				break;
			
			case 0xCF: // rst_08
				cCycles -= 11;
				Rst(0x08);
				break;

			case 0xD0: // ret_nc
				cCycles -= 5;
				cCycles -= Ret1(m_regF & C_FLAG);
				break;
			
			case 0xD1: // pop_de
				cCycles -= 10;
				m_regDE = Pop();
				break;
			
			case 0xD2: // jp_nc
				cCycles -= 10;
				cCycles -= Jp1(m_regF & C_FLAG);
				break;
			
			case 0xD3: // out_byte_a
				cCycles -= 11;
				Out(ImmedByte(), m_regA);
				break;
			
			case 0xD4: // call_nc
				cCycles -= 10;
				cCycles -= Call1(m_regF & C_FLAG);
				break;
			
			case 0xD5: // push_de
				cCycles -= 11;
				Push(m_regDE);
				break;
			
			case 0xD6: // sub_byte
			{
				cCycles -= 7;
				Sub_1(ImmedByte());
				break;
			}
			
			case 0xD7: // rst_10
				cCycles -= 11;
				Rst(0x10);
				break;

			case 0xD8: // ret_c
				cCycles -= 5;
				cCycles -= Ret0(m_regF & C_FLAG);
				break;
			
			case 0xD9: // exx
				cCycles -= 4;
				Exx();
				break;
			
			case 0xDA: // jp_c
				cCycles -= 10;
				cCycles -= Jp0(m_regF & C_FLAG);
				break;
			
			case 0xDB: // in_a_byte 
			{
				BYTE bPort = ImmedByte();
				cCycles -= 11;
				m_regA = InRaw ( bPort );
				break;
			}
			
			case 0xDC: // call_c
				cCycles -= 10;
				cCycles -= Call0(m_regF & C_FLAG);
				break;
			
			case 0xDD: // dd
				cCycles -= HandleDD();
				break;
			
			case 0xDE: // sbc_a_byte
				cCycles -= 7;
				Sbc_1(ImmedByte());
				break;
			
			case 0xDF: // rst_18
				cCycles -= 11;
				Rst(0x18);
				break;

			case 0xE0: // ret_po
				cCycles -= 5;
				cCycles -= Ret1(m_regF & V_FLAG);
				break;
			
			case 0xE1: // pop_hl
				cCycles -= 10;
				m_regHL = Pop();
				break;
			
			case 0xE2: // jp_po
				cCycles -= 10;
				cCycles -= Jp1(m_regF & V_FLAG);
				break;
			
			case 0xE3: // ex_xsp_hl
				cCycles -= 19;
			{
				int i = MemReadWord(GetSP());
				MemWriteWord(GetSP(), m_regHL);
				m_regHL = i;
				break;
			}
			
			case 0xE4: // call_po
				cCycles -= 10;
				cCycles -= Call1(m_regF & V_FLAG);
				break;
			
			case 0xE5: // push_hl
				cCycles -= 11;
				Push(m_regHL);
				break;
			
			case 0xE6: // and_byte
				cCycles -= 7;
				And(ImmedByte());
				break;
			
			case 0xE7: // rst_20
				cCycles -= 11;
				Rst(0x20);
				break;

			case 0xE8: // ret_pe
				cCycles -= 5;
				cCycles -= Ret0(m_regF & V_FLAG);
				break;
			
			case 0xE9: // jp_hl
				cCycles -= 4;
				SetPC(m_regHL);
				break;
			
			case 0xEA: // jp_pe
				cCycles -= 10;
				cCycles -= Jp0(m_regF & V_FLAG);
				break;
			
			case 0xEB: // ex_de_hl
				cCycles -= 4;
				swap(m_regDE, m_regHL);
				break;
			
			case 0xEC: // call_pe
				cCycles -= 10;
				cCycles -= Call0(m_regF & V_FLAG);
				break;
			
			case 0xED: // ed
				cCycles -= HandleED(cCycles);
				break;
			
			case 0xEE: // xor_byte
				cCycles -= 7;
				Xor(ImmedByte());
				break;
			
			case 0xEF: // rst_28
				cCycles -= 11;
				Rst(0x28);
				break;

			case 0xF0: // ret_p
				cCycles -= 5;
				cCycles -= Ret1(m_regF & S_FLAG);
				break;
			
			case 0xF1: // pop_af
				cCycles -= 10;
				m_regAF = Pop();
				break;
			
			case 0xF2: // jp_p
				cCycles -= 10;
				cCycles -= Jp1(m_regF & S_FLAG);
				break;
			
			case 0xF3: // di
				cCycles -= 4;
				Di();
				break;
			
			case 0xF4: // call_p
				cCycles -= 10;
				cCycles -= Call1(m_regF & S_FLAG);
				break;
			
			case 0xF5: // push_af
				cCycles -= 11;
				Push(m_regAF);
				break;
			
			case 0xF6: // or_byte
				cCycles -= 7;
				Or(ImmedByte());
				break;
			
			case 0xF7: // rst_30
				cCycles -= 11;
				Rst(0x30);
				break;

			case 0xF8: // ret_m   
				cCycles -= 5;
				cCycles -= Ret0(m_regF & S_FLAG);
				break;
			
			case 0xF9: // ld_sp_hl
				cCycles -= 6;
				SetSP(m_regHL);
				break;
			
			case 0xFA: // jp_m
				cCycles -= 10;
				cCycles -= Jp0(m_regF & S_FLAG);
				break;
			
			case 0xFB: // ei
				cCycles -= 4;
				cCycles -= Ei();
//				if (m_fPendingInterrupt)
				{
//					return (cCyclesArg - cCycles);
				}
				break;
			
			case 0xFC: // call_m
				cCycles -= 10;
				cCycles -= Call0(m_regF & S_FLAG);
				break;
			
			case 0xFD: // fd
				cCycles -= HandleFD();
				break;
			
			case 0xFE: // cp_byte    
				cCycles -= 7;
				Cp(ImmedByte());
				break;
			
			case 0xFF: // rst_38
				cCycles -= 11;
				Rst(0x38);
				break;

			default:
				assert(FALSE);
				break;
		}
	}

z80ExecBottom:
	z80pc = GetPC(); /* for some of the platforms */
	dwElapsedTicks = origElapsedTicks + (cCyclesArg - cCycles);
	return ( 0x80000000 );
}

UINT32 mz80int ( UINT32 bVal ) {
	if (m_iff1 == 0) // is IRQ disabled?
		return 0xffffffff;		// Interrupt not taken!
		
	m_fPendingInterrupt = FALSE;

	m_iff1 = 0;

	if (m_fHalt)
	{
		SetPC(GetPC() + 1);
		m_fHalt = FALSE;
	}

	if (0 == m_nIM || 1 == m_nIM)
	{
		Rst(z80intAddr);
		return(0);		// We took the interrupt
	}

	Push(GetPC());
	SetPC(MemReadWord(MAKEWORD(bVal, m_regI)));

	return(0); // fix-me
}

UINT32 mz80nmi ( void ) {
	m_iff1 = 0;

	if (m_fHalt)
	{
		SetPC(GetPC() + 1);
		m_fHalt = FALSE;
	}
	
	Rst(z80nmiAddr);

	return(0);
}

/* ***************************************************************************
 * Flow of Control
 * ***************************************************************************
 */

/* inline */ int Jr0(int f) {
	if (f)
	{
		AdjustPC((signed char)ImmedByte());
		return 5;
	}
	else
	{
		AdjustPC(1);
		return 0;
	}
}

/* inline */ int Jr1(int f) {
	if (f)
	{
		AdjustPC(1);
		return 0;
	}
	else
	{
		AdjustPC((signed char)ImmedByte());
		return 5;
	}
}

/* inline */ int Call0(int f) {
	char string[150];

	if (f)
	{
		WORD wAddr = ImmedWord();
		Push(GetPC());
		SetPC(wAddr);
		return 7;
	}
	else
	{
		AdjustPC(2);
		return 0;
	}
}

/* inline */ int Call1(int f) {
	if (f)
	{
		AdjustPC(2);
		return 0;
	}
	else
	{
		WORD wAddr = ImmedWord();
		Push(GetPC());
		SetPC(wAddr);
		return 7;
	}
}

/* inline */ int Jp0(int f) {
	if (f)
	{
		SetPC(ImmedWord());
		return 0; // ????????????????
	}
	else
	{
		AdjustPC(2);
		return 0;
	}
}

/* inline */ int Jp1(int f) {
	if (f)
	{
		AdjustPC(2);
		return 0;
	}
	else
	{
		SetPC(ImmedWord());
		return 0; // ????????????????
	}
}

/* inline */ void Rst(WORD wAddr) {
	Push(GetPC());
	SetPC(wAddr);
}

/* ***************************************************************************
 * CB prefixed instructions
 * ***************************************************************************
 */

int HandleCB() {
	const BYTE bOpcode = ImmedByte();
	m_regR++;

	switch (bOpcode)
	{
		case 0x00: // rlc_b
			m_regB = Rlc(m_regB);
			return 8;
		
		case 0x01: // rlc_c  
			m_regC = Rlc(m_regC);
			return 8;
		
		case 0x02: // rlc_d  
			m_regD = Rlc(m_regD);
			return 8;
		
		case 0x03: // rlc_e  
			m_regE = Rlc(m_regE);
			return 8;
		
		case 0x04: // rlc_h  
			m_regH = Rlc(m_regH);
			return 8;
		
		case 0x05: // rlc_l  
			m_regL = Rlc(m_regL);
			return 8;
		
		case 0x06: // rlc_xhl  
			MemWriteByte(m_regHL, Rlc(MemReadByte(m_regHL)));
			return 15;
		
		case 0x07: // rlc_a  
			m_regA = Rlc(m_regA);
			return 8;

		case 0x08: // rrc_b  
			m_regB = Rrc(m_regB);
			return 8;
		
		case 0x09: // rrc_c  
			m_regC = Rrc(m_regC);
			return 8;
		
		case 0x0A: // rrc_d  
			m_regD = Rrc(m_regD);
			return 8;
		
		case 0x0B: // rrc_e  
			m_regE = Rrc(m_regE);
			return 8;
		
		case 0x0C: // rrc_h  
			m_regH = Rrc(m_regH);
			return 8;
		
		case 0x0D: // rrc_l  
			m_regL = Rrc(m_regL);
			return 8;
		
		case 0x0E: // rrc_xhl  
			MemWriteByte(m_regHL, Rrc(MemReadByte(m_regHL)));
			return 15;
		
		case 0x0F: // rrc_a  
			m_regA = Rrc(m_regA);
			return 8;

		case 0x10: // rl_b   
			m_regB = Rl(m_regB);
			return 8;
		
		case 0x11: // rl_c   
			m_regC = Rl(m_regC);
			return 8;
		
		case 0x12: // rl_d   
			m_regD = Rl(m_regD);
			return 8;
		
		case 0x13: // rl_e   
			m_regE = Rl(m_regE);
			return 8;
		
		case 0x14: // rl_h   
			m_regH = Rl(m_regH);
			return 8;
		
		case 0x15: // rl_l   
			m_regL = Rl(m_regL);
			return 8;
		
		case 0x16: // rl_xhl   
			MemWriteByte(m_regHL, Rl(MemReadByte(m_regHL)));
			return 15;
		
		case 0x17: // rl_a   
			m_regA = Rl(m_regA);
			return 8;

		case 0x18: // rr_b   
			m_regB = Rr(m_regB);
			return 8;
		
		case 0x19: // rr_c   
			m_regC = Rr(m_regC);
			return 8;
		
		case 0x1A: // rr_d   
			m_regD = Rr(m_regD);
			return 8;
		
		case 0x1B: // rr_e   
			m_regE = Rr(m_regE);
			return 8;
		
		case 0x1C: // rr_h   
			m_regH = Rr(m_regH);
			return 8;
		
		case 0x1D: // rr_l   
			m_regL = Rr(m_regL);
			return 8;
		
		case 0x1E: // rr_xhl   
			MemWriteByte(m_regHL, Rr(MemReadByte(m_regHL)));
			return 15;
		
		case 0x1F: // rr_a   
			m_regA = Rr(m_regA);
			return 8;

		case 0x20: // sla_b  
			m_regB = Sla(m_regB);
			return 8;
		
		case 0x21: // sla_c  
			m_regC = Sla(m_regC);
			return 8;
		
		case 0x22: // sla_d  
			m_regD = Sla(m_regD);
			return 8;
		
		case 0x23: // sla_e  
			m_regE = Sla(m_regE);
			return 8;
		
		case 0x24: // sla_h  
			m_regH = Sla(m_regH);
			return 8;
		
		case 0x25: // sla_l  
			m_regL = Sla(m_regL);
			return 8;
		
		case 0x26: // sla_xhl  
			MemWriteByte(m_regHL, Sla(MemReadByte(m_regHL)));
			return 15;
		
		case 0x27: // sla_a  
			m_regA = Sla(m_regA);
			return 8;

		case 0x28: // sra_b  
			m_regB = Sra(m_regB);
			return 8;
		
		case 0x29: // sra_c  
			m_regC = Sra(m_regC);
			return 8;
		
		case 0x2A: // sra_d  
			m_regD = Sra(m_regD);
			return 8;
		
		case 0x2B: // sra_e  
			m_regE = Sra(m_regE);
			return 8;
		
		case 0x2C: // sra_h  
			m_regH = Sra(m_regH);
			return 8;
		
		case 0x2D: // sra_l  
			m_regL = Sra(m_regL);
			return 8;
		
		case 0x2E: // sra_xhl  
			MemWriteByte(m_regHL, Sra(MemReadByte(m_regHL)));
			return 15;
		
		case 0x2F: // sra_a  
			m_regA = Sra(m_regA);
			return 8;

		case 0x30: // sll_b  
			m_regB = Sll(m_regB);
			return 8;
		
		case 0x31: // sll_c  
			m_regC = Sll(m_regC);
			return 8;
		
		case 0x32: // sll_d  
			m_regD = Sll(m_regD);
			return 8;
		
		case 0x33: // sll_e  
			m_regE = Sll(m_regE);
			return 8;
		
		case 0x34: // sll_h  
			m_regH = Sll(m_regH);
			return 8;
		
		case 0x35: // sll_l  
			m_regL = Sll(m_regL);
			return 8;
		
		case 0x36: // sll_xhl  
			MemWriteByte(m_regHL, Sll(MemReadByte(m_regHL)));
			return 15;
		
		case 0x37: // sll_a  
			m_regA = Sll(m_regA);
			return 8;

		case 0x38: // srl_b  
			m_regB = Srl(m_regB);
			return 8;
		
		case 0x39: // srl_c  
			m_regC = Srl(m_regC);
			return 8;
		
		case 0x3A: // srl_d  
			m_regD = Srl(m_regD);
			return 8;
		
		case 0x3B: // srl_e  
			m_regE = Srl(m_regE);
			return 8;
		
		case 0x3C: // srl_h  
			m_regH = Srl(m_regH);
			return 8;
		
		case 0x3D: // srl_l  
			m_regL = Srl(m_regL);
			return 8;
		
		case 0x3E: // srl_xhl  
			MemWriteByte(m_regHL, Srl(MemReadByte(m_regHL)));
			return 15;
		
		case 0x3F: // srl_a  
			m_regA = Srl(m_regA);
			return 8;

		case 0x40: // bit_0_b
			Bit(m_regB, 0);
			return 8;
		
		case 0x41: // bit_0_c
			Bit(m_regC, 0);
			return 8;
		
		case 0x42: // bit_0_d
			Bit(m_regD, 0);
			return 8;
		
		case 0x43: // bit_0_e
			Bit(m_regE, 0);
			return 8;
		
		case 0x44: // bit_0_h
			Bit(m_regH, 0);
			return 8;
		
		case 0x45: // bit_0_l
			Bit(m_regL, 0);
			return 8;
		
		case 0x46: // bit_0_xhl
			Bit(MemReadByte(m_regHL), 0);
			return 12;
		
		case 0x47: // bit_0_a
			Bit(m_regA, 0);
			return 8;

		case 0x48: // bit_1_b
			Bit(m_regB, 1);
			return 8;
		
		case 0x49: // bit_1_c
			Bit(m_regC, 1);
			return 8;
		
		case 0x4A: // bit_1_d
			Bit(m_regD, 1);
			return 8;
		
		case 0x4B: // bit_1_e
			Bit(m_regE, 1);
			return 8;
		
		case 0x4C: // bit_1_h
			Bit(m_regH, 1);
			return 8;
		
		case 0x4D: // bit_1_l
			Bit(m_regL, 1);
			return 8;
		
		case 0x4E: // bit_1_xhl
			Bit(MemReadByte(m_regHL), 1);
			return 12;
		
		case 0x4F: // bit_1_a
			Bit(m_regA, 1);
			return 8;

		case 0x50: // bit_2_b
			Bit(m_regB, 2);
			return 8;
		
		case 0x51: // bit_2_c
			Bit(m_regC, 2);
			return 8;
		
		case 0x52: // bit_2_d
			Bit(m_regD, 2);
			return 8;
		
		case 0x53: // bit_2_e
			Bit(m_regE, 2);
			return 8;
		
		case 0x54: // bit_2_h
			Bit(m_regH, 2);
			return 8;
		
		case 0x55: // bit_2_l
			Bit(m_regL, 2);
			return 8;
		
		case 0x56: // bit_2_xhl
			Bit(MemReadByte(m_regHL), 2);
			return 12;
		
		case 0x57: // bit_2_a
			Bit(m_regA, 2);
			return 8;

		case 0x58: // bit_3_b
			Bit(m_regB, 3);
			return 8;
		
		case 0x59: // bit_3_c
			Bit(m_regC, 3);
			return 8;
		
		case 0x5A: // bit_3_d
			Bit(m_regD, 3);
			return 8;
		
		case 0x5B: // bit_3_e
			Bit(m_regE, 3);
			return 8;
		
		case 0x5C: // bit_3_h
			Bit(m_regH, 3);
			return 8;
		
		case 0x5D: // bit_3_l
			Bit(m_regL, 3);
			return 8;
		
		case 0x5E: // bit_3_xhl
			Bit(MemReadByte(m_regHL), 3);
			return 12;
		
		case 0x5F: // bit_3_a
			Bit(m_regA, 3);
			return 8;

		case 0x60: // bit_4_b
			Bit(m_regB, 4);
			return 8;
		
		case 0x61: // bit_4_c
			Bit(m_regC, 4);
			return 8;
		
		case 0x62: // bit_4_d
			Bit(m_regD, 4);
			return 8;
		
		case 0x63: // bit_4_e
			Bit(m_regE, 4);
			return 8;
		
		case 0x64: // bit_4_h
			Bit(m_regH, 4);
			return 8;
		
		case 0x65: // bit_4_l
			Bit(m_regL, 4);
			return 8;
		
		case 0x66: // bit_4_xhl
			Bit(MemReadByte(m_regHL), 4);
			return 12;
		
		case 0x67: // bit_4_a
			Bit(m_regA, 4);
			return 8;

		case 0x68: // bit_5_b
			Bit(m_regB, 5);
			return 8;
		
		case 0x69: // bit_5_c
			Bit(m_regC, 5);
			return 8;
		
		case 0x6A: // bit_5_d
			Bit(m_regD, 5);
			return 8;
		
		case 0x6B: // bit_5_e
			Bit(m_regE, 5);
			return 8;
		
		case 0x6C: // bit_5_h
			Bit(m_regH, 5);
			return 8;
		
		case 0x6D: // bit_5_l
			Bit(m_regL, 5);
			return 8;
		
		case 0x6E: // bit_5_xhl
			Bit(MemReadByte(m_regHL), 5);
			return 12;
		
		case 0x6F: // bit_5_a
			Bit(m_regA, 5);
			return 8;

		case 0x70: // bit_6_b
			Bit(m_regB, 6);
			return 8;
		
		case 0x71: // bit_6_c
			Bit(m_regC, 6);
			return 8;
		
		case 0x72: // bit_6_d
			Bit(m_regD, 6);
			return 8;
		
		case 0x73: // bit_6_e
			Bit(m_regE, 6);
			return 8;
		
		case 0x74: // bit_6_h
			Bit(m_regH, 6);
			return 8;
		
		case 0x75: // bit_6_l
			Bit(m_regL, 6);
			return 8;
		
		case 0x76: // bit_6_xhl
			Bit(MemReadByte(m_regHL), 6);
			return 12;
		
		case 0x77: // bit_6_a
			Bit(m_regA, 6);
			return 8;

		case 0x78: // bit_7_b
			Bit(m_regB, 7);
			return 8;
		
		case 0x79: // bit_7_c
			Bit(m_regC, 7);
			return 8;
		
		case 0x7A: // bit_7_d
			Bit(m_regD, 7);
			return 8;
		
		case 0x7B: // bit_7_e
			Bit(m_regE, 7);
			return 8;
		
		case 0x7C: // bit_7_h
			Bit(m_regH, 7);
			return 8;
		
		case 0x7D: // bit_7_l
			Bit(m_regL, 7);
			return 8;
		
		case 0x7E: // bit_7_xhl
			Bit(MemReadByte(m_regHL), 7);
			return 12;
		
		case 0x7F: // bit_7_a
			Bit(m_regA, 7);
			return 8;

		case 0x80: // res_0_b
			m_regB = Res(m_regB, 0);
			return 8;
		
		case 0x81: // res_0_c
			m_regC = Res(m_regC, 0);
			return 8;
		
		case 0x82: // res_0_d
			m_regD = Res(m_regD, 0);
			return 8;
		
		case 0x83: // res_0_e
			m_regE = Res(m_regE, 0);
			return 8;
		
		case 0x84: // res_0_h
			m_regH = Res(m_regH, 0);
			return 8;
		
		case 0x85: // res_0_l
			m_regL = Res(m_regL, 0);
			return 8;
		
		case 0x86: // res_0_xhl
			MemWriteByte(m_regHL, Res(MemReadByte(m_regHL), 0));
			return 15;
		
		case 0x87: // res_0_a
			m_regA = Res(m_regA, 0);
			return 8;

		case 0x88: // res_1_b
			m_regB = Res(m_regB ,1);
			return 8;
		
		case 0x89: // res_1_c
			m_regC = Res(m_regC, 1);
			return 8;
		
		case 0x8A: // res_1_d
			m_regD = Res(m_regD, 1);
			return 8;
		
		case 0x8B: // res_1_e
			m_regE = Res(m_regE, 1);
			return 8;
		
		case 0x8C: // res_1_h
			m_regH = Res(m_regH, 1);
			return 8;
		
		case 0x8D: // res_1_l
			m_regL = Res(m_regL, 1);
			return 8;
		
		case 0x8E: // res_1_xhl
			MemWriteByte(m_regHL, Res(MemReadByte(m_regHL), 1));
			return 15;
		
		case 0x8F: // res_1_a
			m_regA = Res(m_regA, 1);
			return 8;

		case 0x90: // res_2_b
			m_regB = Res(m_regB ,2);
			return 8;
		
		case 0x91: // res_2_c
			m_regC = Res(m_regC, 2);
			return 8;
		
		case 0x92: // res_2_d
			m_regD = Res(m_regD, 2);
			return 8;
		
		case 0x93: // res_2_e
			m_regE = Res(m_regE, 2);
			return 8;
		
		case 0x94: // res_2_h
			m_regH = Res(m_regH, 2);
			return 8;
		
		case 0x95: // res_2_l
			m_regL = Res(m_regL, 2);
			return 8;
		
		case 0x96: // res_2_xhl
			MemWriteByte(m_regHL, Res(MemReadByte(m_regHL), 2));
			return 15;
		
		case 0x97: // res_2_a
			m_regA = Res(m_regA, 2);
			return 8;

		case 0x98: // res_3_b
			m_regB = Res(m_regB ,3);
			return 8;
		
		case 0x99: // res_3_c
			m_regC = Res(m_regC, 3);
			return 8;
		
		case 0x9A: // res_3_d
			m_regD = Res(m_regD, 3);
			return 8;
		
		case 0x9B: // res_3_e
			m_regE = Res(m_regE, 3);
			return 8;
		
		case 0x9C: // res_3_h
			m_regH = Res(m_regH, 3);
			return 8;
		
		case 0x9D: // res_3_l
			m_regL = Res(m_regL, 3);
			return 8;
		
		case 0x9E: // res_3_xhl
			MemWriteByte(m_regHL, Res(MemReadByte(m_regHL), 3));
			return 15;
		
		case 0x9F: // res_3_a
			m_regA = Res(m_regA, 3);
			return 8;

		case 0xA0: // res_4_b
			m_regB = Res(m_regB ,4);
			return 8;
		
		case 0xA1: // res_4_c
			m_regC = Res(m_regC, 4);
			return 8;
		
		case 0xA2: // res_4_d
			m_regD = Res(m_regD, 4);
			return 8;
		
		case 0xA3: // res_4_e
			m_regE = Res(m_regE, 4);
			return 8;
		
		case 0xA4: // res_4_h
			m_regH = Res(m_regH, 4);
			return 8;
		
		case 0xA5: // res_4_l
			m_regL = Res(m_regL, 4);
			return 8;
		
		case 0xA6: // res_4_xhl
			MemWriteByte(m_regHL, Res(MemReadByte(m_regHL), 4));
			return 15;
		
		case 0xA7: // res_4_a
			m_regA = Res(m_regA, 4);
			return 8;

		case 0xA8: // res_5_b
			m_regB = Res(m_regB ,5);
			return 8;
		
		case 0xA9: // res_5_c
			m_regC = Res(m_regC, 5);
			return 8;
		
		case 0xAA: // res_5_d
			m_regD = Res(m_regD, 5);
			return 8;
		
		case 0xAB: // res_5_e
			m_regE = Res(m_regE, 5);
			return 8;
		
		case 0xAC: // res_5_h
			m_regH = Res(m_regH, 5);
			return 8;
		
		case 0xAD: // res_5_l
			m_regL = Res(m_regL, 5);
			return 8;
		
		case 0xAE: // res_5_xhl
			MemWriteByte(m_regHL, Res(MemReadByte(m_regHL), 5));
			return 15;
		
		case 0xAF: // res_5_a
			m_regA = Res(m_regA, 5);
			return 8;

		case 0xB0: // res_6_b
			m_regB = Res(m_regB ,6);
			return 8;
		
		case 0xB1: // res_6_c
			m_regC = Res(m_regC, 6);
			return 8;
		
		case 0xB2: // res_6_d
			m_regD = Res(m_regD, 6);
			return 8;
		
		case 0xB3: // res_6_e
			m_regE = Res(m_regE, 6);
			return 8;
		
		case 0xB4: // res_6_h
			m_regH = Res(m_regH, 6);
			return 8;
		
		case 0xB5: // res_6_l
			m_regL = Res(m_regL, 6);
			return 8;
		
		case 0xB6: // res_6_xhl
			MemWriteByte(m_regHL, Res(MemReadByte(m_regHL), 6));
			return 15;
		
		case 0xB7: // res_6_a
			m_regA = Res(m_regA, 6);
			return 8;

		case 0xB8: // res_7_b
			m_regB = Res(m_regB ,7);
			return 8;
		
		case 0xB9: // res_7_c
			m_regC = Res(m_regC, 7);
			return 8;
		
		case 0xBA: // res_7_d
			m_regD = Res(m_regD, 7);
			return 8;
		
		case 0xBB: // res_7_e
			m_regE = Res(m_regE, 7);
			return 8;
		
		case 0xBC: // res_7_h
			m_regH = Res(m_regH, 7);
			return 8;
		
		case 0xBD: // res_7_l
			m_regL = Res(m_regL, 7);
			return 8;
		
		case 0xBE: // res_7_xhl
			MemWriteByte(m_regHL, Res(MemReadByte(m_regHL), 7));
			return 15;
		
		case 0xBF: // res_7_a
			m_regA = Res(m_regA, 7);
			return 8;

		case 0xC0: // set_0_b
			m_regB = Set(m_regB ,0);
			return 8;
		
		case 0xC1: // set_0_c
			m_regC = Set(m_regC, 0);
			return 8;
		
		case 0xC2: // set_0_d
			m_regD = Set(m_regD, 0);
			return 8;
		
		case 0xC3: // set_0_e
			m_regE = Set(m_regE, 0);
			return 8;
		
		case 0xC4: // set_0_h
			m_regH = Set(m_regH, 0);
			return 8;
		
		case 0xC5: // set_0_l
			m_regL = Set(m_regL, 0);
			return 8;
		
		case 0xC6: // set_0_xhl
			MemWriteByte(m_regHL, Set(MemReadByte(m_regHL), 0));
			return 15;
		
		case 0xC7: // set_0_a
			m_regA = Set(m_regA, 0);
			return 8;

		case 0xC8: // set_1_b
			m_regB = Set(m_regB ,1);
			return 8;
		
		case 0xC9: // set_1_c
			m_regC = Set(m_regC, 1);
			return 8;
		
		case 0xCA: // set_1_d
			m_regD = Set(m_regD, 1);
			return 8;
		
		case 0xCB: // set_1_e
			m_regE = Set(m_regE, 1);
			return 8;
		
		case 0xCC: // set_1_h
			m_regH = Set(m_regH, 1);
			return 8;
		
		case 0xCD: // set_1_l
			m_regL = Set(m_regL, 1);
			return 8;
		
		case 0xCE: // set_1_xhl
			MemWriteByte(m_regHL, Set(MemReadByte(m_regHL), 1));
			return 15;
		
		case 0xCF: // set_1_a
			m_regA = Set(m_regA, 1);
			return 8;

		case 0xD0: // set_2_b
			m_regB = Set(m_regB ,2);
			return 8;
		
		case 0xD1: // set_2_c
			m_regC = Set(m_regC, 2);
			return 8;
		
		case 0xD2: // set_2_d
			m_regD = Set(m_regD, 2);
			return 8;
		
		case 0xD3: // set_2_e
			m_regE = Set(m_regE, 2);
			return 8;
		
		case 0xD4: // set_2_h
			m_regH = Set(m_regH, 2);
			return 8;
		
		case 0xD5: // set_2_l
			m_regL = Set(m_regL, 2);
			return 8;
		
		case 0xD6: // set_2_xhl
			MemWriteByte(m_regHL, Set(MemReadByte(m_regHL), 2));
			return 15;
		
		case 0xD7: // set_2_a
			m_regA = Set(m_regA, 2);
			return 8;

		case 0xD8: // set_3_b
			m_regB = Set(m_regB ,3);
			return 8;
		
		case 0xD9: // set_3_c
			m_regC = Set(m_regC, 3);
			return 8;
		
		case 0xDA: // set_3_d
			m_regD = Set(m_regD, 3);
			return 8;
		
		case 0xDB: // set_3_e
			m_regE = Set(m_regE, 3);
			return 8;
		
		case 0xDC: // set_3_h
			m_regH = Set(m_regH, 3);
			return 8;
		
		case 0xDD: // set_3_l
			m_regL = Set(m_regL, 3);
			return 8;
		
		case 0xDE: // set_3_xhl
			MemWriteByte(m_regHL, Set(MemReadByte(m_regHL), 3));
			return 15;
		
		case 0xDF: // set_3_a
			m_regA = Set(m_regA, 3);
			return 8;

		case 0xE0: // set_4_b
			m_regB = Set(m_regB ,4);
			return 8;
		
		case 0xE1: // set_4_c
			m_regC = Set(m_regC, 4);
			return 8;
		
		case 0xE2: // set_4_d
			m_regD = Set(m_regD, 4);
			return 8;
		
		case 0xE3: // set_4_e
			m_regE = Set(m_regE, 4);
			return 8;
		
		case 0xE4: // set_4_h
			m_regH = Set(m_regH, 4);
			return 8;
		
		case 0xE5: // set_4_l
			m_regL = Set(m_regL, 4);
			return 8;
		
		case 0xE6: // set_4_xhl
			MemWriteByte(m_regHL, Set(MemReadByte(m_regHL), 4));
			return 15;
		
		case 0xE7: // set_4_a
			m_regA = Set(m_regA, 4);
			return 8;

		case 0xE8: // set_5_b
			m_regB = Set(m_regB ,5);
			return 8;
		
		case 0xE9: // set_5_c
			m_regC = Set(m_regC, 5);
			return 8;
		
		case 0xEA: // set_5_d
			m_regD = Set(m_regD, 5);
			return 8;
		
		case 0xEB: // set_5_e
			m_regE = Set(m_regE, 5);
			return 8;
		
		case 0xEC: // set_5_h
			m_regH = Set(m_regH, 5);
			return 8;
		
		case 0xED: // set_5_l
			m_regL = Set(m_regL, 5);
			return 8;
		
		case 0xEE: // set_5_xhl
			MemWriteByte(m_regHL, Set(MemReadByte(m_regHL), 5));
			return 15;
		
		case 0xEF: // set_5_a
			m_regA = Set(m_regA, 5);
			return 8;

		case 0xF0: // set_6_b
			m_regB = Set(m_regB ,6);
			return 8;
		
		case 0xF1: // set_6_c
			m_regC = Set(m_regC, 6);
			return 8;
		
		case 0xF2: // set_6_d
			m_regD = Set(m_regD, 6);
			return 8;
		
		case 0xF3: // set_6_e
			m_regE = Set(m_regE, 6);
			return 8;
		
		case 0xF4: // set_6_h
			m_regH = Set(m_regH, 6);
			return 8;
		
		case 0xF5: // set_6_l
			m_regL = Set(m_regL, 6);
			return 8;
		
		case 0xF6: // set_6_xhl
			MemWriteByte(m_regHL, Set(MemReadByte(m_regHL), 6));
			return 15;
		
		case 0xF7: // set_6_a
			m_regA = Set(m_regA, 6);
			return 8;

		case 0xF8: // set_7_b
			m_regB = Set(m_regB ,7);
			return 8;
		
		case 0xF9: // set_7_c
			m_regC = Set(m_regC, 7);
			return 8;
		
		case 0xFA: // set_7_d
			m_regD = Set(m_regD, 7);
			return 8;
		
		case 0xFB: // set_7_e
			m_regE = Set(m_regE, 7);
			return 8;
		
		case 0xFC: // set_7_h
			m_regH = Set(m_regH, 7);
			return 8;
		
		case 0xFD: // set_7_l
			m_regL = Set(m_regL, 7);
			return 8;
		
		case 0xFE: // set_7_xhl
			MemWriteByte(m_regHL, Set(MemReadByte(m_regHL), 7));
			return 15;
		
		case 0xFF: // set_7_a
			m_regA = Set(m_regA, 7);
			return 8;

		default:
			_ASSERT(FALSE);
			return 0;
	}
}

void Exx() {
	swap(m_regBC, m_regBC2);
	swap(m_regDE, m_regDE2);
	swap(m_regHL, m_regHL2);
}

/* ***************************************************************************
 * DD prefixed opcodes
 * ***************************************************************************
 */
/* inline */ WORD IndirectIX() {
	return m_regIX + (signed char)ImmedByte();
}

int HandleDD() {
	WORD wAddr;
	WORD wTemp;

	const BYTE bOpcode = ImmedByte();

	m_regR++;

	switch (bOpcode)
	{
		case 0x00: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x01: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x02: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x03: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x04: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x05: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x06: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x07: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x08: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x09: // add_ix_bc 
			m_regIX = Add_2(m_regIX, m_regBC);
			return 15;
		
		case 0x0A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0C: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0D: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0E: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x10: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x11: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x12: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x13: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x14: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x15: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x16: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x17: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x18: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x19: // add_ix_de 
			m_regIX = Add_2(m_regIX, m_regDE);
			return 15;
		
		case 0x1A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1C: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1D: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1E: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x20: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x21: // ld_ix_word
			m_regIX = ImmedWord();
			return 14;
		
		case 0x22: // ld_xword_ix
			MemWriteWord(ImmedWord(), m_regIX);
			return 20;
		
		case 0x23: // inc_ix
			m_regIX++;
			return 10;
		
		case 0x24: // inc_ixh  
			m_regIXh = Inc(m_regIXh);
			return 9;
		
		case 0x25: // dec_ixh  
			m_regIXh = Dec(m_regIXh);
			return 9;
		
		case 0x26: // ld_ixh_byte
			m_regIXh = ImmedByte();
			return 9;
		
		case 0x27: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x28: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x29: // add_ix_ix 
			m_regIX = Add_2(m_regIX, m_regIX);
			return 15;
		
		case 0x2A: // ld_ix_xword
			m_regIX = MemReadWord(ImmedWord());
			return 20;
		
		case 0x2B: // dec_ix   
			m_regIX--;
			return 10;
		
		case 0x2C: // inc_ixl  
			m_regIXl = Inc(m_regIXl);
			return 9;
		
		case 0x2D: // dec_ixl  
			m_regIXl = Dec(m_regIXl);
			return 9;
		
		case 0x2E: // ld_ixl_byte
			m_regIXl = ImmedByte();
			return 9;
		
		case 0x2F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x30: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x31: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x32: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x33: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x34: // inc_xix  
			wAddr = IndirectIX();
			MemWriteByte(wAddr, Inc(MemReadByte(wAddr)));
			return 23;
		
		case 0x35: // dec_xix  
			wAddr = IndirectIX();
			MemWriteByte(wAddr, Dec(MemReadByte(wAddr)));
			return 23;
		
		case 0x36: // ld_xix_byte
			wAddr = IndirectIX();
			MemWriteByte(wAddr, ImmedByte());
			return 19;
		
		case 0x37: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x38: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x39: // add_ix_sp 
			m_regIX = Add_2(m_regIX, GetSP());
			return 15;
		
		case 0x3A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3C: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3D: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3E: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x40: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x41: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x42: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x43: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x44: // ld_b_ixh 
			m_regB = m_regIXh;
			return 9;
		
		case 0x45: // ld_b_ixl 
			m_regB = m_regIXl;
			return 9;
		
		case 0x46: // ld_b_xix   
			m_regB = MemReadByte(IndirectIX());
			return 19;
		
		case 0x47: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x48: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x49: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x4A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x4B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x4C: // ld_c_ixh 
			m_regC = m_regIXh;
			return 9;
		
		case 0x4D: // ld_c_ixl 
			m_regC = m_regIXl;
			return 9;
		
		case 0x4E: // ld_c_xix   
			m_regC = MemReadByte(IndirectIX());
			return 19;
		
		case 0x4F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x50: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x51: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x52: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x53: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x54: // ld_d_ixh 
			m_regD = m_regIXh;
			return 9;
		
		case 0x55: // ld_d_ixl 
			m_regD = m_regIXl;
			return 9;
		
		case 0x56: // ld_d_xix   
			m_regD = MemReadByte(IndirectIX());
			return 19;
		
		case 0x57: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x58: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x59: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x5A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x5B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x5C: // ld_e_ixh 
			m_regE = m_regIXh;
			return 9;
		
		case 0x5D: // ld_e_ixl 
			m_regE = m_regIXl;
			return 9;
		
		case 0x5E: // ld_e_xix   
			m_regE = MemReadByte(IndirectIX());
			return 19;
		
		case 0x5F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x60: // ld_ixh_b
			m_regIXh = m_regB;
			return 9;
		
		case 0x61: // ld_ixh_c  
			m_regIXh = m_regC;
			return 9;
		
		case 0x62: // ld_ixh_d   
			m_regIXh = m_regD;
			return 9;
		
		case 0x63: // ld_ixh_e 
			m_regIXh = m_regE;
			return 9;
		
		case 0x64: // ld_ixh_h 
			m_regIXh = m_regH;
			return 9;
		
		case 0x65: // ld_ixh_l 
			m_regIXh = m_regL;
			return 9;
		
		case 0x66: // ld_h_xix   
			m_regH = MemReadByte(IndirectIX());
			return 9;
		
		case 0x67: // ld_ixh_a
			m_regIXh = m_regA;
			return 9;

		case 0x68: // ld_ixl_b
			m_regIXl = m_regB;
			return 9;
		
		case 0x69: // ld_ixl_c  
			m_regIXl = m_regC;
			return 9;
		
		case 0x6A: // ld_ixl_d   
			m_regIXl = m_regD;
			return 9;
		
		case 0x6B: // ld_ixl_e 
			m_regIXl = m_regE;
			return 9;
		
		case 0x6C: // ld_ixl_h 
			m_regIXl = m_regH;
			return 9;
		
		case 0x6D: // ld_ixl_l 
			m_regIXl = m_regL;
			return 9;
		
		case 0x6E: // ld_l_xix   
			m_regL = MemReadByte(IndirectIX());
			return 9;
		
		case 0x6F: // ld_ixl_a
			m_regIXl = m_regA;
			return 9;

		case 0x70: // ld_xix_b
			MemWriteByte(IndirectIX(), m_regB);
			return 19;
		
		case 0x71: // ld_xix_c  
			MemWriteByte(IndirectIX(), m_regC);
			return 19;
		
		case 0x72: // ld_xix_d   
			MemWriteByte(IndirectIX(), m_regD);
			return 19;
		
		case 0x73: // ld_xix_e 
			MemWriteByte(IndirectIX(), m_regE);
			return 19;
		
		case 0x74: // ld_xix_h 
			MemWriteByte(IndirectIX(), m_regH);
			return 19;
		
		case 0x75: // ld_xix_l 
			MemWriteByte(IndirectIX(), m_regL);
			return 19;
		
		case 0x76: // no_op      
			SetPC(GetPC() - 1);
			return 19;
		
		case 0x77: // ld_xix_a
			MemWriteByte(IndirectIX(), m_regA);
			return 19;

		case 0x78: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x79: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x7A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x7B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x7C: // ld_a_ixh 
			m_regA = m_regIXh;
			return 9;
		
		case 0x7D: // ld_a_ixl 
			m_regA = m_regIXl;
			return 9;
		
		case 0x7E: // ld_a_xix   
			m_regA = MemReadByte(IndirectIX());
			return 19;
		
		case 0x7F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x80: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x81: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x82: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x83: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x84: // add_a_ixh
			Add_1(m_regIXh);
			return 9;
		
		case 0x85: // add_a_ixl
			Add_1(m_regIXl);
			return 9;
		
		case 0x86: // add_a_xix  
			Add_1(MemReadByte(IndirectIX()));
			return 19;
		
		case 0x87: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x88: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x89: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x8A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x8B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x8C: // adc_a_ixh
			Adc_1(m_regIXh);
			return 9;
		
		case 0x8D: // adc_a_ixl
			Adc_1(m_regIXl);
			return 9;
		
		case 0x8E: // adc_a_xix  
			Adc_1(MemReadByte(IndirectIX()));
			return 19;
		
		case 0x8F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x90: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x91: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x92: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x93: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x94: // sub_ixh  
			Sub_1(m_regIXh);
			return 9;
		
		case 0x95: // sub_ixl  
			Sub_1(m_regIXl);
			return 9;
		
		case 0x96: // sub_xix    
			Sub_1(MemReadByte(IndirectIX()));
			return 19;
		
		case 0x97: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x98: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x99: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x9A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x9B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x9C: // sbc_a_ixh
			Sbc_1(m_regIXh);
			return 9;
		
		case 0x9D: // sbc_a_ixl
			Sbc_1(m_regIXl);
			return 9;
		
		case 0x9E: // sbc_a_xix  
			Sbc_1(MemReadByte(IndirectIX()));
			return 19;
		
		case 0x9F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xA0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA4: // and_ixh  
			And(m_regIXh);
			return 9;
		
		case 0xA5: // and_ixl  
			And(m_regIXl);
			return 9;
		
		case 0xA6: // and_xix    
			And(MemReadByte(IndirectIX()));
			return 19;
		
		case 0xA7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xA8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA9: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xAA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xAB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xAC: // xor_ixh  
			Xor(m_regIXh);
			return 9;
		
		case 0xAD: // xor_ixl  
			Xor(m_regIXl);
			return 9;
		
		case 0xAE: // xor_xix    
			Xor(MemReadByte(IndirectIX()));
			return 19;
		
		case 0xAF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xB0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB4: // or_ixh   
			Or(m_regIXh);
			return 9;
		
		case 0xB5: // or_ixl   
			Or(m_regIXl);
			return 9;
		
		case 0xB6: // or_xix     
			Or(MemReadByte(IndirectIX()));
			return 19;
		
		case 0xB7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xB8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB9: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xBA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xBB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xBC: // cp_ixh   
			Cp(m_regIXh);
			return 9;
		
		case 0xBD: // cp_ixl   
			Cp(m_regIXl);
			return 9;
		
		case 0xBE: // cp_xix     
			Cp(MemReadByte(IndirectIX()));
			return 19;
		
		case 0xBF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xC0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC4: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC5: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC6: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xC8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC9: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCB: // dd_cb    
			return HandleDDCB();
			break;
		
		case 0xCC: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCD: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCE: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xD0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD4: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD5: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD6: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xD8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD9: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDC: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDD: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDE: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xE0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE1: // pop_ix    
			m_regIX = Pop();
			return 14;
		
		case 0xE2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE3: // ex_xsp_ix
			wTemp = MemReadWord(GetSP());
			MemWriteWord(GetSP(), m_regIX);
			m_regIX = wTemp;
			return 23;
		
		case 0xE4: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE5: // push_ix  
			Push(m_regIX);
			return 15;
		
		case 0xE6: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xE8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE9: // jp_ix     
			SetPC(m_regIX);
			return 8;
		
		case 0xEA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xEB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xEC: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xED: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xEE: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xEF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xF0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF4: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF5: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF6: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xF8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF9: // ld_sp_ix  
			SetSP(m_regIX);
			return 10;
		
		case 0xFA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFC: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFD: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFE: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFF: // no_op
			SetPC(GetPC() - 1);
			return 0;

		default:
			return 0;
	}
}

/* ***************************************************************************
 * ED prefixed opcodes
 * ***************************************************************************
 */
int HandleED(int cCyclesArg) {

	const BYTE bOpcode = ImmedByte();
	int cCycles = cCyclesArg;

	m_regR++;

	switch (bOpcode)
	{
		case 0x00: // nop   
			cCycles -= 0;
			break;
		
		case 0x01: // nop    
			cCycles -= 0;
			break;
			
		case 0x02: // nop      
			cCycles -= 0;
			break;
		
		case 0x03: // nop        
			cCycles -= 0;
			break;
		
		case 0x04: // nop
			cCycles -= 0;
			break;
		
		case 0x05: // nop 
			cCycles -= 0;
			break;
		
		case 0x06: // nop  
			cCycles -= 0;
			break;
		
		case 0x07: // nop   
			cCycles -= 0;
			break;

		case 0x08: // nop   
			cCycles -= 0;
			break;
		
		case 0x09: // nop    
			cCycles -= 0;
			break;
		
		case 0x0A: // nop      
			cCycles -= 0;
			break;
		
		case 0x0B: // nop        
			cCycles -= 0;
			break;
		
		case 0x0C: // nop
			cCycles -= 0;
			break;
		
		case 0x0D: // nop 
			cCycles -= 0;
			break;
		
		case 0x0E: // nop  
			cCycles -= 0;
			break;
		
		case 0x0F: // nop   
			cCycles -= 0;
			break;

		case 0x10: // nop   
			cCycles -= 0;
			break;
		
		case 0x11: // nop    
			cCycles -= 0;
			break;
			
		case 0x12: // nop      
			cCycles -= 0;
			break;
		
		case 0x13: // nop        
			cCycles -= 0;
			break;
		
		case 0x14: // nop
			cCycles -= 0;
			break;
		
		case 0x15: // nop 
			cCycles -= 0;
			break;
		
		case 0x16: // nop  
			cCycles -= 0;
			break;
		
		case 0x17: // nop   
			cCycles -= 0;
			break;

		case 0x18: // nop   
			cCycles -= 0;
			break;
		
		case 0x19: // nop    
			cCycles -= 0;
			break;
		
		case 0x1A: // nop      
			cCycles -= 0;
			break;
		
		case 0x1B: // nop        
			cCycles -= 0;
			break;
		
		case 0x1C: // nop
			cCycles -= 0;
			break;
		
		case 0x1D: // nop 
			cCycles -= 0;
			break;
		
		case 0x1E: // nop  
			cCycles -= 0;
			break;
		
		case 0x1F: // nop   
			cCycles -= 0;
			break;

		case 0x20: // nop   
			cCycles -= 0;
			break;
		
		case 0x21: // nop    
			cCycles -= 0;
			break;
			
		case 0x22: // nop      
			cCycles -= 0;
			break;
		
		case 0x23: // nop        
			cCycles -= 0;
			break;
		
		case 0x24: // nop
			cCycles -= 0;
			break;
		
		case 0x25: // nop 
			cCycles -= 0;
			break;
		
		case 0x26: // nop  
			cCycles -= 0;
			break;
		
		case 0x27: // nop   
			cCycles -= 0;
			break;

		case 0x28: // nop   
			cCycles -= 0;
			break;
		
		case 0x29: // nop    
			cCycles -= 0;
			break;
		
		case 0x2A: // nop      
			cCycles -= 0;
			break;
		
		case 0x2B: // nop        
			cCycles -= 0;
			break;
		
		case 0x2C: // nop
			cCycles -= 0;
			break;
		
		case 0x2D: // nop 
			cCycles -= 0;
			break;
		
		case 0x2E: // nop  
			cCycles -= 0;
			break;
		
		case 0x2F: // nop   
			cCycles -= 0;
			break;

		case 0x30: // nop   
			cCycles -= 0;
			break;
		
		case 0x31: // nop    
			cCycles -= 0;
			break;
			
		case 0x32: // nop      
			cCycles -= 0;
			break;
		
		case 0x33: // nop        
			cCycles -= 0;
			break;
		
		case 0x34: // nop
			cCycles -= 0;
			break;
		
		case 0x35: // nop 
			cCycles -= 0;
			break;
		
		case 0x36: // nop  
			cCycles -= 0;
			break;
		
		case 0x37: // nop   
			cCycles -= 0;
			break;

		case 0x38: // nop   
			cCycles -= 0;
			break;
		
		case 0x39: // nop    
			cCycles -= 0;
			break;
		
		case 0x3A: // nop      
			cCycles -= 0;
			break;
		
		case 0x3B: // nop        
			cCycles -= 0;
			break;
		
		case 0x3C: // nop
			cCycles -= 0;
			break;
		
		case 0x3D: // nop 
			cCycles -= 0;
			break;
		
		case 0x3E: // nop  
			cCycles -= 0;
			break;
		
		case 0x3F: // nop   
			cCycles -= 0;
			break;

		case 0x40: // in_b_c
			cCycles -= 12;
			m_regB = In(m_regC);
			break;
		
		case 0x41: // out_c_b
			cCycles -= 12;
			Out(m_regC, m_regB);
			break;
			
		case 0x42: // sbc_hl_bc
			cCycles -= 15;
			m_regHL = Sbc_2(m_regHL, m_regBC);			
			break;
		
		case 0x43: // ld_xword_bc
			cCycles -= 20;
			MemWriteWord(ImmedWord(), m_regBC);
			break;
		
		case 0x44: // neg
			cCycles -= 8;
		{
			BYTE b = m_regA;
			m_regA = 0;
			Sub_1(b);
			break;
		}
		
		case 0x45: // retn
			cCycles -= 8;
			m_iff1 = m_iff2;
			cCycles -= Ret0(TRUE);
			break;
		
		case 0x46: // im_0 
			cCycles -= 8;
			m_nIM = 0;
			break;
		
		case 0x47: // ld_i_a
			cCycles -= 9;
			m_regI = m_regA;
			break;

		case 0x48: // in_c_c
			cCycles -= 12;
			m_regC = In(m_regC);
			break;
		
		case 0x49: // out_c_c
			cCycles -= 12;
			Out(m_regC, m_regC);
			break;
		
		case 0x4A: // adc_hl_bc
			cCycles -= 15;
			m_regHL = Adc_2(m_regHL, m_regBC);
			break;
		
		case 0x4B: // ld_bc_xword
			cCycles -= 20;
			m_regBC = MemReadWord(ImmedWord());
			break;
		
		case 0x4C: // neg
			cCycles -= 8;
		{
			BYTE b = m_regA;
			m_regA = 0;
			Sub_1(b);
			break;
		}
		
		case 0x4D: // reti
			cCycles -= 8;
			cCycles -= Ret0(TRUE);
			break;
		
		case 0x4E: // im_0 
			cCycles -= 8;
			m_nIM = 0;
			break;
		
		case 0x4F: // ld_r_a
			cCycles -= 9;
			m_regR = m_regA;
//			m_regR2 = m_regA;
			break;

		case 0x50: // in_d_c
			cCycles -= 12;
			m_regD = In(m_regC);
			break;
		
		case 0x51: // out_c_d
			cCycles -= 12;
			Out(m_regC, m_regD);
			break;
			
		case 0x52: // sbc_hl_de
			cCycles -= 15;
			m_regHL = Sbc_2(m_regHL, m_regDE);
			break;
		
		case 0x53: // ld_xword_de
			cCycles -= 20;
			MemWriteWord(ImmedWord(), m_regDE);
			break;
		
		case 0x54: // neg
			cCycles -= 8;
		{
			BYTE b = m_regA;
			m_regA = 0;
			Sub_1(b);
			break;
		}
		
		case 0x55: // retn
			cCycles -= 8;
			m_iff1 = m_iff2;
			cCycles -= Ret0(TRUE);
			break;
		
		case 0x56: // im_1 
			cCycles -= 8;
			m_nIM = 1;
			break;
		
		case 0x57: // ld_a_i
			cCycles -= 9;
			m_regA = m_regI;
			m_regF = (m_regF & C_FLAG)| ZSTable[m_regI] | (m_iff2 << 2);
			break;

		case 0x58: // in_e_c
			cCycles -= 12;
			m_regE = In(m_regC);
			break;
		
		case 0x59: // out_c_e
			cCycles -= 12;
			Out(m_regC, m_regE);
			break;
		
		case 0x5A: // adc_hl_de
			cCycles -= 15;
			m_regHL = Adc_2(m_regHL, m_regDE);
			break;
		
		case 0x5B: // ld_de_xword
			cCycles -= 20;
			m_regDE = MemReadWord(ImmedWord());
			break;
		
		case 0x5C: // neg
			cCycles -= 8;
		{
			BYTE b = m_regA;
			m_regA = 0;
			Sub_1(b);
			break;
		}
		
		case 0x5D: // reti
			cCycles -= 8;
			cCycles -= Ret0(TRUE);
			break;
		
		case 0x5E: // im_2 
			cCycles -= 8;
			m_nIM = 2;
			break;
		
		case 0x5F: // ld_a_r
			cCycles -= 9;
			m_regA = (m_regR & 0x7F); /////////////////////////////////////// | (m_regR2 & 0x80);
			m_regF = (m_regF & C_FLAG) | ZSTable[m_regA] | (m_iff2 << 2);
			break;

		case 0x60: // in_h_c
			cCycles -= 12;
			m_regH = In(m_regC);
			break;
		
		case 0x61: // out_c_h
			cCycles -= 12;
			Out(m_regC, m_regH);
			break;
			
		case 0x62: // sbc_hl_hl
			cCycles -= 15;
			m_regHL = Sbc_2(m_regHL, m_regHL);
			break;
		
		case 0x63: // ld_xword_hl
			cCycles -= 20;
			MemWriteWord(ImmedWord(), m_regHL);
			break;
		
		case 0x64: // neg
			cCycles -= 8;
		{
			BYTE b = m_regA;
			m_regA = 0;
			Sub_1(b);
			break;
		}
		
		case 0x65: // retn
			cCycles -= 8;
			m_iff1 = m_iff2;
			cCycles -= Ret0(TRUE);
			break;
		
		case 0x66: // im_0 
			cCycles -= 8;
			m_nIM = 0;
			break;
		
		case 0x67: // rrd   
			cCycles -= 18;
		{
			BYTE i = MemReadByte(m_regHL);
			MemWriteByte(m_regHL, (i>>4) | (m_regA << 4));
			m_regA = (m_regA & 0xF0) | (i&0x0F);
			m_regF =(m_regF&C_FLAG)|ZSPTable[m_regA];
			break;
		}

		case 0x68: // in_l_c
			cCycles -= 12;
			m_regL = In(m_regC);
			break;
		
		case 0x69: // out_c_l
			cCycles -= 12;
			Out(m_regC, m_regL);
			break;
		
		case 0x6A: // adc_hl_hl
			cCycles -= 15;
			m_regHL = Adc_2(m_regHL, m_regHL);
			break;
		
		case 0x6B: // ld_hl_xword
			cCycles -= 20;
			m_regHL = MemReadWord(ImmedWord());			
			break;
		
		case 0x6C: // neg
			cCycles -= 8;
		{
			BYTE b = m_regA;
			m_regA = 0;
			Sub_1(b);
			break;
		}
		
		case 0x6D: // reti
			cCycles -= 8;
			cCycles -= Ret0(TRUE);
			break;
		
		case 0x6E: // im_0 
			cCycles -= 8;
			m_nIM = 0;
			break;
		
		case 0x6F: // rld   
			cCycles -= 18;
		{
			BYTE i;
			i=MemReadByte(m_regHL);
			MemWriteByte(m_regHL,(i<<4)|(m_regA&0x0F));
			m_regA=(m_regA&0xF0)|(i>>4);
			m_regF=(m_regF&C_FLAG)|ZSPTable[m_regA];
			break;
		}

		case 0x70: // in_0_c
			cCycles -= 12;
			In(m_regC);
			break;
		
		case 0x71: // out_c_0
			cCycles -= 12;
			Out(m_regC, 0);
			break;
			
		case 0x72: // sbc_hl_sp
			cCycles -= 15;
			m_regHL = Sbc_2(m_regHL, GetSP());
			break;
		
		case 0x73: // ld_xword_sp
			cCycles -= 20;
			MemWriteWord(ImmedWord(), GetSP());
			break;
		
		case 0x74: // neg
			cCycles -= 8;
		{
			BYTE b = m_regA;
			m_regA = 0;
			Sub_1(b);
			break;
		}
		
		case 0x75: // retn
			cCycles -= 8;
			m_iff1 = m_iff2;
			cCycles -= Ret0(TRUE);
			break;
		
		case 0x76: // im_1 
			cCycles -= 8;
			m_nIM = 1;
			break;
		
		case 0x77: // nop   
			cCycles -= 0;
			break;

		case 0x78: // in_a_c
			cCycles -= 12;
			m_regA = In(m_regC);
			break;
		
		case 0x79: // out_c_a
			cCycles -= 12;
			Out(m_regC, m_regA);
			break;
		
		case 0x7A: // adc_hl_sp
			cCycles -= 15;
			m_regHL = Adc_2(m_regHL, GetSP());
			break;
		
		case 0x7B: // ld_sp_xword
			cCycles -= 20;
			SetSP(MemReadWord(ImmedWord()));
			break;
		
		case 0x7C: // neg
			cCycles -= 8;
		{
			BYTE b = m_regA;
			m_regA = 0;
			Sub_1(b);
			break;
		}
		
		case 0x7D: // reti
			cCycles -= 8;
			cCycles -= Ret0(TRUE);
			break;
		
		case 0x7E: // im_2 
			cCycles -= 8;
			m_nIM = 2;
			break;
		
		case 0x7F: // nop   
			cCycles -= 0;
			break;

		case 0x80: // nop   
			cCycles -= 0;
			break;
		
		case 0x81: // nop    
			cCycles -= 0;
			break;
			
		case 0x82: // nop      
			cCycles -= 0;
			break;
		
		case 0x83: // nop        
			cCycles -= 0;
			break;
		
		case 0x84: // nop
			cCycles -= 0;
			break;
		
		case 0x85: // nop 
			cCycles -= 0;
			break;
		
		case 0x86: // nop  
			cCycles -= 0;
			break;
		
		case 0x87: // nop   
			cCycles -= 0;
			break;

		case 0x88: // nop   
			cCycles -= 0;
			break;
		
		case 0x89: // nop    
			cCycles -= 0;
			break;
		
		case 0x8A: // nop      
			cCycles -= 0;
			break;
		
		case 0x8B: // nop        
			cCycles -= 0;
			break;
		
		case 0x8C: // nop
			cCycles -= 0;
			break;
		
		case 0x8D: // nop 
			cCycles -= 0;
			break;
		
		case 0x8E: // nop  
			cCycles -= 0;
			break;
		
		case 0x8F: // nop   
			cCycles -= 0;
			break;

		case 0x90: // nop   
			cCycles -= 0;
			break;
		
		case 0x91: // nop    
			cCycles -= 0;
			break;
			
		case 0x92: // nop      
			cCycles -= 0;
			break;
		
		case 0x93: // nop        
			cCycles -= 0;
			break;
		
		case 0x94: // nop
			cCycles -= 0;
			break;
		
		case 0x95: // nop 
			cCycles -= 0;
			break;
		
		case 0x96: // nop  
			cCycles -= 0;
			break;
		
		case 0x97: // nop   
			cCycles -= 0;
			break;

		case 0x98: // nop   
			cCycles -= 0;
			break;
		
		case 0x99: // nop    
			cCycles -= 0;
			break;
		
		case 0x9A: // nop      
			cCycles -= 0;
			break;
		
		case 0x9B: // nop        
			cCycles -= 0;
			break;
		
		case 0x9C: // nop
			cCycles -= 0;
			break;
		
		case 0x9D: // nop 
			cCycles -= 0;
			break;
		
		case 0x9E: // nop  
			cCycles -= 0;
			break;
		
		case 0x9F: // nop   
			cCycles -= 0;
			break;

		case 0xA0: // ldi   
			cCycles -= 16;
		{
			MemWriteByte(m_regDE++, MemReadByte(m_regHL++));
			m_regBC--;
			m_regF = (m_regF & 0xE9) | (m_regBC ? V_FLAG : 0);
			break;
		}
		
		case 0xA1: // cpi    
			cCycles -= 16;
		{
			BYTE i,j;
			i=MemReadByte(m_regHL);
			j=m_regA-i;
			++m_regHL;
			--m_regBC;
			m_regF=(m_regF&C_FLAG)|ZSTable[j]|
			((m_regA^i^j)&H_FLAG)|(m_regBC? V_FLAG:0)|N_FLAG;
			break;
		}
			
		case 0xA2: // ini      
			cCycles -= 16;
		{
			MemWriteByte(m_regHL,In(m_regC));
			++m_regHL;
			--m_regB;
			m_regF=(m_regB)? N_FLAG:(N_FLAG|Z_FLAG);
			break;
		}
		
		case 0xA3: // outi       
			cCycles -= 16;
		{
			Out(m_regC, MemReadByte(m_regHL));
			++m_regHL;
			--m_regB;
			m_regF=(m_regB)? N_FLAG:(Z_FLAG|N_FLAG);
			break;
		}
		
		case 0xA4: // nop
			cCycles -= 0;
			break;
		
		case 0xA5: // nop 
			cCycles -= 0;
			break;
		
		case 0xA6: // nop  
			cCycles -= 0;
			break;
		
		case 0xA7: // nop   
			cCycles -= 0;
			break;

		case 0xA8: // ldd   
			cCycles -= 16;
		{
			MemWriteByte(m_regDE,MemReadByte(m_regHL));
			--m_regDE;
			--m_regHL;
			--m_regBC;
			m_regF=(m_regF&0xE9)|(m_regBC? V_FLAG:0);
			break;
		}
		
		case 0xA9: // cpd    
			cCycles -= 16;
		{
			BYTE i,j;
			i=MemReadByte(m_regHL);
			j=m_regA-i;
			--m_regHL;
			--m_regBC;
			m_regF=(m_regF&C_FLAG)|ZSTable[j]|
			((m_regA^i^j)&H_FLAG)|(m_regBC? V_FLAG:0)|N_FLAG;
			break;
		}
		
		case 0xAA: // ind
			cCycles -= 16;
		{
			MemWriteByte(m_regHL,In(m_regC));
			--m_regHL;
			--m_regB;
			m_regF=(m_regB)? N_FLAG:(N_FLAG|Z_FLAG);
			break;
		}
		
		case 0xAB: // outd
			cCycles -= 16;
		{
			Out (m_regC,MemReadByte(m_regHL));
			--m_regHL;
			--m_regB;
			m_regF=(m_regB)? N_FLAG:(Z_FLAG|N_FLAG);
			break;
		}
		
		case 0xAC: // nop
			cCycles -= 0;
			break;
		
		case 0xAD: // nop 
			cCycles -= 0;
			break;
		
		case 0xAE: // nop  
			cCycles -= 0;
			break;
		
		case 0xAF: // nop   
			cCycles -= 0;
			break;

		case 0xB0: // ldir  
			cCycles -= 0;
		{
			m_regR -= 2;
			
/*
			if (m_rgfMemDesc[m_regHL] & MF_READ_DIRECT &&
				m_rgfMemDesc[m_regDE] & MF_WRITE_DIRECT)
			{
				do
				{
					m_regR += 2;
					m_rgbMemory[m_regDE++] = m_rgbMemory[m_regHL++];
					cCycles -= 21;
				}
				while (--m_regBC); //  && cCycles>0);
				
				m_regF=(m_regF&0xE9)|(m_regBC? V_FLAG:0);
				
				if (m_regBC)
	//				m_regPC -= 2;
					AdjustPC(-2);
				else
					cCycles += 5;
			}
			else
*/
			{
				do
				{
					m_regR += 2;
					MemWriteByte(m_regDE++, MemReadByte(m_regHL++));
					cCycles -= 21;
				}
				while (--m_regBC); //  && cCycles>0);
				
				m_regF=(m_regF&0xE9)|(m_regBC? V_FLAG:0);
				
				if (m_regBC)
	//				m_regPC -= 2;
					AdjustPC(-2);
				else
					cCycles += 5;
			}
			
			break;
		}
		
		case 0xB1: // cpir   
			cCycles -= 0;
		{
			BYTE i,j;
			m_regR-=2;
			do
			{
				m_regR+=2;
				i=MemReadByte(m_regHL);
				j=m_regA-i;
				++m_regHL;
				--m_regBC;
				cCycles-=21;
			}
			while (m_regBC && j && cCycles>0);
			m_regF=(m_regF&C_FLAG)|ZSTable[j]|
			((m_regA^i^j)&H_FLAG)|(m_regBC? V_FLAG:0)|N_FLAG;
			if (m_regBC && j) AdjustPC(-2); // m_regPC-=2;
			else cCycles+=5;
			break;
		}
			
		case 0xB2: // inir     
			cCycles -= 0;
		{
			m_regR-=2;
			do
			{
				m_regR+=2;
				MemWriteByte(m_regHL,In(m_regC));
				++m_regHL;
				--m_regB;
				cCycles-=21;
			}
			while (m_regB && cCycles>0);
			m_regF=(m_regB)? N_FLAG:(N_FLAG|Z_FLAG);
			if (m_regB) AdjustPC(-2); // m_regPC-=2;
			else cCycles+=5;
			break;
		}
		
		case 0xB3: // otir       
			cCycles -= 0;
		{
			m_regR-=2;
			do
			{
				m_regR+=2;
				Out (m_regC,MemReadByte(m_regHL));
				++m_regHL;
				--m_regB;
				cCycles-=21;
			}
			while (m_regB && cCycles>0);
			m_regF=(m_regB)? N_FLAG:(Z_FLAG|N_FLAG);
			if (m_regB) AdjustPC(-2); // m_regPC-=2;
			else cCycles+=5;
		}
			break;
		
		case 0xB4: // nop
			cCycles -= 0;
			break;
		
		case 0xB5: // nop 
			cCycles -= 0;
			break;
		
		case 0xB6: // nop  
			cCycles -= 0;
			break;
		
		case 0xB7: // nop   
			cCycles -= 0;
			break;

		case 0xB8: // lddr  
			cCycles -= 0;
		{
			m_regR-=2;
			do
			{
			m_regR+=2;
			MemWriteByte(m_regDE,MemReadByte(m_regHL));
			--m_regDE;
			--m_regHL;
			--m_regBC;
			cCycles-=21;
			}
			while (m_regBC);
			m_regF=(m_regF&0xE9)|(m_regBC? V_FLAG:0);
			if (m_regBC) AdjustPC(-2); // m_regPC-=2;
			else
			 cCycles+=5;
			break;
		}
		
		case 0xB9: // cpdr   
			cCycles -= 0;
		{
			BYTE i,j;
			m_regR-=2;
			do
			{
				m_regR+=2;
				i=MemReadByte(m_regHL);
				j=m_regA-i;
				--m_regHL;
				--m_regBC;
				cCycles-=21;
			}
			while (m_regBC && j && cCycles>0);
			m_regF=(m_regF&C_FLAG)|ZSTable[j]|
			((m_regA^i^j)&H_FLAG)|(m_regBC? V_FLAG:0)|N_FLAG;
			if (m_regBC && j) AdjustPC(-2); // m_regPC-=2;
			else cCycles+=5;
			break;
		}
		
		case 0xBA: // indr     
			cCycles -= 0;
		{
			m_regR-=2;
			do
			{
				m_regR+=2;
				MemWriteByte(m_regHL,In(m_regC));
				--m_regHL;
				--m_regB;
				cCycles-=21;
			}
			while (m_regB && cCycles>0);
			m_regF=(m_regB)? N_FLAG:(N_FLAG|Z_FLAG);
			if (m_regB) AdjustPC(-2); // m_regPC-=2;
			else cCycles+=5;
			break;
		}
		
		case 0xBB: // otdr       
			cCycles -= 0;
		{
			m_regR-=2;
			do
			{
				m_regR+=2;
				Out (m_regC,MemReadByte(m_regHL));
				--m_regHL;
				--m_regB;
				cCycles-=21;
			}
			while (m_regB && cCycles>0);
			m_regF=(m_regB)? N_FLAG:(Z_FLAG|N_FLAG);
			if (m_regB) AdjustPC(-2); // m_regPC-=2;
			else cCycles+=5;
			break;
		}
		
		case 0xBC: // nop
			cCycles -= 0;
			break;
		
		case 0xBD: // nop 
			cCycles -= 0;
			break;
		
		case 0xBE: // nop  
			cCycles -= 0;
			break;
		
		case 0xBF: // nop   
			cCycles -= 0;
			break;

		case 0xC0: // nop   
			cCycles -= 0;
			break;
		
		case 0xC1: // nop    
			cCycles -= 0;
			break;
			
		case 0xC2: // nop      
			cCycles -= 0;
			break;
		
		case 0xC3: // nop        
			cCycles -= 0;
			break;
		
		case 0xC4: // nop
			cCycles -= 0;
			break;
		
		case 0xC5: // nop 
			cCycles -= 0;
			break;
		
		case 0xC6: // nop  
			cCycles -= 0;
			break;
		
		case 0xC7: // nop   
			cCycles -= 0;
			break;

		case 0xC8: // nop   
			cCycles -= 0;
			break;
		
		case 0xC9: // nop    
			cCycles -= 0;
			break;
		
		case 0xCA: // nop      
			cCycles -= 0;
			break;
		
		case 0xCB: // nop        
			cCycles -= 0;
			break;
		
		case 0xCC: // nop
			cCycles -= 0;
			break;
		
		case 0xCD: // nop 
			cCycles -= 0;
			break;
		
		case 0xCE: // nop  
			cCycles -= 0;
			break;
		
		case 0xCF: // nop   
			cCycles -= 0;
			break;

		case 0xD0: // nop   
			cCycles -= 0;
			break;
		
		case 0xD1: // nop    
			cCycles -= 0;
			break;
			
		case 0xD2: // nop      
			cCycles -= 0;
			break;
		
		case 0xD3: // nop        
			cCycles -= 0;
			break;
		
		case 0xD4: // nop
			cCycles -= 0;
			break;
		
		case 0xD5: // nop 
			cCycles -= 0;
			break;
		
		case 0xD6: // nop  
			cCycles -= 0;
			break;
		
		case 0xD7: // nop   
			cCycles -= 0;
			break;

		case 0xD8: // nop   
			cCycles -= 0;
			break;
		
		case 0xD9: // nop    
			cCycles -= 0;
			break;
		
		case 0xDA: // nop      
			cCycles -= 0;
			break;
		
		case 0xDB: // nop        
			cCycles -= 0;
			break;
		
		case 0xDC: // nop
			cCycles -= 0;
			break;
		
		case 0xDD: // nop 
			cCycles -= 0;
			break;
		
		case 0xDE: // nop  
			cCycles -= 0;
			break;
		
		case 0xDF: // nop   
			cCycles -= 0;
			break;

		case 0xE0: // nop   
			cCycles -= 0;
			break;
		
		case 0xE1: // nop    
			cCycles -= 0;
			break;
			
		case 0xE2: // nop      
			cCycles -= 0;
			break;
		
		case 0xE3: // nop        
			cCycles -= 0;
			break;
		
		case 0xE4: // nop
			cCycles -= 0;
			break;
		
		case 0xE5: // nop 
			cCycles -= 0;
			break;
		
		case 0xE6: // nop  
			cCycles -= 0;
			break;
		
		case 0xE7: // nop   
			cCycles -= 0;
			break;

		case 0xE8: // nop   
			cCycles -= 0;
			break;
		
		case 0xE9: // nop    
			cCycles -= 0;
			break;
		
		case 0xEA: // nop      
			cCycles -= 0;
			break;
		
		case 0xEB: // nop        
			cCycles -= 0;
			break;
		
		case 0xEC: // nop
			cCycles -= 0;
			break;
		
		case 0xED: // nop 
			cCycles -= 0;
			break;
		
		case 0xEE: // nop  
			cCycles -= 0;
			break;
		
		case 0xEF: // nop   
			cCycles -= 0;
			break;

		case 0xF0: // nop   
			cCycles -= 0;
			break;
		
		case 0xF1: // nop    
			cCycles -= 0;
			break;
			
		case 0xF2: // nop      
			cCycles -= 0;
			break;
		
		case 0xF3: // nop        
			cCycles -= 0;
			break;
		
		case 0xF4: // nop
			cCycles -= 0;
			break;
		
		case 0xF5: // nop 
			cCycles -= 0;
			break;
		
		case 0xF6: // nop  
			cCycles -= 0;
			break;
		
		case 0xF7: // nop   
			cCycles -= 0;
			break;

		case 0xF8: // nop   
			cCycles -= 0;
			break;
		
		case 0xF9: // nop    
			cCycles -= 0;
			break;
		
		case 0xFA: // nop      
			cCycles -= 0;
			break;
		
		case 0xFB: // nop        
			cCycles -= 0;
			break;
		
		case 0xFC: // nop
			cCycles -= 0;
			break;
		
		case 0xFD: // nop 
			cCycles -= 0;
			break;
		
		case 0xFE: // patch
			cCycles -= 0;
			break;
		
		case 0xFF: // nop
			cCycles -= 0;
			break;

		default:
			_ASSERT(FALSE);
			break;
	}

	return (cCyclesArg - cCycles);
}

int Ei() {
  /* fprintf ( stderr, "Enable Interrupt\n" ); */
  m_iff2 = 1;

  if (m_iff1 != 0)
    return 0;

  m_iff1 = 1;

  return(0);
}

void Di() {
  /* fprintf ( stderr, "Disable Interrupt\n" ); */
  m_iff1 = 0;
  m_iff2 = 0;
}

/* ***************************************************************************
 * FD prefixed opcodes
 * ***************************************************************************
 */
/* inline */ WORD IndirectIY() {
	return m_regIY + (signed char)ImmedByte();
}

int HandleFD() {

	WORD wAddr;
	WORD wTemp;
	const BYTE bOpcode = ImmedByte();

	m_regR++;
	
	switch (bOpcode)
	{
		case 0x00: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x01: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x02: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x03: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x04: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x05: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x06: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x07: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x08: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x09: // add_iy_bc 
			m_regIY = Add_2(m_regIY, m_regBC);
			return 15;
		
		case 0x0A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0C: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0D: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0E: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x0F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x10: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x11: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x12: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x13: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x14: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x15: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x16: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x17: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x18: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x19: // add_iy_de 
			m_regIY = Add_2(m_regIY, m_regDE);
			return 15;
		
		case 0x1A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1C: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1D: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1E: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x1F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x20: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x21: // ld_iy_word
			m_regIY = ImmedWord();
			return 14;
		
		case 0x22: // ld_xword_iy
			MemWriteWord(ImmedWord(), m_regIY);
			return 20;
		
		case 0x23: // inc_iy
			m_regIY++;
			return 10;
		
		case 0x24: // inc_iyh  
			m_regIYh = Inc(m_regIYh);
			return 9;
		
		case 0x25: // dec_iyh  
			m_regIYh = Dec(m_regIYh);
			return 9;
		
		case 0x26: // ld_iyh_byte
			m_regIYh = ImmedByte();
			return 9;
		
		case 0x27: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x28: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x29: // add_iy_iy 
			m_regIY = Add_2(m_regIY, m_regIY);
			return 15;
		
		case 0x2A: // ld_iy_xword
			m_regIY = MemReadWord(ImmedWord());
			return 20;
		
		case 0x2B: // dec_iy   
			m_regIY--;
			return 10;
		
		case 0x2C: // inc_iyl  
			m_regIYl = Inc(m_regIYl);
			return 9;
		
		case 0x2D: // dec_iyl  
			m_regIYl = Dec(m_regIYl);
			return 9;
		
		case 0x2E: // ld_iyl_byte
			m_regIYl = ImmedByte();
			return 9;
		
		case 0x2F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x30: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x31: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x32: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x33: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x34: // inc_xiy  
			wAddr = IndirectIY();
			MemWriteByte(wAddr, Inc(MemReadByte(wAddr)));
			return 23;
		
		case 0x35: // dec_xiy  
			wAddr = IndirectIY();
			MemWriteByte(wAddr, Dec(MemReadByte(wAddr)));
			return 23;
		
		case 0x36: // ld_xiy_byte
			wAddr = IndirectIY();
			MemWriteByte(wAddr, ImmedByte());
			return 19;
		
		case 0x37: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x38: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x39: // add_iy_sp 
			m_regIY = Add_2(m_regIY, GetSP());
			return 15;
		
		case 0x3A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3C: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3D: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3E: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x3F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x40: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x41: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x42: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x43: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x44: // ld_b_iyh 
			m_regB = m_regIYh;
			return 9;
		
		case 0x45: // ld_b_iyl 
			m_regB = m_regIYl;
			return 9;
		
		case 0x46: // ld_b_xiy   
			m_regB = MemReadByte(IndirectIY());
			return 19;
		
		case 0x47: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x48: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x49: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x4A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x4B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x4C: // ld_c_iyh 
			m_regC = m_regIYh;
			return 9;
		
		case 0x4D: // ld_c_iyl 
			m_regC = m_regIYl;
			return 9;
		
		case 0x4E: // ld_c_xiy   
			m_regC = MemReadByte(IndirectIY());
			return 19;
		
		case 0x4F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x50: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x51: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x52: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x53: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x54: // ld_d_iyh 
			m_regD = m_regIYh;
			return 9;
		
		case 0x55: // ld_d_iyl 
			m_regD = m_regIYl;
			return 9;
		
		case 0x56: // ld_d_xiy   
			m_regD = MemReadByte(IndirectIY());
			return 19;
		
		case 0x57: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x58: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x59: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x5A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x5B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x5C: // ld_e_iyh 
			m_regE = m_regIYh;
			return 9;
		
		case 0x5D: // ld_e_iyl 
			m_regE = m_regIYl;
			return 9;
		
		case 0x5E: // ld_e_xiy   
			m_regE = MemReadByte(IndirectIY());
			return 19;
		
		case 0x5F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x60: // ld_iyh_b
			m_regIYh = m_regB;
			return 9;
		
		case 0x61: // ld_iyh_c  
			m_regIYh = m_regC;
			return 9;
		
		case 0x62: // ld_iyh_d   
			m_regIYh = m_regD;
			return 9;
		
		case 0x63: // ld_iyh_e 
			m_regIYh = m_regE;
			return 9;
		
		case 0x64: // ld_iyh_h 
			m_regIYh = m_regH;
			return 9;
		
		case 0x65: // ld_iyh_l 
			m_regIYh = m_regL;
			return 9;
		
		case 0x66: // ld_h_xiy   
			m_regH = MemReadByte(IndirectIY());
			return 9;
		
		case 0x67: // ld_iyh_a
			m_regIYh = m_regA;
			return 9;

		case 0x68: // ld_iyl_b
			m_regIYl = m_regB;
			return 9;
		
		case 0x69: // ld_iyl_c  
			m_regIYl = m_regC;
			return 9;
		
		case 0x6A: // ld_iyl_d   
			m_regIYl = m_regD;
			return 9;
		
		case 0x6B: // ld_iyl_e 
			m_regIYl = m_regE;
			return 9;
		
		case 0x6C: // ld_iyl_h 
			m_regIYl = m_regH;
			return 9;
		
		case 0x6D: // ld_iyl_l 
			m_regIYl = m_regL;
			return 9;
		
		case 0x6E: // ld_l_xiy   
			m_regL = MemReadByte(IndirectIY());
			return 9;
		
		case 0x6F: // ld_iyl_a
			m_regIYl = m_regA;
			return 9;

		case 0x70: // ld_xiy_b
			MemWriteByte(IndirectIY(), m_regB);
			return 19;
		
		case 0x71: // ld_xiy_c  
			MemWriteByte(IndirectIY(), m_regC);
			return 19;
		
		case 0x72: // ld_xiy_d   
			MemWriteByte(IndirectIY(), m_regD);
			return 19;
		
		case 0x73: // ld_xiy_e 
			MemWriteByte(IndirectIY(), m_regE);
			return 19;
		
		case 0x74: // ld_xiy_h 
			MemWriteByte(IndirectIY(), m_regH);
			return 19;
		
		case 0x75: // ld_xiy_l 
			MemWriteByte(IndirectIY(), m_regL);
			return 19;
		
		case 0x76: // no_op      
			SetPC(GetPC() - 1);
			return 19;
		
		case 0x77: // ld_xiy_a
			MemWriteByte(IndirectIY(), m_regA);
			return 19;

		case 0x78: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x79: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x7A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x7B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x7C: // ld_a_iyh 
			m_regA = m_regIYh;
			return 9;
		
		case 0x7D: // ld_a_iyl 
			m_regA = m_regIYl;
			return 9;
		
		case 0x7E: // ld_a_xiy   
			m_regA = MemReadByte(IndirectIY());
			return 19;
		
		case 0x7F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x80: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x81: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x82: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x83: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x84: // add_a_iyh
			Add_1(m_regIYh);
			return 9;
		
		case 0x85: // add_a_iyl
			Add_1(m_regIYl);
			return 9;
		
		case 0x86: // add_a_xiy  
			Add_1(MemReadByte(IndirectIY()));
			return 19;
		
		case 0x87: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x88: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x89: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x8A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x8B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x8C: // adc_a_iyh
			Adc_1(m_regIYh);
			return 9;
		
		case 0x8D: // adc_a_iyl
			Adc_1(m_regIYl);
			return 9;
		
		case 0x8E: // adc_a_xiy  
			Adc_1(MemReadByte(IndirectIY()));
			return 19;
		
		case 0x8F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x90: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x91: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x92: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x93: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x94: // sub_iyh  
			Sub_1(m_regIYh);
			return 9;
		
		case 0x95: // sub_iyl  
			Sub_1(m_regIYl);
			return 9;
		
		case 0x96: // sub_xiy    
			Sub_1(MemReadByte(IndirectIY()));
			return 19;
		
		case 0x97: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0x98: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x99: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x9A: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x9B: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0x9C: // sbc_a_iyh
			Sbc_1(m_regIYh);
			return 9;
		
		case 0x9D: // sbc_a_iyl
			Sbc_1(m_regIYl);
			return 9;
		
		case 0x9E: // sbc_a_xiy  
			Sbc_1(MemReadByte(IndirectIY()));
			return 19;
		
		case 0x9F: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xA0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA4: // and_iyh  
			And(m_regIYh);
			return 9;
		
		case 0xA5: // and_iyl  
			And(m_regIYl);
			return 9;
		
		case 0xA6: // and_xiy    
			And(MemReadByte(IndirectIY()));
			return 19;
		
		case 0xA7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xA8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xA9: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xAA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xAB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xAC: // xor_iyh  
			Xor(m_regIYh);
			return 9;
		
		case 0xAD: // xor_iyl  
			Xor(m_regIYl);
			return 9;
		
		case 0xAE: // xor_xiy    
			Xor(MemReadByte(IndirectIY()));
			return 19;
		
		case 0xAF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xB0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB4: // or_iyh   
			Or(m_regIYh);
			return 9;
		
		case 0xB5: // or_iyl   
			Or(m_regIYl);
			return 9;
		
		case 0xB6: // or_xiy     
			Or(MemReadByte(IndirectIY()));
			return 19;
		
		case 0xB7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xB8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xB9: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xBA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xBB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xBC: // cp_iyh   
			Cp(m_regIYh);
			return 9;
		
		case 0xBD: // cp_iyl   
			Cp(m_regIYl);
			return 9;
		
		case 0xBE: // cp_xiy     
			Cp(MemReadByte(IndirectIY()));
			return 19;
		
		case 0xBF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xC0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC4: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC5: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC6: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xC8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xC9: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCB: // fd_cb    
			return HandleFDCB();
		
		case 0xCC: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCD: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCE: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xCF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xD0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD4: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD5: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD6: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xD8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xD9: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDC: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDD: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDE: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xDF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xE0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE1: // pop_iy    
			m_regIY = Pop();
			return 14;
		
		case 0xE2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE3: // ex_xsp_iy
			wTemp = MemReadWord(GetSP());
			MemWriteWord(GetSP(), m_regIY);
			m_regIY = wTemp;
			return 23;
		
		case 0xE4: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE5: // push_iy  
			Push(m_regIY);
			return 15;
		
		case 0xE6: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xE8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xE9: // jp_iy     
			SetPC(m_regIY);
			return 8;
		
		case 0xEA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xEB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xEC: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xED: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xEE: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xEF: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xF0: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF1: // no_op     
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF2: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF3: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF4: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF5: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF6: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF7: // no_op   
			SetPC(GetPC() - 1);
			return 0;

		case 0xF8: // no_op   
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xF9: // ld_sp_iy  
			SetSP(m_regIY);
			return 10;
		
		case 0xFA: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFB: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFC: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFD: // no_op    
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFE: // no_op      
			SetPC(GetPC() - 1);
			return 0;
		
		case 0xFF: // no_op
			SetPC(GetPC() - 1);
			return 0;

		default:
			_ASSERT(FALSE);
			return 0;
	}
}

/* ***************************************************************************
 * FDCD prefixed opcodes
 * ***************************************************************************
 */

int HandleFDCB() {
	const WORD wAddr = m_regIY + (signed char)ImmedByte();
	const BYTE bOpcode = ImmedByte();

	switch (bOpcode)
	{
		case 0x00: // no_op_xx 
			return 0;
		
		case 0x01: // no_op_xx 
			return 0;
			
		case 0x02: // no_op_xx 
			return 0;
		
		case 0x03: // no_op_xx 
			return 0;
		
		case 0x04: // no_op_xx 
			return 0;
		
		case 0x05: // no_op_xx 
			return 0;
		
		case 0x06: // rlc_xiy  
			MemWriteByte(wAddr, Rlc(MemReadByte(wAddr)));
			return 23;
		
		case 0x07: // no_op_xx 
			return 0;

		case 0x08: // no_op_xx 
			return 0;
		
		case 0x09: // no_op_xx 
			return 0;
		
		case 0x0A: // no_op_xx 
			return 0;
		
		case 0x0B: // no_op_xx 
			return 0;
		
		case 0x0C: // no_op_xx 
			return 0;
		
		case 0x0D: // no_op_xx 
			return 0;
		
		case 0x0E: // rrc_xiy  
			MemWriteByte(wAddr, Rrc(MemReadByte(wAddr)));
			return 23;
		
		case 0x0F: // no_op_xx 
			return 0;

		case 0x10: // no_op_xx 
			return 0;
		
		case 0x11: // no_op_xx 
			return 0;
			
		case 0x12: // no_op_xx 
			return 0;
		
		case 0x13: // no_op_xx 
			return 0;
		
		case 0x14: // no_op_xx 
			return 0;
		
		case 0x15: // no_op_xx 
			return 0;
		
		case 0x16: // rl_xiy   
			MemWriteByte(wAddr, Rl(MemReadByte(wAddr)));
			return 23;
		
		case 0x17: // no_op_xx 
			return 0;

		case 0x18: // no_op_xx 
			return 0;
		
		case 0x19: // no_op_xx 
			return 0;
		
		case 0x1A: // no_op_xx 
			return 0;
		
		case 0x1B: // no_op_xx 
			return 0;
		
		case 0x1C: // no_op_xx 
			return 0;
		
		case 0x1D: // no_op_xx 
			return 0;
		
		case 0x1E: // rr_xiy   
			MemWriteByte(wAddr, Rr(MemReadByte(wAddr)));
			return 23;
		
		case 0x1F: // no_op_xx 
			return 0;

		case 0x20: // no_op_xx 
			return 0;
		
		case 0x21: // no_op_xx 
			return 0;
			
		case 0x22: // no_op_xx 
			return 0;
		
		case 0x23: // no_op_xx 
			return 0;
		
		case 0x24: // no_op_xx 
			return 0;
		
		case 0x25: // no_op_xx 
			return 0;
		
		case 0x26: // sla_xiy  
			MemWriteByte(wAddr, Sla(MemReadByte(wAddr)));
			return 23;
		
		case 0x27: // no_op_xx 
			return 0;

		case 0x28: // no_op_xx 
			return 0;
		
		case 0x29: // no_op_xx 
			return 0;
		
		case 0x2A: // no_op_xx 
			return 0;
		
		case 0x2B: // no_op_xx 
			return 0;
		
		case 0x2C: // no_op_xx 
			return 0;
		
		case 0x2D: // no_op_xx 
			return 0;
		
		case 0x2E: // sra_xiy  
			MemWriteByte(wAddr, Sra(MemReadByte(wAddr)));
			return 23;
		
		case 0x2F: // no_op_xx 
			return 0;

		case 0x30: // no_op_xx 
			return 0;
		
		case 0x31: // no_op_xx 
			return 0;
			
		case 0x32: // no_op_xx 
			return 0;
		
		case 0x33: // no_op_xx 
			return 0;
		
		case 0x34: // no_op_xx 
			return 0;
		
		case 0x35: // no_op_xx 
			return 0;
		
		case 0x36: // sll_xiy  
			MemWriteByte(wAddr, Sll(MemReadByte(wAddr)));
			return 23;
		
		case 0x37: // no_op_xx 
			return 0;

		case 0x38: // no_op_xx 
			return 0;
		
		case 0x39: // no_op_xx 
			return 0;
		
		case 0x3A: // no_op_xx 
			return 0;
		
		case 0x3B: // no_op_xx 
			return 0;
		
		case 0x3C: // no_op_xx 
			return 0;
		
		case 0x3D: // no_op_xx 
			return 0;
		
		case 0x3E: // srl_xiy  
			MemWriteByte(wAddr, Srl(MemReadByte(wAddr)));
			return 23;
		
		case 0x3F: // no_op_xx 
			return 0;

		case 0x40: // bit_0_xiy
		case 0x41: // bit_0_xiy
		case 0x42: // bit_0_xiy
		case 0x43: // bit_0_xiy
		case 0x44: // bit_0_xiy
		case 0x45: // bit_0_xiy
		case 0x46: // bit_0_xiy
		case 0x47: // bit_0_xiy
			Bit(MemReadByte(wAddr), 0);
			return 20;

		case 0x48: // bit_1_xiy
		case 0x49: // bit_1_xiy
		case 0x4A: // bit_1_xiy
		case 0x4B: // bit_1_xiy
		case 0x4C: // bit_1_xiy
		case 0x4D: // bit_1_xiy
		case 0x4E: // bit_1_xiy
		case 0x4F: // bit_1_xiy
			Bit(MemReadByte(wAddr), 1);
			return 20;

		case 0x50: // bit_2_xiy
		case 0x51: // bit_2_xiy
		case 0x52: // bit_2_xiy
		case 0x53: // bit_2_xiy
		case 0x54: // bit_2_xiy
		case 0x55: // bit_2_xiy
		case 0x56: // bit_2_xiy
		case 0x57: // bit_2_xiy
			Bit(MemReadByte(wAddr), 2);
			return 20;

		case 0x58: // bit_3_xiy
		case 0x59: // bit_3_xiy
		case 0x5A: // bit_3_xiy
		case 0x5B: // bit_3_xiy
		case 0x5C: // bit_3_xiy
		case 0x5D: // bit_3_xiy
		case 0x5E: // bit_3_xiy
		case 0x5F: // bit_3_xiy
			Bit(MemReadByte(wAddr), 3);
			return 20;

		case 0x60: // bit_4_xiy
		case 0x61: // bit_4_xiy
		case 0x62: // bit_4_xiy
		case 0x63: // bit_4_xiy
		case 0x64: // bit_4_xiy
		case 0x65: // bit_4_xiy
		case 0x66: // bit_4_xiy
		case 0x67: // bit_4_xiy
			Bit(MemReadByte(wAddr), 4);
			return 20;

		case 0x68: // bit_5_xiy
		case 0x69: // bit_5_xiy
		case 0x6A: // bit_5_xiy
		case 0x6B: // bit_5_xiy
		case 0x6C: // bit_5_xiy
		case 0x6D: // bit_5_xiy
		case 0x6E: // bit_5_xiy
		case 0x6F: // bit_5_xiy
			Bit(MemReadByte(wAddr), 5);
			return 20;

		case 0x70: // bit_6_xiy
		case 0x71: // bit_6_xiy
		case 0x72: // bit_6_xiy
		case 0x73: // bit_6_xiy
		case 0x74: // bit_6_xiy
		case 0x75: // bit_6_xiy
		case 0x76: // bit_6_xiy
		case 0x77: // bit_6_xiy
			Bit(MemReadByte(wAddr), 6);
			return 20;

		case 0x78: // bit_7_xiy
		case 0x79: // bit_7_xiy
		case 0x7A: // bit_7_xiy
		case 0x7B: // bit_7_xiy
		case 0x7C: // bit_7_xiy
		case 0x7D: // bit_7_xiy
		case 0x7E: // bit_7_xiy
		case 0x7F: // bit_7_xiy
			Bit(MemReadByte(wAddr), 7);
			return 20;

		case 0x80: // no_op_xx 
			return 0;
		
		case 0x81: // no_op_xx 
			return 0;
			
		case 0x82: // no_op_xx 
			return 0;
		
		case 0x83: // no_op_xx 
			return 0;
		
		case 0x84: // no_op_xx 
			return 0;
		
		case 0x85: // no_op_xx 
			return 0;
		
		case 0x86: // res_0_xiy
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 0));
			return 23;
		
		case 0x87: // no_op_xx 
			return 0;

		case 0x88: // no_op_xx 
			return 0;
		
		case 0x89: // no_op_xx 
			return 0;
		
		case 0x8A: // no_op_xx 
			return 0;
		
		case 0x8B: // no_op_xx 
			return 0;
		
		case 0x8C: // no_op_xx 
			return 0;
		
		case 0x8D: // no_op_xx 
			return 0;
		
		case 0x8E: // res_1_xiy
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 1));
			return 23;
		
		case 0x8F: // no_op_xx 
			return 0;

		case 0x90: // no_op_xx 
			return 0;
		
		case 0x91: // no_op_xx 
			return 0;
			
		case 0x92: // no_op_xx 
			return 0;
		
		case 0x93: // no_op_xx 
			return 0;
		
		case 0x94: // no_op_xx 
			return 0;
		
		case 0x95: // no_op_xx 
			return 0;
		
		case 0x96: // res_2_xiy
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 2));
			return 23;
		
		case 0x97: // no_op_xx 
			return 0;

		case 0x98: // no_op_xx 
			return 0;
		
		case 0x99: // no_op_xx 
			return 0;
		
		case 0x9A: // no_op_xx 
			return 0;
		
		case 0x9B: // no_op_xx 
			return 0;
		
		case 0x9C: // no_op_xx 
			return 0;
		
		case 0x9D: // no_op_xx 
			return 0;
		
		case 0x9E: // res_3_xiy
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 3));
			return 23;
		
		case 0x9F: // no_op_xx 
			return 0;

		case 0xA0: // no_op_xx 
			return 0;
		
		case 0xA1: // no_op_xx 
			return 0;
			
		case 0xA2: // no_op_xx 
			return 0;
		
		case 0xA3: // no_op_xx 
			return 0;
		
		case 0xA4: // no_op_xx 
			return 0;
		
		case 0xA5: // no_op_xx 
			return 0;
		
		case 0xA6: // res_4_xiy
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 4));
			return 23;
		
		case 0xA7: // no_op_xx 
			return 0;

		case 0xA8: // no_op_xx 
			return 0;
		
		case 0xA9: // no_op_xx 
			return 0;
		
		case 0xAA: // no_op_xx 
			return 0;
		
		case 0xAB: // no_op_xx 
			return 0;
		
		case 0xAC: // no_op_xx 
			return 0;
		
		case 0xAD: // no_op_xx 
			return 0;
		
		case 0xAE: // res_5_xiy
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 5));
			return 23;
		
		case 0xAF: // no_op_xx 
			return 0;

		case 0xB0: // no_op_xx 
			return 0;
		
		case 0xB1: // no_op_xx 
			return 0;
			
		case 0xB2: // no_op_xx 
			return 0;
		
		case 0xB3: // no_op_xx 
			return 0;
		
		case 0xB4: // no_op_xx 
			return 0;
		
		case 0xB5: // no_op_xx 
			return 0;
		
		case 0xB6: // res_6_xiy
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 6));
			return 23;
		
		case 0xB7: // no_op_xx 
			return 0;

		case 0xB8: // no_op_xx 
			return 0;
		
		case 0xB9: // no_op_xx 
			return 0;
		
		case 0xBA: // no_op_xx 
			return 0;
		
		case 0xBB: // no_op_xx 
			return 0;
		
		case 0xBC: // no_op_xx 
			return 0;
		
		case 0xBD: // no_op_xx 
			return 0;
		
		case 0xBE: // res_7_xiy
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 7));
			return 23;
		
		case 0xBF: // no_op_xx 
			return 0;

		case 0xC0: // no_op_xx 
			return 0;
		
		case 0xC1: // no_op_xx 
			return 0;
			
		case 0xC2: // no_op_xx 
			return 0;
		
		case 0xC3: // no_op_xx 
			return 0;
		
		case 0xC4: // no_op_xx 
			return 0;
		
		case 0xC5: // no_op_xx 
			return 0;
		
		case 0xC6: // set_0_xiy
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 0));
			return 23;
		
		case 0xC7: // no_op_xx 
			return 0;

		case 0xC8: // no_op_xx 
			return 0;
		
		case 0xC9: // no_op_xx 
			return 0;
		
		case 0xCA: // no_op_xx 
			return 0;
		
		case 0xCB: // no_op_xx 
			return 0;
		
		case 0xCC: // no_op_xx 
			return 0;
		
		case 0xCD: // no_op_xx 
			return 0;
		
		case 0xCE: // set_1_xiy
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 1));
			return 23;
		
		case 0xCF: // no_op_xx 
			return 0;

		case 0xD0: // no_op_xx 
			return 0;
		
		case 0xD1: // no_op_xx 
			return 0;
			
		case 0xD2: // no_op_xx 
			return 0;
		
		case 0xD3: // no_op_xx 
			return 0;
		
		case 0xD4: // no_op_xx 
			return 0;
		
		case 0xD5: // no_op_xx 
			return 0;
		
		case 0xD6: // set_2_xiy
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 2));
			return 23;
		
		case 0xD7: // no_op_xx 
			return 0;

		case 0xD8: // no_op_xx 
			return 0;
		
		case 0xD9: // no_op_xx 
			return 0;
		
		case 0xDA: // no_op_xx 
			return 0;
		
		case 0xDB: // no_op_xx 
			return 0;
		
		case 0xDC: // no_op_xx 
			return 0;
		
		case 0xDD: // no_op_xx 
			return 0;
		
		case 0xDE: // set_3_xiy
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 3));
			return 23;
		
		case 0xDF: // no_op_xx 
			return 0;

		case 0xE0: // no_op_xx 
			return 0;
		
		case 0xE1: // no_op_xx 
			return 0;
			
		case 0xE2: // no_op_xx 
			return 0;
		
		case 0xE3: // no_op_xx 
			return 0;
		
		case 0xE4: // no_op_xx 
			return 0;
		
		case 0xE5: // no_op_xx 
			return 0;
		
		case 0xE6: // set_4_xiy
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 4));
			return 23;
		
		case 0xE7: // no_op_xx 
			return 0;

		case 0xE8: // no_op_xx 
			return 0;
		
		case 0xE9: // no_op_xx 
			return 0;
		
		case 0xEA: // no_op_xx 
			return 0;
		
		case 0xEB: // no_op_xx 
			return 0;
		
		case 0xEC: // no_op_xx 
			return 0;
		
		case 0xED: // no_op_xx 
			return 0;
		
		case 0xEE: // set_5_xiy
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 5));
			return 23;
		
		case 0xEF: // no_op_xx 
			return 0;

		case 0xF0: // no_op_xx 
			return 0;
		
		case 0xF1: // no_op_xx 
			return 0;
			
		case 0xF2: // no_op_xx 
			return 0;
		
		case 0xF3: // no_op_xx 
			return 0;
		
		case 0xF4: // no_op_xx 
			return 0;
		
		case 0xF5: // no_op_xx 
			return 0;
		
		case 0xF6: // set_6_xiy
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 6));
			return 23;
		
		case 0xF7: // no_op_xx 
			return 0;

		case 0xF8: // no_op_xx 
			return 0;
		
		case 0xF9: // no_op_xx 
			return 0;
		
		case 0xFA: // no_op_xx 
			return 0;
		
		case 0xFB: // no_op_xx 
			return 0;
		
		case 0xFC: // no_op_xx 
			return 0;
		
		case 0xFD: // no_op_xx 
			return 0;
		
		case 0xFE: // set_7_xiy
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 7));
			return 23;
		
		case 0xFF: // no_op_x
			return 0;

		default:
			_ASSERT(FALSE);
			return 0;
	}
}

/* ***************************************************************************
 * DDCB prefixed opcodes
 * ***************************************************************************
 */

int HandleDDCB() {
	const WORD wAddr = m_regIX + (signed char)ImmedByte();
	const BYTE bOpcode = ImmedByte();

	switch (bOpcode)
	{
		case 0x00: // no_op_xx 
			return 0;
		
		case 0x01: // no_op_xx 
			return 0;
			
		case 0x02: // no_op_xx 
			return 0;
		
		case 0x03: // no_op_xx 
			return 0;
		
		case 0x04: // no_op_xx 
			return 0;
		
		case 0x05: // no_op_xx 
			return 0;
		
		case 0x06: // rlc_xix  
			MemWriteByte(wAddr, Rlc(MemReadByte(wAddr)));
			return 23;
		
		case 0x07: // no_op_xx 
			return 0;

		case 0x08: // no_op_xx 
			return 0;
		
		case 0x09: // no_op_xx 
			return 0;
		
		case 0x0A: // no_op_xx 
			return 0;
		
		case 0x0B: // no_op_xx 
			return 0;
		
		case 0x0C: // no_op_xx 
			return 0;
		
		case 0x0D: // no_op_xx 
			return 0;
		
		case 0x0E: // rrc_xix  
			MemWriteByte(wAddr, Rrc(MemReadByte(wAddr)));
			return 23;
		
		case 0x0F: // no_op_xx 
			return 0;

		case 0x10: // no_op_xx 
			return 0;
		
		case 0x11: // no_op_xx 
			return 0;
			
		case 0x12: // no_op_xx 
			return 0;
		
		case 0x13: // no_op_xx 
			return 0;
		
		case 0x14: // no_op_xx 
			return 0;
		
		case 0x15: // no_op_xx 
			return 0;
		
		case 0x16: // rl_xix   
			MemWriteByte(wAddr, Rl(MemReadByte(wAddr)));
			return 23;
		
		case 0x17: // no_op_xx 
			return 0;

		case 0x18: // no_op_xx 
			return 0;
		
		case 0x19: // no_op_xx 
			return 0;
		
		case 0x1A: // no_op_xx 
			return 0;
		
		case 0x1B: // no_op_xx 
			return 0;
		
		case 0x1C: // no_op_xx 
			return 0;
		
		case 0x1D: // no_op_xx 
			return 0;
		
		case 0x1E: // rr_xix   
			MemWriteByte(wAddr, Rr(MemReadByte(wAddr)));
			return 23;
		
		case 0x1F: // no_op_xx 
			return 0;

		case 0x20: // no_op_xx 
			return 0;
		
		case 0x21: // no_op_xx 
			return 0;
			
		case 0x22: // no_op_xx 
			return 0;
		
		case 0x23: // no_op_xx 
			return 0;
		
		case 0x24: // no_op_xx 
			return 0;
		
		case 0x25: // no_op_xx 
			return 0;
		
		case 0x26: // sla_xix  
			MemWriteByte(wAddr, Sla(MemReadByte(wAddr)));
			return 23;
		
		case 0x27: // no_op_xx 
			return 0;

		case 0x28: // no_op_xx 
			return 0;
		
		case 0x29: // no_op_xx 
			return 0;
		
		case 0x2A: // no_op_xx 
			return 0;
		
		case 0x2B: // no_op_xx 
			return 0;
		
		case 0x2C: // no_op_xx 
			return 0;
		
		case 0x2D: // no_op_xx 
			return 0;
		
		case 0x2E: // sra_xix  
			MemWriteByte(wAddr, Sra(MemReadByte(wAddr)));
			return 23;
		
		case 0x2F: // no_op_xx 
			return 0;

		case 0x30: // no_op_xx 
			return 0;
		
		case 0x31: // no_op_xx 
			return 0;
			
		case 0x32: // no_op_xx 
			return 0;
		
		case 0x33: // no_op_xx 
			return 0;
		
		case 0x34: // no_op_xx 
			return 0;
		
		case 0x35: // no_op_xx 
			return 0;
		
		case 0x36: // sll_xix  
			MemWriteByte(wAddr, Sll(MemReadByte(wAddr)));
			return 23;
		
		case 0x37: // no_op_xx 
			return 0;

		case 0x38: // no_op_xx 
			return 0;
		
		case 0x39: // no_op_xx 
			return 0;
		
		case 0x3A: // no_op_xx 
			return 0;
		
		case 0x3B: // no_op_xx 
			return 0;
		
		case 0x3C: // no_op_xx 
			return 0;
		
		case 0x3D: // no_op_xx 
			return 0;
		
		case 0x3E: // srl_xix  
			MemWriteByte(wAddr, Srl(MemReadByte(wAddr)));
			return 23;
		
		case 0x3F: // no_op_xx 
			return 0;

		case 0x40: // bit_0_xix
		case 0x41: // bit_0_xix
		case 0x42: // bit_0_xix
		case 0x43: // bit_0_xix
		case 0x44: // bit_0_xix
		case 0x45: // bit_0_xix
		case 0x46: // bit_0_xix
		case 0x47: // bit_0_xix
			Bit(MemReadByte(wAddr), 0);
			return 20;

		case 0x48: // bit_1_xix
		case 0x49: // bit_1_xix
		case 0x4A: // bit_1_xix
		case 0x4B: // bit_1_xix
		case 0x4C: // bit_1_xix
		case 0x4D: // bit_1_xix
		case 0x4E: // bit_1_xix
		case 0x4F: // bit_1_xix
			Bit(MemReadByte(wAddr), 1);
			return 20;

		case 0x50: // bit_2_xix
		case 0x51: // bit_2_xix
		case 0x52: // bit_2_xix
		case 0x53: // bit_2_xix
		case 0x54: // bit_2_xix
		case 0x55: // bit_2_xix
		case 0x56: // bit_2_xix
		case 0x57: // bit_2_xix
			Bit(MemReadByte(wAddr), 2);
			return 20;

		case 0x58: // bit_3_xix
		case 0x59: // bit_3_xix
		case 0x5A: // bit_3_xix
		case 0x5B: // bit_3_xix
		case 0x5C: // bit_3_xix
		case 0x5D: // bit_3_xix
		case 0x5E: // bit_3_xix
		case 0x5F: // bit_3_xix
			Bit(MemReadByte(wAddr), 3);
			return 20;

		case 0x60: // bit_4_xix
		case 0x61: // bit_4_xix
		case 0x62: // bit_4_xix
		case 0x63: // bit_4_xix
		case 0x64: // bit_4_xix
		case 0x65: // bit_4_xix
		case 0x66: // bit_4_xix
		case 0x67: // bit_4_xix
			Bit(MemReadByte(wAddr), 4);
			return 20;

		case 0x68: // bit_5_xix
		case 0x69: // bit_5_xix
		case 0x6A: // bit_5_xix
		case 0x6B: // bit_5_xix
		case 0x6C: // bit_5_xix
		case 0x6D: // bit_5_xix
		case 0x6E: // bit_5_xix
		case 0x6F: // bit_5_xix
			Bit(MemReadByte(wAddr), 5);
			return 20;

		case 0x70: // bit_6_xix
		case 0x71: // bit_6_xix
		case 0x72: // bit_6_xix
		case 0x73: // bit_6_xix
		case 0x74: // bit_6_xix
		case 0x75: // bit_6_xix
		case 0x76: // bit_6_xix
		case 0x77: // bit_6_xix
			Bit(MemReadByte(wAddr), 6);
			return 20;

		case 0x78: // bit_7_xix
		case 0x79: // bit_7_xix
		case 0x7A: // bit_7_xix
		case 0x7B: // bit_7_xix
		case 0x7C: // bit_7_xix
		case 0x7D: // bit_7_xix
		case 0x7E: // bit_7_xix
		case 0x7F: // bit_7_xix
			Bit(MemReadByte(wAddr), 7);
			return 20;

		case 0x80: // no_op_xx 
			return 0;
		
		case 0x81: // no_op_xx 
			return 0;
			
		case 0x82: // no_op_xx 
			return 0;
		
		case 0x83: // no_op_xx 
			return 0;
		
		case 0x84: // no_op_xx 
			return 0;
		
		case 0x85: // no_op_xx 
			return 0;
		
		case 0x86: // res_0_xix
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 0));
			return 23;
		
		case 0x87: // no_op_xx 
			return 0;

		case 0x88: // no_op_xx 
			return 0;
		
		case 0x89: // no_op_xx 
			return 0;
		
		case 0x8A: // no_op_xx 
			return 0;
		
		case 0x8B: // no_op_xx 
			return 0;
		
		case 0x8C: // no_op_xx 
			return 0;
		
		case 0x8D: // no_op_xx 
			return 0;
		
		case 0x8E: // res_1_xix
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 1));
			return 23;
		
		case 0x8F: // no_op_xx 
			return 0;

		case 0x90: // no_op_xx 
			return 0;
		
		case 0x91: // no_op_xx 
			return 0;
			
		case 0x92: // no_op_xx 
			return 0;
		
		case 0x93: // no_op_xx 
			return 0;
		
		case 0x94: // no_op_xx 
			return 0;
		
		case 0x95: // no_op_xx 
			return 0;
		
		case 0x96: // res_2_xix
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 2));
			return 23;
		
		case 0x97: // no_op_xx 
			return 0;

		case 0x98: // no_op_xx 
			return 0;
		
		case 0x99: // no_op_xx 
			return 0;
		
		case 0x9A: // no_op_xx 
			return 0;
		
		case 0x9B: // no_op_xx 
			return 0;
		
		case 0x9C: // no_op_xx 
			return 0;
		
		case 0x9D: // no_op_xx 
			return 0;
		
		case 0x9E: // res_3_xix
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 3));
			return 23;
		
		case 0x9F: // no_op_xx 
			return 0;

		case 0xA0: // no_op_xx 
			return 0;
		
		case 0xA1: // no_op_xx 
			return 0;
			
		case 0xA2: // no_op_xx 
			return 0;
		
		case 0xA3: // no_op_xx 
			return 0;
		
		case 0xA4: // no_op_xx 
			return 0;
		
		case 0xA5: // no_op_xx 
			return 0;
		
		case 0xA6: // res_4_xix
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 4));
			return 23;
		
		case 0xA7: // no_op_xx 
			return 0;

		case 0xA8: // no_op_xx 
			return 0;
		
		case 0xA9: // no_op_xx 
			return 0;
		
		case 0xAA: // no_op_xx 
			return 0;
		
		case 0xAB: // no_op_xx 
			return 0;
		
		case 0xAC: // no_op_xx 
			return 0;
		
		case 0xAD: // no_op_xx 
			return 0;
		
		case 0xAE: // res_5_xix
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 5));
			return 23;
		
		case 0xAF: // no_op_xx 
			return 0;

		case 0xB0: // no_op_xx 
			return 0;
		
		case 0xB1: // no_op_xx 
			return 0;
			
		case 0xB2: // no_op_xx 
			return 0;
		
		case 0xB3: // no_op_xx 
			return 0;
		
		case 0xB4: // no_op_xx 
			return 0;
		
		case 0xB5: // no_op_xx 
			return 0;
		
		case 0xB6: // res_6_xix
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 6));
			return 23;
		
		case 0xB7: // no_op_xx 
			return 0;

		case 0xB8: // no_op_xx 
			return 0;
		
		case 0xB9: // no_op_xx 
			return 0;
		
		case 0xBA: // no_op_xx 
			return 0;
		
		case 0xBB: // no_op_xx 
			return 0;
		
		case 0xBC: // no_op_xx 
			return 0;
		
		case 0xBD: // no_op_xx 
			return 0;
		
		case 0xBE: // res_7_xix
			MemWriteByte(wAddr, Res(MemReadByte(wAddr), 7));
			return 23;
		
		case 0xBF: // no_op_xx 
			return 0;

		case 0xC0: // no_op_xx 
			return 0;
		
		case 0xC1: // no_op_xx 
			return 0;
			
		case 0xC2: // no_op_xx 
			return 0;
		
		case 0xC3: // no_op_xx 
			return 0;
		
		case 0xC4: // no_op_xx 
			return 0;
		
		case 0xC5: // no_op_xx 
			return 0;
		
		case 0xC6: // set_0_xix
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 0));
			return 23;
		
		case 0xC7: // no_op_xx 
			return 0;

		case 0xC8: // no_op_xx 
			return 0;
		
		case 0xC9: // no_op_xx 
			return 0;
		
		case 0xCA: // no_op_xx 
			return 0;
		
		case 0xCB: // no_op_xx 
			return 0;
		
		case 0xCC: // no_op_xx 
			return 0;
		
		case 0xCD: // no_op_xx 
			return 0;
		
		case 0xCE: // set_1_xix
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 1));
			return 23;
		
		case 0xCF: // no_op_xx 
			return 0;

		case 0xD0: // no_op_xx 
			return 0;
		
		case 0xD1: // no_op_xx 
			return 0;
			
		case 0xD2: // no_op_xx 
			return 0;
		
		case 0xD3: // no_op_xx 
			return 0;
		
		case 0xD4: // no_op_xx 
			return 0;
		
		case 0xD5: // no_op_xx 
			return 0;
		
		case 0xD6: // set_2_xix
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 2));
			return 23;
		
		case 0xD7: // no_op_xx 
			return 0;

		case 0xD8: // no_op_xx 
			return 0;
		
		case 0xD9: // no_op_xx 
			return 0;
		
		case 0xDA: // no_op_xx 
			return 0;
		
		case 0xDB: // no_op_xx 
			return 0;
		
		case 0xDC: // no_op_xx 
			return 0;
		
		case 0xDD: // no_op_xx 
			return 0;
		
		case 0xDE: // set_3_xix
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 3));
			return 23;
		
		case 0xDF: // no_op_xx 
			return 0;

		case 0xE0: // no_op_xx 
			return 0;
		
		case 0xE1: // no_op_xx 
			return 0;
			
		case 0xE2: // no_op_xx 
			return 0;
		
		case 0xE3: // no_op_xx 
			return 0;
		
		case 0xE4: // no_op_xx 
			return 0;
		
		case 0xE5: // no_op_xx 
			return 0;
		
		case 0xE6: // set_4_xix
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 4));
			return 23;
		
		case 0xE7: // no_op_xx 
			return 0;

		case 0xE8: // no_op_xx 
			return 0;
		
		case 0xE9: // no_op_xx 
			return 0;
		
		case 0xEA: // no_op_xx 
			return 0;
		
		case 0xEB: // no_op_xx 
			return 0;
		
		case 0xEC: // no_op_xx 
			return 0;
		
		case 0xED: // no_op_xx 
			return 0;
		
		case 0xEE: // set_5_xix
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 5));
			return 23;
		
		case 0xEF: // no_op_xx 
			return 0;

		case 0xF0: // no_op_xx 
			return 0;
		
		case 0xF1: // no_op_xx 
			return 0;
			
		case 0xF2: // no_op_xx 
			return 0;
		
		case 0xF3: // no_op_xx 
			return 0;
		
		case 0xF4: // no_op_xx 
			return 0;
		
		case 0xF5: // no_op_xx 
			return 0;
		
		case 0xF6: // set_6_xix
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 6));
			return 23;
		
		case 0xF7: // no_op_xx 
			return 0;

		case 0xF8: // no_op_xx 
			return 0;
		
		case 0xF9: // no_op_xx 
			return 0;
		
		case 0xFA: // no_op_xx 
			return 0;
		
		case 0xFB: // no_op_xx 
			return 0;
		
		case 0xFC: // no_op_xx 
			return 0;
		
		case 0xFD: // no_op_xx 
			return 0;
		
		case 0xFE: // set_7_xix
			MemWriteByte(wAddr, Set(MemReadByte(wAddr), 7));
			return 23;
		
		case 0xFF: // no_op_x
			return 0;

		default:
			_ASSERT(FALSE);
			return 0;
	}
}

/* ***************************************************************************
 * Debugger shit
 * ***************************************************************************
 */

/*** Z80Em: Portable Z80 emulator *******************************************/
/***                                                                      ***/
/***                               Z80Dasm.h                              ***/
/***                                                                      ***/
/*** This file contains a routine to disassemble a buffer containing Z80  ***/
/*** opcodes. It is included from Z80Debug.c and z80dasm.c                ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#include <stdio.h>
#include <string.h>

static char *mnemonic_xx_cb[256]=
{
 "#","#","#","#","#","#","rlc Y"  ,"#",
 "#","#","#","#","#","#","rrc Y"  ,"#",
 "#","#","#","#","#","#","rl Y"   ,"#",
 "#","#","#","#","#","#","rr Y"   ,"#",
 "#","#","#","#","#","#","sla Y"  ,"#",
 "#","#","#","#","#","#","sra Y"  ,"#",
 "#","#","#","#","#","#","sll Y"  ,"#",
 "#","#","#","#","#","#","srl Y"  ,"#",
 "#","#","#","#","#","#","bit 0,Y","#",
 "#","#","#","#","#","#","bit 1,Y","#",
 "#","#","#","#","#","#","bit 2,Y","#",
 "#","#","#","#","#","#","bit 3,Y","#",
 "#","#","#","#","#","#","bit 4,Y","#",
 "#","#","#","#","#","#","bit 5,Y","#",
 "#","#","#","#","#","#","bit 6,Y","#",
 "#","#","#","#","#","#","bit 7,Y","#",
 "#","#","#","#","#","#","res 0,Y","#",
 "#","#","#","#","#","#","res 1,Y","#",
 "#","#","#","#","#","#","res 2,Y","#",
 "#","#","#","#","#","#","res 3,Y","#",
 "#","#","#","#","#","#","res 4,Y","#",
 "#","#","#","#","#","#","res 5,Y","#",
 "#","#","#","#","#","#","res 6,Y","#",
 "#","#","#","#","#","#","res 7,Y","#",
 "#","#","#","#","#","#","set 0,Y","#",
 "#","#","#","#","#","#","set 1,Y","#",
 "#","#","#","#","#","#","set 2,Y","#",
 "#","#","#","#","#","#","set 3,Y","#",
 "#","#","#","#","#","#","set 4,Y","#",
 "#","#","#","#","#","#","set 5,Y","#",
 "#","#","#","#","#","#","set 6,Y","#",
 "#","#","#","#","#","#","set 7,Y","#"
};

static char *mnemonic_cb[256]=
{
 "rlc b"  ,"rlc c"  ,"rlc d"  ,"rlc e"  ,"rlc h"  ,"rlc l"  ,"rlc (hl)"  ,"rlc a"  ,
 "rrc b"  ,"rrc c"  ,"rrc d"  ,"rrc e"  ,"rrc h"  ,"rrc l"  ,"rrc (hl)"  ,"rrc a"  ,
 "rl b"   ,"rl c"   ,"rl d"   ,"rl e"   ,"rl h"   ,"rl l"   ,"rl (hl)"   ,"rl a"   ,
 "rr b"   ,"rr c"   ,"rr d"   ,"rr e"   ,"rr h"   ,"rr l"   ,"rr (hl)"   ,"rr a"   ,
 "sla b"  ,"sla c"  ,"sla d"  ,"sla e"  ,"sla h"  ,"sla l"  ,"sla (hl)"  ,"sla a"  ,
 "sra b"  ,"sra c"  ,"sra d"  ,"sra e"  ,"sra h"  ,"sra l"  ,"sra (hl)"  ,"sra a"  ,
 "sll b"  ,"sll c"  ,"sll d"  ,"sll e"  ,"sll h"  ,"sll l"  ,"sll (hl)"  ,"sll a"  ,
 "srl b"  ,"srl c"  ,"srl d"  ,"srl e"  ,"srl h"  ,"srl l"  ,"srl (hl)"  ,"srl a"  ,
 "bit 0,b","bit 0,c","bit 0,d","bit 0,e","bit 0,h","bit 0,l","bit 0,(hl)","bit 0,a",
 "bit 1,b","bit 1,c","bit 1,d","bit 1,e","bit 1,h","bit 1,l","bit 1,(hl)","bit 1,a",
 "bit 2,b","bit 2,c","bit 2,d","bit 2,e","bit 2,h","bit 2,l","bit 2,(hl)","bit 2,a",
 "bit 3,b","bit 3,c","bit 3,d","bit 3,e","bit 3,h","bit 3,l","bit 3,(hl)","bit 3,a",
 "bit 4,b","bit 4,c","bit 4,d","bit 4,e","bit 4,h","bit 4,l","bit 4,(hl)","bit 4,a",
 "bit 5,b","bit 5,c","bit 5,d","bit 5,e","bit 5,h","bit 5,l","bit 5,(hl)","bit 5,a",
 "bit 6,b","bit 6,c","bit 6,d","bit 6,e","bit 6,h","bit 6,l","bit 6,(hl)","bit 6,a",
 "bit 7,b","bit 7,c","bit 7,d","bit 7,e","bit 7,h","bit 7,l","bit 7,(hl)","bit 7,a",
 "res 0,b","res 0,c","res 0,d","res 0,e","res 0,h","res 0,l","res 0,(hl)","res 0,a",
 "res 1,b","res 1,c","res 1,d","res 1,e","res 1,h","res 1,l","res 1,(hl)","res 1,a",
 "res 2,b","res 2,c","res 2,d","res 2,e","res 2,h","res 2,l","res 2,(hl)","res 2,a",
 "res 3,b","res 3,c","res 3,d","res 3,e","res 3,h","res 3,l","res 3,(hl)","res 3,a",
 "res 4,b","res 4,c","res 4,d","res 4,e","res 4,h","res 4,l","res 4,(hl)","res 4,a",
 "res 5,b","res 5,c","res 5,d","res 5,e","res 5,h","res 5,l","res 5,(hl)","res 5,a",
 "res 6,b","res 6,c","res 6,d","res 6,e","res 6,h","res 6,l","res 6,(hl)","res 6,a",
 "res 7,b","res 7,c","res 7,d","res 7,e","res 7,h","res 7,l","res 7,(hl)","res 7,a",
 "set 0,b","set 0,c","set 0,d","set 0,e","set 0,h","set 0,l","set 0,(hl)","set 0,a",
 "set 1,b","set 1,c","set 1,d","set 1,e","set 1,h","set 1,l","set 1,(hl)","set 1,a",
 "set 2,b","set 2,c","set 2,d","set 2,e","set 2,h","set 2,l","set 2,(hl)","set 2,a",
 "set 3,b","set 3,c","set 3,d","set 3,e","set 3,h","set 3,l","set 3,(hl)","set 3,a",
 "set 4,b","set 4,c","set 4,d","set 4,e","set 4,h","set 4,l","set 4,(hl)","set 4,a",
 "set 5,b","set 5,c","set 5,d","set 5,e","set 5,h","set 5,l","set 5,(hl)","set 5,a",
 "set 6,b","set 6,c","set 6,d","set 6,e","set 6,h","set 6,l","set 6,(hl)","set 6,a",
 "set 7,b","set 7,c","set 7,d","set 7,e","set 7,h","set 7,l","set 7,(hl)","set 7,a"
};

static char *mnemonic_ed[256]=
{
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "in b,(c)","out (c),b","sbc hl,bc","ld (W),bc","neg","retn","im 0","ld i,a",
 "in c,(c)","out (c),c","adc hl,bc","ld bc,(W)","!"  ,"reti","!"   ,"ld r,a",
 "in d,(c)","out (c),d","sbc hl,de","ld (W),de","!"  ,"!"   ,"im 1","ld a,i",
 "in e,(c)","out (c),e","adc hl,de","ld de,(W)","!"  ,"!"   ,"im 2","ld a,r",
 "in h,(c)","out (c),h","sbc hl,hl","ld (W),hl","!"  ,"!"   ,"!"   ,"rrd"   ,
 "in l,(c)","out (c),l","adc hl,hl","ld hl,(W)","!"  ,"!"   ,"!"   ,"rld"   ,
 "in 0,(c)","out (c),0","sbc hl,sp","ld (W),sp","!"  ,"!"   ,"!"   ,"!"     ,
 "in a,(c)","out (c),a","adc hl,sp","ld sp,(W)","!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "ldi"     ,"cpi"      ,"ini"      ,"outi"     ,"!"  ,"!"   ,"!"   ,"!"     ,
 "ldd"     ,"cpd"      ,"ind"      ,"outd"     ,"!"  ,"!"   ,"!"   ,"!"     ,
 "ldir"    ,"cpir"     ,"inir"     ,"otir"     ,"!"  ,"!"   ,"!"   ,"!"     ,
 "lddr"    ,"cpdr"     ,"indr"     ,"otdr"     ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"     ,
 "!"       ,"!"        ,"!"        ,"!"        ,"!"  ,"!"   ,"!"   ,"!"
};

static char *mnemonic_xx[256]=
{
  "@"      ,"@"       ,"@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"add I,bc","@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"add I,de","@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"ld I,W"  ,"ld (W),I","inc I"    ,"inc Ih"  ,"dec Ih"  ,"ld Ih,B","@"      ,
  "@"      ,"add I,I" ,"ld I,(W)","dec I"    ,"inc Il"  ,"dec Il"  ,"ld Il,B","@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"inc X"   ,"dec X"   ,"ld X,B" ,"@"      ,
  "@"      ,"add I,sp","@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"ld b,Ih" ,"ld b,Il" ,"ld b,X" ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"ld c,Ih" ,"ld c,Il" ,"ld c,X" ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"ld d,Ih" ,"ld d,Il" ,"ld d,X" ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"ld e,Ih" ,"ld e,Il" ,"ld e,X" ,"@"      ,
  "ld Ih,b","ld Ih,c" ,"ld Ih,d" ,"ld Ih,e"  ,"ld Ih,h" ,"ld Ih,l" ,"ld h,X" ,"ld Ih,a",
  "ld Il,b","ld Il,c" ,"ld Il,d" ,"ld Il,e"  ,"ld Il,h" ,"ld Il,l" ,"ld l,X" ,"ld Il,a",
  "ld X,b" ,"ld X,c"  ,"ld X,d"  ,"ld X,e"   ,"ld X,h"  ,"ld X,l"  ,"@"      ,"ld X,a" ,
  "@"      ,"@"       ,"@"       ,"@"        ,"ld a,Ih" ,"ld a,Il" ,"ld a,X" ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"add a,Ih","add a,Il","add a,X","@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"adc a,Ih","adc a,Il","adc a,X","@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"sub Ih"  ,"sub Il"  ,"sub X"  ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"sbc a,Ih","sbc a,Il","sbc a,X","@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"and Ih"  ,"and Il"  ,"and X"  ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"xor Ih"  ,"xor Il"  ,"xor X"  ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"or Ih"   ,"or Il"   ,"or X"   ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"cp Ih"   ,"cp Il"   ,"cp X"   ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"@"       ,"@"       ,"fd cb"    ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"pop I"   ,"@"       ,"ex (sp),I","@"       ,"push I"  ,"@"      ,"@"      ,
  "@"      ,"jp (I)"  ,"@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"@"       ,"@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"      ,
  "@"      ,"ld sp,I" ,"@"       ,"@"        ,"@"       ,"@"       ,"@"      ,"@"
};

static char *mnemonic_main[256]=
{
 "nop"      ,"ld bc,W"  ,"ld (bc),a","inc bc"    ,"inc b"    ,"dec b"    ,"ld b,B"    ,"rlca"     ,
 "ex af,af'","add hl,bc","ld a,(bc)","dec bc"    ,"inc c"    ,"dec c"    ,"ld c,B"    ,"rrca"     ,
 "djnz R"   ,"ld de,W"  ,"ld (de),a","inc de"    ,"inc d"    ,"dec d"    ,"ld d,B"    ,"rla"      ,
 "jr R"     ,"add hl,de","ld a,(de)","dec de"    ,"inc e"    ,"dec e"    ,"ld e,B"    ,"rra"      ,
 "jr nz,R"  ,"ld hl,W"  ,"ld (W),hl","inc hl"    ,"inc h"    ,"dec h"    ,"ld h,B"    ,"daa"      ,
 "jr z,R"   ,"add hl,hl","ld hl,(W)","dec hl"    ,"inc l"    ,"dec l"    ,"ld l,B"    ,"cpl"      ,
 "jr nc,R"  ,"ld sp,W"  ,"ld (W),a" ,"inc sp"    ,"inc (hl)" ,"dec (hl)" ,"ld (hl),B" ,"scf"      ,
 "jr c,R"   ,"add hl,sp","ld a,(W)" ,"dec sp"    ,"inc a"    ,"dec a"    ,"ld a,B"    ,"ccf"      ,
 "ld b,b"   ,"ld b,c"   ,"ld b,d"   ,"ld b,e"    ,"ld b,h"   ,"ld b,l"   ,"ld b,(hl)" ,"ld b,a"   ,
 "ld c,b"   ,"ld c,c"   ,"ld c,d"   ,"ld c,e"    ,"ld c,h"   ,"ld c,l"   ,"ld c,(hl)" ,"ld c,a"   ,
 "ld d,b"   ,"ld d,c"   ,"ld d,d"   ,"ld d,e"    ,"ld d,h"   ,"ld d,l"   ,"ld d,(hl)" ,"ld d,a"   ,
 "ld e,b"   ,"ld e,c"   ,"ld e,d"   ,"ld e,e"    ,"ld e,h"   ,"ld e,l"   ,"ld e,(hl)" ,"ld e,a"   ,
 "ld h,b"   ,"ld h,c"   ,"ld h,d"   ,"ld h,e"    ,"ld h,h"   ,"ld h,l"   ,"ld h,(hl)" ,"ld h,a"   ,
 "ld l,b"   ,"ld l,c"   ,"ld l,d"   ,"ld l,e"    ,"ld l,h"   ,"ld l,l"   ,"ld l,(hl)" ,"ld l,a"   ,
 "ld (hl),b","ld (hl),c","ld (hl),d","ld (hl),e" ,"ld (hl),h","ld (hl),l","halt"      ,"ld (hl),a",
 "ld a,b"   ,"ld a,c"   ,"ld a,d"   ,"ld a,e"    ,"ld a,h"   ,"ld a,l"   ,"ld a,(hl)" ,"ld a,a"   ,
 "add a,b"  ,"add a,c"  ,"add a,d"  ,"add a,e"   ,"add a,h"  ,"add a,l"  ,"add a,(hl)","add a,a"  ,
 "adc a,b"  ,"adc a,c"  ,"adc a,d"  ,"adc a,e"   ,"adc a,h"  ,"adc a,l"  ,"adc a,(hl)","adc a,a"  ,
 "sub b"    ,"sub c"    ,"sub d"    ,"sub e"     ,"sub h"    ,"sub l"    ,"sub (hl)"  ,"sub a"    ,
 "sbc a,b"  ,"sbc a,c"  ,"sbc a,d"  ,"sbc a,e"   ,"sbc a,h"  ,"sbc a,l"  ,"sbc a,(hl)","sbc a,a"  ,
 "and b"    ,"and c"    ,"and d"    ,"and e"     ,"and h"    ,"and l"    ,"and (hl)"  ,"and a"    ,
 "xor b"    ,"xor c"    ,"xor d"    ,"xor e"     ,"xor h"    ,"xor l"    ,"xor (hl)"  ,"xor a"    ,
 "or b"     ,"or c"     ,"or d"     ,"or e"      ,"or h"     ,"or l"     ,"or (hl)"   ,"or a"     ,
 "cp b"     ,"cp c"     ,"cp d"     ,"cp e"      ,"cp h"     ,"cp l"     ,"cp (hl)"   ,"cp a"     ,
 "ret nz"   ,"pop bc"   ,"jp nz,W"  ,"jp W"      ,"call nz,W","push bc"  ,"add a,B"   ,"rst 00h"  ,
 "ret z"    ,"ret"      ,"jp z,W"   ,"cb"        ,"call z,W" ,"call W"   ,"adc a,B"   ,"rst 08h"  ,
 "ret nc"   ,"pop de"   ,"jp nc,W"  ,"out (B),a" ,"call nc,W","push de"  ,"sub B"     ,"rst 10h"  ,
 "ret c"    ,"exx"      ,"jp c,W"   ,"in a,(B)"  ,"call c,W" ,"dd"       ,"sbc a,B"   ,"rst 18h"  ,
 "ret po"   ,"pop hl"   ,"jp po,W"  ,"ex (sp),hl","call po,W","push hl"  ,"and B"     ,"rst 20h"  ,
 "ret pe"   ,"jp (hl)"  ,"jp pe,W"  ,"ex de,hl"  ,"call pe,W","ed"       ,"xor B"     ,"rst 28h"  ,
 "ret p"    ,"pop af"   ,"jp p,W"   ,"di"        ,"call p,W" ,"push af"  ,"or B"      ,"rst 30h"  ,
 "ret m"    ,"ld sp,hl" ,"jp m,W"   ,"ei"        ,"call m,W" ,"fd"       ,"cp B"      ,"rst 38h"
};

static char Sign (unsigned char a)
{
 return (a&128)? '-':'+';
}

static int Abs (unsigned char a)
{
 if (a&128) return 256-a;
 else return a;
}

/****************************************************************************/
/* Disassemble first opcode in buffer and return number of bytes it takes   */
/****************************************************************************/
int Z80_Dasm (unsigned char *buffer,char *dest,unsigned PC) {
 char *S;
 char *r;
 int i,j,k;
 unsigned char buf[10];
 char Offset;
 i=Offset=0;
 r="INTERNAL PROGRAM ERROR";
 dest[0]='\0';
 switch (buffer[i])
 {
  case 0xCB:
   i++;
   S=mnemonic_cb[buffer[i++]];
   break;
  case 0xED:
   i++;
   S=mnemonic_ed[buffer[i++]];
   break;
  case 0xDD:
   i++;
   r="ix";
   switch (buffer[i])
   {
    case 0xcb:
     i++;
     Offset=buffer[i++];
     S=mnemonic_xx_cb[buffer[i++]];
     break;
    default:
     S=mnemonic_xx[buffer[i++]];
     break;
   }
   break;
  case 0xFD:
   i++;
   r="iy";
   switch (buffer[i])
   {
    case 0xcb:
     i++;
     Offset=buffer[i++];
     S=mnemonic_xx_cb[buffer[i++]];
     break;
    default:
     S=mnemonic_xx[buffer[i++]];
     break;
   }
   break;
  default:
   S=mnemonic_main[buffer[i++]];
   break;
 }
 for (j=0;S[j];++j)
 {
  switch (S[j])
  {
   case 'B':
    sprintf (buf,"$%02x",buffer[i++]);
    strcat (dest,buf);
    break;
   case 'R':
    sprintf (buf,"$%04x",(PC+2+(signed char)buffer[i])&0xFFFF);
    i++;
    strcat (dest,buf);
    break;
   case 'W':
    sprintf (buf,"$%04x",buffer[i]+buffer[i+1]*256);
    i+=2;
    strcat (dest,buf);
    break;
   case 'X':
    sprintf (buf,"(%s%c$%02x)",r,Sign(buffer[i]),Abs(buffer[i]));
    i++;
    strcat (dest,buf);
    break;
   case 'Y':
    sprintf (buf,"(%s%c$%02x)",r,Sign(Offset),Abs(Offset));
    strcat (dest,buf);
    break;
   case 'I':
    strcat (dest,r);
    break;
   case '!':
    sprintf (dest,"db     $ed,$%02x",buffer[1]);
    return 2;
   case '@':
    sprintf (dest,"db     $%02x",buffer[0]);
    return 1;
   case '#':
    sprintf (dest,"db     $%02x,$cb,$%02x",buffer[0],buffer[2]);
    return 2;
   case ' ':
    k=strlen(dest);
    if (k<6) k=7-k;
    else k=1;
    memset (buf,' ',k);
    buf[k]='\0';
    strcat (dest,buf);
    break;
   default:
    buf[0]=S[j];
    buf[1]='\0';
    strcat (dest,buf);
    break;
  }
 }
 return i;
}

UINT32 mz80GetElapsedTicks(UINT32 dwClearIt)
{
	UINT32 dwTempVar = 0;
	char string[150];

	dwTempVar = dwElapsedTicks;
	if (dwClearIt)
		dwElapsedTicks = 0;
	return(dwTempVar);
}

void mz80ReleaseTimeslice()
{
	cCycles = 0;
}
