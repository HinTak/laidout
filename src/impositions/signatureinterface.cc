//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2010 by Tom Lechner
//


#include "../language.h"
#include "signatureinterface.h"
#include "../viewwindow.h"
#include "../headwindow.h"
#include "../utils.h"
#include "../version.h"

#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/filedialog.h>

#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 

//size of the fold indicators on left of screen
#define INDICATOR_SIZE 10



#define SP_None              0

#define SP_Tile_X_top        1
#define SP_Tile_X_bottom     2
#define SP_Tile_Y_left       3
#define SP_Tile_Y_right      4
#define SP_Tile_Gap_X        5
#define SP_Tile_Gap_Y        6

#define SP_Inset_Top         7
#define SP_Inset_Bottom      8
#define SP_Inset_Left        9
#define SP_Inset_Right       10

#define SP_H_Folds_left      11
#define SP_H_Folds_right     12
#define SP_V_Folds_top       13
#define SP_V_Folds_bottom    14

#define SP_Trim_Top          15
#define SP_Trim_Bottom       16
#define SP_Trim_Left         17
#define SP_Trim_Right        18

#define SP_Margin_Top        19
#define SP_Margin_Bottom     20
#define SP_Margin_Left       21
#define SP_Margin_Right      22

#define SP_Binding           23

#define SP_Sheets_Per_Sig    24
#define SP_Num_Pages         25
#define SP_Num_Sigs          26

 //these three currently ignored for the most part:
#define SP_Up                27
#define SP_X                 28
#define SP_Y                 29

#define SP_FOLDS             100

// *************************************
//------------------------------------- ActionArea --------------------------------------
//typedef void RenderActionAreaFunc(Laxkit::Displayer *disp,ActionArea *overlay);

enum ActionAreaType {
	AREA_Handle,
	AREA_Slider,
	AREA_Button,
	AREA_Display_Only,
	AREA_Mode,
	AREA_H_Pan,
	AREA_V_Pan,
	AREA_Menu_Trigger
};

/*! \class ActionArea
 * \brief This can define areas on screen for various purposes.
 */


ActionArea::ActionArea(int what,int ntype,const char *txt,const char *ntip)
{
	text=newstr(txt);
	tip=newstr(ntip);
	outline=NULL;
	npoints=0;
	visible=1;
	real=0;
	action=what;
	type=ntype;
}

ActionArea::~ActionArea()
{
	if (outline) delete[] outline;
	if (tip) delete[] tip;
	if (text) delete[] text;
}

//! Return if point is within the outline (If outline is not NULL), or within the ActionArea DoubleBBox bounds otherwise.
int ActionArea::PointIn(double x,double y)
{
	x-=offset.x; y-=offset.y;
	if (npoints) return point_is_in(flatpoint(x,y),outline,npoints);
	return x>=minx && x<=maxx && y>=miny && y<=maxy;
}

//! Create outline points.
/*! If takethem!=0, then make outline=pts, and that array will be delete[]'d in the destructor.
 * Otherise, the points are copied into a new array.
 *
 * If pts==NULL, but n>0, then allocate (or reallocate) the array.
 * It is presumed that the calling code will then adjust the points.
 *
 * outline will be reallocated only if n>npoints.
 */
flatpoint *ActionArea::Points(flatpoint *pts, int n, int takethem)
{
	if (n<=0) return NULL;

	if (!pts) {
		if (n>npoints && outline) { delete[] outline; outline=NULL; }
		if (!outline) outline=new flatpoint[n];

	} else if (takethem) {
		delete[] outline;
		outline=pts;

	} else {
		if (n>npoints && outline) { delete[] outline; outline=NULL; }
		outline=new flatpoint[n];
		memcpy(outline,pts,sizeof(flatpoint));
	}

	npoints=n;
	return outline;
}


// *************************************

//------------------------------------- SignatureInterface --------------------------------------
	
/*! \class SignatureInterface 
 * \brief Interface to fold around a Signature on the edit space.
 */
/*! \var int SignatureInterface::foldindex
 * \brief Where a potential fold is.
 *
 * If moving up or down, foldindex is the row immediately above the fold.
 * If moving left or right, foldindex is the column immediately to the right of the fold.
 */
/*! \var int SignatureInterface::foldlevel
 * \brief The current unfolding. 0 is totally unfolded.
 */

SignatureInterface::SignatureInterface(int nid,Displayer *ndp,Signature *sig, PaperStyle *p)
	: InterfaceWithDp(nid,ndp) 
{
	showdecs=0;
	papersize=NULL;
	insetmask=15; trimmask=15; marginmask=15;

	if (sig) {
		signature=sig->duplicate();
	} else {
		signature=new Signature;
		reallocateFoldinfo();
	}
	if (p) signature->SetPaper(p);

	foldlevel=0; //how many of the folds are active in display. must be < sig->folds.n
	hasfinal=0;
	foldinfo=signature->foldinfo;
	checkFoldLevel(1);
	signature->resetFoldinfo(NULL);

	foldr1=foldc1=foldr2=foldc2=-1;
	folddirection=0;
	lbdown_row=lbdown_col=-1;

	onoverlay=0;

	if (!p && !sig) {
		signature->totalheight=5;
		signature->totalwidth=5;
	}
}

SignatureInterface::SignatureInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
	showdecs=0;
	signature=new Signature;
	foldinfo=signature->foldinfo;
	papersize=NULL;

	foldr1=foldc1=foldr2=foldc2=-1;
	folddirection=0;
	lbdown_row=lbdown_col=-1;

	onoverlay=0;

	signature->totalheight=5;
	signature->totalwidth=5;
	
	foldlevel=0; //how many of the folds are active in display. must be < sig->folds.n
	hasfinal=0;

	reallocateFoldinfo();
	checkFoldLevel(1);
}

SignatureInterface::~SignatureInterface()
{
	DBG cerr <<"SignatureInterface destructor.."<<endl;

	if (signature) signature->dec_count();
	if (papersize) papersize->dec_count();
}

//! Reallocate foldinfo, usually after adding fold lines.
/*! this will flush any folds stored in the signature.
 */
void SignatureInterface::reallocateFoldinfo()
{
	signature->folds.flush();
	signature->reallocateFoldinfo();
	foldinfo=signature->foldinfo;
	hasfinal=0;

	if (viewport) {
		char str[200];
		sprintf(str,_("Base holds %d pages."),2*(signature->numvfolds+1)*(signature->numhfolds+1));
		viewport->postmessage(str);
	}
}

// *** temp! note to self: remove when not needed
void SignatureInterface::dumpFoldinfo()
{
	for (int r=signature->numhfolds; r>=0; r--) {
		for (int c=0; c<signature->numvfolds+1; c++) {
			cerr <<"pages:"<<foldinfo[r][c].pages.n
				<< " x,yff:"<<foldinfo[r][c].finalxflip<<","<<foldinfo[r][c].finalyflip
				<< " x,yf:"<<foldinfo[r][c].x_flipped<<","<<foldinfo[r][c].y_flipped
				<<" ir,c:"<<foldinfo[r][c].finalindexfront<<","<<foldinfo[r][c].finalindexback
				<<" -- ";
		}
		cerr << endl;
	}
	cerr << endl;
}

//! Call the other applyFold() with the value found in the given fold.
void SignatureInterface::applyFold(Fold *fold)
{
	if (!fold) return;
	applyFold(fold->direction, fold->whichfold, fold->under);
}

//! Low level flipping across folds.
/*! This will flip everything on one side of a fold to the other side (if possible).
 * It is not a selective flipping.
 *
 * This is called to ONLY apply the fold. It does not check and apply final index settings
 * or check for validity of the fold.
 */
void SignatureInterface::applyFold(char folddir, int index, int under)
{
	signature->applyFold(foldinfo, folddir,index,under);
}

//! Check if the signature is totally folded or not.
/*! Remember that if there are no fold lines, then we need to be hasfinal==1 for
 * totally folded, letting us set margin, final trim, and binding.
 *
 * If update!=0, then if the pattern is not totally folded, then make hasfinal=0,
 * make sure binding and updirection is applied to foldinfo.
 *
 * Returns foldlevel.
 */
