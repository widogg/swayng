#extension GL_OES_EGL_image_external : require

precision highp float;

varying vec2 v_texcoord;
uniform samplerExternalOES texture0;
uniform float alpha;
uniform vec4 box;
uniform bool swap_xy;
uniform bool flip_x;
uniform bool flip_y;
uniform float radius_top;
uniform float radius_bottom;
uniform vec4 corners; // rounded corner mask: tl, tr, bl, br

float antialias(float x, float x0, float x1, float fw) {
    float xmax = max(x1, x + fw);
    float xmin = min(x0, x - fw);
    float len = xmax - xmin;
    float d0 = abs(x + fw - x1);
    float d1 = abs(x - fw - x0);
    float overlap = len - d0 - d1;
    float alpha = smoothstep(0.0, 1.0, overlap);
    return alpha;
}

float fw2(float r, vec2 p) {
    vec2 ap = abs(p);
    return 0.5 * r / max(ap.x, ap.y);
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
        vec2 p = rel - vec2(r_tl);
        float r = length(p);
        if (r > r_tl - 1.0) {
            float opacity = antialias(r, r_tl - 1.0, r_tl, fw2(r, p));
            gl_FragColor = texture2D(texture0, v_texcoord) * alpha * opacity;
            return;
        }
    }
    if (r_tr > 0.0 && rel.x > width - (r_tr + 0.5) && rel.y < r_tr + 0.5) {
        vec2 p = rel - vec2(width - r_tr, r_tr);
        float r = length(p);
        if (r > r_tr - 1.0) {
            float opacity = antialias(r, r_tr - 1.0, r_tr, fw2(r, p));
            gl_FragColor = texture2D(texture0, v_texcoord) * alpha * opacity;
            return;
        }
    }
    if (r_bl > 0.0 && rel.x < r_bl + 0.5 && rel.y > height - (r_bl + 0.5)) {
        vec2 p = rel - vec2(r_bl, height - r_bl);
        float r = length(p);
        if (r > r_bl - 1.0) {
            float opacity = antialias(r, r_bl - 1.0, r_bl, fw2(r, p));
            gl_FragColor = texture2D(texture0, v_texcoord) * alpha * opacity;
            return;
        }
    }
    if (r_br > 0.0 && rel.x > width - (r_br + 0.5) && rel.y > height - (r_br + 0.5)) {
        vec2 p = rel - vec2(width - r_br, height - r_br);
        float r = length(p);
        if (r > r_br - 1.0) {
            float opacity = antialias(r, r_br - 1.0, r_br, fw2(r, p));
            gl_FragColor = texture2D(texture0, v_texcoord) * alpha * opacity;
            return;
        }
    }
	gl_FragColor = texture2D(texture0, v_texcoord) * alpha;
}
