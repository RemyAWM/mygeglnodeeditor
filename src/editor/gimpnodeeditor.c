/*
 * gimpnodeeditor.c
 *
 *  Created on: 16.08.2012
 *      Author: remy
 */

#include <string.h>
#include <stdlib.h>
#include "gimpnodeeditor.h"


#define PAD_FONT_SIZE 10
#define NODE_TITLE_FONT_SIZE 12
#define NODE_FONT_FACE "Sans"

#define NUMBER_REAL_PROP_COLUMNS 4

enum
{
	UPDATED,
  	LAST_SIGNAL
};

enum
{
	PROP_0
};



typedef struct _GimpNodeEditorPrivate GimpNodeEditorPrivate;
typedef struct _PropData PropData;
typedef struct _MinMax MinMax;

struct _GimpNodeEditorPrivate
{
	/* top-level real (gegl) graph */
	gpointer 		graph;
	/* node to render using the gegl_node_process() */
	gpointer		sink_node;
	GtkWidget		*node_view;
	/* container for the node properties */
	GtkWidget 		*prop_box;
	GtkWidget		*list;
};

struct _PropData
{
	const gchar		*prop_name;
	GType			prop_type;
	GeglNode 		*node;
	GimpNodeEditor 	*editor;
};

struct _MinMax
{
	gdouble min;
	gdouble max;
};

static GSList	*propDataList = NULL;

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                                       GIMP_NODE_EDITOR_TYPE, \
                                                       GimpNodeEditorPrivate)


static void    	gimp_node_editor_class_init      (GimpNodeEditorClass 	*klass);

static void    	gimp_node_editor_init             (GimpNodeEditor      	*object);

static void 		gimp_node_editor_unrealize		(GtkWidget 				*widget);

static void    	gimp_node_editor_set_property     (GObject          		*object,
                                                 	 	 	 guint        		property_id,
                                                 	 	 	 const GValue  		*value,
                                                 	 	 	 GParamSpec     	*pspec);

static void    	gimp_node_editor_get_property     (GObject            *object,
                                                 	 	 	 guint               property_id,
                                                 	 	 	 GValue             *value,
                                                 	 	 	 GParamSpec         *pspec);

/* Callback */
static void 		gimp_node_editor_node_selected	(GimpNodeView      	*view,
														 GimpNodeItem *node_item,
														 gpointer editor);

static void 		gimp_node_editor_node_add		(GimpNodeView      	*view,
														 GimpNodeItem *node_item,
														 gpointer editor);

static void 		gimp_node_editor_node_remove		(GimpNodeView      	*view,
														 GimpNodeItem *node_item,
														 gpointer editor);

static void 		gimp_node_editor_node_change		(GimpNodeView      	*view,
														 GimpNodeItem *node_item,
														 gpointer editor);

static void		gimp_node_editor_connection_add (GimpNodeView *view,
														 GimpNodeItem *node_item,
														 GimpNodePad *pad,
														 gpointer editor);

static void		gimp_node_editor_connection_remove (GimpNodeView *view,
														 GimpNodeItem *node_item,
														 GimpNodePad *pad,
														 gpointer editor);

static void		gimp_node_editor_property_changed(GtkWidget *widget,
														  gpointer data);

/* Overwrite */
static void		gimp_node_editor_real_set_graph (GimpNodeEditor  *editor,
														 gpointer node_graph);

static gpointer		gimp_node_editor_create_real_node(gpointer graph,
															const gchar *operation);
/* Private */
static void 		gimp_node_editor_add_new_nodes	(GtkWidget *widget,
														 GimpNodeEditor *editor);

static void 		gimp_node_editor_remove_selected_node (GtkWidget *widget,
		 	 	 	 	 	 	 	 	 	 	 	 	 	 GimpNodeEditor *editor);

static GtkWidget* 	gimp_node_editor_select_node_dialog (GimpNodeEditor *editor);
static void 		gimp_node_editor_select_color_dialog (GtkWidget *widget, gpointer data);
static void		gimp_node_editor_load_params		(GimpNodeEditor *editor, GimpNodeItem *node_item);
static void 		gimp_node_editor_free_params		(GimpNodeEditor* editor);

static void 		gimp_node_editor_op_min_max			(MinMax *val,
															 const gchar *op_name,
															 const gchar *param_name);

G_DEFINE_TYPE (GimpNodeEditor, gimp_node_editor, GTK_TYPE_FRAME)

/*
 * Сылка на родительский класс. Измените на нужный вам класс: 
 */
#define parent_class gimp_node_editor_parent_class

static guint gimp_node_editor_signals[LAST_SIGNAL] = { 0 };


static void    
gimp_node_editor_class_init (GimpNodeEditorClass *klass)
{
	GObjectClass		*object_class		= 	G_OBJECT_CLASS(klass);
	GtkWidgetClass		*widget				=	GTK_WIDGET_CLASS(klass);

	gimp_node_editor_signals[UPDATED]=
		g_signal_new("updated",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GimpNodeEditorClass, updated),
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);

	object_class->set_property 	  			= 	gimp_node_editor_set_property;
	object_class->get_property 				= 	gimp_node_editor_get_property;
	widget->unrealize						=   gimp_node_editor_unrealize;

	klass->set_graph 						=	gimp_node_editor_real_set_graph;
	klass->create_real_node					=	gimp_node_editor_create_real_node;
	klass->updated							=	NULL;


	g_type_class_add_private (klass, sizeof (GimpNodeEditorPrivate));
}

