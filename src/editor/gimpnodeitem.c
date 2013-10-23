/*
 * gimpnodeitem.c
 *
 *  Created on: 18.08.2012
 *      Author: remy
 */

#include <string.h>
#include <stdlib.h>

#include "gimpnodeitem.h"
#include "gimpnodeview.h"

#define NODE_ITEM_WIDTH				150
#define NODE_ITEM_HEIGHT			100
#define NODE_ITEM_PADDING_START		3

#define NODE_FONT_FACE 				"Sans"
#define NODE_TITLE_HEIGHT 			20
#define NODE_TITLE_PADDING_LEFT		3
#define NODE_TITLE_FONT_SIZE 		12
#define NODE_PAD_SIZE 				10
#define NODE_PAD_FONT_SIZE 			9

/*
 * список сигналов
 */
enum
{
	CONNECTION_REMOVED,
	CONNECTION_ADDED,
	NODE_CHANGED,
	NODE_DELETED,
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


typedef struct _GimpNodeItemPrivate GimpNodeItemPrivate;

struct _GimpNodeItemPrivate
{
	gchar 				*name;
	GimpNodeItem 		*next;
};

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                                       GIMP_NODE_ITEM_TYPE, \
                                                       GimpNodeItemPrivate)


static void    gimp_node_item_class_init       (GimpNodeItemClass 		*klass);

static void    gimp_node_item_init             (GimpNodeItem      		*object);

static void    gimp_node_item_dispose         (GObject         *object);

static void    gimp_node_item_finalize         (GObject         *object);

static void    gimp_node_item_set_property     (GObject          *object,
                                                 	 guint        		property_id,
                                                 	 const GValue  		*value,
                                                 	 GParamSpec     	*pspec);

static void    gimp_node_item_get_property     (GObject            *object,
                                                 	 guint               property_id,
                                                 	 GValue             *value,
                                                 	 GParamSpec         *pspec);

static void 	gimp_node_item_set_next			(GimpNodeItem *node_item,
												 	 GimpNodeItem *next_node_item);

/* Необходимо переписать для работы с конкретным типом нода,
 * на пример - Гегл-нод:
 * */
static void 	 gimp_node_item_set_pads 			(GimpNodeItem *node_item);

static gpointer gimp_node_item_real_find_real_node (GimpNodeItem *node_item,
														 gpointer real_node);

static void	gimp_node_item_node_change			(GimpNodeItem *node_item);

static void	gimp_node_item_connection_add 		(GimpNodePad *node_pad,
									  	  	  	   	 	 GimpNodeItem *node_item);

static void	gimp_node_item_connection_remove 	(GimpNodePad *node_pad,
									  	  	  	   	 	 GimpNodeItem *node_item);

G_DEFINE_TYPE (GimpNodeItem, gimp_node_item, G_TYPE_OBJECT)

/*
 * Сылка на родительский класс. Измените на нужный вам класс: 
 */
#define parent_class gimp_node_item_parent_class

static guint gimp_node_item_signals[LAST_SIGNAL] = { 0 };

