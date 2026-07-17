#version 450

layout(location = 0) out vec4 out_color;
layout(binding = 0) uniform UBO {
	bool border;
	bool title_bar;
	bool dim;
	float border_width;
	float border_radius;
	float title_bar_height;
	float title_bar_border_radius;
	bool swap_xy;
	bool flip_x;
	bool flip_y;
	int rounded_corners; // bit 0 = tl, 1 = tr, 2 = bl, 3 = br
	float pad2;
	vec4 box;
	vec4 border_top;
	vec4 border_bottom;
	vec4 border_left;
	vec4 border_right;
	vec4 title_bar_color;
	vec4 dim_color;
	vec4 pad3;
} data;

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
    vec4 color = antialias(d, r, r + data.border_width, fw2(d, p), c);
	if (data.dim && d < r + 1.0) {
		return mix(data.dim_color, color, color.a);
	}
	return color;
}

void main() {
    vec2 pos = vec2(gl_FragCoord);
    float title_height;
    float r0, r1;
    vec2 rel = pos.xy - data.box.xy;
	float width, height;
	if (data.flip_x) {
		rel.x = data.box.z - rel.x;
	}
	if (data.flip_y) {
		rel.y = data.box.w - rel.y;
	}
	if (data.swap_xy) {
		rel = rel.yx;
		width = data.box.w;
		height = data.box.z;
	} else {
		width = data.box.z;
		height = data.box.w;
	}
    if (data.title_bar) {
        title_height = data.title_bar_height;
        r0 = 0.0;
        if (rel.y <= data.title_bar_height) {
            // border ring drawn by the title bar itself so it can follow the
            // rounded silhouette (border_width is the title bar border
            // thickness for title bar decorations)
            float tbb = data.border ? data.border_width : 0.0;
            // Here, account for radius
            if (data.title_bar_border_radius > 0.0) {
                float tb_tl = data.title_bar_border_radius * float((data.rounded_corners & 1) != 0);
                float tb_tr = data.title_bar_border_radius * float((data.rounded_corners & 2) != 0);
                vec2 p;
                if (tb_tl > 0.0 && rel.x < tb_tl + 0.5 && rel.y < tb_tl + 0.5) {
                    p = rel - vec2(tb_tl);
                    float r = length(p);
                    if (r > tb_tl - 1.0) {
                        out_color = antialias(r, tb_tl - 1.0, tb_tl, fw2(r, p),
                            tbb > 0.0 ? data.border_top : data.title_bar_color);
                        return;
                    } else if (r > tb_tl - tbb) {
                        out_color = data.border_top;
                        return;
                    }
                } else if (tb_tr > 0.0 && rel.x > width - (tb_tr + 0.5) && rel.y < tb_tr + 0.5) {
                    p = rel - vec2(width - tb_tr, tb_tr);
                    float r = length(p);
                    if (r > tb_tr - 1.0) {
                        out_color = antialias(r, tb_tr - 1.0, tb_tr, fw2(r, p),
                            tbb > 0.0 ? data.border_top : data.title_bar_color);
                        return;
                    } else if (r > tb_tr - tbb) {
                        out_color = data.border_top;
                        return;
                    }
                }
            }
            if (tbb > 0.0 && (rel.x < tbb || rel.x > width - tbb ||
                    rel.y < tbb || rel.y > data.title_bar_height - tbb)) {
                out_color = data.border_top;
                return;
            }
            out_color = data.title_bar_color;
            return;
        }
    } else {
        title_height = 0.0;
        r0 = data.border_radius;
    }
    if (data.border) {
		height = height - title_height;
		rel.y = rel.y - title_height;
        float bw = data.border_width + 0.5;
        if (data.border_radius > 0.0) {
            float r_tl = r0 * float((data.rounded_corners & 1) != 0);
            float r_tr = r0 * float((data.rounded_corners & 2) != 0);
            float r_bl = data.border_radius * float((data.rounded_corners & 4) != 0);
            float r_br = data.border_radius * float((data.rounded_corners & 8) != 0);

            vec2 p;
            if (r_tl > 0.0 && rel.x < r_tl + bw && rel.y < r_tl + bw) {
                p = rel - vec2(r_tl + data.border_width);
                vec4 c;
                if (-p.x < -p.y) {
                    c = data.border_top;
                } else {
                    c = data.border_left;
                }
                out_color = circumference(p, r_tl, c);
                return;
            }
            if (r_tr > 0.0 && rel.x > width - (r_tr + bw) && rel.y < r_tr + bw) {
                p = rel - vec2(width - (r_tr + data.border_width), r_tr + data.border_width);
                vec4 c;
                if (p.x < -p.y) {
                    c = data.border_top;
                } else {
                    c = data.border_right;
                }
                out_color = circumference(p, r_tr, c);
                return;
            }
            if (r_bl > 0.0 && rel.x < r_bl + bw && rel.y > height - (r_bl + bw)) {
                p = rel - vec2(r_bl + data.border_width, height - (r_bl + data.border_width));
                vec4 c;
                if (-p.x < p.y) {
                    c = data.border_bottom;
                } else {
                    c = data.border_left;
                }
                out_color = circumference(p, r_bl, c);
                return;
            }
            if (r_br > 0.0 && rel.x > width - (r_br + bw) && rel.y > height - (r_br + bw)) {
                p = rel - vec2(width - (r_br + data.border_width), height - (r_br + data.border_width));
                vec4 c;
                if (p.x < p.y) {
                    c = data.border_bottom;
                } else {
                    c = data.border_right;
                }
                out_color = circumference(p, r_br, c);
                return;
            }
        }

        if (rel.x < bw) {
            out_color = antialias(rel.x, 0.0, data.border_width, 0.5, data.border_left);
            return;
        }
        if (rel.x > width - bw) {
            out_color = antialias(rel.x, width - data.border_width, width, 0.5, data.border_right);
            return;
        }
        if (rel.y < bw) {
            out_color = antialias(rel.y, 0.0, data.border_width, 0.5, data.border_top);
            return;
        }
        if (rel.y > height - bw) {
            out_color = antialias(rel.y, height - data.border_width, height, 0.5, data.border_bottom);
            return;
        }
    }
	if (data.dim) {
		out_color = data.dim_color;
	} else {
	    discard;
	}
}