/* Standart overload methods */
static void
gimp_node_editor_init (GimpNodeEditor *editor)
{
	GimpNodeEditorPrivate *private = GET_PRIVATE (editor);
	g_object_force_floating (G_OBJECT (editor));

	private->graph		=	NULL;
	private->node_view	=	NULL;
	private->prop_box	=	NULL;
	private->list		=   NULL;
	private->sink_node	=	NULL;

		editor->vbox = gtk_vbox_new(FALSE,0);
		gtk_container_add(GTK_CONTAINER(editor),editor->vbox);
		gtk_widget_show(editor->vbox);

		GtkWidget*	pane = gtk_vpaned_new();
		gtk_paned_set_position(GTK_PANED(pane), 150);
		gtk_box_pack_start(GTK_BOX(editor->vbox), pane, TRUE, TRUE, 0);
		gtk_widget_show(pane);

		private->node_view = gimp_node_view_new();
		gtk_paned_pack1(GTK_PANED(pane), private->node_view, FALSE, FALSE);
		gtk_widget_show(private->node_view);

		private->prop_box = gtk_vbox_new(FALSE, 0);
		gtk_paned_pack2(GTK_PANED(pane), private->prop_box, TRUE, TRUE);
		gtk_widget_show(private->prop_box);

		g_signal_connect(private->node_view, "connection-added",
			    		  (GCallback)gimp_node_editor_connection_add, editor);

		g_signal_connect(private->node_view, "connection-removed",
			    		  (GCallback)gimp_node_editor_connection_remove, editor);

		g_signal_connect(private->node_view, "node-changed",
			    		  (GCallback)gimp_node_editor_node_change, editor);

		g_signal_connect(private->node_view, "node-added",
			    		  (GCallback)gimp_node_editor_node_add, editor);

		g_signal_connect(private->node_view, "node-removed",
			    		  (GCallback)gimp_node_editor_node_remove, editor);

		g_signal_connect(private->node_view, "node-selected",
			    		  (GCallback)gimp_node_editor_node_selected, editor);

		GtkWidget *action_panel = gtk_frame_new (NULL);
		gtk_box_pack_start(GTK_BOX(editor->vbox), action_panel, FALSE, FALSE, 0);
		gtk_widget_show(action_panel);

		GtkWidget *action_panel_box = gtk_hbox_new(FALSE,0);
		gtk_container_add(GTK_CONTAINER(action_panel),action_panel_box);
		gtk_widget_show(action_panel_box);

		GtkWidget *bt_add_nodes = gtk_button_new_with_label("add nodes...");
		gtk_box_pack_start(GTK_BOX(action_panel_box), bt_add_nodes, FALSE, FALSE, 0);
		gtk_widget_show(bt_add_nodes);

		g_signal_connect(bt_add_nodes, "clicked",
			    		  (GCallback)gimp_node_editor_add_new_nodes, editor);

		GtkWidget *bt_remove_nodes = gtk_button_new_with_label("remove node");
		gtk_box_pack_start(GTK_BOX(action_panel_box), bt_remove_nodes, FALSE, FALSE, 0);
		gtk_widget_show(bt_remove_nodes);

		g_signal_connect(bt_remove_nodes, "clicked",
			(GCallback)gimp_node_editor_remove_selected_node, editor);

}

static void
gimp_node_editor_set_property (GObject *object,
                       	   guint 		property_id,
                       	   const 		GValue *value,
                       	   GParamSpec 	*pspec)
{
	  switch (property_id)
	    {
	    case PROP_0:
	    	//Set prop. ...
	      break;
	    default:
	      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	      break;
	    }
}

static void
gimp_node_editor_get_property (GObject *object,
                           guint         property_id,
                           GValue        *value,
                           GParamSpec    *pspec)
{

	  switch (property_id)
	    {
	    case PROP_0:
	    	//Add get object property
	      break;
	    default:
	      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	      break;
	    }
}

static void
gimp_node_editor_unrealize (GtkWidget *widget)
{
	g_return_if_fail(widget!=NULL);
	g_return_if_fail(GIMP_IS_NODE_EDITOR(widget));

	GimpNodeEditor 			*editor 	= GIMP_NODE_EDITOR(widget);
	GimpNodeEditorPrivate	*private 	= GET_PRIVATE (editor);

	if(private->list)
		g_object_unref(private->list);

	private->list=NULL;

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
	               (* GTK_OBJECT_CLASS(parent_class)->destroy)(GTK_OBJECT(editor));
}

