#ifdef PROCESSOR_ARM
     register uint32 reg68k_pc asm ("r7");
     register uint32 *reg68k_regs asm ("r8");
     register t_sr reg68k_sr asm ("r9");
#else
#  ifdef PROCESSOR_SPARC
       register uint32 reg68k_pc asm ("5");
       register uint32 *reg68k_regs asm ("6");
       register t_sr reg68k_sr asm ("7");
#  else
#    ifdef PROCESSOR_INTEL
         register uint32 reg68k_pc asm ("%ebx");
         register uint32 *reg68k_regs asm ("%edi");
         register t_sr reg68k_sr asm ("%esi");
#    else
         extern uint32 reg68k_pc;
         extern uint32 *reg68k_regs;
         extern t_sr reg68k_sr;
#    endif
#  endif
#endif
