// imagebang

#include <m_pd.h>
#include <g_canvas.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#ifdef _MSC_VER
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

static t_class *pic_class;

t_widgetbehavior pic_widgetbehavior;

typedef struct _pic{
     t_object   x_obj;
     t_glist   *glist;
     int        width;
     int        height;
     t_symbol*  x_filename;
     t_symbol*  receive;
     t_symbol*  send;
	 t_outlet*  outlet;
}t_pic;

/* widget helper functions */

static const char* pic_get_filename(t_pic *x, const char *file){
	static char fname[MAXPDSTRING];
	char *bufptr;
	int fd;
	fd = open_via_path(canvas_getdir(glist_getcanvas(x->glist))->s_name,
	    file, "",fname, &bufptr, MAXPDSTRING, 1);
	if(fd > 0){
	  	fname[strlen(fname)]='/';
	  	// post("image file: %s",fname); // debug
	  	close(fd);
	  	return(fname);
	}
    else
		return(0);
}

static int pic_click(t_pic *x, struct _glist *glist, int xpos, int ypos, int shift, int alt, int dbl, int doit){
    glist = NULL, xpos = ypos = shift = alt = dbl = 0;
    if(doit){
        outlet_bang(x->outlet);
        if(x->send && x->send->s_thing)
            pd_bang(x->send->s_thing);
    }
    return(1); // why?
}

static void pic_drawme(t_pic *x, t_glist *glist, int firsttime){
     if(firsttime){
		sys_vgui(".x%lx.c create image %d %d -anchor nw -image %lx_pic -tags %lximage\n",
			glist_getcanvas(glist), text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist), x->x_filename, x);
         sys_vgui("pdsend \"%s _imagesize [image width %lx_pic] [image height %lx_pic]\"\n",
                               x->receive->s_name, x->x_filename, x->x_filename);
     }
     else
         sys_vgui(".x%lx.c coords %lximage %d %d\n", glist_getcanvas(glist),
                x, text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist));
}

void pic_erase(t_pic* x,t_glist* glist){
     sys_vgui(".x%lx.c delete %lximage\n", glist_getcanvas(glist), x);
}

/* ------------------------ image widgetbehaviour----------------------------- */

static void pic_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2){
    t_pic* x = (t_pic*)z;
    *xp1 = text_xpix(&x->x_obj, glist);
    *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = text_xpix(&x->x_obj, glist) + x->width;
    *yp2 = text_ypix(&x->x_obj, glist) + x->height;
}

static void pic_displace(t_gobj *z, t_glist *glist, int dx, int dy){
    t_pic *x = (t_pic *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    sys_vgui(".x%lx.c coords %lxSEL %d %d %d %d\n",
		   glist_getcanvas(glist), x,
		   text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
		   text_xpix(&x->x_obj, glist) + x->width, text_ypix(&x->x_obj, glist) + x->height);
    pic_drawme(x, glist, 0);
    canvas_fixlinesfor(glist, (t_text*) x);
}

static void pic_select(t_gobj *z, t_glist *glist, int state){
    t_pic *x = (t_pic *)z;
    if (state){
	    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %lxSEL -outline blue\n",
		    glist_getcanvas(glist),
		    text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
		    text_xpix(&x->x_obj, glist) + x->width, text_ypix(&x->x_obj, glist) + x->height,
		    x);
     }
     else
	     sys_vgui(".x%lx.c delete %lxSEL\n", glist_getcanvas(glist), x);
}

/* static void pic_activate(t_gobj *z, t_glist *glist, int state){
    t_text *x = (t_text *)z;
    t_rtext *y = glist_findrtext(glist, x);
    if (z->g_pd != gatom_class) rtext_activate(y, state);
} */

static void pic_delete(t_gobj *z, t_glist *glist){
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist, x);
}
       
static void pic_vis(t_gobj *z, t_glist *glist, int vis){
    t_pic* s = (t_pic*)z;
    vis ? pic_drawme(s, glist, 1) : pic_erase(s, glist);
}

static void pic_imagesize_callback(t_pic *x, t_float w, t_float h){ // WIDTH!!!
	post("received w %f h %f", w, h); // debug
	x->width = w;
	x->height = h;
	canvas_fixlinesfor(x->glist,(t_text*) x);
}

void pic_open(t_pic* x, t_symbol *filename){
    const char *file_name_open = pic_get_filename(x, filename->s_name); // Get image file path
    if(file_name_open){
        x->x_filename = gensym(file_name_open);
        sys_vgui("if { [info exists %lx_pic] == 0 } { image create photo %lx_pic -file \"%s\"\n set %lx_pic 1\n} \n",
                x->x_filename, x->x_filename, file_name_open, x->x_filename);
    }
    pic_erase(x, x->glist);
    pic_drawme(x, x->glist, 1);
}

static void pic_free(t_pic *x){
	// if variable is unset and image is unused then delete them
    sys_vgui("if { [info exists %lx_pic] == 1 && [image inuse %lx_pic] == 0} { image delete %lx_pic \n unset %lx_pic\n} \n",
        x->x_filename, x->x_filename, x->x_filename, x->x_filename);
    if(x->receive)
		pd_unbind(&x->x_obj.ob_pd,x->receive);
}

