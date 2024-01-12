$fn = 75;

include <hex_display_params.scad>

lid_thickness = 10;

module Object()
{
    difference() {
	cube([block_side,block_side,lid_thickness],center=true);
	union() {
	    translate([-screw_hole_inset,-screw_hole_inset,0])
		cylinder(r=screw_hole_r,h=lid_thickness+1.0,center=true);
	    translate([screw_hole_inset,-screw_hole_inset,0])
		cylinder(r=screw_hole_r,h=lid_thickness+1.0,center=true);
	    translate([-screw_hole_inset,screw_hole_inset,0])
		cylinder(r=screw_hole_r,h=lid_thickness+1.0,center=true);
	    translate([screw_hole_inset,screw_hole_inset,0])
		cylinder(r=screw_hole_r,h=lid_thickness+1.0,center=true);
	}
    }	
}

Object();