static void
gimp_node_editor_add_new_nodes (GtkWidget *widget,
									GimpNodeEditor *editor)
{
	GimpNodeEditorPrivate 	*private 	= GET_PRIVATE(editor);
	GimpNodeView			*view		= GIMP_NODE_VIEW(private->node_view);

	GtkTreeIter	itr;
	GtkWidget	*add_op_dialog = gimp_node_editor_select_node_dialog(editor);

	/*Run dialog */
	gint result = gtk_dialog_run(GTK_DIALOG(add_op_dialog));
	//GeglNode	*node;
	  if(result == GTK_RESPONSE_ACCEPT)
	    {
	      GtkTreeSelection	*selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(private->list));
	      GtkTreeModel	*model;
	      if(gtk_tree_selection_get_selected(selection, &model, &itr))
		{
		  gchar*	operation;
		  gtk_tree_model_get(model, &itr, 0, &operation, -1);

		  if(gimp_node_editor_add_node_item(editor,private->graph,operation))
			  gimp_node_editor_node_add(view,
					  view->selected_node,
					  editor);
		}
	    }
	gtk_widget_destroy(add_op_dialog);

}

/*
 * Здесь создаем нод и добавляем в граф нужного нам типа - geglnode
 * */
static gpointer
gimp_node_editor_create_real_node(gpointer graph, const gchar *operation)
{
	g_return_val_if_fail (GEGL_IS_NODE (graph),NULL);
	GeglNode *gegl_graph = graph;

	return gegl_node_new_child(gegl_graph,"operation",operation,NULL);


}

gboolean
gimp_node_editor_add_node_item (GimpNodeEditor *editor,
									gpointer graph, const gchar *operation)
{
	g_return_val_if_fail (GIMP_IS_NODE_EDITOR (editor),FALSE);
	g_return_val_if_fail (graph,FALSE);

	GimpNodeEditorPrivate 	*private 	= GET_PRIVATE(editor);
	GimpNodeView 			*view 		= GIMP_NODE_VIEW(private->node_view);
	GimpNodeItem 			*node_item 	= NULL;

	GimpNodeEditorClass *editor_klass = GIMP_NODE_EDITOR_GET_CLASS(editor);

	gpointer new_real_node = editor_klass->create_real_node(graph,operation);

	if(new_real_node)
	{
	node_item = GIMP_NODE_ITEM(gimp_node_item_new(
			new_real_node,
			gimp_node_item_last(gimp_node_view_first_node_item(view))));

	gimp_node_item_set_name(node_item,operation);
	gimp_node_view_set_item(GIMP_NODE_VIEW(private->node_view),node_item);
	view->selected_node = node_item;

	return TRUE;
	}

	return FALSE;
}

static GtkWidget*
gimp_node_editor_select_node_dialog (GimpNodeEditor *editor)
{
	g_return_val_if_fail (GIMP_IS_NODE_EDITOR (editor),NULL);

	GimpNodeEditorPrivate *private = GET_PRIVATE(editor);

	GtkWidget	*dialog = gtk_dialog_new_with_buttons("AddOperation", NULL,
								     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								     GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT, NULL);

	GtkListStore	*store	       = gtk_list_store_new(1, G_TYPE_STRING);
	guint		n_ops;
	gchar**	ops = gegl_list_operations(&n_ops);

	GtkTreeIter	itr;

	int	i;
	for(i = 0; i < n_ops; i++) {
	    gtk_list_store_append(store, &itr);
	    gtk_list_store_set(store, &itr, 0, ops[i], -1);
	  }

	if(private->list)
	{
	    private->list=NULL;
	}

	private->list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

	GtkCellRenderer	*renderer;
	GtkTreeViewColumn	*column;

	renderer = gtk_cell_renderer_text_new ();
	column   = gtk_tree_view_column_new_with_attributes ("Operation",
							       renderer,
							       "text", 0,
							       NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (private->list), column);

	GtkScrolledWindow*	scrolls = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
	gtk_widget_set_size_request(GTK_WIDGET(scrolls), 100, 150);
	gtk_widget_show(GTK_WIDGET(scrolls));
	gtk_container_add(GTK_CONTAINER(scrolls), private->list);

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), GTK_WIDGET(scrolls));
	gtk_widget_show(private->list);

	return dialog;

}

static void
gimp_node_editor_remove_selected_node (GtkWidget *widget,
		 	 	 	 	 	 	 	 	 	 GimpNodeEditor *editor)
{
	g_return_if_fail (GIMP_IS_NODE_EDITOR (editor));

	GimpNodeEditorPrivate *private = GET_PRIVATE(editor);
	GimpNodeView *view = GIMP_NODE_VIEW(private->node_view);

	if(view->selected_node)
		gimp_node_view_remove_node_item(view,view->selected_node);
}

static void gimp_node_editor_node_selected 	(GimpNodeView      	*view,
														 GimpNodeItem *node_item,
														 gpointer editor)
{
	g_return_if_fail (GIMP_IS_NODE_EDITOR (editor));
	g_return_if_fail (GIMP_IS_NODE_ITEM (node_item));
	g_return_if_fail (GIMP_IS_NODE_VIEW (view));

	gimp_node_editor_free_params(GIMP_NODE_EDITOR(editor));
	gimp_node_editor_load_params(GIMP_NODE_EDITOR(editor), node_item);
}

