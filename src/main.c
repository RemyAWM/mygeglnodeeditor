/*
 * main.c
 *
 *  Created on: 11.08.2012
 *      Author: remy
 */


#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gegl.h>

#include "editor/gimpnodeeditor.h"
#include "editor/gegl-editor-layer.h"
#include "editor/gegl-node-widget.h"


typedef struct _GraphData        GraphData;

struct _GraphData
{
	GeglNode *graph;
	GeglNode *node_pixbuf;
	GeglNode *node_image;
	GeglNode *node_blur;
	GdkPixbuf *pixbuf;
};

static GraphData *geglGraph=NULL;

static gboolean gegl_can_process(GeglNode *render_node);

static void image_update(GtkObject *obj, GtkImage *view);

static GraphData *create_graph(void);

static gboolean gegl_can_process(GeglNode *render_node)
{
	GeglRectangle  roi;
	roi      = gegl_node_get_bounding_box (geglGraph->node_pixbuf);
	g_print("GeglRectangle: height:%i, width:%i\n",roi.height,roi.width);
	return TRUE;
}

int main( int argc, char *argv[] )
{
	//g_thread_init (NULL);

    GtkWidget *window, *vbox, *hbox;

    //Init gtk + gegl:
    gtk_init(&argc, &argv);
    gegl_init(&argc,&argv);

    //Create graph:
    geglGraph = create_graph();

    //Build GUI:
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 600);
    gtk_window_set_title(GTK_WINDOW(window), "gtkapp");

    hbox = gtk_hpaned_new();
    gtk_paned_set_position(GTK_PANED(hbox), 600);
    gtk_container_add(GTK_CONTAINER(window),hbox);

    vbox = gtk_vbox_new(FALSE,0);
    gtk_paned_pack1(GTK_PANED(hbox), vbox, TRUE, FALSE);

    //Add graph editor to right column of hbox:
    GtkWidget* scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
    									GTK_POLICY_AUTOMATIC,
    									GTK_POLICY_AUTOMATIC);
    gtk_paned_pack2(GTK_PANED(hbox), scrolledWindow, TRUE, TRUE);

    //Add viewport to make  a scroll effect!
    GtkWidget* viewport =  gtk_viewport_new (NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolledWindow), viewport);

	GtkWidget* vbox_editor = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(viewport), vbox_editor);

	//Add GimpNodeEditor:
    GtkWidget *gimp_editor = gimp_node_editor_new(geglGraph->graph,geglGraph->node_pixbuf);
    gtk_box_pack_start(GTK_BOX(vbox_editor), gimp_editor, TRUE, TRUE, 0);


	/* MENU BAR */
	GtkWidget	*menubar;
	GtkWidget	*file_menu;
	GtkWidget	*file;

	GtkWidget	*graph_menu;
	GtkWidget	*graph;
	GtkWidget	*add_operation;

	menubar = gtk_menu_bar_new();

	//File Menu
	file_menu = gtk_menu_new();
	file	  = gtk_menu_item_new_with_label("File");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), file_menu);

	//Graph Menu
	graph_menu = gtk_menu_new();
	graph	     = gtk_menu_item_new_with_label("Graph");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(graph), graph_menu);

    add_operation = gtk_menu_item_new_with_label("Add Operation");
   // g_signal_connect(add_operation, "activate", (GCallback)add_operation_activated, layer);
	gtk_menu_shell_append(GTK_MENU_SHELL(graph_menu), add_operation);

	//Add menues to menu bar
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), graph);

	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);


    //Add scrolled window for image view
    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
    									GTK_POLICY_AUTOMATIC,
    									GTK_POLICY_AUTOMATIC);

    gtk_box_pack_start(GTK_BOX(vbox), scrolledWindow, TRUE, TRUE, 0);
    //Add viewport to make  a scroll effect!
    viewport =  gtk_viewport_new (NULL, NULL);

	gtk_container_add(GTK_CONTAINER(scrolledWindow), viewport);

	//Add image view:
	GtkWidget *image = gtk_image_new_from_pixbuf(geglGraph->pixbuf);
	gtk_container_add(GTK_CONTAINER(viewport), image);


    //Add frame for buttons:
    GtkWidget* frameButtons = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(vbox), frameButtons, FALSE, FALSE, 0);

    //Add vbox to frameButtons for buttons:
	GtkWidget* vboxButtons =  gtk_vbox_new (FALSE, 0);

	gtk_container_add(GTK_CONTAINER(frameButtons), vboxButtons);

    //Add blur settings:
    GtkObject *adj_blure = gtk_adjustment_new(0.0,0.0,1000.0, 0.1,1.0,0.0);
    GtkWidget *spn_blure = gtk_spin_button_new(GTK_ADJUSTMENT(adj_blure),10.0,2);
    gtk_box_pack_end(GTK_BOX(vboxButtons),spn_blure,TRUE,TRUE,6);

    g_signal_connect(adj_blure, "value-changed",
				    G_CALLBACK (image_update),  (gpointer)image);

    g_signal_connect(window, "destroy",
		    G_CALLBACK (gtk_main_quit), NULL);

    g_signal_connect(GIMP_NODE_EDITOR(gimp_editor), "updated",
		    G_CALLBACK (image_update),   (gpointer)image);

    gtk_widget_show_all(window);

    gtk_main();

    g_object_unref(geglGraph->graph);
    gegl_exit();
    g_free(geglGraph);
    return (EXIT_SUCCESS);
}

static void image_update(GtkObject *obj, GtkImage *view)
{
	if(gegl_can_process(geglGraph->node_pixbuf))
	{
		//g_free(geglGraph->pixbuf);
		geglGraph->pixbuf=NULL;
		gegl_node_process(geglGraph->node_pixbuf);

	}

	gtk_image_clear(view);
	gtk_image_set_from_pixbuf(view,geglGraph->pixbuf);
}

static GraphData *create_graph(void)
{
	GraphData *graphData = g_new(GraphData,1);
	graphData->graph =  gegl_node_new();

	graphData->node_image = gegl_node_new_child(graphData->graph,
    		"operation","gegl:load",
    		"path","001.png",NULL);

    graphData->pixbuf=NULL;
    graphData->node_pixbuf = gegl_node_new_child(graphData->graph,
    		"operation","gegl:save-pixbuf",
    		"pixbuf",&graphData->pixbuf,NULL);

    gegl_node_connect_to(graphData->node_image,"output",graphData->node_pixbuf,"input");

    gegl_node_process (graphData->node_pixbuf);

    return graphData;
}


