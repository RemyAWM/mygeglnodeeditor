/*
 * gimpnodeview.c
 *
 *  Created on: 17.08.2012
 *      Author: remy
 */

#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "gimpnodeview.h"
#include "gimpnodeeditor.h"

/*
 * список сигналов
 */
enum
{
	NODE_ADDED,
	NODE_REMOVED,
	NODE_SELECTED,
	NODE_CHANGED,
	CONNECTION_ADDED,
	CONNECTION_REMOVED,
  	NAME_CHANGED,
  	LAST_SIGNAL
};

/*
 * список свойств
 */
enum
{
	PROP_0,
	PROP_NAME
};

/*
 * Структура приватного объекта.
 */


typedef struct _GimpNodeViewPrivate GimpNodeViewPrivate;

struct _GimpNodeViewPrivate
{
	gchar *name;
	GimpNodeItem *node_item;
	GimpNodeItem *first_item;
};

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                                       GIMP_NODE_VIEW_TYPE, \
                                                       GimpNodeViewPrivate)

/* Standart Init methods: */
static void    gimp_node_view_class_init       (GimpNodeViewClass 		*klass);

static void    gimp_node_view_init              (GimpNodeView      		*object);

/* Virtual overr methods: */
static void    gimp_node_view_finalize         (GObject         *object);

static void    gimp_node_view_set_property     (GObject          *object,
                                                 	 guint        		property_id,
                                                 	 const GValue  		*value,
                                                 	 GParamSpec     	*pspec);

static void    gimp_node_view_get_property     (GObject            *object,
                                                 	 guint               property_id,
                                                 	 GValue             *value,
                                                 	 GParamSpec         *pspec);

static gboolean gimp_node_view_expose_event		(GtkWidget *widget,
													 GdkEventExpose *event);

static gboolean gimp_node_view_draw				(GtkWidget *widget,
													 cairo_t *cr);

static gboolean gimp_node_view_button_release(GtkWidget* widget, GdkEventButton* event);

static gboolean gimp_node_view_button_press(GtkWidget* widget, GdkEventButton* event);

static gboolean gimp_node_view_key_press (GtkWidget *widget, GdkEventKey *event);

static gboolean gimp_node_view_motion(GtkWidget* widget, GdkEventMotion* event);

static void gimp_node_view_node_change(GimpNodeItem *node_item,GimpNodeView  *view);
static void gimp_node_view_connection_remove(GimpNodeItem *node_item,GimpNodePad *pad,GimpNodeView  *view);
static void gimp_node_view_connection_add(GimpNodeItem *node_item,GimpNodePad *pad,GimpNodeView  *view);
static void gimp_node_view_node_delete(GimpNodeItem *node_item,GimpNodeView  *view);


static void gimp_node_view_select_node(GimpNodeView *view, GimpNodeItem *node_item);

/* Private methods */



G_DEFINE_TYPE (GimpNodeView, gimp_node_view, GTK_TYPE_DRAWING_AREA)

/*
 * Сылка на родительский класс. Измените на нужный вам класс: 
 */
#define parent_class gimp_node_view_parent_class

static guint gimp_node_view_signals[LAST_SIGNAL] = { 0 };

static void
g_cclosure_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                          GValue       *return_value G_GNUC_UNUSED,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint G_GNUC_UNUSED,
                                          gpointer      marshal_data)
{
    register GCClosure *cc = (GCClosure *) closure;
    register gpointer data1, data2;

    g_return_if_fail (n_param_values == 3);

    if (G_CCLOSURE_SWAP_DATA (closure))
    {
        data1 = closure->data;
        data2 = g_value_peek_pointer (param_values + 0);
    }
    else
    {
        data1 = g_value_peek_pointer (param_values + 0);
        data2 = closure->data;
    }

    typedef void (*GMarshalFunc_VOID__POINTER_POINTER) (gpointer data1,
                                                        gpointer arg_1,
                                                        gpointer arg_2,
                                                        gpointer data2);

    register GMarshalFunc_VOID__POINTER_POINTER callback = (GMarshalFunc_VOID__POINTER_POINTER) (marshal_data ? marshal_data : cc->callback);

    callback (data1,
              g_value_get_pointer (param_values + 1),
              g_value_get_pointer (param_values + 2),
              data2);
}

