
/* extra declarations for z80stb.c to make life easier.
 */

#ifndef h_z80stb_h
#define h_z80stb_h

/* RC hack */
#define DWORD           UINT32
#define WORD            UINT16
#define BYTE            UINT8
#define LPBYTE          UINT8*
#define ULONG           unsigned long
#define BOOL            UINT8

/* accessors */
#define LOWORD(x)       ( x & 0xFFFF )
#define HIWORD(x)       ( x >> 16 )
#define LOBYTE(x)       ( x & 0xFF )
#define HIBYTE(x)       ( x >> 8 )
#define MAKEWORD(lo,hi) ( lo | ( hi << 8 ) )

/* execz80.h */
#define S_FLAG          0x80
#define Z_FLAG          0x40
#define H_FLAG          0x10
#define V_FLAG          0x04
#define N_FLAG          0x02
#define C_FLAG          0x01

typedef struct {
  WORD AF,BC,DE,HL,IX,IY,PC,SP;
  WORD AF2,BC2,DE2,HL2;
  unsigned IFF1,IFF2,HALT,IM,I,R,R2;
} EXECZ80_CONTEXT;

// typedef BYTE (*EXECZ80_MEMREAD_PROC)(WORD);
// typedef void (*EXECZ80_MEMWRITE_PROC)(WORD, BYTE);
// typedef BYTE (*EXECZ80_PORTREAD_PROC)(BYTE);
// typedef void (*EXECZ80_PORTWRITE_PROC)(BYTE, BYTE);

// #define Z80_END_TABLE     ((EXECZ80_MEMREAD_PROC) 0xFFFFFFFF)
// #define Z80_IGNORE_READ   ((EXECZ80_MEMREAD_PROC) 0xFFFFFFFE)
// #define Z80_IGNORE_WRITE  ((EXECZ80_MEMWRITE_PROC)0xFFFFFFFE)
// #define Z80_ILLEGAL_READ  ((EXECZ80_MEMREAD_PROC) 0xFFFFFFFD)
// #define Z80_ILLEGAL_WRITE ((EXECZ80_MEMWRITE_PROC)0xFFFFFFFD)

// typedef struct {
//   DWORD dwAddrMin;
//   DWORD dwAddrMax;
//   DWORD dwFlags;
//   EXECZ80_MEMREAD_PROC pfnMemRead;
//   EXECZ80_MEMWRITE_PROC pfnMemWrite;
// } EXECZ80_MEM_DESCRIPTOR;

// typedef struct {
//   DWORD dwFlags;
//   LPBYTE rgbMemory;
//   const BYTE *rgbInstructions;
//   EXECZ80_MEMREAD_PROC pfnMemRead;
//   EXECZ80_MEMWRITE_PROC pfnMemWrite;
//   EXECZ80_PORTREAD_PROC pfnPortRead;
//   EXECZ80_PORTWRITE_PROC pfnPortWrite;
//   const EXECZ80_MEM_DESCRIPTOR *pmd;
// } EXECZ80_CREATE;

/* execz80i.h */
#define ARITH_TABLES

/* int Create ( EXECZ80_CREATE &cs, IExecZ80 *&pExec ); */
/* int CExecZ80 ( EXECZ80_CREATE &cs ); */
/* int ~CExecZ80(); */

int Irq(BYTE bVal);
int Nmi(void);
int Exec(int cCyclesArg);

/* inline */ BYTE ImmedByte(void);
/* inline */ WORD ImmedWord(void);
/* inline */ WORD GetPC(void);
/* inline */ void SetPC(WORD wAddr);
/* inline */ void AdjustPC(signed char cb);

/* inline */ void Push(WORD wArg);
/* inline */ WORD Pop(void);
/* inline */ WORD GetSP(void);
/* inline */ void SetSP(WORD wAddr);
	
/* inline */ int Jr0(int f);
/* inline */ int Jr1(int f);
/* inline */ int Call0(int f);
/* inline */ int Call1(int f);
/* inline */ int Ret0(int f);
/* inline */ int Ret1(int f);
/* inline */ int Jp0(int f);
/* inline */ int Jp1(int f);
	
/* inline */ void Rst(WORD wAddr);
/* inline */ WORD MemReadWord(WORD wAddr);
/* inline */ BYTE MemReadByte(WORD wAddr);
/* inline */ void MemWriteByte(WORD wAddr, BYTE bVal);
/* inline */ void MemWriteWord(WORD wAddr, WORD wVal);
WORD CallbackMemReadWord(WORD wAddr);
BYTE CallbackMemReadByte(WORD wAddr);
void CallbackMemWriteByte(WORD wAddr, BYTE bVal);
void CallbackMemWriteWord(WORD wAddr, WORD wVal);
	
/* inline */ void Add_1(BYTE b);
/* inline */ void Adc_1(BYTE b);
/* inline */ void Sub(BYTE b);
/* inline */ void Sbc_1(BYTE b);
/* inline */ void And(BYTE b);
/* inline */ void Or(BYTE b);
/* inline */ void Xor(BYTE b);
/* inline */ void Cp(BYTE b);
/* inline */ BYTE Set(BYTE b,int);
/* inline */ BYTE Res(BYTE b,int);
/* inline */ void Bit(BYTE b,int);
/* inline */ void Rlca(void);
/* inline */ void Rrca(void);
/* inline */ void Rla(void);
/* inline */ void Rra(void);
/* inline */ BYTE Rlc(BYTE b);
/* inline */ BYTE Rrc(BYTE b);
/* inline */ BYTE Rl(BYTE b);
/* inline */ BYTE Rr(BYTE b);
/* inline */ BYTE Sll(BYTE b);
/* inline */ BYTE Srl(BYTE b);
/* inline */ BYTE Sla(BYTE b);
/* inline */ BYTE Sra(BYTE b);
/* inline */ BYTE Inc(BYTE b);
/* inline */ BYTE Dec(BYTE b);

/* inline */ WORD Add_2(WORD wArg1, WORD wArg2);
/* inline */ WORD Adc_2(WORD wArg1, WORD wArg2);
/* inline */ WORD Sbc_2(WORD wArg1, WORD wArg2);

int Ei(void);
void Di(void);

int HandleCB(void);
int HandleDD(void);
int HandleED(int cCycles);
int HandleFD(void);
int HandleDDCB(void);
int HandleFDCB(void);

void Out(BYTE bPort, BYTE bVal);
BYTE In(BYTE bPort);

void Exx(void);

/* inline */ WORD IndirectIX(void);
/* inline */ WORD IndirectIY(void);

void Dump(void);

extern BYTE ZSTable[256];
extern BYTE ZSPTable[256];
extern short DAATable[2048];
extern BYTE rgfAdd[256][256];
extern BYTE rgfAddc[256][256][2];
extern BYTE rgfSub[256][256];
extern BYTE rgfSubc[256][256][2];
extern BYTE rgfInc[256];
extern BYTE rgfDec[256];
extern BYTE rgfBit[256][8];

#define ISF_PENDING   0x00000008
#define ISF_MODE_MASK 0x00000003
#define ISF_HALT      0x00000004
#define ISF_IFF1      0x00000010
#define ISF_IFF2      0x00000020
#define ISF_IREG_MASK 0x00000F00

#endif
