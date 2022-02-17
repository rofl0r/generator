/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* event.h                                                                   */
/*                                                                           */
/*****************************************************************************/

#ifndef EVENT_HEADER_FILE
#define EVENT_HEADER_FILE

void event_doframe(void);
void event_dostep(void);
void event_freeze_clocks(unsigned int clocks);
void event_freeze(unsigned int bytes);

#endif /* EVENT_HEADER_FILE */