static void
gimp_node_item_class_init (GimpNodeItemClass *klass)
{
	GObjectClass      	*object_class      	= G_OBJECT_CLASS (klass);

	/* Регистрируем сигналы */
	gimp_node_item_signals[NAME_CHANGED]=
		g_signal_new("name_changed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeItemClass, name_changed),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);

	gimp_node_item_signals[NODE_CHANGED]=
		g_signal_new("node-changed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeItemClass, node_changed),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	gimp_node_item_signals[NODE_DELETED]=
		g_signal_new("node-deleted",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeItemClass, node_deleted),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);

	gimp_node_item_signals[CONNECTION_ADDED]=
		g_signal_new("connection-added",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeItemClass, connection_added),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1,
			G_TYPE_POINTER);

	gimp_node_item_signals[CONNECTION_REMOVED]=
		g_signal_new("connection-removed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeItemClass, connection_removed),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,1,
			G_TYPE_POINTER);

	object_class->finalize		= 	gimp_node_item_finalize;
	object_class->dispose  		=	gimp_node_item_dispose;
	object_class->set_property 	= 	gimp_node_item_set_property;
	object_class->get_property 	= 	gimp_node_item_get_property;

	klass->connection_added		=	NULL;
	klass->node_changed			=	NULL;
	klass->connection_removed	=	NULL;
	klass->name_changed			=	NULL;
	klass->node_deleted			=	NULL;

	klass->set_pads				=	gimp_node_item_set_pads;
	klass->find_real_node		=	gimp_node_item_real_find_real_node;

	/* Регистрируем параметры объекта */
  	g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL,
                                                        NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT)));

	/* Закоментируйте если не используется приватный объект */
	g_type_class_add_private (klass, sizeof (GimpNodeItemPrivate));
}

static void
gimp_node_item_init (GimpNodeItem *node_item)
{
	GimpNodeItemPrivate *private = GET_PRIVATE (node_item);

	private->name 			= 	NULL;
	private->next 			= 	NULL;
	node_item->x 			= 	node_item->y 		= NODE_ITEM_PADDING_START;

	node_item->width 		= 	NODE_ITEM_WIDTH;
	node_item->height 		= 	NODE_ITEM_HEIGHT;
	node_item->inputs		= 	NULL;
	node_item->outputs		=	NULL;

	node_item->title_height	=	NODE_TITLE_HEIGHT;
}

static void
gimp_node_item_dispose (GObject *object)
{
	GimpNodeItem *node_item = GIMP_NODE_ITEM(object);

	g_signal_emit (node_item, gimp_node_item_signals[NODE_DELETED], 0);

	//Remove pads - ode_item->inputs and node_item->outputs

	G_OBJECT_CLASS (gimp_node_item_parent_class)->dispose (object);
}


