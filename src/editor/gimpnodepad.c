/*
 * gimpnodepad.c
 *
 *  Created on: 21.08.2012
 *      Author: remy
 */

#include <string.h>
#include "gimpnodepad.h"


enum
{
	CONNECTION_REMOVED,
	CONNECTION_ADDED,
  	NAME_CHANGED,
  	LAST_SIGNAL
};


enum
{
	PROP_0,
	PROP_NAME
};

/*
 * Структура приватного объекта.
 */

static void    gimp_node_pad_class_init       (GimpNodePadClass 		*klass);

static void    gimp_node_pad_init             (GimpNodePad      		*object);

static void    gimp_node_pad_finalize         (GObject         *object);

static void    gimp_node_pad_set_property     (GObject          *object,
                                                 	 guint        		property_id,
                                                 	 const GValue  		*value,
                                                 	 GParamSpec     	*pspec);

static void    gimp_node_pad_get_property     (GObject            *object,
                                                 	 guint               property_id,
                                                 	 GValue             *value,
                                                 	 GParamSpec         *pspec);

G_DEFINE_TYPE (GimpNodePad, gimp_node_pad, GTK_TYPE_OBJECT)

/*
 * Сылка на родительский класс. Измените на нужный вам класс: 
 */
#define parent_class gimp_node_pad_parent_class

static guint gimp_node_pad_signals[LAST_SIGNAL] = { 0 };


static void    
gimp_node_pad_class_init (GimpNodePadClass *klass)
{
	GObjectClass      	*object_class      	= G_OBJECT_CLASS (klass);

	
	/* Регистрируем сигналы */
	gimp_node_pad_signals[NAME_CHANGED]=
		g_signal_new("name_changed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodePadClass, name_changed),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);

	gimp_node_pad_signals[CONNECTION_ADDED]=
		g_signal_new("connection-added",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodePadClass, connection_added),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);

	gimp_node_pad_signals[CONNECTION_REMOVED]=
		g_signal_new("connection-removed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodePadClass, connection_removed),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);

	object_class->finalize		= 	gimp_node_pad_finalize;
	object_class->set_property 	= 	gimp_node_pad_set_property;
	object_class->get_property 	= 	gimp_node_pad_get_property;

	klass->connection_added		=	NULL;
	klass->connection_removed	=	NULL;
	klass->name_changed			=	NULL;

	/* Регистрируем параметры объекта */
  	g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL,
                                                        NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT)));
}

static void
gimp_node_pad_init (GimpNodePad *object)
{
	object->height			=	0;
	object->width			=	0;
	object->x				=	0;
	object->y				=	0;

	object->next			=	NULL;

	object->input_nodes		=	NULL;
	object->input_pad_names	=	NULL;
	object->num_input_nodes	=	0;

	object->output_node		=	NULL;
	object->output_pad_name	=	NULL;

	object->type			=	GIMP_NODE_PAD_OUTPUT;
}

