/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* compile-arm.c                                                             */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <sys/swis.h>
#include <sys/os.h>

#include <generator.h>
#include <cpu68k.h>
#include <mem68k.h>

#define MAXBLOCKSIZE 10240
#define true 1
#define false 0

/* static global variables */

static uint8 *curpos;
static sint32 regstate[6]; /* 0 = unused, 1 = used, 2 = updated */
static sint32 regnum[6];   /* register number or -1 */
static sint32 regtimer[6]; /* instructions since used */

/* forward references */

inline uint8 *compile_w(uint32 i)
{
  *(uint32 *)curpos = i;
  curpos+= 4;
  return curpos;
}

/*** compile_initregs - set all registers to unused ***/

void compile_initregs(void)
{
  int i;

  for (i = 1; i < 6; i++) {
    regstate[i] = 0;
    regnum[i] = -1;
    regtimer[i] = 0;
  }
}

/*** compile_getreg - given a 68000 register or -1 return an ARM register ***/

int compile_getreg(int reg)
{
  int i;
  int free = -1;
  int reg_last = -1;
  int reg_lasttime = -1;

  /* check for an ARM register already holding the 68000 register or a free
     ARM register if we can't find it */

  for (i = 1; i < 6; i++) {
    if (reg != -1 && regstate[i] && regnum[i] == reg) {
      regtimer[i] = 0;
      return i;
    }
    if (!regstate[i]) {
      free = i;
    }
  }
  if (free == -1) {
    /* no free registers, find a register not holding a specific 68000
       register if pos - if we can't find one then use the oldest 68000
       register cached even if it was used this instruction */

    for (i = 1; i < 6; i++) {
      if (regtimer[i] && regnum[i] == -1) {
	free = i;
      } else {
	if (regtimer[i] >= reg_lasttime) {
	  /* we always prefer higher registers */
	  reg_last = i;
	  reg_lasttime = regtimer[i];
	}
      }
    }
    if (free == -1) {
      free = reg_last;
      if (free == -1) {
	fprintf(stderr, "free: %d\n", free);
	fprintf(stderr, "reg_last: %d\n", reg_last);
	fprintf(stderr, "reg_lasttime: %d\n", reg_lasttime);
	fprintf(stderr, "5 state: %d\n", regstate[5]);
	fprintf(stderr, "5 num: %d\n", regnum[5]);
	fprintf(stderr, "5 timer: %d\n", regtimer[5]);
	compile_w(0);
	compile_w((uint32)(-1));
	compile_finalregs();
	fprintf(stderr, "5 state: %d\n", regstate[5]);
	fprintf(stderr, "5 num: %d\n", regnum[5]);
	fprintf(stderr, "5 timer: %d\n", regtimer[5]);
	fprintf(stderr, "Insufficient registers\n");
	exit(1);
      }
      if (regstate[reg_last] == 2) {
	/* STR r(reg_last),[r8,#x] */
	compile_w(0xE5880000 | (regnum[reg_last]*4) | reg_last<<12);
      }
    }
  }
  regstate[free] = 1;
  regnum[free] = reg;
  regtimer[free] = 0;
  if (reg != -1) {
    compile_w(0xE5980000 | (reg*4) | free<<12); /* LDR rfree,[r8,#x] */
  }
  return free;
}

/*** compile_findreg - given a 68000 register return an ARM register ***/

int compile_findreg(int reg)
{
  int i;

  for (i = 1; i < 6; i++) {
    if (regstate[i] && regnum[i] == reg) {
      return i;
    }
  }
  return -1;
}

/*** compile_updatereg - we have changed a register ***/

void compile_updatereg(int reg)
{
  if (!regstate[reg]) {
    fprintf(stderr, "Compiler error - updatereg %d\n", reg);
    exit(1);
  }
  regstate[reg] = 2;
}

/*** compile_inctimer - update timers on registers ***/

void compile_inctimer(void) {
  int i;

  for (i = 1; i < 6; i++) {
    if (regstate[i]) {
      if (regnum[i] == -1)
	regstate[i] = 0;
      else
	regtimer[i]++;
    }
  }
}

/*** compile_finalregs - write back all changed registers ***/

void compile_finalregs(void) {
  int i;

  for (i = 1; i < 6; i++) {
    if (regstate[i] == 2 && regnum[i] != -1) {
      compile_w(0xE5880000 | (regnum[i]*4) | i<<12); /* STR ri,[r8,#x] */
    }
    regstate[i] = 0;
    regnum[i] = -1;
    regtimer[i] = 0;
  }
}