int SignatureInterface::checkFoldLevel(int update)
{
	hasfinal=signature->checkFoldLevel(foldinfo,&finalr,&finalc);
	return foldlevel;
}

#define SIGM_Portrait        2000
#define SIGM_Landscape       2001
#define SIGM_SaveAsResource  2002

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *SignatureInterface::ContextMenu(int x,int y, int deviceid)
{
	MenuInfo *menu=new MenuInfo(_("Signature Interface"));

	int landscape=0;
	const char *paper="";
	if (signature->paperbox) {
		landscape=signature->paperbox->landscape();
		paper=signature->paperbox->name;
	}

	menu->AddItem(_("Portrait"),  SIGM_Portrait,  LAX_ISTOGGLE|(landscape?0:LAX_CHECKED));
	menu->AddItem(_("Landscape"), SIGM_Landscape, LAX_ISTOGGLE|(landscape?LAX_CHECKED:0));
	menu->AddSep();	
	menu->AddItem(_("Paper Size"),999);
	menu->SubMenu(_("Paper Size"));
	for (int c=0; c<laidout->papersizes.n; c++) {
		menu->AddItem(laidout->papersizes.e[c]->name,c,
				LAX_ISTOGGLE
				| (!strcmp(paper,laidout->papersizes.e[c]->name) ? LAX_CHECKED : 0));
	}
	menu->EndSubMenu();
	if (hasfinal) {
		menu->AddSep();
		menu->AddItem(_("Save as resource..."),SIGM_SaveAsResource);
	}
	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int SignatureInterface::Event(const Laxkit::EventData *data,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		int i=s->info2;

		if (i==SIGM_SaveAsResource) {
			char *impdir=laidout->default_path_for_resource("impositions");
			app->rundialog(new FileDialog(NULL,NULL,_("Save As..."),
						ANXWIN_REMEMBER,
						0,0,0,0,0, object_id,"saveAsPopup",
						FILES_FILES_ONLY|FILES_SAVE_AS,
						NULL,impdir));
			delete[] impdir;
			return 0;

		} else if (i==SIGM_Landscape) {
			if (signature->paperbox && !signature->paperbox->landscape()) {
				signature->paperbox->landscape(1);
				double t=signature->totalheight;
				signature->totalheight=signature->totalwidth;
				signature->totalwidth=t;
				remapHandles();
				needtodraw=1;
			}
			return 0;

		} else if (i==SIGM_Portrait) {
			if (signature->paperbox && signature->paperbox->landscape()) {
				signature->paperbox->landscape(0);
				double t=signature->totalheight;
				signature->totalheight=signature->totalwidth;
				signature->totalwidth=t;
				remapHandles();
				needtodraw=1;
			}
			return 0;

		} else if (i<999) {
			 //selecting new paper size
			if (i>=0 && i<laidout->papersizes.n) {
				signature->SetPaper(laidout->papersizes.e[i]);
				remapHandles();
				needtodraw=1;
			}
			return 0;
		}
		return 1;

	} else if (!strcmp(mes,"saveAsPopup")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s || isblank(s->str)) return 1;

		char *error=NULL;
		FILE *f=open_file_for_writing(s->str,1,&error);
		if (!f) {
			if (error) viewport->postmessage(error);
			return 0;
		}

		fprintf(f,"#Laidout %s Imposition\n",LAIDOUT_VERSION);
		fprintf(f,"type SignatureImposition\n\n");
		if (signature->paperbox) {
			fprintf(f,"paper\n");
			signature->paperbox->dump_out(f,2,0,NULL);
		}

		signature->dump_out(f,0,0,NULL);

		fclose(f);

		viewport->postmessage(_("Saved."));

		return 0;
	}

	return 1;
}

void SignatureInterface::createHandles()
{
	controls.push(new ActionArea(SP_Tile_X_top, AREA_Handle, NULL, _("Tile horizontally, wheel or drag changes")));

	controls.push(new ActionArea(SP_Tile_X_bottom    , AREA_Handle, NULL, _("Tile horizontally, wheel or drag changes")));
	controls.push(new ActionArea(SP_Tile_Y_left      , AREA_Handle, NULL, _("Tile vertically, wheel or drag changes")));
	controls.push(new ActionArea(SP_Tile_Y_right     , AREA_Handle, NULL, _("Tile vertically, wheel or drag changes")));

	controls.push(new ActionArea(SP_Tile_Gap_X       , AREA_Handle, NULL, _("Tile gap horizontally")));
	controls.push(new ActionArea(SP_Tile_Gap_Y       , AREA_Handle, NULL, _("Tile gap vertically")));

	controls.push(new ActionArea(SP_Inset_Top        , AREA_Handle, NULL, _("Inset top")));
	controls.push(new ActionArea(SP_Inset_Bottom     , AREA_Handle, NULL, _("Inset bottom")));
	controls.push(new ActionArea(SP_Inset_Left       , AREA_Handle, NULL, _("Inset left")));
	controls.push(new ActionArea(SP_Inset_Right      , AREA_Handle, NULL, _("Inset right")));

	controls.push(new ActionArea(SP_H_Folds_left     , AREA_Handle, NULL, _("Number of horizontal folds")));
	controls.push(new ActionArea(SP_H_Folds_right    , AREA_Handle, NULL, _("Number of horizontal folds")));
	controls.push(new ActionArea(SP_V_Folds_top      , AREA_Handle, NULL, _("Number of vertical folds")));
	controls.push(new ActionArea(SP_V_Folds_bottom   , AREA_Handle, NULL, _("Number of vertical folds")));

	controls.push(new ActionArea(SP_Trim_Top         , AREA_Handle, NULL, _("Trim top of page")));
	controls.push(new ActionArea(SP_Trim_Bottom      , AREA_Handle, NULL, _("Trim bottom of page")));
	controls.push(new ActionArea(SP_Trim_Left        , AREA_Handle, NULL, _("Trim left of page")));
	controls.push(new ActionArea(SP_Trim_Right       , AREA_Handle, NULL, _("Trim right of page")));

	controls.push(new ActionArea(SP_Margin_Top       , AREA_Handle, NULL, _("Margin top of page")));
	controls.push(new ActionArea(SP_Margin_Bottom    , AREA_Handle, NULL, _("Margin bottom of page")));
	controls.push(new ActionArea(SP_Margin_Left      , AREA_Handle, NULL, _("Margin left of page")));
	controls.push(new ActionArea(SP_Margin_Right     , AREA_Handle, NULL, _("Margin right of page")));

	controls.push(new ActionArea(SP_Binding          , AREA_Handle, NULL, _("Binding edge, drag to place")));

	controls.push(new ActionArea(SP_Sheets_Per_Sig   , AREA_Slider, NULL, _("Wheel or drag changes")));
	controls.push(new ActionArea(SP_Num_Pages        , AREA_Slider, NULL, _("Wheel or drag changes")));
	controls.push(new ActionArea(SP_Num_Sigs         , AREA_Slider, NULL, _("Wheel or drag changes")));

}

