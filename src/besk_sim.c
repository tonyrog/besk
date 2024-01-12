#include <unistd.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <poll.h>
#include <sys/time.h>


#include "epx.h"

#include "besk.h"

#define DECAY  3 // decay alpha / ms 
typedef struct {
    epx_backend_t* be;
    epx_pixmap_t*  screen;         // mapped pixmap
    // panel window
    epx_pixmap_t*  px;             // front pixmap
    epx_pixmap_t*  fpx;            // function display (copied)
    epx_pixmap_t*  sfpx;           // "decay" pixmap subtracted
    epx_window_t*  wn;
    epx_gc_t*      gc;
    epx_pixmap_t*  bg;
    epx_pixmap_t*  x_sharp[16];    // sharp/overlay  hex digits
    epx_pixmap_t*  x_dark[16];     // dark/off hex digits
    epx_pixmap_t*  x_glow[16];     // glow hex digits    

    epx_pixmap_t* background;
    EPX_HANDLE_T  evt;
    epx_pixmap_t* led_on;
    epx_pixmap_t* knob;

    // function display
    epx_gc_t*      fgc;    

    int need_panel_redraw;     // redraw panel + function
    int need_function_redraw;  // redraw function

    uint32_t px_width;
    uint32_t px_height;

    int key_mask;
    uint64_t func_last;  // function display last update
    uint64_t led_last;  // led last update
} besk_sim_t;


#define WINDOW_WIDTH         1280
#define WINDOW_HEIGHT        811

#define BACKGROUND_WIDTH     1280
#define BACKGROUND_HEIGHT    811

#define FUNCTION_XOFFS       16
#define FUNCTION_YOFFS       16
#define FUNCTION_WIDTH       288
#define FUNCTION_HEIGHT      288

#define START_BUTTON_X       859
#define START_BUTTON_Y       702
#define START_BUTTON_WIDTH   60
#define START_BUTTON_HEIGHT  32
#define START_KEY            0x01

#define STOP_BUTTON_X        488
#define STOP_BUTTON_Y        702
#define STOP_BUTTON_WIDTH    53
#define STOP_BUTTON_HEIGHT   35
#define STOP_KEY             0x02

// SKRIVMASKIN | ORDER | STANS
#define UTMATNING_KNOB_X          860
#define UTMATNING_KNOB_Y          587

// KONTROLL_UTSKRIFT=OFF | E2_UTSKRIFT | OFF | STEGVIS
#define KONTROLL_UTSKRIFT_KNOB_X  979
#define KONTROLL_UTSKRIFT_KNOB_Y  587

// GANG | STEP=1 | ?=2 | VARIABLE=3
#define GANG_KNOB_X               1168
#define GANG_KNOB_Y               587


#define KNOB_CX     22  // center x
#define KNOB_CY     28  // center y
#define KNOB_WIDTH  40
#define KNOB_HEIGHT 52

#define LED_MD_Y 48
#define LED_MR_Y 79
#define LED_AR_Y 111
#define BUT1_Y   143   // 1st row of buttons
#define LED_AS_Y 171
#define LED_OP_Y 171
#define LED_KR_Y 171
#define BUT2_Y   203   // 2nd row of buttons

#define SEDECIMAL_INPUT_WIDTH  282
#define SEDECIMAL_INPUT_HEIGHT 282

#define SEDECIMAL_WIDTH  70
#define SEDECIMAL_HEIGHT 70

#define XSED_ARv_Y 271
#define XSED_ARh_Y 406
#define XSED_AS_Y  540
#define XSED_OP_Y  540
#define XSED_KR_Y  676

#define XSED_X0    70
#define XSED_X1    209
#define XSED_X2    350
#define XSED_X3    487
#define XSED_X4    626

// step 24 between leds in nybble-group
#define LED_XOFFS 24
#define LED_X0 103
//#define LED_X1 127
//#define LED_X2 151
//#define LED_X3 175

// step 34 between nybble-groups
#define NYBBLE_GROUP_XOFFS 34
#define LED_X4 209
#define LED_X8  313
#define LED_X12 418
#define LED_X16 523

// halvord group
#define LED_X20 661
#define LED_X24 766
#define LED_X28 870
#define LED_X32 976
#define LED_X36 1081

#define LED_X00 69
#define LED_X40 1188

#define BUT_MDv_X 45
#define BUT_MRv_X 45
#define BUT_ARv_X 45
#define BUT_ASv_X 45
#define BUT_OP_X  615
#define BUT_KR_X  1212
#define BUT_X00   69
#define BUT_X40   1188
#define BUT_MDh_X 1212
#define BUT_MRh_X 1212
#define BUT_ARh_X 1212

// 67 steps from last led in nybble group to first in next halvord
#define HALVORD_XOFFS 67

#define LED_WIDTH  23
#define LED_HEIGHT 22

#define BUT_WIDTH  23
#define BUT_HEIGHT 22

