/*
 * gimpnodeeditor.h
 *
 *  Created on: 16.08.2012
 *      Author: remy
 */

#ifndef GIMPNODEEDITOR_H_
#define GIMPNODEEDITOR_H_


#include <glib-object.h>
#include <gtk/gtk.h>
#include <gegl.h>

#include "gimpnodeview.h"


G_BEGIN_DECLS
/* 
 * Стандартные макросы для класса. 
 * Переименуйте "G_OBJ", "G_IS_OBJ", "GObj"  и "g_obj"  на префикс/имя вашего класса соответственно.
 */
#define GIMP_NODE_EDITOR_TYPE           	(gimp_node_editor_get_type ())
#define GIMP_NODE_EDITOR(obj)            	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_NODE_EDITOR_TYPE, GimpNodeEditor))
#define GIMP_NODE_EDITOR_CLASS(klass)    	(G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_NODE_EDITOR_TYPE, GimpNodeEditorClass))
#define GIMP_IS_NODE_EDITOR(obj)         	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_NODE_EDITOR_TYPE))
#define GIMP_IS_NODE_EDITOR_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_NODE_EDITOR_TYPE))
#define GIMP_NODE_EDITOR_GET_CLASS(obj)  	(G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_NODE_EDITOR_TYPE, GimpNodeEditorClass))

/*
 * Структуры обекта и класса
 */
typedef struct _GimpNodeEditor GimpNodeEditor;
typedef struct _GimpNodeEditorClass GimpNodeEditorClass;

struct _GimpNodeEditor
{
	GtkFrame parent_instance;

	GtkWidget *vbox;
};

struct _GimpNodeEditorClass
{
	GtkFrameClass parent_class;

	void 		(* name_changed)				(GimpNodeEditor* obj);
	void 		(* updated)					(GimpNodeEditor* obj);

	void 		(* set_graph) 			(GimpNodeEditor  *editor, gpointer node_graph);
	gpointer 	(* create_real_node) 	(gpointer graph, const gchar *operation);
};


GType           		gimp_node_editor_get_type     	(void) G_GNUC_CONST;

/*  
 * public function declarations  
 */

GtkWidget* 				gimp_node_editor_new 				(gpointer gegl_graph, gpointer sink_node);

void					gimp_node_editor_set_name 		(GimpNodeEditor  *object,
                      	  	  	  	  	 	 	 	 	 	 const gchar *name);

//Установить граф в редактор.
void 					gimp_node_editor_set_graph		(GimpNodeEditor  *editor,
															 gpointer gegl_graph);

void					gimp_node_editor_set_sink_node	(GimpNodeEditor  *editor,
															 gpointer sink_node);

gpointer				gimp_node_editor_get_sink_node	(GimpNodeEditor  *editor);

gboolean				gimp_node_editor_add_node_item 		(GimpNodeEditor *editor,
															 gpointer graph,
															 const gchar *operation);

G_END_DECLS

#endif /* GIMPNODEEDITOR_H_ */