static void
gimp_node_item_finalize (GObject *object)
{
	GimpNodeItem		*node_item = GIMP_NODE_ITEM(object);
	GimpNodeItemPrivate *private = GET_PRIVATE (node_item);

	if (private->name)
	    {
	      g_free(private->name);
	      private->name = NULL;
	    }


	if(node_item->inputs)
	{
		//g_free(node_item->inputs);
		node_item->inputs=NULL;
	}

	if(node_item->outputs)
	{
		//g_free(node_item->outputs);
		node_item->outputs=NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_node_item_set_property (GObject *object,
                       	   	   	   guint 		property_id,
                       	   	   	   const 		GValue *value,
                       	   	   	   GParamSpec 	*pspec)
{
	  GimpNodeItem *gimp_node_item = GIMP_NODE_ITEM (object);

	  switch (property_id)
	    {
	    case PROP_NAME:
	    	gimp_node_item_set_name (gimp_node_item, g_value_get_string (value));
	      break;

	    default:
	      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	      break;
	    }
}

static void
gimp_node_item_get_property (GObject *object,
                           	   	   guint         property_id,
                           	   	   GValue        *value,
                           	   	   GParamSpec    *pspec)
{
	GimpNodeItemPrivate *private = GET_PRIVATE (object);

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


/*
 * Add one  input or
 * */
static void
gimp_node_item_add_pad (GimpNodeItem *node_item,
								const gchar* title,
								gint ninputs, gchar** inputs,
								gint noutputs, gchar** outputs)
{
	  int		i;
	  GimpNodePad*	pad;
	  GimpNodePad*	last_pad;

	  //add inputs to node
	  for(i = 0, last_pad = NULL; i < ninputs; i++)
	    {
	      pad	 = GIMP_NODE_PAD(gimp_node_pad_new(GIMP_NODE_PAD_INPUT));

	      if(node_item->inputs == NULL)
	    	  node_item->inputs = pad;

	      pad->next	     = NULL;
	      pad->output_node = NULL;
	      gimp_node_pad_set_name(pad,inputs[i]);

	      g_signal_connect(pad, "connection-added",
	    		  (GCallback)gimp_node_item_connection_add, node_item);

	      g_signal_connect(pad, "connection-removed",
	    		  (GCallback)gimp_node_item_connection_remove, node_item);

	      if(last_pad != NULL)
	    	  last_pad->next = pad;

	      last_pad = pad;
	    }

	  //add outputs to node
	  for(i = 0, last_pad = NULL; i < noutputs; i++)
	    {
	      pad	      = GIMP_NODE_PAD(gimp_node_pad_new(GIMP_NODE_PAD_OUTPUT));
	      if(node_item->outputs == NULL)
	    	  node_item->outputs = pad;

	      pad->next	     = NULL;
	      pad->output_node = NULL; //Array of GimpNodeItem to connect this output pad
	      gimp_node_pad_set_name(pad,outputs[i]);

	      g_signal_connect(pad, "connection-added",
	    		  (GCallback)gimp_node_item_connection_add, node_item);

	      g_signal_connect(pad, "connection-removed",
	    		  (GCallback)gimp_node_item_connection_remove, node_item);

	      if(last_pad != NULL)
	    	  last_pad->next = pad;

	      last_pad = pad;
	    }

	  g_signal_emit (node_item, gimp_node_item_signals[NODE_CHANGED], 0);
	  //gimp_node_item_connection_change(node_item);

}

//Add pads to this node_item:
static void
gimp_node_item_set_pads (GimpNodeItem *node_item)
{
	  GeglNode *node = node_item->node;
	  GSList	*pads	    = gegl_node_get_input_pads(node);
	  guint		 num_inputs = g_slist_length(pads);
	  gchar**	 inputs	    = malloc(sizeof(gchar*)*num_inputs);

	  int		 i;
	  for(i = 0; pads != NULL; pads = pads->next, i++)
	    {
	      inputs[i] = (gchar*)gegl_pad_get_name(pads->data);
	    }

	  //gint	id;
	  if(gegl_node_get_pad(node, "output") == NULL)
	    {
		  gimp_node_item_add_pad(node_item,gegl_node_get_operation(node),num_inputs,inputs,0,NULL);
	    }
	  else
	    {
	      gchar*	output = "output";
	      gchar* outputs[] = {output};

	      gimp_node_item_add_pad(node_item,gegl_node_get_operation(node),num_inputs,inputs,1,outputs);
	    }
}


static void
gimp_node_item_set_next (GimpNodeItem *node_item,
								GimpNodeItem *next_node_item)
{
	g_return_if_fail (GIMP_IS_NODE_ITEM (node_item));
	g_return_if_fail (GIMP_IS_NODE_ITEM (next_node_item));

	GimpNodeItemPrivate *private = GET_PRIVATE (node_item);
	private->next = next_node_item;

}


/*Callback */
static void
gimp_node_item_node_change (GimpNodeItem *node_item)
{
	g_return_if_fail (GIMP_IS_NODE_ITEM (node_item));

	printf("node item - node-changed\n");

	g_signal_emit (node_item, gimp_node_item_signals[NODE_CHANGED], 0);
}

static void
gimp_node_item_connection_remove (GimpNodePad *node_pad,
									  GimpNodeItem *node_item)
{
	g_return_if_fail (GIMP_IS_NODE_PAD (node_pad));
	g_return_if_fail (GIMP_IS_NODE_ITEM (node_item));
	printf("node item - connection-removed\n");

	g_signal_emit (node_item, gimp_node_item_signals[CONNECTION_REMOVED], 0,node_pad);

	gimp_node_item_node_change(node_item);
}

static void
gimp_node_item_connection_add (GimpNodePad *node_pad,
									  GimpNodeItem *node_item)
{
	g_return_if_fail (GIMP_IS_NODE_PAD (node_pad));
	g_return_if_fail (GIMP_IS_NODE_ITEM (node_item));
	printf("node item - connection-added\n");
	g_signal_emit (node_item, gimp_node_item_signals[CONNECTION_ADDED], 0,node_pad);

	gimp_node_item_node_change(node_item);
}

/* Public functions */

/* Создаем новый нод_итем.
 *
 * prev_node_item -  указывает на предыдущий добавленный нод или NULL если это
 * первый нод.
 *
 * Передаем указатель на реальный нод с данными - может быть что угодно.
 * Для задания педов используется вирт. функция - gimp_node_item_set_pads!
 * gimp_node_item_set_pads - должна быть переписана для работы с
 * конкретным типом нода, в нашем случае с ГеглНод!
 * */
GObject*
gimp_node_item_new (gpointer node, GimpNodeItem *prev_node_item)
{
	GimpNodeItem *node_item = g_object_new (GIMP_NODE_ITEM_TYPE,NULL);
	GimpNodeItemClass *node_item_klass = GIMP_NODE_ITEM_GET_CLASS(node_item);

	if(GEGL_IS_NODE(node))
	{
		node_item->node = node;
		node_item_klass->set_pads(node_item);
	}

	if(prev_node_item && GIMP_IS_NODE_ITEM(prev_node_item))
	{
		gimp_node_item_set_next(prev_node_item,node_item);
	}

	return G_OBJECT(node_item);
}

GimpNodeItem*
gimp_node_item_next (GimpNodeItem *node_item)
{
	g_return_val_if_fail (GIMP_IS_NODE_ITEM (node_item),NULL);

	GimpNodeItemPrivate *private = GET_PRIVATE (node_item);

	return private->next;
}

GimpNodeItem*
gimp_node_item_last (GimpNodeItem *node_item)
{
	g_return_val_if_fail (GIMP_IS_NODE_ITEM (node_item),NULL);

	GimpNodeItem*	last = node_item;
	while(last)
	{
		if(!GET_PRIVATE(last)->next) break;
		last=gimp_node_item_next(last);
	}

	return last;
}

void
gimp_node_item_set_name (GimpNodeItem  *object,
                      	  	  const gchar *name)
{
  g_return_if_fail (GIMP_IS_NODE_ITEM (object));

  GimpNodeItemPrivate *private = GET_PRIVATE (object);

  if (! g_strcmp0 (private->name, name))
    return;

  private->name = g_strdup (name);

  g_signal_emit (object, gimp_node_item_signals[NAME_CHANGED], 0);

  g_object_notify (G_OBJECT (object), "name");

  gimp_node_item_node_change(object);

}


/*
 * Move node_item at the end.
 * Needed prev_node
 * */
GimpNodeItem*
gimp_node_item_make_lasted (GimpNodeItem  *node_item,
									GimpNodeItem  *first_node)
{
	g_return_val_if_fail  (GIMP_IS_NODE_ITEM (node_item),first_node);
	g_return_val_if_fail  (GIMP_IS_NODE_ITEM (first_node),first_node);

	if(GET_PRIVATE (node_item)->next == NULL)
		return first_node;

	if(node_item == first_node)
	{
		GimpNodeItem*	tn = first_node;
		GimpNodeItem* next = GET_PRIVATE(tn)->next;
		GimpNodeItem* last = gimp_node_item_last(first_node);

		GET_PRIVATE (last)->next = first_node;
		GET_PRIVATE (node_item)->next = NULL;

		return next;
	}


	GimpNodeItem*	node = first_node;
	while(node)
	{
		GimpNodeItemPrivate *private = GET_PRIVATE (node);
		if(private->next == node_item)
		{
			private->next = GET_PRIVATE(node_item)->next;
		}
		else if(private->next == NULL)
		{
			private->next=node_item;
			GET_PRIVATE(node_item)->next = NULL;
			break;
		}
		node = gimp_node_item_next(node);
	}

	return first_node;
}

void
gimp_node_item_draw (GimpNodeItem  *node_item,
							GtkWidget *view,
							cairo_t *cr)
{
	g_return_if_fail (GIMP_IS_NODE_ITEM (node_item));
	g_return_if_fail (GIMP_IS_NODE_VIEW (view));

//g_print("Start draw %s...\n",gimp_node_item_name(node_item));

	GimpNodeItemPrivate *private = GET_PRIVATE (node_item);
	GimpNodeView* node_view = GIMP_NODE_VIEW (view);

	gint x, y;
	gint	width, height;

	cairo_set_line_cap (cr,CAIRO_LINE_CAP_ROUND);

	cairo_font_extents_t	fe;
	cairo_font_extents(cr, &fe);

	cairo_text_extents_t	te;
	cairo_text_extents(cr, private->name, &te);

	cairo_select_font_face(cr, NODE_FONT_FACE,
				 CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);


	node_item->title_height = NODE_TITLE_HEIGHT;

	x = node_item->x;
	y = node_item->y;

	if(node_item == node_view->dragged_node)
	    {
	      x = node_item->x+node_view->px-node_view->dx;
	      y = node_item->y+node_view->py-node_view->dy;
	    }

	  if(node_item->width < 100)
		  node_item->width	 = 100;
	  if(node_item->height < 50)
		  node_item->height = 50;

	  //if resizeing ....
	  if(node_item == node_view->resized_node)
	    {
	      width  = node_item->width+node_view->px-node_view->dx;
	      height = node_item->height+node_view->py-node_view->dy;
	    }
	  else
	    {
		  width = node_item->width;
		  height = node_item->height;
	    }


	  if(width < 100)
	    width  = 100;
	  if(height < 50)
	    height = 50;


/*
	//Font for body node
	cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
	cairo_select_font_face(cr, NODE_FONT_FACE,
				 CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, NODE_TITLE_FONT_SIZE);
*/
	cairo_set_line_width(cr, (node_view->selected_node == node_item)?2:0.5);

	//Рисуем окно нода!
	cairo_rectangle(cr, x, y, width, height); //все окно
	cairo_set_source_rgb(cr, 0.90, 0.90, 0.90); //цвет заливки
	cairo_fill(cr); //заливка сброс контура

	//Линия под заголовком
	cairo_move_to(cr, x, y+node_item->title_height);
	cairo_line_to(cr, x+width, y+node_item->title_height);//линия под заголовком
	if (node_view->selected_node == node_item)
		cairo_set_source_rgb(cr, 1, 1, 1);
	else
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); //цвет обводки
	cairo_stroke (cr);//обводка, сброс онтура

	//Заголовок - фон и текст
	cairo_rectangle(cr, x, y, width, node_item->title_height); //регион
	if (node_view->selected_node == node_item)
		cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
	else
		cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
	cairo_fill_preserve(cr); //заливка с сохр. региона

	//Текст заголовка
	cairo_clip(cr); //клип и сброс региона
	cairo_set_font_size(cr, NODE_TITLE_FONT_SIZE);

	if (node_view->selected_node == node_item)
		 cairo_set_source_rgb(cr, 1, 1, 0.6);//цвет текста
	else
		cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);

	cairo_move_to (cr, x-te.x_bearing +NODE_TITLE_PADDING_LEFT,
			 	 	 	 y - te.y_bearing +((node_item->title_height-te.height)/2));
	cairo_show_text (cr, private->name);
	cairo_reset_clip(cr);

	cairo_rectangle(cr, x, y, width, height); //все окно под обводку
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); //цвет обводки
	cairo_stroke (cr); //обводка, сброс онтура

//g_print("... draw node-window - done.");

	/*Start draw output pads:*/
	cairo_select_font_face(cr, NODE_FONT_FACE,
					 CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, NODE_PAD_FONT_SIZE);

	int		i   = 0;
	GimpNodePad*	pad = node_item->outputs;
	//gdouble outputs_height = 0;
	for(;pad!=NULL;pad = pad->next, i++)
	{
	  gdouble _x = x+width-NODE_PAD_SIZE;
	  gdouble _y = (y+NODE_TITLE_HEIGHT)+(NODE_PAD_SIZE/1.5)+(NODE_PAD_SIZE/1.5*i);
	  gdouble _width = NODE_PAD_SIZE;
	  gdouble _height = NODE_PAD_SIZE;

	  cairo_text_extents(cr, pad->name, &te);
	  cairo_set_source_rgb(cr, 0, 0.20, 0.40);
	  cairo_rectangle(cr, _x, _y, _width, _height);
	  if((pad == node_view->gragged_pad) && (node_view->selected_node == node_item))
	  {
		  cairo_fill_preserve(cr);
		  cairo_set_source_rgb(cr, 1, 1, 1);
		  cairo_set_line_width(cr, 1.5);
		  cairo_stroke(cr);
		  cairo_set_source_rgb(cr, 0, 0.20, 0.40);
		  cairo_set_line_width(cr, 2);
	  }
	  else
	  {
		  cairo_fill(cr);
	  }
	  cairo_move_to(cr, _x-te.width-3, _y+te.height/2+(_height/2));
	  cairo_show_text(cr, pad->name);

	  //outputs_height =+(_height+(NODE_PAD_SIZE/1.5)+(NODE_PAD_SIZE/1.5*i));

	 //Save pad coord. and size:
	  pad->x = _x;
	  pad->y = _y;
	  pad->width = _width;
	  pad->height = _height;

	  //TODO: render all connections underneath all nodes

	}
//g_print("... draw output-pad - done.");

	/*Start draw input pads:*/
	//gdouble inputs_height = 0;
	i   = 0;
	pad = node_item->inputs;
	for(;pad!=NULL;pad = pad->next, i++)
	{
	  gdouble _x = x;
	  gdouble _y = (y+height)-(NODE_PAD_SIZE*1.5)-(NODE_PAD_SIZE*1.5*i);
	  gdouble _width = NODE_PAD_SIZE;
	  gdouble _height = NODE_PAD_SIZE;

	  cairo_text_extents(cr, pad->name, &te);
	  cairo_set_source_rgb(cr, 0.6, 0, 0);
	  cairo_rectangle(cr, _x, _y, _width, _height);
	  if((pad == node_view->gragged_pad) && (node_view->selected_node == node_item))
	  {
		  cairo_fill_preserve(cr);
		  cairo_set_source_rgb(cr, 1, 1, 1);
		  cairo_set_line_width(cr, 1.2);
		  cairo_stroke(cr);
		  cairo_set_source_rgb(cr, 0.6, 0, 0);
		  cairo_set_line_width(cr, 2);
	  }
	  else
	  {
		  cairo_fill(cr);
	  }
	  cairo_move_to(cr, _x+NODE_PAD_SIZE+3, _y+te.height/2+(NODE_PAD_SIZE/2));
	  cairo_show_text(cr, pad->name);

	  //Save pad coord. and size:
	  pad->x = _x;
	  pad->y = _y;
	  pad->width = _width;
	  pad->height = _height;

	 // inputs_height =+ (_height+(NODE_PAD_SIZE/1.5)+((NODE_PAD_SIZE+(NODE_PAD_SIZE)/1.5)*i));
	}
//g_print("... draw input-pad - done.");
	/*Start draw body content:*/
	//cairo_set_source_rgb(cr, 0.2, 0.6, 0.0);
	//cairo_rectangle(cr, x, y+NODE_TITLE_HEIGHT+outputs_height, width, height-NODE_TITLE_HEIGHT-outputs_height-inputs_height);
	//cairo_stroke(cr);

	//Resize
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_move_to(cr, x+width-15, y+height);
	cairo_line_to(cr, x+width, y+height-15);
	cairo_stroke(cr);

	/*Draw connections :
	 * */
	pad = node_item->inputs;
	for(;pad!=NULL;pad = pad->next, i++)
	{
		if((pad == node_view->gragged_pad) && (node_view->selected_node == node_item))
		{
			GimpNodeItem *outnode = NULL;
			outnode = GIMP_NODE_ITEM(pad->output_node);
			if(outnode){
				GimpNodePad* outpad = NULL;
				outpad = gimp_node_item_get_pad_by_name(outnode,pad->output_pad_name);
				if(outpad)
				{
				cairo_move_to(cr, outpad->x+outpad->width, outpad->y+(outpad->height/2));

				if(node_view->px - outpad->x > 200)
					cairo_curve_to(cr, (outpad->x+node_view->px)/2, outpad->y,
										 (outpad->x+node_view->px)/2, node_view->py,
									      node_view->px, node_view->py);
				else
					cairo_curve_to(cr, outpad->x+100, outpad->y,
									 node_view->px-100, node_view->py,
								      node_view->px, node_view->py);
				cairo_stroke(cr);
				}
			}
		}
	}
//g_print("... draw moving input-connect - done.\n");

	pad = node_item->outputs;
	for(;pad!=NULL;pad = pad->next, i++)
	{
		/* Draw move pad connection. (Only output pad may make connection!) */
		if((pad == node_view->gragged_pad) && (node_view->selected_node == node_item))
		{
			cairo_move_to(cr, pad->x+pad->width, pad->y+(pad->height/2));
			cairo_curve_to(cr, (pad->x+node_view->px)/2, pad->y,
								 (pad->x+node_view->px)/2, node_view->py,
							      node_view->px, node_view->py);
			cairo_stroke(cr);

		}
//g_print("... draw moving output-connect - done.\n");

		/* Draw already existed conntection by current node/pad... */
		//g_print("draw input/output connections for node %i - pad %s\n",node_item, pad->name);
		for(i=0; i<pad->num_input_nodes;i++)
		{
			GimpNodeItem *node_input = pad->input_nodes[i];
			GimpNodePad *pad_input = gimp_node_item_get_pad_by_name(node_input,pad->input_pad_names[i]);
			if(pad_input && pad_input!=node_view->gragged_pad)
			{
				cairo_move_to(cr, pad->x+pad->width, pad->y+(pad->height/2));

				if(pad_input->x - pad->x > 200)
					cairo_curve_to(cr, (pad->x+pad_input->x)/2, pad->y,
										 (pad->x+pad_input->x)/2, pad_input->y,
										  pad_input->x, pad_input->y+(pad_input->height/2));
				else
					cairo_curve_to(cr, pad->x+100, pad->y,
										 pad_input->x-100, pad_input->y,
										 pad_input->x, pad_input->y+(pad_input->height/2));

				cairo_stroke(cr);
			}
		}
	}

//g_print("End draw %s...\n",gimp_node_item_name(node_item));
}