static int led_md_x_coords[] = {
    // halvord 20-bit
    LED_X0,  LED_X0+LED_XOFFS, LED_X0+2*LED_XOFFS, LED_X0+3*LED_XOFFS,
    LED_X4,  LED_X4+LED_XOFFS, LED_X4+2*LED_XOFFS, LED_X4+3*LED_XOFFS,
    LED_X8,  LED_X8+LED_XOFFS, LED_X8+2*LED_XOFFS, LED_X8+3*LED_XOFFS,
    LED_X12, LED_X12+LED_XOFFS, LED_X12+2*LED_XOFFS, LED_X12+3*LED_XOFFS,
    LED_X16, LED_X16+LED_XOFFS, LED_X16+2*LED_XOFFS, LED_X16+3*LED_XOFFS,

    // halvord 20-bit
    LED_X20, LED_X20+LED_XOFFS, LED_X20+2*LED_XOFFS, LED_X20+3*LED_XOFFS,
    LED_X24, LED_X24+LED_XOFFS, LED_X24+2*LED_XOFFS, LED_X24+3*LED_XOFFS,
    LED_X28, LED_X28+LED_XOFFS, LED_X28+2*LED_XOFFS, LED_X28+3*LED_XOFFS,
    LED_X32, LED_X32+LED_XOFFS, LED_X32+2*LED_XOFFS, LED_X32+3*LED_XOFFS,
    LED_X36, LED_X36+LED_XOFFS, LED_X36+2*LED_XOFFS, LED_X36+3*LED_XOFFS    
};

static int led_mr_x_coords[] = {
    // halvord 20-bit    
    LED_X0,  LED_X0+LED_XOFFS, LED_X0+2*LED_XOFFS, LED_X0+3*LED_XOFFS,
    LED_X4,  LED_X4+LED_XOFFS, LED_X4+2*LED_XOFFS, LED_X4+3*LED_XOFFS,
    LED_X8,  LED_X8+LED_XOFFS, LED_X8+2*LED_XOFFS, LED_X8+3*LED_XOFFS,
    LED_X12, LED_X12+LED_XOFFS, LED_X12+2*LED_XOFFS, LED_X12+3*LED_XOFFS,
    LED_X16, LED_X16+LED_XOFFS, LED_X16+2*LED_XOFFS, LED_X16+3*LED_XOFFS,

    // halvord 20-bit
    LED_X20, LED_X20+LED_XOFFS, LED_X20+2*LED_XOFFS, LED_X20+3*LED_XOFFS,
    LED_X24, LED_X24+LED_XOFFS, LED_X24+2*LED_XOFFS, LED_X24+3*LED_XOFFS,
    LED_X28, LED_X28+LED_XOFFS, LED_X28+2*LED_XOFFS, LED_X28+3*LED_XOFFS,
    LED_X32, LED_X32+LED_XOFFS, LED_X32+2*LED_XOFFS, LED_X32+3*LED_XOFFS,
    LED_X36, LED_X36+LED_XOFFS, LED_X36+2*LED_XOFFS, LED_X36+3*LED_XOFFS    
};

static int led_ar_x_coords[] = {
    // halvord 20-bit    
    LED_X0,  LED_X0+LED_XOFFS, LED_X0+2*LED_XOFFS, LED_X0+3*LED_XOFFS,
    LED_X4,  LED_X4+LED_XOFFS, LED_X4+2*LED_XOFFS, LED_X4+3*LED_XOFFS,
    LED_X8,  LED_X8+LED_XOFFS, LED_X8+2*LED_XOFFS, LED_X8+3*LED_XOFFS,
    LED_X12, LED_X12+LED_XOFFS, LED_X12+2*LED_XOFFS, LED_X12+3*LED_XOFFS,
    LED_X16, LED_X16+LED_XOFFS, LED_X16+2*LED_XOFFS, LED_X16+3*LED_XOFFS,

    // halvord 20-bit
    LED_X20, LED_X20+LED_XOFFS, LED_X20+2*LED_XOFFS, LED_X20+3*LED_XOFFS,
    LED_X24, LED_X24+LED_XOFFS, LED_X24+2*LED_XOFFS, LED_X24+3*LED_XOFFS,
    LED_X28, LED_X28+LED_XOFFS, LED_X28+2*LED_XOFFS, LED_X28+3*LED_XOFFS,
    LED_X32, LED_X32+LED_XOFFS, LED_X32+2*LED_XOFFS, LED_X32+3*LED_XOFFS,
    LED_X36, LED_X36+LED_XOFFS, LED_X36+2*LED_XOFFS, LED_X36+3*LED_XOFFS    
};

static int led_as_x_coords[] = {
    // 12-bit address from INS
    LED_X0,  LED_X0+LED_XOFFS, LED_X0+2*LED_XOFFS, LED_X0+3*LED_XOFFS,
    LED_X4,  LED_X4+LED_XOFFS, LED_X4+2*LED_XOFFS, LED_X4+3*LED_XOFFS,
    LED_X8,  LED_X8+LED_XOFFS, LED_X8+2*LED_XOFFS, LED_X8+3*LED_XOFFS
};

static int led_op_x_coords[] = {
    // 8-bit op from INS
    LED_X12, LED_X12+LED_XOFFS, LED_X12+2*LED_XOFFS, LED_X12+3*LED_XOFFS,
    LED_X16, LED_X16+LED_XOFFS, LED_X16+2*LED_XOFFS, LED_X16+3*LED_XOFFS,
};

// 12-bit kontroll registers
static int led_kr_x_coords[] = {
    LED_X20, LED_X20+LED_XOFFS, LED_X20+2*LED_XOFFS, LED_X20+3*LED_XOFFS,
    LED_X24, LED_X24+LED_XOFFS, LED_X24+2*LED_XOFFS, LED_X24+3*LED_XOFFS,    
    LED_X28, LED_X28+LED_XOFFS, LED_X28+2*LED_XOFFS, LED_X28+3*LED_XOFFS,
};

