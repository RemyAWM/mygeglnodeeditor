/*
 * gimpnodeitem.h
 *
 *  Created on: 18.08.2012
 *      Author: remy
 */

#ifndef GIMPNODEITEM_H_
#define GIMPNODEITEM_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gegl.h>
#include "gimpnodepad.h"


G_BEGIN_DECLS
/* 
 * Рисует окно нода на GimpNodeView
 *
 */
#define GIMP_NODE_ITEM_TYPE           	(gimp_node_item_get_type ())
#define GIMP_NODE_ITEM(obj)            	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_NODE_ITEM_TYPE, GimpNodeItem))
#define GIMP_NODE_ITEM_CLASS(klass)    	(G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_NODE_ITEM_TYPE, GimpNodeItemClass))
#define GIMP_IS_NODE_ITEM(obj)         	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_NODE_ITEM_TYPE))
#define GIMP_IS_NODE_ITEM_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_NODE_ITEM_TYPE))
#define GIMP_NODE_ITEM_GET_CLASS(obj)  	(G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_NODE_ITEM_TYPE, GimpNodeItemClass))


enum {
	NODE_ITEM_PART_NONE = 0,
	NODE_ITEM_PART_TITLE,
	NODE_ITEM_PART_BODY,
	NODE_ITEM_PART_PAD,
	NODE_ITEM_PART_RESIZE
};

/*
 * Структуры обекта и класса
 */
typedef struct _GimpNodeItem GimpNodeItem;
typedef struct _GimpNodeItemClass GimpNodeItemClass;

struct _GimpNodeItem
{
	GObject 	parent_instance;

	gint 				x, y, width, height;
	gint				title_height;
	GimpNodePad			*inputs;
	GimpNodePad			*outputs;

	/*pointer at real node struct (gegl, etc. ...)*/
	gpointer			node;
};

struct _GimpNodeItemClass
{
	GObjectClass parent_class;

	void (* name_changed)			(GimpNodeItem* obj);
	void (* node_changed)			(GimpNodeItem* obj);
	void (* node_deleted)			(GimpNodeItem* obj);
	void (* connection_removed)		(GimpNodeItem* obj, GimpNodePad *pad);
	void (* connection_added)		(GimpNodeItem* obj, GimpNodePad *pad);

	void 		(* set_pads)		(GimpNodeItem* node_item);
	gpointer 	(* find_real_node) 	(GimpNodeItem *node_item, gpointer real_node);
};

GType           gimp_node_item_get_type          (void) G_GNUC_CONST;

/* "Imported" non-API functions from gegl lib: */
gpointer 		gegl_node_get_pad(GeglNode* self, const gchar* name);
const gchar*	gegl_pad_get_name(gpointer pad);
GSList*			gegl_node_get_pads(GeglNode *self);
GSList*			gegl_node_get_input_pads(GeglNode *self);


/*  
 * Public function declarations
 */
GObject* 			gimp_node_item_new 				(gpointer node,
														 GimpNodeItem *prev_node_item);

void				gimp_node_item_set_name 			(GimpNodeItem  *object,
                      	  	  	  	  	 	 	 	 	 const gchar *name);

gchar*				gimp_node_item_name				(GimpNodeItem  *node_item);

void 				gimp_node_item_draw				(GimpNodeItem  *node_item,
														 GtkWidget *view,
														 cairo_t *cr);

GimpNodeItem*  		gimp_node_item_next				(GimpNodeItem *node_item);

GimpNodeItem*  		gimp_node_item_last				(GimpNodeItem *node_item);

gint 				gimp_node_item_part 				(GimpNodeItem  *node_item,
														 gint x,
														 gint y);

GimpNodeItem* 		gimp_node_item_make_lasted		(GimpNodeItem  *node_item,
														 GimpNodeItem  *first_node);

gpointer 			gimp_node_item_find_real_node 	(GimpNodeItem *node_item,
														 gpointer real_node);

GimpNodePad* 		gimp_node_item_get_pad_by_name 	(GimpNodeItem  *node_item,
														 const gchar *pad_name);

GimpNodePad*		gimp_node_item_pick_pad 			(GimpNodeItem  *node_item,
														 gint x,
														 gint y);

void 				gimp_node_item_remove_item		(GimpNodeItem	*node_item,
														 GimpNodeItem  *first_node);

G_END_DECLS

#endif /* GIMPNODEITEM_H_ */