GimpNodePad*
gimp_node_item_get_pad_by_name (GimpNodeItem  *node_item, const gchar *pad_name)
{
	g_return_val_if_fail  (GIMP_IS_NODE_ITEM (node_item),NULL);

	int i;
	GimpNodePad *pad = node_item->inputs;
	for(;pad!=NULL;pad = pad->next, i++)
	{
		if(!g_strcmp0(pad->name, pad_name))
			return pad;
	}

	pad = node_item->outputs;
	for(;pad!=NULL;pad = pad->next, i++)
	{
		if(!g_strcmp0(pad->name, pad_name))
			return pad;
	}

	return NULL;
}

GimpNodePad*
gimp_node_item_pick_pad (GimpNodeItem  *node_item,
							gint x,
							gint y)
{
	g_return_val_if_fail  (GIMP_IS_NODE_ITEM (node_item),NULL);

	int		i   = 0;
	GimpNodePad*	pad = node_item->outputs;
	for(;pad!=NULL;pad = pad->next, i++)
	{
		if((x >= pad->x) && (x < pad->x+pad->width) &&
		   (y >= pad->y) && (y < pad->y+pad->height))
			return pad;
	}

	pad = node_item->inputs;
	for(;pad!=NULL;pad = pad->next, i++)
	{
		if((x >= pad->x) && (x < pad->x+pad->width) &&
		   (y >= pad->y) && (y < pad->y+pad->height))
			return pad;
	}

	return NULL;
}