void compile_ea(t_iib *iib, t_ipc *ipc, t_type type, int update, int *reg)
{
  t_datatype datatype = type ? iib->dtype : iib->stype;
  int reg68;
  int a, b, c, pos;

  /* compile EA to register(s) regea/regval */

  switch(datatype) {
  case dt_Dreg:
  case dt_Areg:
    *reg = -1;
    break;
  case dt_Aind:
    reg68 = 8 + ((ipc->opcode >> (type ? iib->dbitpos : iib->sbitpos)) & 7);
    a = compile_getreg(reg68);
    b = compile_getreg(-1); /* effective address */
    compile_w(0xE1A00000 | b<<12 | a); /* MOV rb,ra */
    *reg = b;
    break;
  case dt_Ainc:
    reg68 = 8 + ((ipc->opcode >> (type ? iib->dbitpos : iib->sbitpos)) & 7);
    a = compile_getreg(reg68); /* 68000 register */
    if (update) {
      b = compile_getreg(-1);       /* effective address */
      regnum[a] = -1;
      regnum[b] = reg68; /* swap */
      /* ADD rb,ra,#size */
      compile_w(0xE2800000 | 1<<(iib->size-1) | a<<16 | b<<12);
      compile_updatereg(b);
    }
    *reg = a;
    break;
  case dt_Adec:
    reg68 = 8 + ((ipc->opcode >> (type ? iib->dbitpos : iib->sbitpos)) & 7);
    a = compile_getreg(reg68); /* 68000 register */
    if (update) {
      /* SUB ra,ra,#size */
      compile_w(0xE2400000 | 1<<(iib->size-1) | a<<16 | a<<12);
      compile_updatereg(a);
    }
    *reg = a;
    break;
  case dt_Adis:
    reg68 = 8 + ((ipc->opcode >> (type ? iib->dbitpos : iib->sbitpos)) & 7);
    a = compile_getreg(reg68);    /* 68000 register */
    b = compile_getreg(-1);       /* effective address */
    pos = (uint32)(type ? &ipc->dst : &ipc->src) - (uint32)ipc;
    /* LDR rb,[r6,#pos] ADD rb,rb,ra */
    compile_w(0xE5960000 | pos | b<<12);
    compile_w(0xE0800000 | b<<16 | b<<12 | a);
    *reg = b;
    break;
  case dt_Aidx:
    reg68 = 8 + ((ipc->opcode >> (type ? iib->dbitpos : iib->sbitpos)) & 7);
    b = compile_getreg(-1);       /* effective address */
    c = compile_getreg(((type ? ipc->dst : ipc->src)>>28) & 15); /* idx reg */
    pos = (uint32)(type ? &ipc->dst : &ipc->src) - (uint32)ipc;
    /* LDR rb,[r6,#pos] MOV rb,rb,LSL#8 MOV rb,rb,ASR#8 */
    compile_w(0xE5960000 | pos | b<<12);
    compile_w(0xE1A00400 | b<<12 | b);
    compile_w(0xE1A00440 | b<<12 | b);
    if ((ipc->src>>27) & 1) {
      /* ADD rb,rb,rc */
      compile_w(0xE0800000 | b<<16 | b<<12 | c);
    } else {
      /* MOV r14,rc,LSL#16 MOV r14,r14,ASR#16 ADD rb,rb,r14 */
      compile_w(0xE1A0E800 | c);
      compile_w(0xE1A0E84E);
      compile_w(0xE080000E | b<<16 | b<<12);
    }
    a = compile_getreg(reg68);    /* 68000 register */
    /* ADD rb,rb,ra */
    compile_w(0xE0800000 | b<<16 | b<<12 | a);
    *reg = b;
    break;
  case dt_AbsW:
  case dt_AbsL:
  case dt_Pdis:
    a = compile_getreg(-1); /* effective address */
    pos = (uint32)(type ? &ipc->dst : &ipc->src) - (uint32)ipc;
    /* LDR ra,[r6,#pos] */
    compile_w(0xE5960000 | pos | a<<12);
    *reg = a;
    break;
  case dt_Pidx:
    b = compile_getreg(-1);       /* effective address */
    c = compile_getreg(((type ? ipc->dst : ipc->src)>>28) & 15); /* idx reg */
    pos = (uint32)(type ? &ipc->dst : &ipc->src) - (uint32)ipc;
    /* LDR rb,[r6,#pos] MOV rb,rb,LSL#8 MOV rb,rb,ASL#8 */
    compile_w(0xE5960000 | pos | b<<12);
    compile_w(0xE1A00400 | b<<12 | b);
    compile_w(0xE1A00440 | b<<12 | b);
    if ((ipc->src>>27) & 1) {
      /* ADD rb,rb,rc */
      compile_w(0xE0800000 | b<<16 | b<<12 | c);
    } else {
      /* MOV r14,rc,LSL#16 MOV r14,r14,ASR#16 ADD rb,rb,r14 */
      compile_w(0xE1A0E800 | c);
      compile_w(0xE1A0E84E);
      compile_w(0xE080000E | b<<16 | b<<12);
    }
    *reg = b;
    break;
  case dt_ImmB:
  case dt_ImmW:
  case dt_ImmL:
  case dt_ImmS:
  case dt_Imm3:
  case dt_Imm4:
  case dt_Imm8:
  case dt_Imm8s:
    *reg = -1;
    break;
  default:
    fprintf(stderr, "Invalid datatype %d\n", datatype);
    exit(1);
  }
}

