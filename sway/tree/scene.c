#include "wlr/types/wlr_scene.h"
#include "sway/scene_descriptor.h"
#include "sway/tree/container.h"
#include "sway/tree/root.h"
#include "sway/tree/view.h"

static bool scene_fullscreen_global_enabled(void) {
	return root->fullscreen_global != NULL;
}

static bool scene_overview_workspaces_enabled(void) {
	return false;
}

static void *scene_node_get_workspace(struct wlr_scene_node *node);

static bool scene_node_at(struct wlr_scene_node *node, double lx, double ly,
		struct wlr_scene_node_at_data *data) {
	struct sway_workspace *workspace = scene_node_get_workspace(node);
	if (workspace && data->data && workspace != data->data) {
		return false;
	}
	struct wlr_box *box = wlr_scene_node_info_get_workspace_box(node);
	if (box) {
		if (data->lx < box->x || data->lx >= box->x + box->width ||
				data->ly < box->y || data->ly >= box->y + box->height) {
			return false;
		}
	}

	double rx = data->lx - lx;
	double ry = data->ly - ly;

	if (node->type == WLR_SCENE_NODE_BUFFER) {
		struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);

		double total_scale = 1.0;
		struct wlr_scene_surface *scene_surface =
			wlr_scene_surface_try_from_buffer(scene_buffer);

		if (scene_surface) {
			struct sway_view *view = view_from_wlr_surface(scene_surface->surface);
			if (view) {
				total_scale = 1.0;
			}
		}

		rx /= total_scale;
		ry /= total_scale;
		if (scene_buffer->point_accepts_input &&
				!scene_buffer->point_accepts_input(scene_buffer, &rx, &ry)) {
			return false;
		}
	} else if (node->type == WLR_SCENE_NODE_DECORATION) {
		struct wlr_scene_decoration *deco = wlr_scene_decoration_from_node(node);
		struct sway_view *view = deco->view;
		struct sway_container *con = view ? view->container : NULL;
		if (con && deco == con->title_bar.decoration) {
			// sway title bar fill: keep input for tab focus / drag
		} else if (con && deco == con->decoration.full) {
			double bw = deco->border && deco->border_width > 0 ?
				deco->border_width : 0;
			double tb = deco->title_bar ? deco->title_bar_height : 0;
			const double margin = 1.0;
			bool on_border = rx <= bw + margin || ry <= tb + bw + margin ||
				rx >= deco->width - bw - margin ||
				ry >= deco->height - bw - margin;
			bool dimmed = deco->dim && deco->dim_color[3] > 0.f;
			if (!on_border && !dimmed) {
				return false;
			}
		} else if (con) {
			const double cx = rx + con->pending.x - con->pending.content_x;
			const double cy = ry + con->pending.y - con->pending.content_y;
			const double MARGIN = 1.0;
			if (cx > MARGIN && cx < con->pending.content_width - MARGIN &&
					cy > MARGIN && cy < con->pending.content_height - MARGIN) {
				return false;
			}
		}
	} else if (node->type == WLR_SCENE_NODE_SHADOW) {
		return false;
	}

	data->rx = rx;
	data->ry = ry;
	data->node = node;
	return true;
}

static void *scene_node_get_workspace(struct wlr_scene_node *node) {
	struct wlr_scene_tree *tree;
	if (node->type == WLR_SCENE_NODE_TREE) {
		tree = wlr_scene_tree_from_node(node);
	} else {
		if (node->info.workspace != NULL) {
			return node->info.workspace;
		}
		tree = node->parent;
	}

	while (tree != NULL) {
		if (tree->node.info.workspace != NULL) {
			return tree->node.info.workspace;
		}
		tree = tree->node.parent;
	}
	return NULL;
}

static bool scene_workspace_data(struct wlr_scene_node *node, struct wlr_scene_workspace_data *data) {
	(void)node;
	(void)data;
	return false;
}

static bool scene_view_data(struct wlr_surface *surface, struct wlr_scene_view_data *data) {
	struct sway_view *view = view_from_wlr_surface(surface);
	data->radius_top = data->radius_bottom = 0.0f;
	data->rounded_corners = 0;
	if (view) {
		data->total_scale = 1.0;
		data->wscale = data->hscale = 1.0;
		if (view->container && !container_is_fullscreen_or_child(view->container) &&
				!view->using_csd && view->container->decoration.full) {
			struct wlr_scene_decoration *deco = view->container->decoration.full;
			data->rounded_corners = deco->rounded_corners;
			if (!deco->title_bar) {
				data->radius_top = deco->border_radius;
			}
			data->radius_bottom = deco->border_radius;
		}
		return true;
	}
	data->total_scale = data->wscale = data->hscale = 1.0;
	return false;
}

static bool scene_node_get_parent_total_scale(struct wlr_scene_node *node, double *scale) {
	struct wlr_scene_tree *tree;
	if (node->type == WLR_SCENE_NODE_TREE) {
		tree = wlr_scene_tree_from_node(node);
	} else {
		tree = node->parent;
	}

	while (tree) {
		struct sway_view *view = scene_descriptor_try_get(&tree->node, SWAY_SCENE_DESC_VIEW);
		if (view) {
			*scale = 1.0;
			return &tree->node == node;
		}
		struct sway_popup_desc *desc = scene_descriptor_try_get(&tree->node, SWAY_SCENE_DESC_POPUP);
		if (desc && desc->view) {
			*scale = 1.0;
			return &tree->node == node;
		}
		tree = tree->node.parent;
	}
	if (node->type == WLR_SCENE_NODE_BUFFER) {
		struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
		struct wlr_scene_surface *scene_surface =
			wlr_scene_surface_try_from_buffer(scene_buffer);

		if (scene_surface) {
			struct sway_view *view = view_from_wlr_surface(scene_surface->surface);
			if (view) {
				*scale = 1.0;
				return true;
			}
		}
	}
	*scale = -1.0;
	return false;
}

static double scene_view_content_scale(struct wlr_surface *surface) {
	(void)surface;
	return 1.0;
}

static bool scene_layer_surface_data(struct wlr_layer_surface_v1 *layer_surface,
		struct wlr_scene_layer_surface_data *data) {
	data->x = data->y = 0.0;
	if (layer_surface->surface->mapped) {
		data->width = layer_surface->surface->current.width;
		data->height = layer_surface->surface->current.height;
	} else {
		data->width = layer_surface->current.desired_width;
		data->height = layer_surface->current.desired_height;
	}
	return true;
}

static void scene_animate(struct wlr_output *output) {
	(void)output;
}

const struct wlr_scene_callbacks swayng_scene_cbs = {
	.fullscreen_global_enabled = scene_fullscreen_global_enabled,
	.overview_workspaces_enabled = scene_overview_workspaces_enabled,
	.node_at = scene_node_at,
	.workspace_data = scene_workspace_data,
	.view_data = scene_view_data,
	.node_get_parent_total_scale = scene_node_get_parent_total_scale,
	.view_content_scale = scene_view_content_scale,
	.layer_surface_data = scene_layer_surface_data,
	.animate = scene_animate,
};