/*
 * Return type of picked part of a node item or -1 if node_item is not correct.
 *
 * x and y are the global (node_view) coordinates.
 * */
gint
gimp_node_item_part (GimpNodeItem  *node_item,
							gint x,
							gint y)
{
	g_return_val_if_fail  (GIMP_IS_NODE_ITEM (node_item),-1);


	GimpNodePad *pad = gimp_node_item_pick_pad(node_item,x,y);
	//node_item->selected_pad = pad;
	if(pad)
	{
		return NODE_ITEM_PART_PAD;
	}


	if(x >= node_item->x+node_item->width-15 &&
		y >= node_item->y+node_item->height-15+(node_item->x+node_item->width-x))
	{
		return NODE_ITEM_PART_RESIZE;
	}
	else if((x >= node_item->x) && (x < node_item->x+node_item->width) &&
			 (y >= node_item->y) && (y < node_item->y+NODE_TITLE_HEIGHT))
	{
		return NODE_ITEM_PART_TITLE;
	}
	else
	{
		return NODE_ITEM_PART_BODY;
	}

	return NODE_ITEM_PART_NONE;
}

gchar*
gimp_node_item_name (GimpNodeItem  *node_item)
{
	g_return_val_if_fail  (GIMP_IS_NODE_ITEM (node_item),NULL);
	GimpNodeItemPrivate *private = GET_PRIVATE (node_item);
	return private->name;
}