void compile_eaval(t_iib *iib, t_ipc *ipc, t_type type, int update,
		    int eareg, int *eaval)
{
  t_datatype datatype = type ? iib->dtype : iib->stype;
  int reg68;
  int a, b, pos;

  /* compile EA value */

  switch(datatype) {
  case dt_Dreg:
  case dt_Areg:
    reg68 = ((datatype == dt_Areg ? 8 : 0) +
	     ((ipc->opcode >> (type ? iib->dbitpos : iib->sbitpos)) & 7));
    a = compile_getreg(reg68);
    b = compile_getreg(-1);
    switch(iib->size) {
    case sz_byte:
      compile_w(0xE20000FF | b<<12 | a<<16); /* AND rb,ra,#&FF */
      break;
    case sz_word:
      compile_w(0xE1A00800 | b<<12 | a); /* MOV rb,ra,LSL#16 */
      compile_w(0xE1A00820 | b<<12 | b); /* MOV rb,rb,LSR#16 */
      break;
    case sz_long:
      compile_w(0xE1A00000 | b<<12 | a); /* MOV rb,ra */
      break;
    default:
      fprintf(stderr, "Invalid size %d\n", iib->size);
      exit(1);
    }
    *eaval = b;
    break;
  case dt_Aind:
  case dt_Ainc:
  case dt_Adec:
  case dt_Adis:
  case dt_Aidx:
  case dt_AbsW:
  case dt_AbsL:
  case dt_Pdis:
  case dt_Pidx:
    a = compile_getreg(-1);
    compile_w(0xE92D100E);             /* STMFD r13!,{r1-r3,r12} */
    compile_w(0xE3C004FF | eareg<<16); /* BIC r0,eareg,#&FF000000 */
    compile_w(0xE1A03620);             /* MOV r3,r0,LSR#12 */
    compile_w(0xE59F2004);             /* LDR r2,[PC,#4] */
    compile_w(0xE28FE004);             /* ADD r14,PC,#4 */
    compile_w(0xE792F103);             /* LDR PC,[r2,r3,LSL#2]; */
    /* EQUD <array address> */
    switch (iib->size) {
    case sz_byte:
      compile_w((uint32)mem68k_fetch_byte);
      break;
    case sz_word:
      compile_w((uint32)mem68k_fetch_word);
      break;
    case sz_long:
      compile_w((uint32)mem68k_fetch_long);
      break;
    default:
      fprintf(stderr, "Invalid size %d\n", iib->size);
      exit(1);
    }
    compile_w(0xE8BD100E);         /* LDMFD r13!,{r1-r3,r12} */
    compile_w(0xE1A00000 | a<<12); /* MOV ra, r0 */
    *eaval = a;
    break;
  case dt_ImmB:
    a = compile_getreg(-1);
    /* MOV eaval,#data */
    compile_w(0xE3A00000 | (type ? ipc->dst : ipc->src) | a<<12);
    *eaval = a;
    break;
  case dt_ImmW:
  case dt_ImmL:
  case dt_Imm3: /* I want optimising */
  case dt_Imm4:
  case dt_Imm8:
  case dt_Imm8s:
    a = compile_getreg(-1);
    pos = (uint32)(type ? &ipc->dst : &ipc->src) - (uint32)ipc;
    /* LDR eaval,[r6,#pos] */
    compile_w(0xE5960000 | pos | a<<12);
    *eaval = a;
    break;
  case dt_ImmS:
    a = compile_getreg(-1);
    /* MOV a,#val */
    compile_w(0xE3A00000 | iib->immvalue | a<<12);
    *eaval = a;
    break;
  default:
    fprintf(stderr, "Invalid datatype %d\n", datatype);
    exit(1);
  }
}