void SignatureInterface::remapHandles()
{
	if (controls.n==0) createHandles();

	ActionArea *area;
	flatpoint *p;
	int n;
	double scale=1;
	double ww=signature->totalwidth, hh=signature->totalheight;
	double w=signature->PageWidth(0), h=signature->PageHeight(0);

	area=controls.e[SP_Tile_X_top-1]; //trapezoidal area above paper
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(0,hh);  p[1]=flatpoint(ww,hh); p[2]=flatpoint(ww*1.1,hh*1.1); p[3]=flatpoint(-ww*.1,hh*1.1);

	area=controls.e[SP_Tile_X_bottom-1]; //trapezoidal area below paper
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(0,0);  p[1]=flatpoint(ww,0); p[2]=flatpoint(ww*1.1,-hh*.1); p[3]=flatpoint(-ww*.1,-hh*.1);

	area=controls.e[SP_Tile_Y_left-1];
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(0,0);  p[1]=flatpoint(0,hh); p[2]=flatpoint(-ww*.1,hh*1.1); p[3]=flatpoint(-ww*.1,-hh*.1);

	area=controls.e[SP_Tile_Y_right-1];
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(ww,0);  p[1]=flatpoint(ww,hh); p[2]=flatpoint(ww*1.1,hh*1.1); p[3]=flatpoint(ww*1.1,-hh*.1);

	area=controls.e[SP_Tile_Gap_X-1];
	p=draw_thing_coordinates(THING_Double_Arrow_Horizontal, NULL,0, &n, scale);
	area->Points(p,n,1);

	area=controls.e[SP_Tile_Gap_Y-1];
	p=draw_thing_coordinates(THING_Double_Arrow_Vertical, NULL,0, &n, scale);
	area->Points(p,n,1);

	area=controls.e[SP_Inset_Top-1];
	p=draw_thing_coordinates(THING_Arrow_Down, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(ww/2,hh-signature->insettop);

	area=controls.e[SP_Inset_Bottom-1];
	p=draw_thing_coordinates(THING_Arrow_Up, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(ww/2,signature->insettop-scale);

	area=controls.e[SP_Inset_Left-1];
	p=draw_thing_coordinates(THING_Arrow_Right, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(signature->insetleft-scale,hh/2);

	area=controls.e[SP_Inset_Right-1];
	p=draw_thing_coordinates(THING_Arrow_Left, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(ww-signature->insetright,hh/2);

	 //-----folds
	area=controls.e[SP_H_Folds_left-1];
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(0,0);  p[1]=flatpoint(w*.1,h*.1); p[2]=flatpoint(w*.1,h*.9); p[3]=flatpoint(0,h);

	area=controls.e[SP_H_Folds_right-1];
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(w,0);  p[1]=flatpoint(w,h); p[2]=flatpoint(w*.9,h*.9); p[3]=flatpoint(w*.9,h*.1);

	area=controls.e[SP_V_Folds_top-1];
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(0,h);  p[1]=flatpoint(w,h); p[2]=flatpoint(w*.9,h*.9); p[3]=flatpoint(w*.1,h*.9);

	area=controls.e[SP_V_Folds_bottom-1];
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(0,0);  p[1]=flatpoint(w,0); p[2]=flatpoint(w*.9,h*.1); p[3]=flatpoint(w*.1,h*.1);

	 //----trim
	area=controls.e[SP_Trim_Top-1];
	p=draw_thing_coordinates(THING_Arrow_Down, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(w/2,h-signature->trimtop);

	area=controls.e[SP_Trim_Bottom-1];
	p=draw_thing_coordinates(THING_Arrow_Up, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(w/2,signature->trimbottom-scale);

	area=controls.e[SP_Trim_Left-1];
	p=draw_thing_coordinates(THING_Arrow_Right, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(signature->trimleft-scale,h/2);

	area=controls.e[SP_Trim_Right-1];
	p=draw_thing_coordinates(THING_Arrow_Left, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(w-signature->trimright,h/2);

	 //------margins
	area=controls.e[SP_Margin_Top-1];
	p=draw_thing_coordinates(THING_Arrow_Down, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(w/2,h-signature->margintop);

	area=controls.e[SP_Margin_Bottom-1];
	p=draw_thing_coordinates(THING_Arrow_Up, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(w/2,signature->marginbottom-scale);

	area=controls.e[SP_Margin_Left-1];
	p=draw_thing_coordinates(THING_Arrow_Right, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(signature->marginleft-scale,h/2);

	area=controls.e[SP_Margin_Right-1];
	p=draw_thing_coordinates(THING_Arrow_Left, NULL,0, &n, scale);
	area->Points(p,n,1);
	area->offset=flatpoint(w-signature->marginright,h/2);

	 //--------binding
	area=controls.e[SP_Binding-1];
	area->Points(NULL,4,1);

	 //----- sheets and pages
	area=controls.e[SP_Sheets_Per_Sig-1];
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(0,-hh*.1);  p[1]=flatpoint(ww,-hh*.1); p[2]=flatpoint(ww,-hh*.2); p[3]=flatpoint(0,-hh*.2);

	area=controls.e[SP_Num_Pages-1];
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(0,-hh*.2);  p[1]=flatpoint(ww/2,-hh*.2); p[2]=flatpoint(ww/2,-hh*.3); p[3]=flatpoint(0,-hh*.3);

	area=controls.e[SP_Num_Sigs-1];
	p=area->Points(NULL,4,0);
	p[0]=flatpoint(ww/2,-hh*.2);  p[1]=flatpoint(ww,-hh*.2); p[2]=flatpoint(ww,-hh*.3); p[3]=flatpoint(ww/2,-hh*.3);
}

int SignatureInterface::UseThisImposition(SignatureImposition *sigimp)
{
	// ***
	return 1;
}

int SignatureInterface::UseThisSignature(Signature *sig)
{
	// ***
	return 1;
}



/*! PaperGroup or PaperBoxData.
 */
int SignatureInterface::draws(const char *atype)
{ return !strcmp(atype,"SignatureData"); }


//! Return a new SignatureInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *SignatureInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new SignatureInterface(id,NULL);
	else if (!dynamic_cast<SignatureInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int SignatureInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

int SignatureInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void SignatureInterface::Clear(SomeData *d)
{
}

	
///*! This will be called by viewports, and the papers will be displayed opaque with
// * drop shadow.
// */
//int SignatureInterface::DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
//			Laxkit::anObject *a1,Laxkit::anObject *a2,int info)
//{
//	return 1;
//}

/*! \todo draw arrow to indicate paper up direction
 */
int SignatureInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	double patternheight=signature->PatternHeight();
	double patternwidth =signature->PatternWidth();

	double x,y,w,h;
	static char str[150];

	 //----------------draw whole outline
	dp->NewFG(1.,0.,1.); //purple for paper outline, like custom papergroup border color
	w=signature->totalwidth;
	h=signature->totalheight;
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
	dp->drawline(0,0, w,0);
	dp->drawline(w,0, w,h);
	dp->drawline(w,h, 0,h);
	dp->drawline(0,h, 0,0);
	
	 //----------------draw inset
	dp->NewFG(.5,0.,0.); //dark red for inset
	if (signature->insetleft) dp->drawline(signature->insetleft,0, signature->insetleft,h);
	if (signature->insetright) dp->drawline(w-signature->insetright,0, w-signature->insetright,h);
	if (signature->insettop) dp->drawline(0,h-signature->insettop, w,h-signature->insettop);
	if (signature->insetbottom) dp->drawline(0,signature->insetbottom, w,signature->insetbottom);


	 //------------------draw fold pattern in each tile
	double ew=patternwidth/(signature->numvfolds+1);
	double eh=patternheight/(signature->numhfolds+1);
	w=patternwidth;
	h=patternheight;

	flatpoint pts[4],fp;
	int facedown=0;
	int hasface;
	int rrr,ccc;
	double xx,yy;
	int xflip, yflip;

	x=signature->insetleft;
	for (int tx=0; tx<signature->tilex; tx++) {
	  y=signature->insetbottom;
	  for (int ty=0; ty<signature->tiley; ty++) {
		 //fill in light gray for elements with no current faces
		 //or draw orientation arrow and number for existing faces
		for (int rr=0; rr<signature->numhfolds+1; rr++) {
		  for (int cc=0; cc<signature->numvfolds+1; cc++) {
			hasface=foldinfo[rr][cc].pages.n;

			 //first draw filled face, grayed if no current faces
			dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
			pts[0]=flatpoint(x+cc*ew,y+rr*eh);
			pts[1]=pts[0]+flatpoint(ew,0);
			pts[2]=pts[0]+flatpoint(ew,eh);
			pts[3]=pts[0]+flatpoint(0,eh);

			if (hasface) dp->NewFG(1.,1.,1.);
			else dp->NewFG(.75,.75,.75);

			dp->drawlines(pts,4,1,1);

			//if (hasface && foldlevel==0) {
			if (hasface) {
				rrr=foldinfo[rr][cc].pages[foldinfo[rr][cc].pages.n-2];
				ccc=foldinfo[rr][cc].pages[foldinfo[rr][cc].pages.n-1];

				if (foldinfo[rr][cc].finalindexfront>=0) {
					 //there are faces in this spot, draw arrow and page number
					dp->NewFG(.75,.75,.75);
					if (hasfinal) {
						xflip=foldinfo[rrr][ccc].x_flipped^foldinfo[rrr][ccc].finalxflip;
						yflip=foldinfo[rrr][ccc].y_flipped^foldinfo[rrr][ccc].finalyflip;
					} else {
						xflip=foldinfo[rrr][ccc].x_flipped;
						yflip=foldinfo[rrr][ccc].y_flipped;
					}
					facedown=((xflip && !yflip) || (!xflip && yflip));

					//if (facedown) dp->LineAttributes(1,LineOnOffDash, CapButt, JoinMiter);
					//else dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
					dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);

					pts[0]=flatpoint(x+(cc+.5)*ew,y+(rr+.25+.5*(yflip?1:0))*eh);
					dp->drawarrow(pts[0],flatpoint(0,yflip?-1:1)*eh/4, 0,eh/2,1);
					fp=dp->realtoscreen(pts[0]);

					//if (facedown) sprintf(str,"%d/%d",foldinfo[rrr][ccc].finalindexback,foldinfo[rrr][ccc].finalindexfront);
					//else sprintf(str,"%d/%d",foldinfo[rrr][ccc].finalindexfront,foldinfo[rrr][ccc].finalindexback);
					//dp->textout(fp.x,fp.y, str,-1, LAX_CENTER);
					//-------------
					sprintf(str,"%d/%d",foldinfo[rrr][ccc].finalindexfront,foldinfo[rrr][ccc].finalindexback);
					//if (facedown) sprintf(str,"(%d)",foldinfo[rrr][ccc].finalindexback);
					//	sprintf(str,"%d",foldinfo[rrr][ccc].finalindexfront);
					dp->textout(fp.x,fp.y, str,-1, LAX_CENTER);
				}
			}

			 //draw markings for final page binding edge, up, trim, margin
			if (hasfinal && foldlevel==signature->folds.n && rr==finalr && cc==finalc) {
				dp->LineAttributes(2,LineSolid, CapButt, JoinMiter);
				
				xx=x+cc*ew;
				yy=y+rr*eh;

				 //draw gray margin edge
				dp->NewFG(.75,.75,.75);
				dp->drawline(xx,yy+signature->marginbottom,   xx+ew,yy+signature->marginbottom);			
				dp->drawline(xx,yy+eh-signature->margintop,   xx+ew,yy+eh-signature->margintop);			
				dp->drawline(xx+signature->marginleft,yy,     xx+signature->marginleft,yy+eh);			
				dp->drawline(xx+ew-signature->marginright,yy, xx+ew-signature->marginright,yy+eh);			

				 //draw red trim edge
				dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
				dp->NewFG(1.,0.,0.);
				if (signature->trimbottom>0) dp->drawline(xx,yy+signature->trimbottom, xx+ew,yy+signature->trimbottom);			
				if (signature->trimtop>0)    dp->drawline(xx,yy+eh-signature->trimtop, xx+ew,yy+eh-signature->trimtop);			
				if (signature->trimleft>0)   dp->drawline(xx+signature->trimleft,yy, xx+signature->trimleft,yy+eh);			
				if (signature->trimright>0)  dp->drawline(xx+ew-signature->trimright,yy, xx+ew-signature->trimright,yy+eh);			

				 //draw green binding edge
				dp->LineAttributes(2,LineSolid, CapButt, JoinMiter);
				dp->NewFG(0.,1.,0.);

				 //todo: draw a solid line, but a dashed line toward the inner part of the page...??
				int b=signature->binding;
				double in=ew*.05;
				if (b=='l')      dp->drawline(xx+in,yy,    xx+in,yy+eh);
				else if (b=='r') dp->drawline(xx-in+ew,yy, xx-in+ew,yy+eh);
				else if (b=='t') dp->drawline(xx,yy-in+eh, xx+ew,yy-in+eh);
				else if (b=='b') dp->drawline(xx,yy+in,    xx+ew,yy+in);

			}
		  } //cc
		}  //rr

		 //draw fold pattern outline
		dp->NewFG(1.,0.,0.);
		dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
		dp->drawline(x,    y, x+w,  y);
		dp->drawline(x+w,  y, x+w,y+h);
		dp->drawline(x+w,y+h, x  ,y+h);
		dp->drawline(x,  y+h, x,y);

		 //draw all fold lines
		dp->NewFG(.5,.5,.5);
		dp->LineAttributes(1,LineOnOffDash, CapButt, JoinMiter);
		for (int c=0; c<signature->numvfolds; c++) { //verticals
			dp->drawline(x+(c+1)*ew,y, x+(c+1)*ew,y+h);
		}

		for (int c=0; c<signature->numhfolds; c++) { //horizontals
			dp->drawline(x,y+(c+1)*eh, x+w,y+(c+1)*eh);
		}
		
		y+=patternheight+signature->tilegapy;
	  } //tx
	  x+=patternwidth+signature->tilegapx;
	} //ty

	 //draw in progress folding
	int device=0;
	DBG cerr <<"----------------any "<<buttondown.any(0,LEFTBUTTON,&device)<<endl;
	if (buttondown.any(0,LEFTBUTTON,&device) && folddirection && folddirection!='x') {
		 //this will draw a light gray tilting region across foldindex, in folddirection, with foldunder.
		 //it will correspond to foldr1,foldr2, and foldc1,foldc2.

		DBG cerr <<"--------------------------------showing dir"<<endl;
		int mx,my;
		buttondown.getinitial(device,LEFTBUTTON,&mx,&my);

		flatpoint p=dp->screentoreal(mx,my);
		flatpoint dir;
		if (folddirection=='t') dir.y=1;
		else if (folddirection=='b') dir.y=-1;
		else if (folddirection=='l') dir.x=-1;
		else if (folddirection=='r') dir.x=1;

		dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
		//dp->drawarrow(p,dir,0,25,0);


		 //draw partially folded region foldr1..foldr2, foldc1..foldc2
		double ew=patternwidth/(signature->numvfolds+1);
		double eh=patternheight/(signature->numhfolds+1);
		w=ew*(foldc2-foldc1+1);
		h=eh*(foldr2-foldr1+1);

		double rotation=foldprogress*M_PI;
		if (folddirection=='r' || folddirection=='t') rotation=M_PI-rotation;
		flatpoint axis;
		if (folddirection=='l' || folddirection=='r')
			axis=rotate(flatpoint(1,0),rotation,0);
		else axis=rotate(flatpoint(0,1),-rotation,0);

		if (foldunder) {
			dp->NewFG(.2,.2,.2);
			dp->LineAttributes(2, LineOnOffDash, CapButt, JoinMiter);
			if (folddirection=='l' || folddirection=='r') axis.y=-axis.y;
			else axis.x=-axis.x;
		} else {
			dp->NewFG(.9,.9,.9);
			dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);
		}

		x=signature->insetleft;
		flatpoint pts[4];
		for (int tx=0; tx<signature->tilex; tx++) {
		  y=signature->insetbottom;
		  for (int ty=0; ty<signature->tiley; ty++) {

			if (folddirection=='l' || folddirection=='r') { //horizontal fold
				pts[0]=flatpoint(x+foldindex*ew,y+eh*foldr1);
				pts[1]=pts[0]+axis*w;
				pts[2]=pts[1]+flatpoint(0,h);
				pts[3]=pts[0]+flatpoint(0,h);
			} else { 							//vertical fold
				pts[0]=flatpoint(x+ew*foldc1,y+foldindex*eh);
				pts[1]=pts[0]+axis*h;
				pts[2]=pts[1]+flatpoint(w,0);
				pts[3]=pts[0]+flatpoint(w,0);
			}
			if (foldunder) dp->drawlines(pts,4,0,0);
			else dp->drawlines(pts,4,1,1);
			
			y+=patternheight+signature->tilegapy;
		  }
		  x+=patternwidth+signature->tilegapx;
		}
	}

	 //draw fold indicator overlays on left side of screen
	dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);

	int thing;
	for (int c=signature->folds.n-1; c>=-1; c--) {
		if (c==-1) thing=THING_Circle;
		else if (c==signature->folds.n-1 && hasfinal) thing=THING_Square;
		else thing=THING_Triangle_Down;
		getFoldIndicatorPos(c, &x,&y,&w,&h);

		 //color hightlighted to show which fold we are currently on
		if (foldlevel==c+1) dp->NewFG(1.,.5,1.);
		else dp->NewFG(1.,1.,1.);

		dp->drawthing(x+w/2,y+h/2, w/2,h/2, 1, thing); //filled
		dp->NewFG(1.,0.,1.);
		dp->drawthing(x+w/2,y+h/2, w/2,h/2, 0, thing); //outline
	}

	 //write out final page dimensions
	dp->NewFG(0.,0.,0.);
	sprintf(str,_("%d pages per pattern, Final size: %g x %g %s"),
				signature->PagesPerPattern(),
				signature->PageWidth(1)*laidout->unitmultiplier,
				signature->PageHeight(1)*laidout->unitmultiplier,
				laidout->unitname);
	dp->textout(0,0, str,-1, LAX_LEFT|LAX_TOP);

	 //draw sheet per signature indicator
	dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);
	dp->NewFG(.5,0.,0.); //dark red for inset
	y=-signature->totalheight*.1;
	dp->drawline(0,y, signature->totalwidth,y);
	y-=signature->totalheight*.01;
	for (int c=1; c<5 && c<signature->sheetspersignature; c++) {
		dp->drawline(0,y, signature->totalwidth,y);
		y-=signature->totalheight*.01;
	}
	if (signature->autoaddsheets) sprintf(str,_("Many sheets in a single signature"));
	else if (signature->sheetspersignature==1) sprintf(str,_("1 sheet per signature"));
	else sprintf(str,_("%d sheets per signature"),signature->sheetspersignature);
	pts[0]=dp->realtoscreen(signature->totalwidth/2,y);
	dp->textout(pts[0].x,pts[0].y, str,-1, LAX_HCENTER|LAX_TOP);


	 //-----------------draw control handles
	ActionArea *area;
	for (int c=0; c<controls.n; c++) {
		area=controls.e[c];
		if (!area->visible) continue;

		if (controls.e[c]->action==SP_Sheets_Per_Sig) {
			// ***
		} else {
			if (area->outline) {
				if (area->real) dp->DrawReal(); else dp->DrawScreen();
				dp->NewFG(area->color);
				dp->drawlines(area->outline,area->npoints,1,1);
			}

		}
	}
	dp->DrawReal();

//	int handlepos=3;
//	if (foldlevel==0 && signature->folds.n==0) {
//		handlepos=4;
//	}
//
//	 //draw arrow handles for inset and tile gap
//	if (foldlevel==0) {
//		dp->NewFG(1.,0.,1.);
//	}
//
//	 //draw arrow handles for trim and margin
//	if (hasfinal && foldlevel==signature->folds.n-1) {
//		 //trim handle
//		dp->NewFG(1.,0.,0.);
//
//		 //margin handle
//		dp->NewFG(0.,0.,1.);
//
//		 //binding edge handle
//		dp->NewFG(1.,0.,0.);
//	}

	return 0;
}

/*! which==-1 means all unfolded. 0 or more means that fold.
 */
void SignatureInterface::getFoldIndicatorPos(int which, double *x,double *y, double *w,double *h)
{
	int radius=INDICATOR_SIZE;

	*x=dp->Minx;
	*y=(dp->Maxy+dp->Miny)/2 - (signature->folds.n+1)*(radius-3);
	*w=2*radius;
	*h=2*radius;

	*y+=(which+1)*(2*radius-3);
}



//! Scan for handles to control variables, not for row and column.
int SignatureInterface::scanhandle(int x,int y)
{
	flatpoint fp=screentoreal(x,y);

	//double xx=fp.x;
	//double yy=fp.y;

	int point=SP_None;

//	if (foldlevel==0) {
//		 //check for inset handles, tilex/y, tile gap x/y
//		if (xx<0 || xx>totalwidth) {
//			if (yy>0 && yy<totalheight/2) point=SP_Inset_Left;
//			else if (yy>=totalheight/2 && yy<totalheight) point=SP_Tile_Y;
//		} else if (yy<0 || yy>totalheight) {
//			if (xx>0 && xx<totalwidth/2) point=SP_Tile_Y;
//			else if (yy>=totalheight/2 && yy<totalheight) point=SP_Inset_;
//		}
//	}
//
//	if (foldlevel==signature->folds.n-1) {
//		 //check for final page trim lrtb, margin, binding edge, up, x,y
//		***
//	}

	return point;
}

//! Return 0 for not in pattern, or the row and column in a folding section.
/*! x and y are real coordinates.
 *
 * Returns the real coordinate within an element of the folding pattern in ex,ey.
 */
int SignatureInterface::scan(int x,int y,int *row,int *col,double *ex,double *ey, int *tile_row, int *tile_col)
{
	flatpoint fp=screentoreal(x,y);
	DBG cerr <<"fp:"<<fp.x<<','<<fp.y<<endl;

	fp.x-=signature->insetleft;
	fp.y-=signature->insetbottom;

	double patternheight=signature->PatternHeight();
	double patternwidth =signature->PatternWidth();
	double elementheight=patternheight/(signature->numhfolds+1);
	double elementwidth =patternwidth /(signature->numvfolds+1);

	int tilex,tiley;
	tilex=floorl(fp.x/(patternwidth +signature->tilegapx));
	tiley=floorl(fp.y/(patternheight+signature->tilegapy));
	if (tile_col) *tile_col=tilex;
	if (tile_row) *tile_row=tiley;

	fp.x-=tilex*(patternwidth +signature->tilegapx);
	fp.y-=tiley*(patternheight+signature->tilegapy);

	DBG cerr <<"tilex,y: "<<tilex<<","<<tiley<<endl;

	*row=floorl(fp.y/elementheight);
	*col=floorl(fp.x/elementwidth);

	 //find coordinates within an element cell
	if (ey) *ey=fp.y-(*row)*elementheight;
	if (ex) *ex=fp.x-(*col)*elementwidth;

	return *row>=0 && *row<=signature->numhfolds && *col>=0 && *col<=signature->numvfolds;
}

//! Respond to spinning controls.
int SignatureInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	flatpoint fp=screentoreal(x,y);
	if (fp.x>0 && fp.x<signature->totalwidth && fp.y<-signature->totalheight*.1 && fp.y>-signature->totalheight*.2) {
		signature->sheetspersignature--;
		if (signature->sheetspersignature<=0) {
			signature->autoaddsheets=1;
			signature->sheetspersignature=0;
		}
		needtodraw=1;
		return 0;
	}
	return 1;
}

//! Respond to spinning controls.
int SignatureInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	flatpoint fp=screentoreal(x,y);
	if (fp.x>0 && fp.x<signature->totalwidth && fp.y<-signature->totalheight*.1 && fp.y>-signature->totalheight*.2) {
		signature->sheetspersignature++;
		signature->autoaddsheets=0;
		needtodraw=1;
		return 0;
	}
	return 1;
}

int SignatureInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int row,col,tilerow,tilecol;
	int over=scan(x,y, &row,&col, NULL,NULL, &tilerow,&tilecol);
	DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<endl;

	if (buttondown.any()) return 0;

	buttondown.down(d->id,LEFTBUTTON, x,y, row,col);

	 //check overlays first
	double xx,yy,w,h;
	onoverlay=SP_None;
	for (int c=signature->folds.n-1; c>=-1; c--) {
		getFoldIndicatorPos(c, &xx,&yy,&w,&h);
		if (x>=xx && x<xx+w && y>=yy && y<yy+h) {
			onoverlay=SP_FOLDS+1+c;
			foldprogress=-1;
			buttondown.moveinfo(d->id,LEFTBUTTON, c,0); //record which indicator is clicked down in
			return 0;
		}
	}

	if (row<0 || row>signature->numhfolds || col<0 || col>signature->numvfolds
		  || foldinfo[row][col].pages.n==0
		  || tilerow<0 || tilecol<0
		  || tilerow>signature->tiley || tilecol>signature->tilex) {
		lbdown_row=lbdown_col=-1;
	} else {
		lbdown_row=row;
		lbdown_col=col;
		folddirection=0;
		foldprogress=0;
	}


	return 0;
}

int SignatureInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!(buttondown.isdown(d->id,LEFTBUTTON))) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	if (onoverlay) {
		if (onoverlay>=SP_FOLDS) {
			 //selecting different fold maybe...
			if (!dragged) {
				 //we clicked down then up on the same overlay

				int folds=onoverlay-SP_FOLDS; //0 means totally unfolded

				if (foldlevel==folds) return 0; //already at that fold level

				 //we must remap the folds to reflect the new fold level
				signature->resetFoldinfo(NULL);
				for (int c=0; c<folds; c++) {
					applyFold(signature->folds.e[c]);
				}
				foldlevel=folds;
				//checkFoldLevel(1);
				needtodraw=1;
			}
		}
		return 0;
	}

	if (folddirection && folddirection!='x' && foldprogress>.9) {
		 //apply the fold, and add to the signature...

		applyFold(folddirection,foldindex,foldunder);

		if (foldlevel<signature->folds.n) {
			 //we have tried to fold when there are already further folds, so we must remove any
			 //after the current foldlevel.
			while (foldlevel<signature->folds.n) signature->folds.remove();
			hasfinal=0;
		}

		signature->folds.push(new Fold(folddirection,foldunder,foldindex),1);
		foldlevel=signature->folds.n;

		checkFoldLevel(1);

		folddirection=0;
		foldprogress=0;
	}

	needtodraw=1;
	return 0;
}

//int SignatureInterface::MBDown(int x,int y,unsigned int state,int count)
//int SignatureInterface::MBUp(int x,int y,unsigned int state)
//int SignatureInterface::RBDown(int x,int y,unsigned int state,int count)
//int SignatureInterface::RBUp(int x,int y,unsigned int state)

int SignatureInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	int row,col,tilerow,tilecol;
	flatpoint mm;
	int over=scan(x,y, &row,&col, &mm.x,&mm.y, &tilerow,&tilecol);
	//fp now holds coordinates relative to the element cell

	DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<"  mm="<<mm.x<<','<<mm.y<<"  tile r,c:"<<tilerow<<','<<tilecol;
	DBG if (row>=0 && row<signature->numhfolds+1 && col>=0 && col<signature->numvfolds+1)
	DBG    cerr <<"  xflip: "<<foldinfo[row][col].x_flipped<<"  yflip:"<<foldinfo[row][col].y_flipped
	DBG         <<"  pages:"<<foldinfo[row][col].pages.n<<endl;

	int mx,my;
	int lx,ly;
	if (!buttondown.any()) return 0;
	buttondown.move(mouse->id,x,y, &lx,&ly);

	if (onoverlay!=SP_None) {
		if (onoverlay>=SP_FOLDS) {
			if (ly-y==0) return 0; //return if not moved vertically

			int startindicator,lasty;
			buttondown.getinitial(mouse->id,LEFTBUTTON, &mx,&my);
			buttondown.getlast(mouse->id,LEFTBUTTON, NULL,&lasty);
			buttondown.getextrainfo(mouse->id,LEFTBUTTON, &startindicator,NULL);
			double curdist =(    y-my)/(INDICATOR_SIZE-3) + startindicator;
			double lastdist=(lasty-my)/(INDICATOR_SIZE-3) + startindicator;

			if ((int)curdist==(int)lastdist || (lastdist<0 && curdist<0) 
					|| (lastdist>=signature->folds.n && curdist>=signature->folds.n)) {
				 //only change foldprogress, still in same fold indicator
				if (foldprogress==-1) {
					 //we have not moved before, so we must do an initial map of affected cells
					remapAffectedCells((int)curdist);
				}
				foldprogress=curdist-lastdist;
				if (foldprogress<0) foldprogress=0;
				if (foldprogress>1) foldprogress=1;
				needtodraw=1;
				return 0;

			} else if ((int)curdist<(int)lastdist) {
				 //need to unfold a previous fold, for this, we need to get the map of state not including
				 //the current fold, to know which cells are actually affected by the unfolding..
				//***

			} else {
				 //need to advance one fold
				if (curdist>signature->folds.n) curdist=signature->folds.n;
				applyFold(signature->folds.e[(int)curdist-1]);
				remapAffectedCells((int)curdist);
				foldprogress=curdist-floor(curdist);
				needtodraw=1;
				return 0;
			}
		}
		return 0;
	}


	if (lbdown_row<0 || lbdown_col<0) return 0;
	if ((hasfinal && foldlevel==signature->folds.n-1)
			|| row<0 || row>signature->numhfolds || col<0 || col>signature->numvfolds) {
		if (folddirection!=0) {
			folddirection=0;
			needtodraw=1;
		}
		return 0;
	}
	foldunder=(state&LAX_STATE_MASK)!=0;

	buttondown.getinitial(mouse->id,LEFTBUTTON, &mx,&my);
	int ocol,orow, otrow,otcol;
	flatpoint om;
	scan(mx,my, &orow,&ocol, &om.x,&om.y, &otrow,&otcol);

	if (tilerow<otrow) row-=100;
	else if (tilerow>otrow) row+=100;
	if (tilecol<otcol) col-=100;
	else if (tilecol>otcol) col+=100;
	

	flatpoint d=screentoreal(x,y)-screentoreal(mx,my); 

	 //find the direction we are trying to fold in
	if (folddirection==0) {
		if (fabs(d.x)>fabs(d.y)) {
			if (d.x>0) {
				folddirection='r';
			} else if (d.x<0) {
				folddirection='l';
			}
		} else {
			if (d.y>0) {
				folddirection='t';
			} else if (d.y<0) {
				folddirection='b';
			}
		}
	}

	 //find how far we fold based on proximity of mouse to fold crease

	 //figure out which elements are affected by folding
	double elementwidth =signature->PageWidth();
	double elementheight=signature->PageHeight();
	if (folddirection=='r') {
		int adjacentcol=ocol+1; //edge is between ocol and adjacentcol
		int prevcol=ocol;
		while (prevcol>0 && foldinfo[orow][prevcol-1].pages.n!=0) prevcol--;
		int nextcol=adjacentcol;
		while (nextcol<signature->numvfolds) nextcol++; //it's ok to fold over onto blank areas

		if (nextcol>signature->numvfolds || nextcol-adjacentcol+1<ocol-prevcol+1
				|| (adjacentcol<=signature->numvfolds && foldinfo[orow][adjacentcol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=ocol+1;

			 //find the fold progress
			if (col==ocol) foldprogress=.5-(elementwidth-mm.x)/(elementwidth-om.x)/2;
			else if (col==ocol+1) foldprogress=.5+(mm.x)/(elementwidth-om.x)/2;
			else if (col<ocol) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldc1=prevcol;
			foldc2=ocol;
			foldr1=orow;
			foldr2=orow;
			while (foldr1>0 && foldinfo[foldr1-1][ocol].pages.n!=0) foldr1--;
			while (foldr2<signature->numhfolds && foldinfo[foldr2+1][ocol].pages.n!=0) foldr2++;
		}
		needtodraw=1;

	} else if (folddirection=='l') {
		int adjacentcol=ocol-1; //edge is between ocol and adjacentcol
		int nextcol=ocol;
		while (nextcol<signature->numvfolds && foldinfo[orow][nextcol+1].pages.n!=0) nextcol++;
		int prevcol=adjacentcol;
		while (prevcol>0) prevcol--; //it's ok to fold over onto blank areas

		if (prevcol<0 || adjacentcol-prevcol+1<nextcol-ocol+1
				|| (adjacentcol>=0 && foldinfo[orow][adjacentcol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=ocol;

			 //find the fold progress
			if (col==ocol) foldprogress=.5-mm.x/om.x/2;
			else if (col==ocol-1) foldprogress=.5+(elementwidth-mm.x)/om.x/2;
			else if (col>ocol) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldc1=ocol;
			foldc2=nextcol;
			foldr1=orow;
			foldr2=orow;
			while (foldr1>0 && foldinfo[foldr1-1][ocol].pages.n!=0) foldr1--;
			while (foldr2<signature->numhfolds && foldinfo[foldr2+1][ocol].pages.n!=0) foldr2++;
		}
		needtodraw=1;

	} else if (folddirection=='t') {
		int adjacentrow=orow+1; //edge is between ocol and adjacentcol
		int prevrow=orow;
		while (prevrow>0 && foldinfo[prevrow-1][ocol].pages.n!=0) prevrow--;
		int nextrow=adjacentrow;
		while (nextrow<signature->numhfolds) nextrow++; //it's ok to fold over onto blank areas

		if (nextrow>signature->numhfolds || nextrow-adjacentrow+1<orow-prevrow+1
				|| (adjacentrow<=signature->numhfolds && foldinfo[adjacentrow][ocol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=orow+1;

			 //find the fold progress
			if (row==orow) foldprogress=.5-(elementheight-mm.y)/(elementheight-om.y)/2;
			else if (row==orow+1) foldprogress=.5+(mm.y)/(elementheight-om.y)/2;
			else if (row<orow) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldr1=prevrow;
			foldr2=orow;
			foldc1=ocol;
			foldc2=ocol;
			while (foldc1>0 && foldinfo[orow][foldc1-1].pages.n!=0) foldc1--;
			while (foldc2<signature->numvfolds && foldinfo[orow][foldc2+1].pages.n!=0) foldc2++;
		}
		needtodraw=1;

	} else if (folddirection=='b') {
		int adjacentrow=orow-1; //edge is between orow and adjacentrow
		int nextrow=orow;
		while (nextrow<signature->numhfolds && foldinfo[nextrow+1][ocol].pages.n!=0) nextrow++;
		int prevrow=adjacentrow;
		while (prevrow>0) prevrow--; //it's ok to fold over onto blank areas

		if (prevrow<0 || adjacentrow-prevrow+1<nextrow-orow+1
				|| (adjacentrow>=0 && foldinfo[adjacentrow][ocol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=orow;

			 //find the fold progress
			if (row==orow) foldprogress=.5-mm.y/om.y/2;
			else if (row==orow-1) foldprogress=.5+(elementheight-mm.y)/om.y/2;
			else if (row>orow) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldr1=orow;
			foldr2=nextrow;
			foldc1=ocol;
			foldc2=ocol;
			while (foldc1>0 && foldinfo[orow][foldc1-1].pages.n!=0) foldc1--;
			while (foldc2<signature->numvfolds && foldinfo[orow][foldc2+1].pages.n!=0) foldc2++;
		}
		needtodraw=1;
	}

	DBG cerr <<"folding progress: "<<foldprogress<<",  om="<<om.x<<','<<om.y<<"  mm="<<mm.x<<','<<mm.y<<endl;


	return 0;
}

//! Adjust foldr1,foldr2,foldc1,foldc2 to reflect which cells get moved for whichfold.
void SignatureInterface::remapAffectedCells(int whichfold)
{
	FoldedPageInfo **finfo=NULL;
	if (whichfold==foldlevel+1) {
		 //we can use the current foldinfo
		finfo=foldinfo;
	} else {
		 //we need to use a temporary foldinfo
		finfo=new FoldedPageInfo*[signature->numhfolds+2];
		int r;
		for (r=0; r<signature->numhfolds+1; r++) {
			finfo[r]=new FoldedPageInfo[signature->numvfolds+2];
			for (int c=0; c<signature->numvfolds+1; c++) {
				finfo[r][c].pages.push(r);
				finfo[r][c].pages.push(c);
			}
		}
		finfo[r]=NULL; //terminating NULL, so we don't need to remember sig->n

		signature->applyFold(finfo,whichfold);
	}

	// *** figure out cells based on direction and index

	if (finfo!=foldinfo) {
		for (int c=0; finfo[c]; c++) delete[] finfo[c];
		delete[] finfo;
	}
}

static const char *masktostr(int m)
{
	if (m==15) return _("all");
	if (m==1) return _("top");
	if (m==2) return _("right");
	if (m==4) return _("bottom");
	if (m==8) return _("left");
	return "?";
}

/*!
 * \todo inset shortcuts should be able to selectively adjust lrt or b insets and other things, not just all at once,
 * 		maybe have 'i' toggle which inset value to adjust, then subsequent arrow keys adjust values, modifiers adjust
 * 		the scale of the adjustment step
 */
int SignatureInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" SignatureInterface got ch:"<<(int)ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch=='d' && (state&LAX_STATE_MASK)==0) {
		showdecs++;
		if (showdecs>2) showdecs=0;
		needtodraw=1;
		return 0;

		//--------------select how many folds to display
	} else if (ch>='0' && ch <='9') {
		// ***
		return 0;

	} else if (ch==' ' && (state&LAX_STATE_MASK)==0) {
		int h=signature->totalheight;
		int w=signature->totalwidth;
		viewport->dp->Center(-w*.15,w*1.15, -h*.15,h*1.15);
		needtodraw=1;
		return 0;
		
		//--------------change inset
	} else if (ch=='i') {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (insetmask==15) insetmask=1;
			else { insetmask<<=1; if (insetmask>15) insetmask=15; }
			char str[100];
			sprintf(str,_("Sets %s inset"),masktostr(insetmask));
			viewport->postmessage(str);
			return 0;
		}
		if (insetmask&1) signature->insettop   +=signature->totalheight*.01;
		if (insetmask&4) signature->insetbottom+=signature->totalheight*.01;
		if (insetmask&8) signature->insetleft  +=signature->totalheight*.01;
		if (insetmask&2) signature->insetright +=signature->totalheight*.01;
		needtodraw=1;
		return 0;

	} else if (ch=='I') {
		double step=signature->totalheight*.01;
		if (insetmask&1) { signature->insettop   -=step; if (signature->insettop<0)    signature->insettop=0; }
		if (insetmask&4) { signature->insetbottom-=step; if (signature->insetbottom<0) signature->insetbottom=0; }
		if (insetmask&8) { signature->insetleft  -=step; if (signature->insetleft<0)   signature->insetleft=0; }
		if (insetmask&2) { signature->insetright -=step; if (signature->insetright<0)  signature->insetright=0; }
		needtodraw=1;
		return 0;

		//--------------change tile gap
	} else if (ch=='g') {
		signature->tilegapx+=signature->totalheight*.01;
		signature->tilegapy+=signature->totalheight*.01;
		needtodraw=1;
		return 0;

	} else if (ch=='G') {
		signature->tilegapx-=signature->totalheight*.01;
		if (signature->tilegapx<0) signature->tilegapx=0;
		signature->tilegapy=signature->tilegapx;
		needtodraw=1;
		return 0;

		//-------------tilex and tiley
	} else if (ch=='x') {
		signature->tilex++;
		needtodraw=1;
		return 0;

	} else if (ch=='X') {
		signature->tilex--;
		if (signature->tilex<=1) signature->tilex=1;
		needtodraw=1;
		return 0;

	} else if (ch=='y') {
		signature->tiley++;
		needtodraw=1;
		return 0;

	} else if (ch=='Y') {
		signature->tiley--;
		if (signature->tiley<=1) signature->tiley=1;
		needtodraw=1;
		return 0;

		//-----------numhfolds and numvfolds
	} else if (ch=='v') {
		if (foldlevel!=0) return 0;
		signature->numvfolds++;
		reallocateFoldinfo();
		checkFoldLevel(1);
		needtodraw=1;
		return 0;

	} else if (ch=='V') {
		if (foldlevel!=0) return 0;
		int old=signature->numvfolds;
		signature->numvfolds--;
		if (signature->numvfolds<=0) signature->numvfolds=0;
		if (old!=signature->numvfolds) {
			reallocateFoldinfo();
			checkFoldLevel(1);
			needtodraw=1;
		}
		return 0;

	} else if (ch=='h') {
		if (foldlevel!=0) return 0;
		signature->numhfolds++;
		reallocateFoldinfo();
		checkFoldLevel(1);
		needtodraw=1;
		return 0;

	} else if (ch=='H') {
		if (foldlevel!=0) return 0;
		int old=signature->numhfolds;
		signature->numhfolds--;
		if (signature->numhfolds<=0) signature->numhfolds=0;
		if (old!=signature->numhfolds) {
			reallocateFoldinfo();
			checkFoldLevel(1);
			needtodraw=1;
		}
		return 0;

		//---------------move binding edge
	} else if (ch=='b') {
		if (signature->binding=='l') signature->binding='t';
		else if (signature->binding=='t') signature->binding='r';
		else if (signature->binding=='r') signature->binding='b';
		else if (signature->binding=='b') signature->binding='l';
		needtodraw=1;
		return 0;

	} else if (ch=='B') {
		if (signature->binding=='l') signature->binding='b';
		else if (signature->binding=='b') signature->binding='r';
		else if (signature->binding=='r') signature->binding='t';
		else if (signature->binding=='t') signature->binding='l';
		needtodraw=1;
		return 0;

		//---------------page orientation
	} else if (ch=='u') { //up direction
		if (signature->up=='l') signature->up='t';
		else if (signature->up=='t') signature->up='r';
		else if (signature->up=='r') signature->up='b';
		else if (signature->up=='b') signature->up='l';
		needtodraw=1;
		return 0;

		//--------------change page trim
	} else if (ch=='t') {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (trimmask==15) trimmask=1;
			else { trimmask<<=1; if (trimmask>15) trimmask=15; }
			char str[100];
			sprintf(str,_("Set %s trim"),masktostr(trimmask));
			viewport->postmessage(str);
			return 0;
		}
		if (trimmask&1) signature->trimtop   +=signature->PageHeight()*.01;
		if (trimmask&4) signature->trimbottom+=signature->PageHeight()*.01;
		if (trimmask&8) signature->trimleft  +=signature->PageHeight()*.01;
		if (trimmask&2) signature->trimright +=signature->PageHeight()*.01;
		needtodraw=1;
		return 0;

	} else if (ch=='T') {
		double step=signature->PageHeight()*.01;
		if (trimmask&1) { signature->trimtop   -=step; if (signature->trimtop<0)    signature->trimtop=0; }
		if (trimmask&4) { signature->trimbottom-=step; if (signature->trimbottom<0) signature->trimbottom=0; }
		if (trimmask&8) { signature->trimleft  -=step; if (signature->trimleft<0)   signature->trimleft=0; }
		if (trimmask&2) { signature->trimright -=step; if (signature->trimright<0)  signature->trimright=0; }
		needtodraw=1;
		return 0;

		//--------------change page margin
	} else if (ch=='m') {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (marginmask==15) marginmask=1;
			else { marginmask<<=1; if (marginmask>15) marginmask=15; }
			char str[100];
			sprintf(str,_("Set %s margin"),masktostr(marginmask));
			viewport->postmessage(str);
			return 0;
		}
		if (marginmask&1) signature->margintop   +=signature->PageHeight()*.01;
		if (marginmask&4) signature->marginbottom+=signature->PageHeight()*.01;
		if (marginmask&8) signature->marginleft  +=signature->PageHeight()*.01;
		if (marginmask&2) signature->marginright +=signature->PageHeight()*.01;
		needtodraw=1;
		return 0;

	} else if (ch=='M') {
		double step=signature->PageHeight()*.01;
		if (marginmask&1) { signature->margintop   -=step; if (signature->margintop<0)    signature->margintop=0; }
		if (marginmask&4) { signature->marginbottom-=step; if (signature->marginbottom<0) signature->marginbottom=0; }
		if (marginmask&8) { signature->marginleft  -=step; if (signature->marginleft<0)   signature->marginleft=0; }
		if (marginmask&2) { signature->marginright -=step; if (signature->marginright<0)  signature->marginright=0; }
		needtodraw=1;
		return 0;

	}

	return 1;
}

int SignatureInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
//	if (ch==LAX_Shift) {
//		return 0;
//	}
	return 1;
}


//----------------------- SignatureEditor --------------------------------------

/*! \class SignatureEditor
 * \brief A Laxkit::ViewerWindow that gets filled with stuff appropriate for signature editing.
 *
 * This creates the window with a SignatureInterface.
 */


//! Make the window using project.
/*! Inc count of ndoc.
 */
SignatureEditor::SignatureEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						Laxkit::anXWindow *nowner, const char *mes,
						SignatureImposition *sigimp, PaperStyle *p)
	: ViewerWindow(parnt,nname,ntitle,
				   ANXWIN_REMEMBER
					|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_NO_SCROLLERS|VIEWPORT_NO_RULERS, 
					0,0,500,500, 0, NULL)
{
	SetOwner(nowner,mes);

	//signature=sig;
	//if (signature) signature->inc_count();

	if (!viewport) {
		viewport=new ViewportWindow(this,"signature-editor-viewport","signature-editor-viewport",
									ANXWIN_HOVER_FOCUS|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,
									0,0,0,0,0,NULL);
		app->reparent(viewport,this);
		viewport->dec_count();
	}

	win_colors->bg=rgbcolor(200,200,200);
	viewport->dp->NewBG(200,200,200);

	needtodraw=1;
	tool=new SignatureInterface(1,viewport->dp,(sigimp?sigimp->signature:NULL),p);
	AddTool(tool,1,1); // local, and select it
	// *** add signature and paper if any...
}

SignatureEditor::~SignatureEditor()
{ }

//! Passes off to SignatureInterface::dump_out().
void SignatureEditor::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	// *** ((SignatureInterface *)curtool)->dump_out(f,indent,what,context);
}

//! Passes off to SignatureInterface::dump_in_atts().
void SignatureEditor::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	// *** ((SignatureInterface *)curtool)->dump_in_atts(att,flag,context);
}

