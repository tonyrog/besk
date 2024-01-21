//
// Paramters for hex display
// 
screw_extra     = 25.0;
glass_height    = 56.0;
glass_width     = 56.0;
glass_thickness = 2.0;
led_base_width  = 2.55;
led_base_height = 4.9;
led_base_depth  = 4.9;
led_top_radius  = (1.99/2.0);
led_top_height  = 4.3;

glass_slit_thickness = glass_thickness+0.1;
glass_slit_width  = glass_width+0.2;
glass_slit_height = glass_height+0.2;
glass_slit_spacing = 2.5;
glass_slit_margin  = 2.0;    // depth of slit on every side

block_side = glass_slit_width+screw_extra;
block_depth = 6.0+(glass_slit_spacing+glass_slit_thickness)*16+6.0;
hole_width  = glass_width-(glass_slit_margin*2);
hole_height = glass_height-(glass_slit_margin*2);
hole_depth = block_depth + 2.0;

screw_hole_r = 3.2;
screw_hole_inset = (block_side-14)/2;
block_border = (block_side-hole_height)/2;
