precision highp float;

uniform bool border;
uniform bool title_bar;
uniform bool dim;
uniform float border_width;
uniform float border_radius;
uniform float title_bar_height;
uniform float title_bar_border_radius;
uniform bool swap_xy;
uniform bool flip_x;
uniform bool flip_y;
uniform vec4 box;
uniform vec4 border_top;
uniform vec4 border_bottom;
uniform vec4 border_left;
uniform vec4 border_right;
uniform vec4 title_bar_color;
uniform vec4 dim_color;
uniform vec4 corners; // rounded corner mask: tl, tr, bl, br

vec4 antialias(float x, float x0, float x1, float fw, vec4 color) {
    float xmax = max(x1, x + fw);
    float xmin = min(x0, x - fw);
    float len = xmax - xmin;
    float d0 = abs(x + fw - x1);
    float d1 = abs(x - fw - x0);
    float overlap = len - d0 - d1;
    float alpha = smoothstep(0.0, 1.0, overlap);
    return color * alpha;
}

float fw2(float r, vec2 p) {
    vec2 ap = abs(p);
    return 0.5 * r / max(ap.x, ap.y);
}

vec4 circumference(vec2 p, float r, vec4 c) {
    float d = length(p);
    vec4 color = antialias(d, r, r + border_width, fw2(d, p), c);
	if (dim && d < r + 1.0) {
		return mix(dim_color, color, color.a);
	}
	return color;
}

void main() {
    vec2 pos = vec2(gl_FragCoord);
    float title_height;
    float r0, r1;
    vec2 rel = pos.xy - box.xy;
	float width, height;
	if (flip_x) {
		rel.x = box.z - rel.x;
	}
	if (flip_y) {
		rel.y = box.w - rel.y;
	}
	if (swap_xy) {
		rel = rel.yx;
		width = box.w;
		height = box.z;
	} else {
		width = box.z;
		height = box.w;
	}
    if (title_bar) {
        title_height = title_bar_height;
        r0 = 0.0;
        if (rel.y <= title_bar_height) {
            // border ring drawn by the title bar itself so it can follow the
            // rounded silhouette (border_width is the title bar border
            // thickness for title bar decorations)
            float tbb = border ? border_width : 0.0;
            // Here, account for radius
            if (title_bar_border_radius > 0.0) {
                float tb_tl = title_bar_border_radius * corners.x;
                float tb_tr = title_bar_border_radius * corners.y;
                vec2 p;
                if (tb_tl > 0.0 && rel.x < tb_tl + 0.5 && rel.y < tb_tl + 0.5) {
                    p = rel - vec2(tb_tl);
                    float r = length(p);
                    if (r > tb_tl - 1.0) {
                        gl_FragColor = antialias(r, tb_tl - 1.0, tb_tl, fw2(r, p),
                            tbb > 0.0 ? border_top : title_bar_color);
                        return;
                    } else if (r > tb_tl - tbb) {
                        gl_FragColor = border_top;
                        return;
                    }
                } else if (tb_tr > 0.0 && rel.x > width - (tb_tr + 0.5) && rel.y < tb_tr + 0.5) {
                    p = rel - vec2(width - tb_tr, tb_tr);
                    float r = length(p);
                    if (r > tb_tr - 1.0) {
                        gl_FragColor = antialias(r, tb_tr - 1.0, tb_tr, fw2(r, p),
                            tbb > 0.0 ? border_top : title_bar_color);
                        return;
                    } else if (r > tb_tr - tbb) {
                        gl_FragColor = border_top;
                        return;
                    }
                }
            }
            if (tbb > 0.0 && (rel.x < tbb || rel.x > width - tbb ||
                    rel.y < tbb || rel.y > title_bar_height - tbb)) {
                gl_FragColor = border_top;
                return;
            }
            gl_FragColor = title_bar_color;
            return;
        }
    } else {
        title_height = 0.0;
        r0 = border_radius;
    }
    if (border) {
		height = height - title_height;
		rel.y = rel.y - title_height;
        float bw = border_width + 0.5;
        if (border_radius > 0.0) {
            float r_tl = r0 * corners.x;
            float r_tr = r0 * corners.y;
            float r_bl = border_radius * corners.z;
            float r_br = border_radius * corners.w;

            vec2 p;
            if (r_tl > 0.0 && rel.x < r_tl + bw && rel.y < r_tl + bw) {
                p = rel - vec2(r_tl + border_width);
                vec4 c;
                if (-p.x < -p.y) {
                    c = border_top;
                } else {
                    c = border_left;
                }
                gl_FragColor = circumference(p, r_tl, c);
                return;
            }
            if (r_tr > 0.0 && rel.x > width - (r_tr + bw) && rel.y < r_tr + bw) {
                p = rel - vec2(width - (r_tr + border_width), r_tr + border_width);
                vec4 c;
                if (p.x < -p.y) {
                    c = border_top;
                } else {
                    c = border_right;
                }
                gl_FragColor = circumference(p, r_tr, c);
                return;
            }
            if (r_bl > 0.0 && rel.x < r_bl + bw && rel.y > height - (r_bl + bw)) {
                p = rel - vec2(r_bl + border_width, height - (r_bl + border_width));
                vec4 c;
                if (-p.x < p.y) {
                    c = border_bottom;
                } else {
                    c = border_left;
                }
                gl_FragColor = circumference(p, r_bl, c);
                return;
            }
            if (r_br > 0.0 && rel.x > width - (r_br + bw) && rel.y > height - (r_br + bw)) {
                p = rel - vec2(width - (r_br + border_width), height - (r_br + border_width));
                vec4 c;
                if (p.x < p.y) {
                    c = border_bottom;
                } else {
                    c = border_right;
                }
                gl_FragColor = circumference(p, r_br, c);
                return;
            }
        }

        if (rel.x < bw) {
            gl_FragColor = antialias(rel.x, 0.0, border_width, 0.5, border_left);
            return;
        }
        if (rel.x > width - bw) {
            gl_FragColor = antialias(rel.x, width - border_width, width, 0.5, border_right);
            return;
        }
        if (rel.y < bw) {
            gl_FragColor = antialias(rel.y, 0.0, border_width, 0.5, border_top);
            return;
        }
        if (rel.y > height - bw) {
            gl_FragColor = antialias(rel.y, height - border_width, height, 0.5, border_bottom);
            return;
        }
    }
	if (dim) {
		gl_FragColor = dim_color;
	} else {
	    discard;
	}
}
