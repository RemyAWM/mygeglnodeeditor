/*
 * gimpnodepad.h
 *
 *  Created on: 21.08.2012
 *      Author: remy
 */

#ifndef GIMPNODEPAD_H_
#define GIMPNODEPAD_H_


#include <gtk/gtk.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS
/* 
 * Стандартные макросы для класса. 
 * Переименуйте "G_OBJ", "GObj"  и "g_obj"  на префикс/имя вашего класса соответственно.
 */
#define GIMP_NODE_PAD_TYPE           	(gimp_node_pad_get_type ())
#define GIMP_NODE_PAD(obj)            	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_NODE_PAD_TYPE, GimpNodePad))
#define GIMP_NODE_PAD_CLASS(klass)    	(G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_NODE_PAD_TYPE, GimpNodePadClass))
#define GIMP_IS_NODE_PAD(obj)         	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_NODE_PAD_TYPE))
#define GIMP_IS_NODE_PAD_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_NODE_PAD_TYPE))
#define GIMP_NODE_PAD_GET_CLASS(obj)  	(G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_NODE_PAD_TYPE, GimpNodePadClass))

enum
{
  	GIMP_NODE_PAD_OUTPUT = 0,
  	GIMP_NODE_PAD_INPUT = 1
};

/*
 * Структуры обекта и класса
 */
typedef struct _GimpNodePad GimpNodePad;
typedef struct _GimpNodePadClass GimpNodePadClass;

struct _GimpNodePad
{
	GtkObject parent_instance;

	/* Pad name - "output, input, aux, etc. ..." */
	const gchar 	*name;
	/* Type: 0 - output; 1 - input; */
	gint			type;

	/* Only for output-type pad */
	//Array of node_item that contains this pad or null if it is an input pad!
	gpointer		*input_nodes;
	//Array of name pads - input, aux
	const gchar   	**input_pad_names;
	gint			num_input_nodes;

	/* Only for input-type pad */
	//Ссылка на исходящий пед к которому ведет связи от этого педа. NULL if none
	gpointer		output_node;
	//Название  исходящего педа - как правило output
	const gchar		*output_pad_name;

	GimpNodePad		*next;		//the next pad in the linked list

	gint x, y, width, height;
};

struct _GimpNodePadClass
{
	GtkObjectClass parent_class;

	/* обработчики сигналов */
	void (* name_changed)		(GimpNodePad* obj);
	void (* connection_added)		(GimpNodePad* pad);
	void (* connection_removed)	(GimpNodePad* pad);

	/* виртуальные функции */
};

GType           gimp_node_pad_get_type       		(void) G_GNUC_CONST;

/*  
 * public function declarations  
 */

GtkObject* 		gimp_node_pad_new 					(gint type);

void			gimp_node_pad_set_name 				(GimpNodePad  *object,
                      	  	  	  	  	 	 	 	 	 const gchar *name);

gboolean		gimp_node_pad_has_connection			(GimpNodePad  *pad);

void 			gimp_node_pad_make_connection		(GimpNodePad *output_pad,
														 gpointer to_node_item,
														 GimpNodePad *input_pad,
														 gpointer from_node_item);

void 			gimp_node_pad_remove_connection 	(GimpNodePad *input_pad,
														 gpointer input_node,
														 GimpNodePad *output_pad,
														 gpointer output_node);

G_END_DECLS

#endif /* GIMPNODEPAD_H_ */