void compile_eastore(t_iib *iib, t_ipc *ipc, t_type type, int eareg,
		     int eaval)
{
  t_datatype datatype = type ? iib->dtype : iib->stype;
  int reg68;
  int a, b, pos;

  /* store EA value */

  switch(datatype) {
  case dt_Dreg:
  case dt_Areg:
    reg68 = ((datatype == dt_Areg ? 8 : 0) +
	     ((ipc->opcode >> (type ? iib->dbitpos : iib->sbitpos)) & 7));
    a = compile_findreg(reg68);
    if (a == eaval) {
      fprintf(stderr, "Compiler error - eastore\n");
      exit(1);
    }
    switch(iib->size) {
    case sz_byte:
      if (a == -1)
	a = compile_getreg(reg68);
      compile_w(0xE3C000FF | a<<16 | a<<12); /* BIC ra,ra,#&FF */
      compile_w(0xE1800000 | a<<16 | a<<12 | eaval); /* ORR ra,ra,eaval */
      compile_updatereg(a);
      break;
    case sz_word:
      if (a == -1)
	a = compile_getreg(reg68);
      compile_w(0xE1A00820 | a<<12 | a); /* MOV ra,ra,LSR#16 */
      /*  ORR ra,eaval,ra,LSL#16 */
      compile_w(0xE1800800 | eaval<<16 | a<<12 | a);
      compile_updatereg(a);
      break;
    case sz_long:
      if (a != -1) {
	/* the 68000 reg is in an ARM register */
	regnum[a] = -1;
	regnum[eaval] = reg68;
	regstate[eaval] = 2; /* needs writing back */
      } else {
	/* the 68000 reg is not in an ARM reg and we're writing all 32
	   bits so we can just name the ARM reg as the 68000 reg */
	regnum[eaval] = reg68;
	regstate[eaval] = 2; /* needs writing back */
      }
      break;
    default:
      fprintf(stderr, "Invalid size %d\n", iib->size);
      exit(1);
    }
    break;
  case dt_Aind:
  case dt_Ainc:
  case dt_Adec:
  case dt_Adis:
  case dt_Aidx:
  case dt_AbsW:
  case dt_AbsL:
  case dt_Pdis:
  case dt_Pidx:
    compile_w(0xE92D100E);             /* STMFD r13!,{r1-r3,r12} */
    compile_w(0xE3C004FF | eareg<<16); /* BIC r0,eareg,#&FF000000 */
    compile_w(0xE1A01000 | eaval);     /* MOV r1,eaval */
    compile_w(0xE1A03620);             /* MOV r3,r0,LSR#12 */
    compile_w(0xE59F2004);             /* LDR r2,[PC,#4] */
    compile_w(0xE28FE004);             /* ADD r14,PC,#4 */
    compile_w(0xE792F103);             /* LDR PC,[r2,r3,LSL#2]; */
    /* EQUD <array address> */
    switch (iib->size) {
    case sz_byte:
      compile_w((uint32)mem68k_store_byte);
      break;
    case sz_word:
      compile_w((uint32)mem68k_store_word);
      break;
    case sz_long:
      compile_w((uint32)mem68k_store_long);
      break;
    default:
      fprintf(stderr, "Invalid size %d\n", iib->size);
      exit(1);
    }
    compile_w(0xE8BD100E);         /* LDMFD r13!,{r1-r3,r12} */
    break;
  case dt_ImmB:
  case dt_ImmW:
  case dt_ImmL:
  case dt_Imm3: /* I want optimising */
  case dt_Imm4:
  case dt_Imm8:
  case dt_Imm8s:
  case dt_ImmS:
  default:
    fprintf(stderr, "Invalid datatype %d\n", datatype);
    exit(1);
  }
}
    
void compile_clrflag_v(t_iib *iib)
{
  compile_w(0xE3C99000 | SR_VFLAG); /* BIC r9,r9,#SR_VFLAG */
}

void compile_clrflag_c(t_iib *iib)
{
  compile_w(0xE3C99000 | SR_CFLAG); /* BIC r9,r9,#SR_CFLAG */
}

void compile_stdflag_n(t_iib *iib, int reg)
{
  switch(iib->size) {
  case sz_byte:
    compile_w(0xE3100080 | reg<<16); /* TST reg,#1<<7 */
    break;
  case sz_word:
    compile_w(0xE3100902 | reg<<16); /* TST reg,#1<<15 */
    break;
  case sz_long:
    compile_w(0xE3100102 | reg<<16); /* TST reg,#1<<31 */
    break;
  default:
    fprintf(stderr, "Invalid size %d\n", iib->size);
    exit(1);
  }
  compile_w(0x13899000 | SR_NFLAG); /* ORRNE r9,r9,#SR_NFLAG */
  compile_w(0x03C99000 | SR_NFLAG); /* BICEQ r9,r9,#SR_NFLAG */
}