static char *defXbmString = "#define def_img_width 16\n\
#define def_img_height 16\n\
static unsigned char def_img_bits[] = {\n\
   0xff, 0xff, 0xff, 0xff, 0x3f, 0xfc, 0x0f, 0xf0, 0x07, 0xe0, 0x07, 0xe0,\n\
   0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x07, 0xe0, 0x07, 0xe0,\n\
   0x0f, 0xf0, 0x3f, 0xfc, 0xff, 0xff, 0xff, 0xff };\0";

static void pic_def_image(t_symbol *image, char *xbmString){
    sys_vgui("if { [info exists %lx_pic] == 0 } { image create bitmap %lx_pic -data \"%s\"\n set %lx_pic 1\n} \n",
             image, image, xbmString, image);
} 

static void pic_status(t_pic *x){
    post("width:      %d", x->width);
    post("height:     %d", x->height);
    post("x_filename: %s", x->x_filename->s_name);
    post("receive:    %s", (x->receive) ? x->receive->s_name : "-");
    post("send:       %s", (x->send) ? x->send->s_name : "-");
}

static void pic_setsend(t_pic *x, t_symbol *newSend){
    if(newSend != gensym(""))
        x->send = newSend;
}

static void pic_setreceive(t_pic *x, t_symbol *newReceive){
    if (x->receive)
		pd_unbind(&x->x_obj.ob_pd,x->receive);
    if(newReceive != gensym("")){
        x->receive = newReceive;
        pd_bind(&x->x_obj.ob_pd, x->receive );
    }
}

static void *pic_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_pic *x = (t_pic *)pd_new(pic_class);
    x->glist = (t_glist*) canvas_getcurrent();
    x->width = x->height = 50; // needed?
	x->x_filename = NULL;
    x->send = NULL;
	t_symbol* x_filename = NULL;
	const char *fname;
	if(argc && (argv)->a_type == A_SYMBOL){ // CREATE IMAGES
// images only created if they haven't been yet | symbol pointer used to distinguish between image files
		x_filename = atom_getsymbol(argv);
		fname = pic_get_filename(x, x_filename->s_name); // Get image file path
		if(fname){
			x->x_filename = gensym(fname);
			//sys_vgui("set %x_a \"%s\" \n",x,fname);
			// Create image only if the class hasn't loaded the same image (with the same symbolic path name)
			sys_vgui("if { [info exists %lx_pic] == 0 } { image create photo %lx_pic -file \"%s\"\n set %lx_pic 1\n} \n",
                    x->x_filename, x->x_filename, fname, x->x_filename);
//            sys_vgui("pdsend {test %x_pic}\n", x->x_filename);
		}
        else // error
            logpost(NULL, 2, "[pic]: error loading file \"%s\"", x_filename->s_name);
    }
	if(x->x_filename == NULL){ // if null, use default image
        x_filename = gensym("def_img");
        pic_def_image(x_filename, defXbmString);
        x->x_filename = x_filename;
    }
	if((argv+1)->a_type == A_SYMBOL)
		x->send = atom_getsymbol(argv+1);
	if((argv+2)->a_type == A_SYMBOL)
		x->receive = atom_getsymbol(argv+2);
    else{ // Create default receiver if none is given
		char buf[MAXPDSTRING];
		sprintf(buf, "#%lx", (long)x);
		x->receive = gensym(buf);
	}
   pd_bind(&x->x_obj.ob_pd, x->receive);
   x->outlet = outlet_new(&x->x_obj, &s_float);
   return(x);
}

void pic_setup(void){
    pic_class = class_new(gensym("pic"), (t_newmethod)pic_new, (t_method)pic_free,
				sizeof(t_pic),0, A_GIMME,0);
    class_addmethod(pic_class, (t_method)pic_imagesize_callback, gensym("_imagesize"),
                A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(pic_class, (t_method)pic_open, gensym("open"), A_SYMBOL, 0);
    class_addmethod(pic_class, (t_method)pic_status, gensym("status"), 0);
    class_addmethod(pic_class, (t_method)pic_setsend, gensym("send"), A_DEFSYMBOL, 0);
    class_addmethod(pic_class, (t_method)pic_setreceive, gensym("receive"), A_DEFSYMBOL, 0);
    pic_widgetbehavior.w_getrectfn =  pic_getrect;
    pic_widgetbehavior.w_displacefn = pic_displace;
    pic_widgetbehavior.w_selectfn =   pic_select;
//    pic_widgetbehavior.w_activatefn = pic_activate;
    pic_widgetbehavior.w_deletefn =   pic_delete;
    pic_widgetbehavior.w_visfn =      pic_vis;
    pic_widgetbehavior.w_clickfn = (t_clickfn)pic_click;
    class_setwidget(pic_class,&pic_widgetbehavior);
// class_setsavefn(pic_class, &pic_save);
}