// SEDECIMAL display

static int sed_arv_x_coords[] = { XSED_X0, XSED_X1, XSED_X2, XSED_X3, XSED_X4 };
static int sed_arh_x_coords[] = { XSED_X0, XSED_X1, XSED_X2, XSED_X3, XSED_X4 };
static int sed_as_x_coords[] = { XSED_X0, XSED_X1, XSED_X2 };
static int sed_op_x_coords[] = { XSED_X3, XSED_X4 };
static int sed_kr_x_coords[] = { XSED_X0, XSED_X1, XSED_X2 };

extern epx_pixmap_t* epx_image_load_png(char* filename, epx_format_t format);


static int but_x_coords[] = {
    // halvord 20-bit
    LED_X0,  LED_X0+LED_XOFFS, LED_X0+2*LED_XOFFS, LED_X0+3*LED_XOFFS,
    LED_X4,  LED_X4+LED_XOFFS, LED_X4+2*LED_XOFFS, LED_X4+3*LED_XOFFS,
    LED_X8,  LED_X8+LED_XOFFS, LED_X8+2*LED_XOFFS, LED_X8+3*LED_XOFFS,
    LED_X12, LED_X12+LED_XOFFS, LED_X12+2*LED_XOFFS, LED_X12+3*LED_XOFFS,
    LED_X16, LED_X16+LED_XOFFS, LED_X16+2*LED_XOFFS, LED_X16+3*LED_XOFFS,

    // halvord 20-bit
    LED_X20, LED_X20+LED_XOFFS, LED_X20+2*LED_XOFFS, LED_X20+3*LED_XOFFS,
    LED_X24, LED_X24+LED_XOFFS, LED_X24+2*LED_XOFFS, LED_X24+3*LED_XOFFS,
    LED_X28, LED_X28+LED_XOFFS, LED_X28+2*LED_XOFFS, LED_X28+3*LED_XOFFS,
    LED_X32, LED_X32+LED_XOFFS, LED_X32+2*LED_XOFFS, LED_X32+3*LED_XOFFS,
    LED_X36, LED_X36+LED_XOFFS, LED_X36+2*LED_XOFFS, LED_X36+3*LED_XOFFS    
};


static epx_rect_t start_button = {{START_BUTTON_X,START_BUTTON_Y},
				  {START_BUTTON_WIDTH,START_BUTTON_HEIGHT}};
static epx_rect_t stop_button = {{STOP_BUTTON_X,STOP_BUTTON_Y},
				 {STOP_BUTTON_WIDTH,STOP_BUTTON_HEIGHT}};
static epx_rect_t utmatning_knob =
{{UTMATNING_KNOB_X-KNOB_WIDTH/2,UTMATNING_KNOB_Y-KNOB_HEIGHT/2},
 {KNOB_WIDTH,KNOB_HEIGHT}};
static epx_rect_t kontroll_utskrift_knob =
{{KONTROLL_UTSKRIFT_KNOB_X-KNOB_WIDTH/2,KONTROLL_UTSKRIFT_KNOB_Y-KNOB_HEIGHT/2},
 {KNOB_WIDTH,KNOB_HEIGHT}};
static epx_rect_t gang_knob =
{{GANG_KNOB_X-KNOB_WIDTH/2,GANG_KNOB_Y-KNOB_HEIGHT/2},
 {KNOB_WIDTH,KNOB_HEIGHT}};

enum {
    MDv_Noll = 1,
    MDh_Noll,
    MRv_Noll,
    MRh_Noll,
    ARv_Noll,
    ARh_Noll,
    AS_Noll,
    OP_Noll,
    KR_Noll,
    X00_Set,
    X40_Set
};
    