void compile_stdflag_z(t_iib *iib, int reg)
{
  switch(iib->size) {
  case sz_byte:
    compile_w(0xE1B0EC00 | reg); /* MOVS r14,r1,LSL#24 */
    break;
  case sz_word:
    compile_w(0xE1B0E800 | reg); /* MOVS r14,r1,LSL#16 */
    break;
  case sz_long:
    compile_w(0xE1B00000 | reg<<12 | reg); /* MOVS r1,r1 */
    break;
  default:
    fprintf(stderr, "Invalid size %d\n", iib->size);
    exit(1);
  }
  compile_w(0x03899000 | SR_ZFLAG); /* ORREQ r9,r9,#SR_ZFLAG */
  compile_w(0x13C99000 | SR_ZFLAG); /* BICNE r9,r9,#SR_ZFLAG */
}

/*
 *  R0-R5 = work registers
 *  R6 = IPC
 *  R7 = PC
 *  R8-> registers
 *  R9 = SR
 */

char *compile_make(t_ipclist *list)
{
  int instrs = 0;
  uint8 *block = malloc(MAXBLOCKSIZE);
  t_ipc *ipc = (t_ipc *)(list+1);
  t_iib *iib;
  int len;
  uint8 *stmia, *p;
  uint32 r[10];
  int srcreg, dstreg, srcval, dstval;
  static int blockno = 1;
  static char tmp[256];
  FILE *file;
  int normal;

  *(uint32 *)0x7004 = (uint32)block;
  *(uint32 *)0x7008 = (uint32)list->pc;

  if (!block) {
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }
  curpos = block;

  compile_w(0xE92D4070); /* STMDB r13!,{r4-r6,r14} */
  compile_w(0xE1A06000); /* MOV r6,r0 */

  compile_initregs();

  while(*(int *)ipc) {
    if ((curpos - block) > MAXBLOCKSIZE-1024) {
      fprintf(stderr, "Block too short\n");
      exit(1);
    }
    instrs++;
    iib = cpu68k_iibtable[ipc->opcode];

    normal = 1;
    
    switch(iib->mnemonic) {
    case i_MOVE:
      if ((iib->stype == dt_Dreg || iib->stype == dt_Areg) &&
	  (iib->dtype == dt_Dreg || iib->dtype == dt_Areg)) {
	compile_ea(iib, ipc, tp_src, true, &srcreg);
	compile_eaval(iib, ipc, tp_src, true, srcreg, &srcval);
	if (srcreg != -1) 
	  regtimer[srcreg]++;
	compile_ea(iib, ipc, tp_dst, true, &dstreg);
	compile_eastore(iib, ipc, tp_dst, dstreg, srcval);
	if (ipc->set & IIB_FLAG_V)
	  compile_clrflag_v(iib);
	if (ipc->set & IIB_FLAG_C)
	  compile_clrflag_c(iib);
	if (ipc->set & IIB_FLAG_N)
	  compile_stdflag_n(iib, srcval);
	if (ipc->set & IIB_FLAG_Z)
	  compile_stdflag_z(iib, srcval);
	compile_w(0xE2877000 | 2*iib->wordlen); /* ADD r7,r7,#wordlen */
	normal = 0;
      }
      break;
    default:
      break;
    }
    if (normal) {
      compile_finalregs();
      compile_w(0xE1A00006); /* MOV r0,r6 */
      /* BL ipc->function */
      compile_w(0xEB000000 + ( ((((uint32)ipc->function -
				  (uint32)curpos)>>2)-2) & 0xFFFFFF));
    }
    ipc++;
    if (*(int *)ipc) {
      compile_w(0xE2866000 + sizeof(t_ipc)); /* ADD r6,r6,#sizeof(t_ipc) */
      compile_inctimer();
    }
  }
  compile_finalregs();
  compile_w(0xE8FD8070); /* LDMIA r13!,{r4-r6,PC}^ */
  compile_w(list->pc);
  p = realloc(block, curpos - block);
  if (p != block) {
    fprintf(stderr, "realloc moved block!\n");
    exit(1);
  }
  *(uint32 *)0x7000 = (uint32)block;

  /*
  sprintf(tmp, "b/%02x/blk%d", blockno/77, blockno++ % 77);
  file = fopen(tmp, "w");
  fwrite(block, 1, curpos - block, file);
  fclose(file);
  */

  r[0] = (uint32)1;
  r[1] = (uint32)block;
  r[2] = (uint32)(curpos-4);
  os_swi(0x20000 | OS_SynchroniseCodeAreas, r);

  return block;
}
