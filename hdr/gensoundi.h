extern uint8 soundi_regs1[256];
extern uint8 soundi_regs2[256];
extern uint8 soundi_address1;
extern uint8 soundi_address2;
extern uint8 soundi_keys[8];

uint8 soundi_ym2612fetch(uint8 addr);
void soundi_ym2612store(uint8 addr, uint8 data);