static void gimp_node_editor_free_params(GimpNodeEditor* editor)
{
	g_return_if_fail (GIMP_IS_NODE_EDITOR (editor));

	GimpNodeEditorPrivate *private = GET_PRIVATE(editor);
	 GList			*children, *iter;

	  children = gtk_container_get_children(GTK_CONTAINER(private->prop_box));
	  for(iter = children; iter != NULL; iter = g_list_next(iter))
	    gtk_widget_destroy(GTK_WIDGET(iter->data));
	  g_list_free(children);

	  if(propDataList)
	  {
		  while(propDataList)
		  {
			  g_free(propDataList->data);
			  propDataList=g_slist_next(propDataList);
		  }
		  g_slist_free(propDataList);
	  }

	propDataList=NULL;
}

static void gimp_node_editor_select_color_dialog (GtkWidget *widget,
	     	 	 	 	 	 	 	 	 	 	 	 	 gpointer data)
{
	//todo put the old color selection in
	GtkColorSelectionDialog* dialog =
			GTK_COLOR_SELECTION_DIALOG(gtk_color_selection_dialog_new("Select Color"));

	  gint result = gtk_dialog_run(GTK_DIALOG(dialog));

	  //

	  if(result == GTK_RESPONSE_OK)
	    {
		  PropData 		*prop_data 		= 	data;
	      GtkColorSelection* colsel = GTK_COLOR_SELECTION(dialog->colorsel);

	      GdkColor* sel_color = malloc(sizeof(GdkColor));
	      gtk_color_selection_get_current_color(colsel, sel_color);

	      GeglColor* color = gegl_color_new(NULL);
	      gegl_color_set_rgba(color, (double)sel_color->red/65535.0,
				  (double)sel_color->green/65535.0,
				  (double)sel_color->blue/65535.0,
				  (double)gtk_color_selection_get_current_alpha(colsel)/65535.0);

	      free(sel_color);

	      gegl_node_set(prop_data->node, prop_data->prop_name, color, NULL);

	      g_signal_emit (prop_data->editor, gimp_node_editor_signals[UPDATED], 0);
	    }

	  gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void gimp_node_editor_property_changed (GtkWidget *widget,
												     gpointer data)
{
	const gchar		*text = NULL;
	gdouble numberic = 0;
	gboolean boolean = FALSE;



	PropData 		*prop_data 		= 	data;

	GeglNode 		*node 		= 	prop_data->node;
	const gchar	 	*property 	= 	prop_data->prop_name;
	GType 			type		=	prop_data->prop_type;

	if(GTK_IS_TOGGLE_BUTTON(widget))
	{
		boolean = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	}
	else if(GTK_IS_SPIN_BUTTON(widget))
	{

		numberic  = gtk_adjustment_get_value(GTK_ADJUSTMENT(gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(widget))));
	}
	else
	{
		text 		= 	gtk_entry_get_text(GTK_ENTRY(widget));
	}

	  gint	i_value;
	  gdouble	 d_value;

	  switch(type)
	    {
	    case G_TYPE_INT:
	      i_value = (int)numberic;
	      gegl_node_set(node, property, i_value, NULL);
	      break;
	    case G_TYPE_DOUBLE:
	      d_value = numberic;
	      gegl_node_set(node, property, d_value, NULL);
	      break;
	    case G_TYPE_STRING:
	      gegl_node_set(node, property, text, NULL);
	      break;
	    case G_TYPE_BOOLEAN:
	    	gegl_node_set(node, property, boolean, NULL);
	    	break;
	    default:
	      g_print("Unknown property type: %s (%s)\n", property, g_type_name(type));
	      break;
	    }

	  g_signal_emit (prop_data->editor, gimp_node_editor_signals[UPDATED], 0);
}