/*! Removes rulers and adds Apply, Reset, and Update Thumbs.
 */
int SignatureEditor::init()
{
	ViewerWindow::init();
	viewport->dp->NewBG(200,200,200);

	anXWindow *last=NULL;
	Button *tbut;

	last=tbut=new Button(this,"ok",NULL, 0, 0,0,0,0,1, last,object_id,"ok",0,_("Ok"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	last=tbut=new Button(this,"cancel",NULL, 0, 0,0,0,0,1, last,object_id,"cancel",0,_("Cancel"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	Sync(1);	

	int h=tool->signature->totalheight;
	int w=tool->signature->totalwidth;
	viewport->dp->Center(-w*.15,w*1.15, -h*.15,h*1.15);
	return 0;
}

//! Send the current imposition to win_owner.
void SignatureEditor::send()
{
	SignatureImposition *sigimp=new SignatureImposition(tool->signature);
	sigimp->SetPaperSize(sigimp->signature->paperbox);
	RefCountedEventData *data=new RefCountedEventData(sigimp);
	sigimp->dec_count();

	app->SendMessage(data, win_owner, win_sendthis, object_id);
}

/*! Responds to: "ok", "cancel"
 *
 */
int SignatureEditor::Event(const Laxkit::EventData *data,const char *mes)
{
	DBG cerr <<"SignatureEditor got message: "<<(mes?mes:"?")<<endl;

	if (!strcmp(mes,"ok")) {
		send();
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	} else if (!strcmp("cancel",mes)) {
		EventData *e=new EventData(LAX_onCancel);
		app->SendMessage(e, win_owner, win_sendthis, object_id);

		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	}
	return 1;
}

int SignatureEditor::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

//	} else if (ch==LAX_F1 && (state&LAX_STATE_MASK)==0) {
//		app->addwindow(new HelpWindow());
//		return 0;
	}
	return 1;
}



//} // namespace Laidout

