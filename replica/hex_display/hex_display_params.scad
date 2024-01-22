//
// Paramters for hex display
// 
screw_extra     = 15.0;
glass_height    = 56.0;
glass_width     = 56.0;
glass_thickness = 2.0;
led_base_width  = 2.8;
led_base_height = 5.2;
led_base_depth  = 4.9;
led_top_radius  = (1.99/2.0);
led_top_height  = 4.5;

glass_slit_thickness = glass_thickness+0.1;
glass_slit_width  = glass_width+0.2;
glass_slit_height = glass_height+0.2;
glass_slit_spacing = 1.5; // 2.5;
glass_slit_margin  = 2.0;    // depth of slit on every side

block_side = glass_slit_width+screw_extra;
block_depth = 3.0+glass_slit_spacing*15+glass_slit_thickness*16+3.0;
hole_width  = glass_width-(glass_slit_margin*2);
hole_height = glass_height-(glass_slit_margin*2);
hole_depth = block_depth + 2.0;

screw_hole_d = 4.2;
screw_hole_inset = block_side/2 - (block_side-hole_width)/4;
block_border = (block_side-hole_height)/2;