static void gimp_node_editor_op_min_max (MinMax *val, const gchar *op_name, const gchar *name)
{

	val->min = -10000;
	val->max = 10000;

	  if(!g_strcmp0(op_name,"gegl:layer"))
	  {
		  if(!g_strcmp0(name,"opacity"))
				  {
			  	  	val->min = 0.0;
			  	  	val->max = 1.0;
				  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:vignette"))
	  {
		  if(!g_strcmp0(name,"radius"))
		  {
			  val->min = 0.0;
			  val->max = 3.0;
		  }
		  else if(!g_strcmp0(name,"y") || !g_strcmp0(name,"x"))
		  {
			  val->min=-1;
			  val->max=2;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:c2g"))
	  {
		  if(!g_strcmp0(name,"radius"))
		  {
			  val->min = 2;
			  val->max = 3000;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:snn-mean"))
	  {
		  if(!g_strcmp0(name,"radius"))
		  {
			  val-> min = 0;
			  val->max = 100;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:fractal-explorer"))
	  {
		  if(!g_strcmp0(name,"fractaltype"))
		  {
			  val->min = 0;
			  val->max = 8;
		  }
		  else if(!g_strcmp0(name,"xmin") ||!g_strcmp0(name,"xmax") || !g_strcmp0(name,"ymin") || !g_strcmp0(name,"ymax"))
		  {
			  val->min = -3;
			  val->max = 3;
		  }
		  else if(!g_strcmp0(name,"iter"))
		  {
			  val->min = 1;
			  val->max = 1000;
		  }
		  else if(!g_strcmp0(name,"cx") || !g_strcmp0(name,"cy"))
		  {
			  val-> min = -2.5;
			  val-> max = 2.5;
		  }
		  else if(!g_strcmp0(name,"redstretch") || !g_strcmp0(name,"greenstretch")|| !g_strcmp0(name,"bluestretch"))
		  {
			  val-> min = 0;
			  val->max = 1;
		  }
		  else if(!g_strcmp0(name,"redmode") || !g_strcmp0(name,"greenmode")|| !g_strcmp0(name,"bluemode"))
		  {
			  val-> min = 0;
			  val->max = 2;
		  }
		  else if(!g_strcmp0(name,"ncolors"))
		  {
			  val-> min = 2;
			  val->max = 8192;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:stress"))
	  {
		  if(!g_strcmp0(name,"radius"))
		  {
			  val->min = 2;
			  val->max = 5000;
		  }
		  else if(!g_strcmp0(name,"samples"))
		  {
			  val->min=2;
			  val->max=200;
		  }
		  else if(!g_strcmp0(name,"iterations"))
		  {
			  val->min=1;
			  val->max=200;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:noise-reduction"))
	  {
		  if(!g_strcmp0(name,"iterations"))
		  {
			  val->min=0;
			  val-> max=32;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:perlin-noise"))
	  {
		  if(!g_strcmp0(name,"n"))
		  {
			  val->min=0;
			  val->max=20;
		  }
		  else
		  {
			  val->min = -10000;
			  val->max = 10000;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:polar-coordinates"))
	  {
		  if(!g_strcmp0(name,"angle"))
		  {
			  val->min=0;
			  val->max=359.90;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:reinhard05"))
	  {
		  if(!g_strcmp0(name,"brightness"))
		  {
			  val->min=-100;
			  val->max=100;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:ripple"))
	  {
		  if(!g_strcmp0(name,"angle"))
		  {
			  val->min=-180;
			  val->max=180;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:vector-stroke"))
	  {
		  if(!g_strcmp0(name,"width"))
		  {
			  val->min=0;
			  val-> max=200;
		  }
	  }
	  else if(!g_strcmp0(op_name,"gegl:add"))
	  {
		  if(!g_strcmp0(name,"value"))
		  {
			  val->min=-1;
			  val-> max=1;
		  }
	  }
	  /* for all */
	  else
	  {
		  if(!g_strcmp0(name,"radius1") ||!g_strcmp0(name,"radius2"))
		  {
			  val->min = 0;
			  val->max = 1000;
		  }
		  else if(!g_strcmp0(name,"opacity") ||
			  !g_strcmp0(name,"stroke-opacity") ||
			  !g_strcmp0(name,"fill-opacity"))
		  {
			  val-> min = -2.0;
			  val-> max = 2.0;
		  }
		  else if(!g_strcmp0(name,"radius") || !g_strcmp0(name,"blur-radius"))
		  {
			  val->min = 0.0;
			  val->max = 1000.0;
		  }
		  else if(!g_strcmp0(name,"edge-preservation") || !g_strcmp0(name,"scale"))
		  {
			  val-> min=0;
			  val->max=100;
		  }
		  else if(!g_strcmp0(name,"contrast"))
		  {
			  val->min=-5;
			  val->max=5;
		  }
		  else if(!g_strcmp0(name,"brightness"))
		  {
			  val->min=-3;
			  val-> max=3;
		  }
		  else if(!g_strcmp0(name,"samples") || !g_strcmp0(name,"iterations"))
		  {
			  val->min=1;
			  val->max=1000;
		  }
		  else if(!g_strcmp0(name,"original-temperature") || !g_strcmp0(name,"intended-temperature"))
		  {
			  val->min=1000;
			  val->max=12000;
		  }
		  else if(!g_strcmp0(name,"sampling-points"))
		  {
			  val->min=0;
			  val->max=65536;
		  }
		  else if(!g_strcmp0(name,"steps"))
		  {
			  val-> min=8;
			  val-> max=32;
		  }
		  else if(!g_strcmp0(name,"sigma"))
		  {
			  val->min=0;
			  val->max=32;
		  }
		  else if(!g_strcmp0(name,"tile"))
		  {
			  val-> min=0;
			  val->max=2048;
		  }
		  else if(!g_strcmp0(name,"alpha"))
		  {
			  val->min=0;
			  val->max=2;
		  }
		  else if(!g_strcmp0(name,"beta"))
		  {
			  val->min=0.1;
			  val->max=2;
		  }
		  else if(!g_strcmp0(name,"saturation")||!g_strcmp0(name,"noise") ||
				  !g_strcmp0(name,"contrast") || !g_strcmp0(name,"dampness") ||
				  !g_strcmp0(name,"c-x") || !g_strcmp0(name,"c-y") ||
				  !g_strcmp0(name,"stroke-hardness") || !g_strcmp0(name,"chromatic") ||
				  !g_strcmp0(name,"light") || !g_strcmp0(name,"softness") || !g_strcmp0(name,"proportion")
				  )
		  {
			  val->min=0;
			  val->max=1;
		  }
		  else if(!g_strcmp0(name,"frame"))
		  {
			  val->min=0;
			  val->max=1000000;
		  }
		  else if(!g_strcmp0(name,"std-dev-x") ||!g_strcmp0(name,"std-dev-y") )
		  {
			  val->min=0;
			  val->max=10000;
		  }

		  else if(!g_strcmp0(name,"quality"))
		  {
			  val->min=1;
			  val->max=100;
		  }
		  else if(!g_strcmp0(name,"smoothing") || !g_strcmp0(name,"depth"))
		  {
			  val-> min=0;
			  val->max=100;
		  }
		  else if(!g_strcmp0(name,"main") || !g_strcmp0(name,"zoom")||
				  !g_strcmp0(name,"edge")|| !g_strcmp0(name,"brighten")||
				  !g_strcmp0(name,"x-shift")|| !g_strcmp0(name,"y-shift"))
		  {
			  val->min=-100;
			  val->max=100;
		  }
		  else if(!g_strcmp0(name,"in-low") || !g_strcmp0(name,"in-high") ||
				  !g_strcmp0(name,"out-low" ) || !g_strcmp0(name,"out-high"))
		  {
			  val-> min=-1;
			  val->max=4;
		  }
		  else if(!g_strcmp0(name,"detail"))
		  {
			  val-> min=1;
			  val->max=99;
		  }
		  else if(!g_strcmp0(name,"scaling"))
		  {
			  val-> min=0;
			  val->max=5000;
		  }
		  else if(!g_strcmp0(name,"iterations"))
		  {
			  val->min=1;
			  val->max=10000;
		  }
		  else if(!g_strcmp0(name,"m-angle"))
		  {
			  val-> min=0;
			  val->max=180;
		  }
		  else if(!g_strcmp0(name,"r-angle"))
		  {
			  val-> min=0;
			  val->max=160;
		  }
		  else if(!g_strcmp0(name,"n-segs"))
		  {
			  val->min=2;
			  val->max=24;
		  }

		  else if(!g_strcmp0(name,"o-x") || !g_strcmp0(name,"o-y") || !g_strcmp0(name,"phi") ||
				  !g_strcmp0(name,"squeeze"))
		  {
			  val->min=-1;
			  val-> max=1;
		  }
		  else if(!g_strcmp0(name,"trim-x") || !g_strcmp0(name,"trim-y"))
		  {
			  val->min=0;
			  val->max=0.5;
		  }
		  else if(!g_strcmp0(name,"input-scale") || !g_strcmp0(name,"output-scale"))
		  {
			  val->min=0.1;
			  val->max=100;
		  }
		  else if(!g_strcmp0(name,"red") || !g_strcmp0(name,"green") ||
				  !g_strcmp0(name,"blue") || !g_strcmp0(name,"value"))
		  {
			  val->min=-10;
			  val->max=10;
		  }
		  else if(!g_strcmp0(name,"angle"))
		  {
			  val->min=-360;
			  val->max=360;
		  }
		  else if(!g_strcmp0(name,"length"))
		  {
			  val->min=0;
			  val->max=1000;
		  }
		  else if(!g_strcmp0(name,"stroke-width"))
		  {
			  val->min=0;
			  val->max=200;
		  }
		  else if(!g_strcmp0(name,"size-x") || !g_strcmp0(name,"size-y"))
		  {
			  val->min=1;
			  val->max=123456;
		  }
		  else if(!g_strcmp0(name,"levels"))
		  {
			  val->min=1;
			  val->max=64;
		  }
		  else if(!g_strcmp0(name,"bitdepth"))
		  {
			  val->min=8;
			  val->max=16;
		  }

		  else if(!g_strcmp0(name,"lanczos-width"))
		  {
			  val->min=3;
			  val->max=6;
		  }
		  else if(!g_strcmp0(name,"amplitude") || !g_strcmp0(name,"period"))
		  {
			  val->min=0;
			  val->max=1000;
		  }
		  else if(!g_strcmp0(name,"pairs"))
		  {
			  val-> min=1;
			  val->max=2;
		  }
		  else if(!g_strcmp0(name,"width") || !g_strcmp0(name,"height"))
		  {
			  val->min = 1;
			  val->max = 10000000;
		  }
		  else if(!g_strcmp0(name,"size"))
		  {
			  val-> min=1;
			  val->max=2048;
		  }
		  else if(!g_strcmp0(name,"wrap"))
		  {
			  val-> min=-1;
			  val-> max=10000;
		  }
		  else if(!g_strcmp0(name,"std-dev"))
		  {
			  val->min=0;
			  val->max=500;
		  }
		  else if(!g_strcmp0(name,"gamma"))
		  {
			  val->min=1;
			  val-> max=20;
		  }
		  else if(!g_strcmp0(name,"rotation"))
		  {
			  val->min=0;
			  val->max=360;
		  }
	  }
}

static void gimp_node_editor_load_params (GimpNodeEditor* editor, GimpNodeItem *node_item)
{
	g_return_if_fail(node_item);

	GimpNodeEditorPrivate 	*private = GET_PRIVATE(editor);
	GeglNode 				*node = GEGL_NODE(node_item->node);
	const gchar 			*op_name = gegl_node_get_operation(node);

	guint			n_props;
	GParamSpec**	properties = gegl_operation_list_properties(op_name, &n_props);

	GtkTable	*prop_table = GTK_TABLE(gtk_table_new(n_props,2, TRUE));

	  int i;
	  int d;
	  //int c = 0;
	  for(d = 0, i = 0; i < n_props; i++, d++)
	    {
	      GParamSpec*		prop = properties[i];
	      GType				type = prop->value_type;
	      const gchar*		name = prop->name;
	      GtkWidget*		name_label = gtk_label_new(name);

	      gtk_misc_set_alignment(GTK_MISC(name_label), 0, 0.5);

	      GtkAdjustment *value_adj		=	NULL;
	      GtkWidget		*value_entry	= 	NULL;

	      gint	i_value;
	      gdouble	d_value;
	      gchar*	str_value;
	      gboolean  bool_value;
	      gboolean skip = FALSE;

	      MinMax vals;

	      switch(type)
		{
		case G_TYPE_INT:
		  gegl_node_get(node, name, &i_value, NULL);
		  gimp_node_editor_op_min_max(&vals, op_name, name);
		  value_adj = (GtkAdjustment *) gtk_adjustment_new (i_value, vals.min, vals.max, 1.0, 5.0, 0);
		  value_entry = gtk_spin_button_new (value_adj,1,0);
		  break;
		case G_TYPE_DOUBLE:
		  gegl_node_get(node, name, &d_value, NULL);
		  gimp_node_editor_op_min_max(&vals, op_name, name);
		  value_adj = (GtkAdjustment *) gtk_adjustment_new (d_value, vals.min, vals.max, 0.1, 1.0, 0);
		  value_entry = gtk_spin_button_new (value_adj,1,3);
		  break;
		case G_TYPE_STRING:
		  gegl_node_get(node, name, &str_value, NULL);
		  value_entry = gtk_entry_new();
		  gtk_entry_set_text(GTK_ENTRY(value_entry), str_value);
		  break;
		case G_TYPE_BOOLEAN:
			gegl_node_get(node, name, &bool_value, NULL);
			value_entry=gtk_toggle_button_new_with_label("x");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(value_entry),bool_value);
			break;
		}

		if(type == GEGL_TYPE_BUFFER || type == G_TYPE_POINTER)
		{
			skip = TRUE;
			d--;
		}
	    else if( type == GEGL_TYPE_COLOR)
	    {
	    	skip = TRUE;
	    	GtkWidget *color_button = gtk_button_new_with_label("Select");

	    	PropData *data_color = malloc(sizeof(PropData));
	    	data_color->node 		= node;
	    	data_color->prop_type 	= type;
	    	data_color->prop_name 	= name;
	    	data_color->editor		= editor;

	    	propDataList = g_slist_append(propDataList, data_color);

	    	g_signal_connect(color_button, "clicked", (GCallback)gimp_node_editor_select_color_dialog, data_color);

	    	gtk_table_attach(prop_table, name_label, 0, 1, d, d+1, GTK_FILL, GTK_FILL, 1, 1);
	    	gtk_table_attach(prop_table, color_button, 1, 2, d, d+1, GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 1, 1);
	    }


	    if(!skip)
		{
	    	if(!g_strcmp0(name,"path"))
	    	{
	    		//add "browse" button
	    	}

		  PropData *data = malloc(sizeof(PropData));
		  data->node 		= node;
		  data->prop_type 	= type;
		  data->prop_name 	= name;
		  data->editor		= editor;

		  propDataList = g_slist_append(propDataList, data);

		  if(type == G_TYPE_STRING)
			  g_signal_connect(value_entry, "activate", G_CALLBACK(gimp_node_editor_property_changed), data);
		  else if(type == G_TYPE_BOOLEAN)
			  g_signal_connect(value_entry, "toggled", G_CALLBACK(gimp_node_editor_property_changed), data);
		  else if(type == G_TYPE_INT || type == G_TYPE_DOUBLE)
			  g_signal_connect(value_entry, "changed", G_CALLBACK(gimp_node_editor_property_changed), data);

		  gtk_table_attach(prop_table, name_label, 0, 1, d, d+1, GTK_FILL, GTK_FILL, 1, 1);
		  gtk_table_attach(prop_table, value_entry, 1, 2, d, d+1, GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 1, 1);
		}

	    }
	  gtk_box_pack_start(GTK_BOX(private->prop_box), GTK_WIDGET(prop_table), FALSE, FALSE, 0);
	  gtk_widget_show_all(private->prop_box);
}

static void
gimp_node_editor_node_add (GimpNodeView *view,
							  GimpNodeItem *node_item,
							  gpointer editor)
{
g_print("Node Editor - node added!\n");
gimp_node_editor_node_selected(view,node_item,editor);
}

static void
gimp_node_editor_node_remove	 (GimpNodeView *view,
								  GimpNodeItem *node_item,
								  gpointer editor)
{
	g_print("Node Editor - node removed: %s\n",gimp_node_item_name(node_item));
	GimpNodeEditorPrivate *private = GET_PRIVATE(editor);
	GeglNode *gegl_graph = private->graph;
	gegl_node_remove_child(gegl_graph, node_item->node);

	//emit update!
	g_signal_emit (editor, gimp_node_editor_signals[UPDATED], 0);
}

/* on node and connection change */
static void
gimp_node_editor_node_change (GimpNodeView *view,
		  	  	  	  	  	  	  GimpNodeItem *node_item,
		  	  	  	  	  	  	  gpointer editor)
{
	g_print("Node Editor - node changed!\n");
	g_signal_emit (editor, gimp_node_editor_signals[UPDATED], 0);
}

static void
gimp_node_editor_connection_add (GimpNodeView *view,
		  	  	  	  	  	  	  GimpNodeItem *node_item,
		  	  	  	  	  	  	  GimpNodePad *pad,
		  	  	  	  	  	  	  gpointer editor)
{
	g_print("Node Editor - connection_add %s::%s\n",
			gimp_node_item_name(node_item),pad->name);

	gint i;
	for(i=0; i<pad->num_input_nodes;i++)
	{
		GeglNode *node = GIMP_NODE_ITEM(pad->input_nodes[i])->node;
		gegl_node_connect_to(node_item->node,pad->name,node,pad->input_pad_names[i]);
	}

	g_signal_emit (editor, gimp_node_editor_signals[UPDATED], 0);

}

static void
gimp_node_editor_connection_remove (GimpNodeView *view,
		  	  	  	  	  	  	  	  	 GimpNodeItem *node_item,
		  	  	  	  	  	  	  	  	 GimpNodePad *pad,
		  	  	  	  	  	  	  	  	 gpointer editor)
{
	g_return_if_fail (GIMP_IS_NODE_ITEM (node_item));
	g_return_if_fail (GIMP_IS_NODE_PAD (pad));

	gegl_node_disconnect((GeglNode*)node_item->node, pad->name);

	g_signal_emit (editor, gimp_node_editor_signals[UPDATED], 0);
}


/* Public methods */

/* Еще не адаптирован по какой либо тип нодов...
 * */
GtkWidget*
gimp_node_editor_new (gpointer node_graph, gpointer sink_node)
{

	GimpNodeEditor *editor;
	editor =  g_object_new (GIMP_NODE_EDITOR_TYPE,NULL);

	//Если задан граф-нодов - добавить его к редактору:
	if(node_graph)
	{
		gimp_node_editor_set_graph(editor, node_graph);
	}

	if(sink_node)
		gimp_node_editor_set_sink_node(editor, sink_node);

	return GTK_WIDGET(editor);
}

/*Здесь адаптируем работу редактора с Гегл-нодами!
 * */
static void
gimp_node_editor_real_set_graph (GimpNodeEditor  *editor, gpointer node_graph)
{
	g_return_if_fail (GIMP_IS_NODE_EDITOR (editor));
	g_return_if_fail (GEGL_IS_NODE (node_graph)); /*Должен быть нужного нам типа - Гегл-нод*/

	GimpNodeEditorPrivate *private = GET_PRIVATE (editor);
	GimpNodeItem *node_item = NULL;

	private->graph = node_graph;

	GSList *list = gegl_node_get_children(node_graph);
	for(;list != NULL; list = list->next)
	{
		GeglNode* node = GEGL_NODE(list->data);
		g_print("Loading %s\n", gegl_node_get_operation(node));

		node_item = GIMP_NODE_ITEM(gimp_node_item_new(node,node_item));

		gimp_node_item_set_name(node_item,gegl_node_get_operation(node));
		gimp_node_view_set_item(GIMP_NODE_VIEW(private->node_view),node_item);
	}

	/*
	 * Setup connection between node_items
	 * */
	if(node_item)
		gimp_node_view_set_connections(GIMP_NODE_VIEW(private->node_view),private->graph);

}

void
gimp_node_editor_set_graph (GimpNodeEditor  *editor, gpointer node_graph)
{

	g_return_if_fail (GIMP_IS_NODE_EDITOR (editor));
	g_return_if_fail (node_graph);

	GimpNodeEditorClass *editor_klass = GIMP_NODE_EDITOR_GET_CLASS(editor);

	editor_klass->set_graph(editor,node_graph);

}

void
gimp_node_editor_set_sink_node (GimpNodeEditor  *editor,
									 gpointer sink_node)
{
	g_return_if_fail (GIMP_IS_NODE_EDITOR (editor));
	g_return_if_fail (sink_node);

	GimpNodeEditorPrivate *private = GET_PRIVATE(editor);
	private->sink_node = sink_node;
}

gpointer
gimp_node_editor_get_sink_node (GimpNodeEditor  *editor)
{
	g_return_val_if_fail (GIMP_IS_NODE_EDITOR (editor),NULL);
	GimpNodeEditorPrivate *private = GET_PRIVATE(editor);
	return private->sink_node;
}

