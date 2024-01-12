// An attempt to generate a lamp design to be used
// with the BESK scale model
//
$fa = 3;

// top cone
cone_h=6;
cone_bot_r=5;
cone_top_r=1;
// attachment disc
disc_h=1;
disc_r=5;
// bottom cylinder "tube"
cyl_h=6;
cyl_r=4;

thickness=1;  // 1 mm material
eps=0.01;     // alternative?

union() {
    difference() {  // top cone
	cylinder(cone_h,    cone_bot_r, cone_top_r);
	cylinder(cone_h-thickness,  cone_bot_r-thickness, cone_top_r-0.5);
    }
    translate([0,0,-(disc_h-eps)])
	difference() { // disc cylinder
	cylinder(disc_h, cone_bot_r, cone_bot_r);
	cylinder(disc_h, cone_bot_r-thickness, cone_bot_r-thickness);
    }
    translate([0,0,-(disc_h+cyl_h+2*eps)])
    difference() { // attaching "tube"
	cylinder(cyl_h, cyl_r, cyl_r);
	cylinder(cyl_h, cyl_r-thickness, cyl_r-thickness);
    }
}