static void
gimp_node_pad_finalize (GObject *object)
{
	GimpNodePad *pad = GIMP_NODE_PAD (object);
	if (pad->name)
	    {
	      g_object_unref (pad->name);
	      pad->name = NULL;
	    }
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_node_pad_set_property (GObject *object,
                       	   guint 		property_id,
                       	   const 		GValue *value,
                       	   GParamSpec 	*pspec)
{
	  GimpNodePad *gimp_node_pad = GIMP_NODE_PAD (object);

	  switch (property_id)
	    {
	    case PROP_NAME:
	    	gimp_node_pad_set_name (gimp_node_pad, g_value_get_string (value));
	      break;

	    default:
	      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	      break;
	    }
}

static void
gimp_node_pad_get_property (GObject *object,
                           guint         property_id,
                           GValue        *value,
                           GParamSpec    *pspec)
{

	GimpNodePad *pad = GIMP_NODE_PAD (object);

	  switch (property_id)
	    {
	    case PROP_NAME:
	      if (pad->name)
	        g_value_set_string (value, pad->name);
	      break;
	    default:
	      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	      break;
	    }
}


/* Public functions */
GtkObject*
gimp_node_pad_new(gint type)
{
	GimpNodePad *pad = g_object_new (GIMP_NODE_PAD_TYPE,NULL);

	pad->type = type;

	return GTK_OBJECT(pad);
}

void
gimp_node_pad_set_name (GimpNodePad  *object,
                      const gchar *name)
{
  g_return_if_fail (GIMP_IS_NODE_PAD (object));

  if (! g_strcmp0 (object->name, name))
    return;

  object->name = g_strdup (name);

  g_signal_emit (object, gimp_node_pad_signals[NAME_CHANGED], 0);

  g_object_notify (G_OBJECT (object), "name");
}

gboolean
gimp_node_pad_has_connection(GimpNodePad  *pad)
{
	g_return_val_if_fail  (GIMP_IS_NODE_PAD (pad), FALSE);

	if(pad->type == GIMP_NODE_PAD_INPUT)
	{
		if(pad->output_node && pad->output_pad_name)
			return TRUE;
	}
	else
	{
		if(pad->input_nodes && pad->input_pad_names && (pad->num_input_nodes > 0))
			return TRUE;
	}

	return FALSE;
}

/*
 * Set connection between output and input pads.
 * output_pad -output pad;
 * to_node_item - output node;
 * input_pad - input pad
 * from_node_item - input pad;
 *
 * */
void
gimp_node_pad_make_connection (GimpNodePad *output_pad,
									gpointer to_node_item,
									GimpNodePad *input_pad,
									gpointer from_node_item)
{
	g_return_if_fail (GIMP_IS_NODE_PAD (output_pad));
	g_return_if_fail (GIMP_IS_NODE_PAD (input_pad));
	g_return_if_fail (to_node_item);
	g_return_if_fail (from_node_item);
	g_return_if_fail (output_pad->type == GIMP_NODE_PAD_OUTPUT);

	const gchar *input_pad_name = input_pad->name;

	//Если такое соединение уже есть...
	gint i;
	for(i=0;  i<output_pad->num_input_nodes; i++)
	{
		if(output_pad->input_nodes[i] == to_node_item &&
				!g_strcmp0(output_pad->input_pad_names[i], input_pad_name))
		{
			return;
		}
	}

	output_pad->num_input_nodes++;

	gint ch_size = (output_pad->num_input_nodes + 1) * sizeof (gpointer);
	gpointer *input_nodes = g_malloc (ch_size);

	gint sh_size = (output_pad->num_input_nodes + 1) * sizeof (gchar *);

	for(i=0; i<output_pad->num_input_nodes; i++)
	{
		if(i < output_pad->num_input_nodes-1)
		{
			sh_size+=strlen(output_pad->input_pad_names[i])+1;
		}
		else
		{
			sh_size+=strlen(input_pad_name)+1;
		}
	}

	const gchar **input_pad_names = g_malloc (sh_size);

	gint new_num_input = 0;
	for(i=0; i<output_pad->num_input_nodes; i++)
	{
		if(i < output_pad->num_input_nodes-1)
		{
			input_nodes[i]  =	 output_pad->input_nodes[i];
			input_pad_names[i] = output_pad->input_pad_names[i];
		}
		else
		{
			input_nodes[i] = to_node_item;
			input_pad_names[i] = input_pad_name;
		}
		new_num_input++;
	}

	g_free(output_pad->input_nodes);
	g_free(output_pad->input_pad_names);

	output_pad->input_nodes = g_malloc(ch_size);
	output_pad->input_pad_names = g_malloc(sh_size);

	memcpy(output_pad->input_nodes, input_nodes, ch_size);
	memcpy(output_pad->input_pad_names, input_pad_names,sh_size);

	input_pad->output_node = from_node_item;
	input_pad->output_pad_name = output_pad->name;

	g_free(input_nodes);
	g_free(input_pad_names);

	g_signal_emit (output_pad, gimp_node_pad_signals[CONNECTION_ADDED], 0);
}


void
gimp_node_pad_remove_connection (GimpNodePad *input_pad,
											gpointer input_node,
											GimpNodePad *output_pad,
											gpointer output_node)
{
	g_return_if_fail (GIMP_IS_NODE_PAD (input_pad));
	g_return_if_fail (GIMP_IS_NODE_PAD (output_pad));
	g_return_if_fail (input_node || output_node);

	gint i;
	gint new_num_input = output_pad->num_input_nodes-1;

	gint ch_size = (new_num_input + 1) * sizeof (gpointer);
	gpointer *input_nodes = g_malloc (ch_size);

	gint sh_size = (new_num_input + 1) * sizeof (gchar *);
	for(i=0;i<output_pad->num_input_nodes;i++)
	{
		if(!(output_pad->input_nodes[i]==input_node && 0 == g_strcmp0(output_pad->input_pad_names[i],input_pad->name)))
		{
			sh_size+=strlen(output_pad->input_pad_names[i])+1;
		}
	}
	const gchar **input_pad_names = g_malloc (sh_size);

	new_num_input=0;
	for(i=0;i<output_pad->num_input_nodes;i++)
	{
		if(!(output_pad->input_nodes[i]==input_node && 0 == g_strcmp0(output_pad->input_pad_names[i],input_pad->name)))
		{
			input_nodes[new_num_input] = output_pad->input_nodes[i];
			input_pad_names[new_num_input]=g_strdup(output_pad->input_pad_names[i]);
			new_num_input++;
		}
	}

	g_free(output_pad->input_nodes);
	g_free(output_pad->input_pad_names);

	output_pad->input_nodes = g_malloc(ch_size);
	output_pad->input_pad_names = g_malloc(sh_size);

	memcpy(output_pad->input_nodes, input_nodes, ch_size);
	memcpy(output_pad->input_pad_names, input_pad_names,sh_size);
	output_pad->num_input_nodes = new_num_input;

	input_pad->output_node=NULL;
	input_pad->output_pad_name=NULL;

	g_free(input_nodes);
	g_free(input_pad_names);

	g_signal_emit (input_pad, gimp_node_pad_signals[CONNECTION_REMOVED], 0);

}