static gpointer gimp_node_item_real_find_real_node (GimpNodeItem *node_item, gpointer real_node)
{
	g_return_val_if_fail  (GIMP_IS_NODE_ITEM (node_item),NULL);
	g_return_val_if_fail  (real_node,NULL);

	GSList *list = gegl_node_get_children(real_node);
	for(;list != NULL; list = list->next)
	{
		GeglNode* gegl_node = GEGL_NODE(list->data);
		if(gegl_node == node_item->node)
			return gegl_node;
	}

	return NULL;
}

gpointer
gimp_node_item_find_real_node (GimpNodeItem *node_item, gpointer real_node)
{
	g_return_val_if_fail  (GIMP_IS_NODE_ITEM (node_item),NULL);
	g_return_val_if_fail  (real_node,NULL);

	GimpNodeItemClass *node_item_klass = GIMP_NODE_ITEM_GET_CLASS(node_item);

	return node_item_klass->find_real_node(node_item,real_node);
}

void
gimp_node_item_remove_item (GimpNodeItem	*node_item,GimpNodeItem  *first_node)
{
	g_return_if_fail (GIMP_IS_NODE_ITEM (node_item));
	g_return_if_fail (GIMP_IS_NODE_ITEM (first_node));

	GimpNodeItem *next_item = first_node;
	GimpNodeItem *prev_item = NULL;
	GimpNodeItem *post_item = NULL;

		while(next_item)
		{
			if(GET_PRIVATE(next_item)->next==node_item)
				prev_item=next_item;

			if(next_item==node_item)
				post_item = GET_PRIVATE(next_item)->next;


			next_item = gimp_node_item_next(next_item);
		}

	if(prev_item){
		GET_PRIVATE(prev_item)->next = post_item;

	}

	g_object_unref (node_item);

}