static void    
gimp_node_view_class_init (GimpNodeViewClass *klass)
{
	GObjectClass      	*object_class      	= G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	
	/* Регистрируем сигналы */
	gimp_node_view_signals[NAME_CHANGED]=
		g_signal_new("name_changed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeViewClass, name_changed),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);

	gimp_node_view_signals[NODE_ADDED]=
		g_signal_new("node-added",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeViewClass, node_added),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,1,
			G_TYPE_POINTER);

	gimp_node_view_signals[NODE_REMOVED]=
		g_signal_new("node-removed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeViewClass, node_removed),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,1,
			G_TYPE_POINTER);

	gimp_node_view_signals[NODE_SELECTED]=
		g_signal_new("node-selected",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeViewClass, node_selected),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,1,
			G_TYPE_POINTER);

	gimp_node_view_signals[NODE_CHANGED]=
		g_signal_new("node-changed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeViewClass, node_changed),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,1,
			G_TYPE_POINTER);
	gimp_node_view_signals[CONNECTION_ADDED]=
		g_signal_new("connection-added",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeViewClass, connection_added),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER_POINTER,
			G_TYPE_NONE,2,
			G_TYPE_POINTER,G_TYPE_POINTER);

	gimp_node_view_signals[CONNECTION_REMOVED]=
		g_signal_new("connection-removed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeViewClass, connection_removed),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER_POINTER,
			G_TYPE_NONE,2,
			G_TYPE_POINTER,G_TYPE_POINTER);

	object_class->finalize		= 	gimp_node_view_finalize;
	object_class->set_property 	= 	gimp_node_view_set_property;
	object_class->get_property 	= 	gimp_node_view_get_property;

	#if GTK_MAJOR_VERSION == (3)
	  widget_class->draw		= gimp_node_view_draw;
	#else
	  widget_class->expose_event= gimp_node_view_expose_event;
	#endif

	widget_class->motion_notify_event  =	gimp_node_view_motion;
	widget_class->button_press_event   = 	gimp_node_view_button_press;
	widget_class->button_release_event = 	gimp_node_view_button_release;
	widget_class->key_press_event	   = 	gimp_node_view_key_press;

	/* Регистрируем параметры объекта */
  	g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL,
                                                        NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT)));

	/* Закоментируйте если не используется приватный объект */
	g_type_class_add_private (klass, sizeof (GimpNodeViewPrivate));
}

static void
gimp_node_view_init (GimpNodeView *object)
{
	GimpNodeViewPrivate *private = GET_PRIVATE (object);

	  gtk_widget_add_events (GTK_WIDGET(object),
				 GDK_POINTER_MOTION_MASK |
				 GDK_BUTTON_PRESS_MASK   |
				 GDK_BUTTON_RELEASE_MASK |
				 GDK_KEY_PRESS_MASK |
				 GDK_KEY_RELEASE_MASK);

	  gtk_widget_set_can_focus(GTK_WIDGET(object), TRUE);

	private->name = NULL;
	private->node_item = NULL;
	private->first_item = NULL;

	object->dragged_node = NULL;
	object->selected_node = NULL;
	object->gragged_pad   = NULL;
}

