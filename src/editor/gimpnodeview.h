/*
 * gimpnodeview.h
 *
 *  Created on: 17.08.2012
 *      Author: remy
 */

#ifndef GIMPNODEVIEW_H_
#define GIMPNODEVIEW_H_


#include <glib-object.h>
#include <gtk/gtk.h>

#include "gimpnodeitem.h"

G_BEGIN_DECLS
/* 
 * GimpNodeView - отрисовывает окна нодов.
 *
 */
#define GIMP_NODE_VIEW_TYPE           	(gimp_node_view_get_type ())
#define GIMP_NODE_VIEW(obj)            	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_NODE_VIEW_TYPE, GimpNodeView))
#define GIMP_NODE_VIEW_CLASS(klass)    	(G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_NODE_VIEW_TYPE, GimpNodeViewClass))
#define GIMP_IS_NODE_VIEW(obj)         	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_NODE_VIEW_TYPE))
#define GIMP_IS_NODE_VIEW_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_NODE_VIEW_TYPE))
#define GIMP_NODE_VIEW_GET_CLASS(obj)  	(G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_NODE_VIEW_TYPE, GimpNodeViewClass))

/*
 * Структуры обекта и класса
 */

typedef struct _GimpNodeView GimpNodeView;
typedef struct _GimpNodeViewClass GimpNodeViewClass;

struct _GimpNodeView
{

	GtkDrawingArea parent_instance;

	/*current mouse coordinates*/
	gint		px, py;

	/*last mouse coordinates when mouse button pressed*/
	gint		dx, dy;

	GimpNodeItem*	resized_node;
	GimpNodeItem*	dragged_node;
	GimpNodePad*	gragged_pad;
	GimpNodeItem*	selected_node;
};

struct _GimpNodeViewClass
{
	/* родительский класс */
	GtkDrawingAreaClass parent_class;

	/* обработчики сигналов */
	void (* name_changed)		(GimpNodeView* obj);

	void (* node_added)			(GimpNodeView* wiew, GimpNodeItem *node_item);

	void (* node_removed)		(GimpNodeView* wiew, GimpNodeItem *node_item);

	void (* node_selected)		(GimpNodeView* wiew, GimpNodeItem *node_item);

	void (* node_changed)		(GimpNodeView* wiew, GimpNodeItem *node_item);

	void (* connection_added)	(GimpNodeView* wiew, GimpNodeItem *node_item,GimpNodePad *pad);

	void (* connection_removed)	(GimpNodeView* wiew, GimpNodeItem *node_item,GimpNodePad *pad);
	/* виртуальные функции */
};

GType           gimp_node_view_get_type          (void) G_GNUC_CONST;

/*  
 * public function declarations  
 */

GtkWidget* 		gimp_node_view_new 				(void);

void			gimp_node_view_set_name 			(GimpNodeView  *view,
                      	  	  	  	  	 	 	 	 const gchar *name);

void 			gimp_node_view_set_item			(GimpNodeView  *view,
												 	 GimpNodeItem *node_item);

GimpNodeItem*  	gimp_node_view_pick_node_at		(GimpNodeView  *view);

void 			gimp_node_view_set_connections	(GimpNodeView  *view,
													 gpointer graph);

void 			gimp_node_view_connect_pads 		(GimpNodeView  *view,
													 GimpNodeItem  *node_item,
													 gpointer real_node);

GimpNodeItem 	*gimp_node_view_get_node_by_real(GimpNodeView *view,
													 gpointer real_node);

void 			gimp_node_view_remove_node_item(GimpNodeView *view,
													GimpNodeItem *node_item);

GimpNodeItem*	gimp_node_view_first_node_item	(GimpNodeView *view);

G_END_DECLS

#endif /* GIMPNODEVIEW_H_ */
