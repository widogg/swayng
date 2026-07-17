#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

// struct wlr_vk_frag_texture_pcr_data
layout(push_constant, row_major) uniform UBO {
	layout(offset = 80) mat4 matrix;
	float alpha;
	float luminance_multiplier;
	bool swap_xy;
	bool flip_x;
	bool flip_y;
	float radius_top;
	float radius_bottom;
	int rounded_corners;
	vec4 box;
} data;

layout (constant_id = 0) const int TEXTURE_TRANSFORM = 0;

// Matches enum wlr_vk_texture_transform
#define TEXTURE_TRANSFORM_IDENTITY 0
#define TEXTURE_TRANSFORM_SRGB 1
#define TEXTURE_TRANSFORM_ST2084_PQ 2
#define TEXTURE_TRANSFORM_GAMMA22 3
#define TEXTURE_TRANSFORM_BT1886 4

float srgb_channel_to_linear(float x) {
	return mix(x / 12.92,
		pow((x + 0.055) / 1.055, 2.4),
		x > 0.04045);
}

vec3 srgb_color_to_linear(vec3 color) {
	return vec3(
		srgb_channel_to_linear(color.r),
		srgb_channel_to_linear(color.g),
		srgb_channel_to_linear(color.b)
	);
}

vec3 pq_color_to_linear(vec3 color) {
	float inv_m1 = 1 / 0.1593017578125;
	float inv_m2 = 1 / 78.84375;
	float c1 = 0.8359375;
	float c2 = 18.8515625;
	float c3 = 18.6875;
	vec3 num = max(pow(color, vec3(inv_m2)) - c1, 0);
	vec3 denom = c2 - c3 * pow(color, vec3(inv_m2));
	return pow(num / denom, vec3(inv_m1));
}

vec3 bt1886_color_to_linear(vec3 color) {
	float Lmin = 0.01;
	float Lmax = 100.0;
	float lb = pow(Lmin, 1.0 / 2.4);
	float lw = pow(Lmax, 1.0 / 2.4);
	float a  = pow(lw - lb, 2.4);
	float b  = lb / (lw - lb);
	vec3 L = a * pow(color + vec3(b), vec3(2.4));
	return (L - Lmin) / (Lmax - Lmin);
}

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
	vec4 in_color = textureLod(tex, uv, 0);

	// Convert from pre-multiplied alpha to straight alpha
	float alpha = in_color.a;
	vec3 rgb;
	if (alpha == 0) {
		rgb = vec3(0);
	} else {
		rgb = in_color.rgb / alpha;
	}

	if (TEXTURE_TRANSFORM != TEXTURE_TRANSFORM_IDENTITY) {
		rgb = max(rgb, vec3(0));
	}
	if (TEXTURE_TRANSFORM == TEXTURE_TRANSFORM_SRGB) {
		rgb = srgb_color_to_linear(rgb);
	} else if (TEXTURE_TRANSFORM == TEXTURE_TRANSFORM_ST2084_PQ) {
		rgb = pq_color_to_linear(rgb);
	} else if (TEXTURE_TRANSFORM == TEXTURE_TRANSFORM_GAMMA22) {
		rgb = pow(rgb, vec3(2.2));
	} else if (TEXTURE_TRANSFORM == TEXTURE_TRANSFORM_BT1886) {
		rgb = bt1886_color_to_linear(rgb);
	}

	rgb *= data.luminance_multiplier;

	rgb = mat3(data.matrix) * rgb;

	// Back to pre-multiplied alpha
	out_color = vec4(rgb * alpha, alpha);

    vec2 pos = vec2(gl_FragCoord);
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
    // rounded corner mask: bit 0 = tl, 1 = tr, 2 = bl, 3 = br
    float r_tl = data.radius_top * float((data.rounded_corners & 1) != 0);
    float r_tr = data.radius_top * float((data.rounded_corners & 2) != 0);
    float r_bl = data.radius_bottom * float((data.rounded_corners & 4) != 0);
    float r_br = data.radius_bottom * float((data.rounded_corners & 8) != 0);
    if (r_tl > 0.0 && rel.x < r_tl + 0.5 && rel.y < r_tl + 0.5) {
        vec2 p = rel - vec2(r_tl);
        float r = length(p);
        if (r > r_tl - 1.0) {
            float opacity = antialias(r, r_tl - 1.0, r_tl, fw2(r, p));
            out_color *= data.alpha * opacity;
            return;
        }
    }
    if (r_tr > 0.0 && rel.x > width - (r_tr + 0.5) && rel.y < r_tr + 0.5) {
        vec2 p = rel - vec2(width - r_tr, r_tr);
        float r = length(p);
        if (r > r_tr - 1.0) {
            float opacity = antialias(r, r_tr - 1.0, r_tr, fw2(r, p));
            out_color *= data.alpha * opacity;
            return;
        }
    }
    if (r_bl > 0.0 && rel.x < r_bl + 0.5 && rel.y > height - (r_bl + 0.5)) {
        vec2 p = rel - vec2(r_bl, height - r_bl);
        float r = length(p);
        if (r > r_bl - 1.0) {
            float opacity = antialias(r, r_bl - 1.0, r_bl, fw2(r, p));
            out_color *= data.alpha * opacity;
            return;
        }
    }
    if (r_br > 0.0 && rel.x > width - (r_br + 0.5) && rel.y > height - (r_br + 0.5)) {
        vec2 p = rel - vec2(width - r_br, height - r_br);
        float r = length(p);
        if (r > r_br - 1.0) {
            float opacity = antialias(r, r_br - 1.0, r_br, fw2(r, p));
            out_color *= data.alpha * opacity;
            return;
        }
    }

	out_color *= data.alpha;
}
