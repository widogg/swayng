precision highp float;

varying vec2 v_texcoord;
uniform sampler2D tex;
uniform float alpha;
uniform vec4 box;
uniform bool swap_xy;
uniform bool flip_x;
uniform bool flip_y;
uniform float radius_top;
uniform float radius_bottom;
uniform vec4 corners; // rounded corner mask: tl, tr, bl, br

// analytic ~1px box-filter coverage of a disc of radius r at distance d
float circle_cov(float d, float r) {
    return clamp(r + 0.5 - d, 0.0, 1.0);
}

void main() {
    vec2 pos = vec2(gl_FragCoord);
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
    float r_tl = radius_top * corners.x;
    float r_tr = radius_top * corners.y;
    float r_bl = radius_bottom * corners.z;
    float r_br = radius_bottom * corners.w;
    if (r_tl > 0.0 && rel.x < r_tl + 0.5 && rel.y < r_tl + 0.5) {
        float cov = circle_cov(length(rel - vec2(r_tl)), r_tl);
        if (cov < 1.0) {
            gl_FragColor = vec4(texture2D(tex, v_texcoord).rgb, 1.0) * alpha * cov;
            return;
        }
    }
    if (r_tr > 0.0 && rel.x > width - (r_tr + 0.5) && rel.y < r_tr + 0.5) {
        float cov = circle_cov(length(rel - vec2(width - r_tr, r_tr)), r_tr);
        if (cov < 1.0) {
            gl_FragColor = vec4(texture2D(tex, v_texcoord).rgb, 1.0) * alpha * cov;
            return;
        }
    }
    if (r_bl > 0.0 && rel.x < r_bl + 0.5 && rel.y > height - (r_bl + 0.5)) {
        float cov = circle_cov(length(rel - vec2(r_bl, height - r_bl)), r_bl);
        if (cov < 1.0) {
            gl_FragColor = vec4(texture2D(tex, v_texcoord).rgb, 1.0) * alpha * cov;
            return;
        }
    }
    if (r_br > 0.0 && rel.x > width - (r_br + 0.5) && rel.y > height - (r_br + 0.5)) {
        float cov = circle_cov(length(rel - vec2(width - r_br, height - r_br)), r_br);
        if (cov < 1.0) {
            gl_FragColor = vec4(texture2D(tex, v_texcoord).rgb, 1.0) * alpha * cov;
            return;
        }
    }
	gl_FragColor = vec4(texture2D(tex, v_texcoord).rgb, 1.0) * alpha;
}
