//
// 
//
$fn = 75;

include <hex_display_params.scad>

module Led1(extra_base_depth=0)
{
    union() {
    translate([0,0,led_top_height])
	    cylinder(r1=led_top_radius, h=led_top_height, center=true);
    translate([0,0,-(extra_base_depth/2)])
	    cube([led_base_width,led_base_height,led_base_depth+extra_base_depth],center=true);
    }
}

module Object()
{
    difference() {
	cube([block_side,block_side,block_depth],center=true);
	union() {
	    cube([hole_width,hole_height,hole_depth],center=true);
	    for (i=[-7:0]) {
		translate([0,0,-glass_slit_spacing+(glass_slit_thickness+glass_slit_spacing)*i])
		    union() {
		    cube([glass_slit_width,glass_slit_height,glass_slit_thickness],center=true);
		    rotate([0,270,0])
			translate([0,0,-glass_slit_width/2-led_top_height-glass_slit_margin+0.1])
			Led1(extra_base_depth=block_border-led_top_height-led_base_height-glass_slit_margin+0.5);
		}
	    }
	    for (i=[0:7]) {
		translate([0,0,glass_slit_spacing+(glass_slit_thickness+glass_slit_spacing)*i])
		    union() {
		    cube([glass_slit_width,glass_slit_height,glass_slit_thickness],center=true);
		    rotate([0,270,0])
			translate([0,0,-glass_slit_width/2-led_top_height-glass_slit_margin+0.1])
			Led1(extra_base_depth=block_border-led_top_height-led_base_height-glass_slit_margin+0.5);
		}
	    }
	    translate([-screw_hole_inset,-screw_hole_inset,0])
		cylinder(r=screw_hole_r,h=block_depth+1.0,center=true);
	    translate([screw_hole_inset,-screw_hole_inset,0])
		cylinder(r=screw_hole_r,h=block_depth+1.0,center=true);
	    translate([-screw_hole_inset,screw_hole_inset,0])
		cylinder(r=screw_hole_r,h=block_depth+1.0,center=true);
	    translate([screw_hole_inset,screw_hole_inset,0])
		cylinder(r=screw_hole_r,h=block_depth+1.0,center=true);	
	}
    }
}

aside = block_border/4;
adepth = block_depth-10;

if (true) {
    union() {
	intersection() {
	    Object();
        translate([-block_side/2,0,0])
		cube([block_depth,block_side,block_side],center=true);
	}
	translate([aside/2-0.6,-screw_hole_inset,-adepth/2])	
	    cube([aside, aside, adepth],center=false);
	translate([aside/2-0.6,screw_hole_inset,-adepth/2])	
	    cube([aside, aside, adepth],center=false);	    
    }
}
else {
    difference() {
	intersection() {
	    Object();
        translate([block_side/2,0,0])
		cube([block_depth,block_side,block_side],center=true);
	}
	translate([-aside/2-0.5,-screw_hole_inset, -adepth/2])
	    cube([aside, aside, adepth],center=false);
	translate([-aside/2-0.5,screw_hole_inset, -adepth/2])
	    cube([aside, aside, adepth],center=false);	    
    }
}