static void
gimp_node_view_finalize (GObject *object)
{
	GimpNodeViewPrivate *private = GET_PRIVATE (object);
	if (private->name)
	    {
	      g_object_unref (private->name);
	      private->name = NULL;
	    }
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_node_view_set_property (GObject *object,
                       	   guint 		property_id,
                       	   const 		GValue *value,
                       	   GParamSpec 	*pspec)
{
	  GimpNodeView *gimp_node_view = GIMP_NODE_VIEW (object);

	  switch (property_id)
	    {
	    case PROP_NAME:
	    	gimp_node_view_set_name (gimp_node_view, g_value_get_string (value));
	      break;

	    default:
	      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	      break;
	    }
}

static void
gimp_node_view_get_property (GObject *object,
                           guint         property_id,
                           GValue        *value,
                           GParamSpec    *pspec)
{

	GimpNodeViewPrivate *private = GET_PRIVATE (object);

	  switch (property_id)
	    {
	    case PROP_NAME:
	      if (private->name)
	        g_value_set_string (value, private->name);
	      break;
	    default:
	      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	      break;
	    }
}



/*Virtual override methods: */
static gboolean
gimp_node_view_button_release(GtkWidget* widget,
								  GdkEventButton* event)
{
	GimpNodeView*	view = GIMP_NODE_VIEW(widget);

	gint mov_x = view->px-view->dx;
	gint mov_y = view->py-view->dy;

	if(!view->selected_node)
		return FALSE;

	  if(view->dragged_node)
	    {
		  view->dragged_node->x += mov_x;
		  view->dragged_node->y += mov_y;
		  view->dragged_node     = NULL;
	    }
	  else if(view->resized_node)
	    {
		  view->resized_node->width  += mov_x;
		  view->resized_node->height += mov_y;
		  view->resized_node	    = NULL;
	    }
	  else if(view->gragged_pad)
	  {
		  //Если передвигаемый пед выходящий - output:
		  if(view->gragged_pad->type == GIMP_NODE_PAD_OUTPUT)
		  {
			  GimpNodeItem*	picked = gimp_node_view_pick_node_at(view);

			  if(picked && (picked != view->selected_node))
			  {
				  gint piked_type = gimp_node_item_part(picked,view->px, view->py);
				  GimpNodePad *current_pad = gimp_node_item_pick_pad(picked,view->px, view->py);

				  if((piked_type == NODE_ITEM_PART_PAD) &&
						  (current_pad->type == GIMP_NODE_PAD_INPUT))
				  {
					 if(!gimp_node_pad_has_connection(current_pad))
					 {
						 gimp_node_pad_make_connection(view->gragged_pad,picked,current_pad,view->selected_node);
					 }
				  }
			  }
		  }
		  //Если передвигаемый пед входящий - input:
		  else if(view->gragged_pad->type == GIMP_NODE_PAD_INPUT && gimp_node_pad_has_connection(view->gragged_pad))
		  {
			  GimpNodeItem*	picked = gimp_node_view_pick_node_at(view);
			  GimpNodePad *current_pad = NULL;

			  if(picked)
				 current_pad = gimp_node_item_pick_pad(picked,view->px, view->py);

			   if(current_pad!=view->gragged_pad)
			  {
				   GimpNodeItem		*intput_node		= view->selected_node;
				   GimpNodePad		*intput_pad			= view->gragged_pad;
				   GimpNodeItem		*output_node 		= view->gragged_pad->output_node;
				   const gchar		*output_pad_name 	= view->gragged_pad->output_pad_name;
				   GimpNodePad		*output_pad 		= gimp_node_item_get_pad_by_name (output_node,output_pad_name);

				   gimp_node_pad_remove_connection (intput_pad,
						   	   	   	   	   	   	   intput_node,
						   	   	   	   	   	   	   output_pad,
						   	   	   	   	   	   	   output_node);


				   //Если мыщь отжата на другом инпут педе и он не имеет соединений
				   if(current_pad && current_pad->type == GIMP_NODE_PAD_INPUT &&
						   !gimp_node_pad_has_connection(current_pad))
				   {
					  gimp_node_pad_make_connection(output_pad,picked,current_pad,output_node);
				   }
			  }
		  }
	  }
	  else
	  {
		  //If relasse on blank node view...
	  }

    view->gragged_pad = NULL;

	gtk_widget_queue_draw(widget);
	return FALSE;
}

static gboolean
gimp_node_view_button_press(GtkWidget* widget,
								GdkEventButton* event)
{
	GimpNodeView*	view = GIMP_NODE_VIEW(widget);
	GimpNodeViewPrivate *private = GET_PRIVATE (view);

	if(event->type == GDK_BUTTON_PRESS)
	{
		gtk_widget_grab_focus(widget);

		view->dx = view->px;
		view->dy = view->py;

	    GimpNodeItem*	focus = NULL;
	    GimpNodeItem*	picked = gimp_node_view_pick_node_at(view);

	    view->gragged_pad=NULL;
	    view->resized_node = NULL;
	    view->dragged_node = NULL;

	    if(picked)
	    {

	    	focus = picked;

	    	gint piked_type = gimp_node_item_part(picked,view->px, view->py);
	    	if(piked_type==NODE_ITEM_PART_BODY)
	    	{
	    		view->dragged_node = picked;
	    	}
	    	else if(piked_type == NODE_ITEM_PART_RESIZE)
	    	{

	    		view->resized_node = picked;
	    	}
	    	else if (piked_type == NODE_ITEM_PART_TITLE)
	    	{

	    		view->dragged_node = picked;
	    	}
	    	else if (piked_type == NODE_ITEM_PART_PAD)
	    	{
	    		view->gragged_pad = gimp_node_item_pick_pad(picked,view->px, view->py);
	    	}
	    	else
	    	{
	    		g_print("NODE_ITEM_PART_NONE :(((\n");
	    	}

	    	if(focus)
	    	{
	    		private->first_item = gimp_node_item_make_lasted(focus, private->first_item);
	    		if(view->selected_node != focus)
	    			gimp_node_view_select_node(view, focus);
	    	}
	    }

		gtk_widget_queue_draw(widget);
	}
	else if(event->type == GDK_2BUTTON_PRESS)
	{
	  g_print("Double click!\n");
	}

	return FALSE;
}

static gboolean
gimp_node_view_key_press (GtkWidget *widget,
							 GdkEventKey *event)
{
	GimpNodeView*	view = GIMP_NODE_VIEW(widget);
	GimpNodeItem *node_item = view->selected_node;

	if((event->keyval == GDK_KEY_Delete) &&  node_item)
	{
		gimp_node_view_remove_node_item(view,node_item);
	}

	return FALSE;


}

void gimp_node_view_remove_node_item(GimpNodeView *view, GimpNodeItem *node_item)
{
	//Remove all conections
	GimpNodeItem *first_item = GET_PRIVATE(view)->first_item;
	GimpNodePad *pad = node_item->outputs; //Выходные педы
	for(;pad;pad =pad->next)
	{
		while(pad->num_input_nodes>0)
		{
			gint num=pad->num_input_nodes-1;
			gpointer *input_node = pad->input_nodes;
			const gchar **input_pad_name = pad->input_pad_names;
			GimpNodePad *input_pad = gimp_node_item_get_pad_by_name(input_node[num],input_pad_name[num]);
			gimp_node_pad_remove_connection (input_pad,
											 input_node[num],
											 pad,
											 node_item);
		}
	}

	pad = node_item->inputs;
	for(;pad;pad =pad->next)
	{
		if(pad->output_node && pad->output_pad_name)
		{
			GimpNodePad *output_pad = gimp_node_item_get_pad_by_name(pad->output_node,pad->output_pad_name);
			gimp_node_pad_remove_connection (pad,
											 node_item,
											 output_pad,
											 pad->output_node);
		}
	}

	//Remove node
	gimp_node_item_remove_item (node_item,first_item);

	gtk_widget_queue_draw(GTK_WIDGET(view));
}


static gboolean
gimp_node_view_motion(GtkWidget* widget,
						 GdkEventMotion* event)
{
	  GimpNodeView*	view = GIMP_NODE_VIEW(widget);
	  view->px	       = (gint)event->x;
	  view->py	       = (gint)event->y;

	  /* redraw */
	  gtk_widget_queue_draw(widget);

	  return FALSE;
}

#if GTK_MAJOR_VERSION == (2)
static gboolean
gimp_node_view_expose_event (GtkWidget *widget,
								 GdkEventExpose *event)
{

	cairo_t	*cr = gdk_cairo_create(widget->window);

	gimp_node_view_draw(widget, cr);

	cairo_destroy(cr);

	return FALSE;
}
#endif

static gboolean
gimp_node_view_draw(GtkWidget *widget, cairo_t *cr)
{
	GimpNodeView*	view = GIMP_NODE_VIEW(widget);
	GimpNodeViewPrivate *private = GET_PRIVATE (view);

	//Заливаем канву белым
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	//Draw each node ...
	private->node_item = private->first_item;

	while(private->node_item)
	{
		gimp_node_item_draw(private->node_item, widget, cr);
		private->node_item = gimp_node_item_next(private->node_item);
	}

	return FALSE;
}


/*
 * Callback
 * */

static void
gimp_node_view_node_change (GimpNodeItem *node_item,
										GimpNodeView  *view)
{
	g_return_if_fail (GIMP_IS_NODE_VIEW (view));
	g_return_if_fail (GIMP_IS_NODE_ITEM (node_item));

	g_signal_emit (view, gimp_node_view_signals[NODE_CHANGED], 0,node_item);

}

static void
gimp_node_view_connection_add (GimpNodeItem *node_item,
										GimpNodePad *pad,
										GimpNodeView  *view)
{
	g_signal_emit (view, gimp_node_view_signals[CONNECTION_ADDED], 0,node_item,pad);
}

static void
gimp_node_view_connection_remove (GimpNodeItem *node_item,
										GimpNodePad *pad,
										GimpNodeView  *view)
{
	g_signal_emit (view, gimp_node_view_signals[CONNECTION_REMOVED], 0, node_item,pad);
}

static void gimp_node_view_node_delete(GimpNodeItem *node_item,GimpNodeView  *view)
{
	gimp_node_view_select_node(view,
			gimp_node_item_last (gimp_node_view_first_node_item(view)));

	g_signal_emit (view, gimp_node_view_signals[NODE_REMOVED], 0,node_item);

}

static void gimp_node_view_select_node(GimpNodeView *view, GimpNodeItem *node_item)
{
	view->selected_node = node_item;

	g_signal_emit (view, gimp_node_view_signals[NODE_SELECTED], 0, node_item);

}

/*
 *                       Public functions
 *
 * */

GtkWidget*
gimp_node_view_new()
{
	  return GTK_WIDGET (g_object_new (GIMP_NODE_VIEW_TYPE,NULL));
}

void
gimp_node_view_set_name (GimpNodeView  *view,
                      const gchar *name)
{
  g_return_if_fail (GIMP_IS_NODE_VIEW(view));

  GimpNodeViewPrivate *private = GET_PRIVATE (view);

  if (! g_strcmp0 (private->name, name))
    return;

  private->name = g_strdup (name);

  g_signal_emit (view, gimp_node_view_signals[NAME_CHANGED], 0);

  g_object_notify (G_OBJECT (view), "name");
}

/*
 * Добавляет нод к вью
 * */
void
gimp_node_view_set_item (GimpNodeView  *view, GimpNodeItem *node_item)
{
	g_return_if_fail (GIMP_IS_NODE_VIEW(view));
	g_return_if_fail (GIMP_IS_NODE_ITEM(node_item));

	GimpNodeViewPrivate *private = GET_PRIVATE (view);

	private->node_item = node_item;

    g_signal_connect(node_item, "node-changed",
  		  (GCallback)gimp_node_view_node_change, view);

    g_signal_connect(node_item, "connection-added",
  		  (GCallback)gimp_node_view_connection_add, view);

    g_signal_connect(node_item, "connection-removed",
  		  (GCallback)gimp_node_view_connection_remove, view);

    g_signal_connect(node_item, "node-deleted",
  		  (GCallback)gimp_node_view_node_delete, view);

    g_signal_emit (view, gimp_node_view_signals[NODE_ADDED], 0,node_item);

	if(!private->first_item)
		private->first_item = private->node_item;

	gtk_widget_queue_draw(GTK_WIDGET(view));

}


/*
 * Return picked node item by mouse x/y coord. or null
 * */
GimpNodeItem*
gimp_node_view_pick_node_at	(GimpNodeView  *view)
{
	GimpNodeViewPrivate *private	= GET_PRIVATE (view);
	GimpNodeItem		*picked 	= NULL;

	private->node_item = private->first_item;
	while(private->node_item)
	{
		GimpNodeItem* node_item = private->node_item;
		if(view->px >  node_item->x &&
				view->px < node_item->x+node_item->width &&
				view->py > node_item->y &&
				view->py < node_item->y+node_item->height)
		{
			picked = node_item;
		}
		private->node_item = gimp_node_item_next(private->node_item);
	}

	return picked;
}


/*
 * Устанавливает связи для всех нодов добавленных во вью
 * */
void
gimp_node_view_set_connections	(GimpNodeView  *view,  gpointer graph)
{
	GimpNodeViewPrivate *private = GET_PRIVATE(view);
	GimpNodeItem *node_item = private->first_item;

	while(node_item)
	{
		GeglNode *gegl_node = gimp_node_item_find_real_node(node_item, graph);

		if(gegl_node)
		{
			gimp_node_view_connect_pads(view, node_item,gegl_node);
		}
		node_item = gimp_node_item_next(node_item);
	}
}

/*
 * Устанавливает соединение для всех педов конкретного нода
 * */
void
gimp_node_view_connect_pads (GimpNodeView  *view, GimpNodeItem  *node_item,
										gpointer real_node)
{
	//Устанавливаем соединения от выходящих педов к входящим
	//Преребираем все выходящие педы
	GimpNodePad *pad = node_item->outputs;
	int i=0;
	for(;pad!=NULL;pad = pad->next, i++)
	{
		const gchar*	 pad_name = pad->name;

		//Получаем все выходящие связи реального ногда
		GeglNode** nodes; //ноды с которыми связан выход с именем
		const gchar** pads; //имена педов
		gint num = gegl_node_get_consumers(node_item->node, pad_name, &nodes, &pads);
		pad->num_input_nodes = 0;

		g_free(pad->input_nodes);
		g_free(pad->input_pad_names);

		gint ch_size = (num + 1) * sizeof (gchar *);
		pad->input_pad_names = g_malloc (ch_size);

		ch_size = (num + 1) * sizeof (void *);
		pad->input_nodes = g_malloc (ch_size);

		int i;
		for(i = 0; i < num; i++)
		  {
			pad->input_nodes[i]  =	 gimp_node_view_get_node_by_real(view,nodes[i]);
			pad->input_pad_names[i] = pads[i];
			pad->num_input_nodes++;
		  }
	}

	//Устанавливаем соединения от входящик к выходящим.
	//Только одно соединение может быть установленно!
	pad = node_item->inputs;
	i=0;
	for(;pad!=NULL;pad = pad->next, i++)
	{
		gchar*	 pad_name = (gchar*)pad->name;
		gchar*	 output_name = NULL;
		pad->output_node = NULL;
		GeglNode* output_node  = gegl_node_get_producer(node_item->node, pad_name, &output_name);
		if(output_node)
		{
			pad->output_pad_name = g_strdup(output_name);
			pad->output_node = gimp_node_view_get_node_by_real(view,output_node);
		}
	}
}

GimpNodeItem*
gimp_node_view_get_node_by_real (GimpNodeView *view, gpointer real_node)
{
	g_return_val_if_fail (GIMP_IS_NODE_VIEW(view), NULL);
	g_return_val_if_fail (real_node, NULL);

	GimpNodeViewPrivate *private = GET_PRIVATE(view);
	GeglNode *gegl_node = GEGL_NODE(real_node);

	GimpNodeItem *node_item = private->first_item;
	while(node_item)
	{
		if(node_item->node == gegl_node)
			return node_item;
		node_item=gimp_node_item_next(node_item);
	}
	return NULL;
}

GimpNodeItem*	gimp_node_view_first_node_item	(GimpNodeView *view)
{
	g_return_val_if_fail (GIMP_IS_NODE_VIEW(view),NULL);
	GimpNodeViewPrivate *private = GET_PRIVATE(view);

	return private->first_item;

}
