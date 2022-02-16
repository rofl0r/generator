extern unsigned int vdp_event;
extern unsigned int vdp_vislines;
extern unsigned int vdp_totlines;
extern unsigned int vdp_framerate;
extern unsigned int vdp_clock;
extern unsigned int vdp_68kclock;
extern unsigned int vdp_clksperline_68k;
extern unsigned int vdp_oddframe;
extern unsigned int vdp_hinton;
extern unsigned int vdp_vblank;
extern unsigned int vdp_hblank;
extern unsigned int vdp_interlace;
extern unsigned int vdp_line;
extern unsigned int vdp_vsync;
extern unsigned int vdp_pal;
extern unsigned int vdp_overseas;
extern unsigned int vdp_basepixel;
extern unsigned int vdp_layerB;
extern unsigned int vdp_layerBp;
extern unsigned int vdp_layerA;
extern unsigned int vdp_layerAp;
extern unsigned int vdp_layerW;
extern unsigned int vdp_layerWp;
extern unsigned int vdp_layerH;
extern unsigned int vdp_layerS;
extern unsigned int vdp_layerSp;
extern uint8 vdp_cram[];
extern uint8 vdp_vsram[];
extern uint8 vdp_vram[];
extern unsigned int vdp_cramchange;
extern uint8 vdp_cramf[];
extern unsigned int vdp_event_start;
extern unsigned int vdp_event_vint;
extern unsigned int vdp_event_hint;
extern unsigned int vdp_event_hdisplay;
extern unsigned int vdp_event_end;
extern signed int vdp_hskip_countdown;
extern unsigned int vdp_event_startofcurrentline;

void vdp_reset(void);
int vdp_init(void);
uint16 vdp_status(void);
void vdp_storectrl(uint16 data);
void vdp_storedata_byte(uint8 data);
void vdp_storedata_word(uint16 data);
uint8  vdp_fetchdata_byte(void);
uint16 vdp_fetchdata_word(void);
void vdp_renderline(unsigned int line, uint8 *linedata);
void vdp_renderline_interlace2(unsigned int line, uint8 *linedata);
void vdp_showregs(void);
void vdp_describe(void);
void vdp_spritelist(void);
void vdp_endfield(void);
void vdp_renderframe(uint8 *framedata, unsigned int lineoffset);
void vdp_setupvideo(void);
uint8 vdp_gethpos(void);

#define LEN_CRAM 128
#define LEN_VSRAM 40*2*2
#define LEN_VRAM 64*1024

/* an estimate of the total cell width including HBLANK, for calculations */
#define TOTAL_CELLWIDTH 64

extern uint8 vdp_reg[];