static struct {
    int val;
    epx_rect_t hit;
} but_noll_array[] = {
    {MDv_Noll,{{BUT_MDv_X,LED_MD_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {MRv_Noll,{{BUT_MRv_X,LED_MR_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {ARv_Noll,{{BUT_ARv_X,LED_AR_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {AS_Noll, {{BUT_ASv_X,LED_AS_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {OP_Noll, {{BUT_OP_X,LED_OP_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {MDh_Noll,{{BUT_MDh_X,LED_MD_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {MRh_Noll,{{BUT_MRh_X,LED_MR_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {ARh_Noll,{{BUT_ARh_X,LED_AR_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {KR_Noll, {{BUT_KR_X,LED_KR_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {X00_Set, {{BUT_X00,BUT1_Y},{BUT_WIDTH,BUT_HEIGHT}}},
    {X40_Set, {{BUT_X40,BUT1_Y},{BUT_WIDTH,BUT_HEIGHT}}},    
    {0,{{0,0},{0,0}}}
};

uint64_t time_tick(void)
{
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_sec*(uint64_t)1000000 + t.tv_usec;
}


static void button(besk_sim_t* bst, besk_t* st, int x, int y, int pressed)
{
    // printf("button [pressed=%d] x=%d, y=%d\n", pressed, x, y);
    if (epx_point_xy_in_rect(x, y, &start_button)) {
	if (pressed) bst->key_mask |= START_KEY;
	else bst->key_mask &= ~START_KEY;
    }
    else if (epx_point_xy_in_rect(x, y, &stop_button)) {
	if (pressed) bst->key_mask |= STOP_KEY;
	else bst->key_mask &= ~STOP_KEY;
    }
    else if (epx_point_xy_in_rect(x, y, &utmatning_knob)) {
	if (pressed) {
	    switch(st->utmatning_pos) {
	    case 0:  st->utmatning_pos=2; break;
	    case 2:  st->utmatning_pos=4; break;
	    case 4:  st->utmatning_pos=-2; break;
	    case -2: st->utmatning_pos=0; break;
	    default: st->utmatning_pos=0; break;
	    }
	}
    }
    else if (epx_point_xy_in_rect(x, y, &kontroll_utskrift_knob)) {
	if (pressed) {
	    switch(st->kontroll_utskrift_pos) {
	    case 0: st->kontroll_utskrift_pos=2; break;
	    case 2: st->kontroll_utskrift_pos=4; break;
	    case 4: st->kontroll_utskrift_pos=-2; break;
	    case -2: st->kontroll_utskrift_pos=0; break;
	    default: st->kontroll_utskrift_pos=2; break;
	    }
	}
    }
    else if (epx_point_xy_in_rect(x, y, &gang_knob)) {
	if (pressed) {
	    switch(st->gang_pos) {
	    case 2: st->gang_pos=7;     break;
	    case 7: st->gang_pos=10;    break;
	    case 10: st->gang_pos=-7;   break;
	    case -7: st->gang_pos=2;    break;
	    default: st->gang_pos=10;   break;
	    }
	}
    }
    else {
	epx_rect_t r;
	// check if we hit a "noll" button
	int i;
	for (i = 0; but_noll_array[i].val; i++) {
	    if (epx_point_xy_in_rect(x, y, &but_noll_array[i].hit)) {
		switch(but_noll_array[i].val) {
		case MDv_Noll: st->MD = st->MD & 0x00000FFFFF; break;
		case MDh_Noll: st->MD = st->MD & 0xFFFFF00000; break;
		case MRv_Noll: st->MR = st->MR & 0x00000FFFFF; break;
		case MRh_Noll: st->MR = st->MR & 0xFFFFF00000; break;
		case ARv_Noll: st->AR = st->AR & 0x00000FFFFF; break;
		case ARh_Noll: st->AR = st->AR & 0xFFFFF00000; break;
		case AS_Noll:  st->INS = st->INS & 0x000FF; break;
		case OP_Noll:  st->INS = st->INS & 0xFFF00; break;
		case KR_Noll:  st->KR  = 0; break;
		case X00_Set: st->AR00 = 1; break;
		case X40_Set: st->AR40 = 1; break;
		default: break;
		}
		return;
	    }
	}
	// check AR set buttons
	r.wh.width = BUT_WIDTH;
	r.wh.height = BUT_HEIGHT;	
	for (i = 0; i < 40; i++) {
	    r.xy.x = but_x_coords[i];
	    r.xy.y = BUT1_Y;
	    if (epx_point_xy_in_rect(x, y, &r)) {
		st->AR |= ((helord_t)1 << (39-i));  // besk bits
		return;
	    }
	}
	for (i = 0; i < 40; i++) {
	    r.xy.x = but_x_coords[i];
	    r.xy.y = BUT2_Y;
	    if (epx_point_xy_in_rect(x, y, &r)) {
		if (i < 20) { // set in INS
		    st->INS |= ((helord_t)1 << (19-i));
		}
		else if (i < 32) { // set in KR
		    st->KR |= ((helord_t)1 << (31-i));
		}
		return;
	    }
	}
	
    }
}

// draw only function part
static void update_function(besk_sim_t* bst)
{
    epx_window_draw_begin(bst->wn);
    if (bst->px != bst->screen)    
	epx_pixmap_copy_area(bst->px, bst->screen,
			     WINDOW_WIDTH, 0, WINDOW_WIDTH, 0,
			     FUNCTION_WIDTH, FUNCTION_HEIGHT, 0);
    epx_backend_pixmap_draw(bst->be, bst->screen, bst->wn,
			    WINDOW_WIDTH, 0, WINDOW_WIDTH, 0,
			    FUNCTION_WIDTH, FUNCTION_HEIGHT);
    epx_window_draw_end(bst->wn, 0);
    epx_window_swap(bst->wn);
    bst->need_function_redraw = 0;
}

// both panel & function parts of window
static void update_window(besk_sim_t* bst)
{
    epx_window_draw_begin(bst->wn);
    if (bst->px != bst->screen)
	epx_pixmap_copy_area(bst->px, bst->screen,
			     0, 0, 0, 0,
			     bst->px_width, bst->px_height, 0);
    epx_backend_pixmap_draw(bst->be, bst->screen, bst->wn, 0, 0, 0, 0,
			    bst->px_width, bst->px_height);
    epx_window_draw_end(bst->wn, 0);
    epx_window_swap(bst->wn);
    bst->need_panel_redraw = 0;
    bst->need_function_redraw = 0;    
}


char xchar[] = "0123456789ABCDEF";

static void draw_sedecimal_background(besk_sim_t* bst, int x, int y)
{
    epx_pixmap_copy_area(bst->bg, bst->px, 0, 0, x, y,
			 SEDECIMAL_WIDTH, SEDECIMAL_HEIGHT,
			 0);
//    epx_pixmap_fill_area(bst->px, x, y, SEDECIMAL_WIDTH, SEDECIMAL_HEIGHT,
//			 epx_pixel_rgb(40, 60, 40), 0);
}

static void draw_sedecimal(besk_sim_t* bst, int x, int y, int d)
{
    int i;
    // int mode = EPX_FLAG_BLEND;

    d &= 0xf;
    for (i = 15; i >= 0; i--) {
	if (i != d) {
	    /*
	    epx_pixmap_copy_area(bst->x_dark[i & 0xf], bst->px, 0, 0, x, y,
				 SEDECIMAL_WIDTH, SEDECIMAL_HEIGHT,
				 mode); 
	    */
	}
	else {
	    epx_pixmap_copy_area(bst->x_glow[d & 0xf], bst->px, 0, 0, x, y,
				 BACKGROUND_WIDTH, BACKGROUND_HEIGHT,
				 0);
	    /*
	    epx_pixmap_copy_area(bst->x_sharp[d & 0xf], bst->px, 0, 0, x, y,
				 BACKGROUND_WIDTH, BACKGROUND_HEIGHT,
				 mode);
	    */
	    
	}
    }
}

// fist copy/crop the image into 258x258 image (black 1 px border removed)
// then scale into 70x70 format (SEDECIMAL_WIDTHxSEDECIMAL_HEIGHT)

static epx_pixmap_t* scale_digit(epx_pixmap_t* px, int w, int h)
{
    epx_pixmap_t* px_dst;

    assert(px->width == SEDECIMAL_INPUT_WIDTH);
    assert(px->height == SEDECIMAL_INPUT_HEIGHT);
    px_dst = epx_pixmap_create(w, h, EPX_FORMAT_ARGB);
    epx_pixmap_scale_area(px, px_dst, 1, 1, 0, 0, 280, 280, w, h, 0);

    return px_dst;
}

// add a random distributed scatter dot
// for every pixel where a>0

static epx_pixmap_t* scatter_pixmap(epx_pixmap_t* px, int size,
				    epx_pixel_t color)
{
    epx_pixmap_t* px1;
    epx_pixmap_t* brush = NULL;
    int x, y;
    
    assert(px->width == SEDECIMAL_INPUT_WIDTH);
    assert(px->height == SEDECIMAL_INPUT_HEIGHT);

    // create a "brush"
    if (size > 0) {
	int xc = size / 2;
	int yc = size / 2;
	float h = size/2.0;
	float w = size/2.0;
	brush = epx_pixmap_create(size, size, EPX_FORMAT_ARGB);
	epx_pixmap_fill(brush, epx_pixel_transparent);
	for (y = 0; y < size; y++) {
	    float yf = (y - yc) / h;
	    for (x = 0; x < size; x++) {
		float xf = (x - xc) / w;
		float d = sqrtf(xf*xf + yf*yf);
		long  a, r;

		d = (d < 0.1) ? 0.1 : d;           // 0.1 -> 1.4
		a = 110.2*((1/d) - (1/sqrtf(2)));  // 1024 -> 0
		r = random() % 100;

		epx_pixel_t c;
		if ((d < 1.1) || (r < a)) {
		    c = color;
		    a = a >> 3; // 0..255
		    c.a = a;
		}
		else
		    c = epx_pixel_black;
		epx_pixmap_put_pixel(brush, x, y, 0, c);
	    }
	}
    }

    px1 = epx_pixmap_create(px->width, px->height, EPX_FORMAT_ARGB);
    epx_pixmap_fill(px1, epx_pixel_transparent);

    for (y = 1; y < px->height-1; y++) {
	for (x = 1; x < px->width-1; x++) {
	    epx_pixel_t p = epx_pixmap_get_pixel(px, x, y);
	    uint16_t a = p.a;
	    if (a > 0) {
		epx_pixmap_operation_area(brush, px1, OP_ADD,
					  0, 0,
					  x-(size/2), y-(size/2),
					  size, size);
	    }
	}
    }
    if (brush)
	epx_pixmap_destroy(brush);
    return px1;
}


static char* xdigit = "0123456789ABCDEF";

static void load_sedecimal(besk_sim_t* bst)
{
    int i;
    char filename[80];
    epx_pixmap_t* px;
    epx_pixmap_t* px1;

    px = epx_image_load_png("images/besk_din/1x/Black.png", EPX_FORMAT_ARGB);
    bst->bg = scale_digit(px, SEDECIMAL_WIDTH, SEDECIMAL_HEIGHT);
    epx_pixmap_destroy(px);
    
    for (i = 0; i < 16; i++) {
	// Digit turned off
	strcpy(filename, "images/besk_din/1x/x dark.png");
	filename[19] = xdigit[i];
	px = epx_image_load_png(filename, EPX_FORMAT_ARGB);
	bst->x_dark[i] = scale_digit(px, SEDECIMAL_WIDTH, SEDECIMAL_HEIGHT);
	epx_pixmap_destroy(px);

        // Digit glowing/turned on
	strcpy(filename, "images/besk_din/1x/x glow.png");
	filename[19] = xdigit[i];
	px = epx_image_load_png(filename, EPX_FORMAT_ARGB);
	px1 = scatter_pixmap(px, 8, epx_pixel_white);
	bst->x_glow[i] = scale_digit(px1, SEDECIMAL_WIDTH, SEDECIMAL_HEIGHT);
	epx_pixmap_destroy(px1);
	epx_pixmap_destroy(px);

	// epx_pixmap_destroy(px1);

	// Digit sharp edges for turned on
	strcpy(filename, "images/besk_din/1x/x sharp.png");
	filename[19] = xdigit[i];
	px = epx_image_load_png(filename, EPX_FORMAT_ARGB);
	bst->x_sharp[i] = scale_digit(px, SEDECIMAL_WIDTH, SEDECIMAL_HEIGHT);
	epx_pixmap_destroy(px);
	// epx_pixmap_destroy(px1);	
    }
}

static void draw_led(besk_sim_t* bst, int x, int y, int state)
{
    int w = LED_WIDTH;
    int h = LED_HEIGHT;

    if (state)
	epx_pixmap_copy_area(bst->led_on, bst->px, 0, 0, x, y, w, h,
			     EPX_FLAG_BLEND);
}

static void draw_knob(besk_sim_t* bst, int x, int y, float angle)
{
    epx_pixmap_rotate_area(bst->knob, bst->px, angle,
			   0, 0,
			   KNOB_CX, KNOB_CY, x, y,
			   KNOB_WIDTH, KNOB_HEIGHT,
			   EPX_FLAG_BLEND);
}

float deg_to_rad(float deg)
{
    return ((deg/180.0)*M_PI);
}

static void draw_knobs(besk_sim_t* bst, besk_t* st)
{
    float a;
    
    // Draw UTMATNING KNOB
    // pos=0 (STANS)  pos=2 (ORDER) pos=4 (SKRIVMASKIN)
    
    a = deg_to_rad(abs(st->utmatning_pos) * 10.0);
    draw_knob(bst, UTMATNING_KNOB_X, UTMATNING_KNOB_Y, a);

    // Draw KONTROLL_UTSKRIFT KNOB
    // pos=0 (STEGVIS) pos=2 (OFF)  pos=4 (E2-UTSKRIFT)
    a = deg_to_rad(abs(st->kontroll_utskrift_pos) * 10.0);
    draw_knob(bst, KONTROLL_UTSKRIFT_KNOB_X, KONTROLL_UTSKRIFT_KNOB_Y, a);

    // Draw GANG_KNOB
    // pos=7 (STEGVIS/STEP) pos=10 (GANG/RUN) pos=2 (VARIABLE HASTIGHET)
    a = deg_to_rad(abs(st->gang_pos) * 10.0);
    draw_knob(bst, GANG_KNOB_X, GANG_KNOB_Y, a);
}

static void draw_state_background(besk_sim_t* bst)
{
    int i;
    for (i = 0; i < 5; i++)
	draw_sedecimal_background(bst, sed_arv_x_coords[4-i], XSED_ARv_Y);
    // draw sedecimal ARh
    for (i = 0; i < 5; i++)
	draw_sedecimal_background(bst, sed_arh_x_coords[4-i], XSED_ARh_Y);
    // draw sedecimal AS
    for (i = 0; i < 3; i++)
	draw_sedecimal_background(bst, sed_as_x_coords[2-i], XSED_AS_Y);
    // draw sedecimal OP
    for (i = 0; i < 2; i++)
	draw_sedecimal_background(bst, sed_op_x_coords[1-i], XSED_OP_Y);
    // draw sedecimal KR
    for (i = 0; i < 3; i++)
	draw_sedecimal_background(bst, sed_kr_x_coords[2-i], XSED_KR_Y);
}

static void draw_led_state(besk_sim_t* bst, besk_t* st)
{
    int i;
    halvord_t AS  = W(st->INS);
    oktet_t OP    = O(st->INS);
    halvord_t KR = st->KR;
    helord_t MD   = st->MD;
    helord_t MR   = st->MR;
    helord_t AR   = st->AR;    
    helord_t AR00 = st->AR00;
    helord_t AR40 = st->AR40;    

    // draw MD
    for (i = 0; i < 40; i++)
	draw_led(bst, led_md_x_coords[i], LED_MD_Y,
		 (MD & (1L << (39-i))) != 0);
    // draw MR    
    for (i = 0; i < 40; i++)
	draw_led(bst, led_mr_x_coords[i], LED_MR_Y,
		 (MR & (1L << (39-i))) != 0);
    // draw AR
    for (i = 0; i < 40; i++)
	draw_led(bst, led_ar_x_coords[i], LED_AR_Y,
		 (AR & (1L << (39-i))) != 0);

    draw_led(bst, LED_X00, LED_AR_Y, AR00);
    draw_led(bst, LED_X40, LED_AR_Y, AR40);

    // draw AS (reversed numbering) !
    for (i = 0; i < 12; i++)
	draw_led(bst, led_as_x_coords[11-i], LED_AS_Y,
		 (AS & (1L << i)) != 0);
    // draw OP (reversed numbering) !
    for (i = 0; i < 8; i++)
	draw_led(bst, led_op_x_coords[7-i], LED_OP_Y,
		 (OP & (1L << i)) != 0);
    // draw KR (reversed numbering) !
    for (i = 0; i < 12; i++)
	draw_led(bst, led_kr_x_coords[11-i], LED_KR_Y,
		 (KR & (1L << i)) != 0);
}

static void draw_sedecimal_state(besk_sim_t* bst, besk_t* st)
{
    int i;
    halvord_t AS  = W(st->INS);
    oktet_t OP    = O(st->INS);
    helord_t AR   = st->AR;    

    // draw sedecimal ARv
    for (i = 0; i < 5; i++)
	draw_sedecimal(bst, sed_arv_x_coords[4-i], XSED_ARv_Y,(AR>>(20+4*i)));

    // draw sedecimal ARh
    for (i = 0; i < 5; i++)
	draw_sedecimal(bst, sed_arh_x_coords[4-i], XSED_ARh_Y, (AR >> 4*i));

    // draw sedecimal AS
    for (i = 0; i < 3; i++)
	draw_sedecimal(bst, sed_as_x_coords[2-i], XSED_AS_Y,(AS >> 4*i));

    // draw sedecimal OP
    for (i = 0; i < 2; i++)
	draw_sedecimal(bst, sed_op_x_coords[1-i], XSED_OP_Y,(OP >> 4*i));

    // draw sedecimal KR
    for (i = 0; i < 3; i++)
	draw_sedecimal(bst, sed_kr_x_coords[2-i], XSED_KR_Y, (st->KR >> 4*i));
}

static void draw_state(besk_sim_t* bst, besk_t* st)
{
    draw_led_state(bst, st);
    draw_sedecimal_state(bst, st);
}

static void draw_panel(besk_sim_t* bst, besk_t* st, int active)
{
    int x = 0;
    int y = 0;

    // Draw the background image
    epx_pixmap_copy_area(bst->background, bst->px, 0, 0, x, y,
			 BACKGROUND_WIDTH, BACKGROUND_HEIGHT, 0);
    draw_state_background(bst);
    if (active)
	draw_state(bst, st);
    else
	draw_led_state(bst, st);
    draw_knobs(bst, st);
}

static void draw_function_grid(besk_sim_t* bst, besk_t* st)
{
    int x, y;

    epx_pixmap_fill_area(bst->fpx, 0, 0,
			 FUNCTION_WIDTH, FUNCTION_HEIGHT,
			 epx_pixel_black, 0); 
    
    epx_gc_set_foreground_color(bst->fgc, epx_pixel_white);
    for (x = 0; x <= 256; x += 32) {
	for (y = 0; y <= 256; y += 32) {
	    epx_pixmap_draw_point(bst->fpx, bst->fgc,
				  x+FUNCTION_XOFFS,
				  y+FUNCTION_YOFFS);
	}
    }
}

// -1.0, -0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75, 1
static void draw_function(besk_sim_t* bst, besk_t* st)
{
    helord_t x0, y0;
    double xd, yd;
    double x, y;
    int xi, yi;
    
    if (st->Fop == 0)
	return;

//  fprintf(stdout, "Fx = %010lX, %f\n",  st->Fx, helord_to_double(st->Fx));
//  fprintf(stdout, "Fy = %010lX, %f\n",  st->Fy, helord_to_double(st->Fy));

    // x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11....x39
    x0 = st->Fx;
    xd = helord_sign_bit(x0) ? -255.0 : 255.0;
    x  = ((helord_abs(x0) >> (32-st->Fpos_x)) & 0xFF) / xd;

    y0 = st->Fy;
    yd = helord_sign_bit(y0) ? -255.0 : 255.0;
    y  = ((helord_abs(y0) >> (32-st->Fpos_y)) & 0xFF) / yd;
	
    epx_gc_set_foreground_color(bst->fgc, epx_pixel_greenYellow);

    // -1 .. 1 => 0 ... 255
    xi = 128*(x + 0.9922);
    yi = 255 - 128*(y + 0.9922);  // => 255 ... 0
    // xi += FUNCTION_XOFFS;
    // yi += FUNCTION_YOFFS;
    // fprintf(stdout, "x=%f, y=%f  (%d,%d)\n", x, y, xi, yi);

    if (st->Fop == 1) {
	epx_pixmap_draw_point(bst->fpx, bst->fgc, xi, yi);
    }
    else {
	epx_pixmap_draw_ellipse(bst->fpx, bst->fgc, xi, yi, 5, 5);
    }
    
    bst->need_function_redraw = 1;
    st->Fop = 0;
}

void simulator_run(besk_t* st)
{
    epx_event_t e;
    besk_sim_t* bst = (besk_sim_t*) st->user_data;
    int running = st->running;
    int stop = 0;
    int start = 0;
    int func_redraw = 0;
    int r = 1;
    uint64_t t;
    
    if (!running) {
	// block for event
	struct pollfd fds;

	draw_panel(bst, st, 1);
	update_window(bst);	

	fds.fd = (int)((long)bst->evt);
	fds.events = POLLIN;
	r = poll(&fds, 1, 100);  // 1=event, 0==timeout  -1=error
	// printf("poll %d (revents=%x)\n", r, fds.revents);
    }
    // FIXME: if running only check events every 50ms?
    if ((r==1) && (epx_backend_event_read(bst->be, &e) > 0)) {
	if ((e.type == EPX_EVENT_BUTTON_PRESS) && (e.pointer.button == 1)) {
	    button(bst, st, e.pointer.x, e.pointer.y, 1);
	    if (bst->key_mask & STOP_KEY) {
		stop = 1;
	    }
	    else if (bst->key_mask & START_KEY) {
		start = 1;
	    }
	}
	else if ((e.type == EPX_EVENT_BUTTON_RELEASE) &&
		 (e.pointer.button == 1)) {
	    button(bst, st, e.pointer.x, e.pointer.y, 0);
	}
	else if (e.type == EPX_EVENT_CLOSE) {
	    st->quit = 1;
	}
	else if (e.type == EPX_EVENT_KEY_PRESS) {
	    if (e.key.sym == 'q')
		st->quit = 1;
	    else if (e.key.sym == ' ') {
		if (running)
		    stop = 1;
		else
		    start = 1;
	    }
	    else if (e.key.sym == 's') {
		start = 1;
	    }
	}
    }

    if (!running && start) {
	st->running = 1;
	draw_panel(bst, st, 0);
	bst->need_panel_redraw = 1;
    }    else if (running && stop) {    
//    else if (!running || (running && stop)) {
	st->running = 0;
	draw_panel(bst, st, 1);
	bst->need_panel_redraw = 1;
    }

    t = time_tick();
    if ((t - bst->func_last) >= 100000) {
	// printf("%ld: tick\n", t - bst->func_last);
	epx_pixmap_operation_area(bst->sfpx, bst->fpx, OP_SUB, 0, 0, 0, 0,
				  FUNCTION_WIDTH, FUNCTION_HEIGHT);
	bst->func_last = t;
	func_redraw = 1;
    }
    if ((t - bst->led_last) >= 30000) {
	// animate the LED blinking
	if (running) {
	    draw_panel(bst, st, 0);
	    bst->need_panel_redraw = 1;
	}    
	bst->led_last = t;
    }
	
    // draw_function_grid(bst, st);
    // remove sfpx from fpx

    draw_function(bst, st);

    if (bst->need_function_redraw)
	func_redraw = bst->need_function_redraw;

    if (func_redraw || bst->need_panel_redraw) {
	epx_pixmap_copy_area(bst->fpx, bst->px,
			     0, 0, WINDOW_WIDTH, 0,
			     FUNCTION_WIDTH, FUNCTION_HEIGHT, 0);
    }

    if (bst->need_panel_redraw)
	update_window(bst);
    else if (bst->need_function_redraw)
	update_function(bst);
    
    usleep(100);
}

int backend_info_get_bool(epx_backend_t* be, char* key)
{
    epx_dict_t* dict;
    int value = 0;

    dict = epx_dict_create();
    epx_dict_set_boolean(dict, key, 0);
    epx_backend_info(be, dict);
    epx_dict_lookup_boolean(dict, key, &value);
    fprintf(stderr, "%s = %d\n", key, value);
    epx_dict_destroy(dict);
    return value;
}


void simulator_init(int argc, char** argv, besk_t* st)
{
    // char* backend = NULL;
    besk_sim_t* bst = (besk_sim_t*) calloc(1, sizeof(besk_sim_t));

    st->user_data = bst;
    
    if ((bst->be = epx_backend_create(getenv("EPX_BACKEND"), NULL)) == NULL) {
	fprintf(stderr, "besk_sim: no epx backend found\n");
	exit(1);
    }
    epx_init(EPX_SIMD_AUTO);

    bst->px_width = WINDOW_WIDTH + FUNCTION_WIDTH;
    bst->px_height = WINDOW_HEIGHT;
    
    bst->wn = epx_window_create(50, 50, bst->px_width, bst->px_height);
    epx_backend_window_attach(bst->be, bst->wn);

    bst->screen = epx_pixmap_create(bst->px_width,bst->px_height,
				    EPX_FORMAT_ARGB);

    if (backend_info_get_bool(bst->be, "double_buffer"))
	bst->px = bst->screen;
    else 
	bst->px = epx_pixmap_create(bst->px_width,bst->px_height,
				    EPX_FORMAT_ARGB);
    
    bst->fpx = epx_pixmap_create(FUNCTION_WIDTH, FUNCTION_HEIGHT, 
				 EPX_FORMAT_ARGB);
    bst->sfpx = epx_pixmap_create(FUNCTION_WIDTH, FUNCTION_HEIGHT, 
				  EPX_FORMAT_ARGB);
    epx_backend_pixmap_attach(bst->be, bst->screen);
    epx_pixmap_fill(bst->px, epx_pixel_black);
    epx_pixmap_fill(bst->fpx, epx_pixel_black);
    epx_pixmap_fill(bst->sfpx, epx_pixel_argb(DECAY,0,0,0));

    bst->gc = epx_gc_copy(&epx_default_gc);

    bst->evt = epx_backend_event_attach(bst->be);

    epx_window_set_event_mask(bst->wn, EPX_EVENT_BUTTON_PRESS | 
			      EPX_EVENT_BUTTON_RELEASE |
			      EPX_EVENT_KEY_PRESS |
			      EPX_EVENT_CLOSE);
    
    load_sedecimal(bst);
    
    bst->background = epx_image_load_png("images/panel_wo_knobs.png",
					 EPX_FORMAT_ARGB);
    bst->led_on = epx_image_load_png("images/led_on.png",EPX_FORMAT_ARGB);
    bst->knob = epx_image_load_png("images/knob1.png",EPX_FORMAT_ARGB);

    bst->fgc = epx_gc_copy(&epx_default_gc);

    draw_panel(bst, st, 0);
    draw_function_grid(bst, st);
    draw_function(bst, st);

    update_window(bst);
    bst->func_last = time_tick();
    bst->led_last = bst->func_last;
}
