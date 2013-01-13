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
// Copyright (C) 2013 by Tom Lechner
//
#ifndef PLUGIN_H
#define PLUGIN_H


#include <lax/anobject.h>
#include <lax/newwindowobject.h>
//#include "../calculator/values.h"
//#include "../filetypes/filefilters.h"
#include "../laidout.h"


namespace Laidout {

class PluginBase : public Laxkit::anObject
{
  public:
	virtual const char *PluginName()  = 0;
	virtual const char *Version()     = 0;
	virtual const char *Description() = 0;
	virtual const char *Author()      = 0;
	virtual const char *ReleaseDate() = 0;
	virtual const char *License()     = 0;
	virtual LaxFiles::Attribute *OtherMeta() = 0;

	virtual unsigned long WhatYouGot();

	 //return NULL terminated list
	virtual Laxkit::NewWindowObject    **WindowPanes() { return NULL; }
	//virtual ImageImportFilter        **ImageImportFilters() { return NULL; }
	virtual ImportFilter               **ImportFilters() { return NULL; }
	virtual ExportFilter               **ExportFilters() { return NULL; }
	virtual LaxInterfaces::anInterface **Tools() { return NULL; } //must specify what window types they should appear in
	//virtual LaidoutAction            **Actions() { return NULL; }//maybe scripted code actions to be available for key bindings in windows
	//virtual Interpreter                **Interpreters() { return NULL; } //like python. selecting "Run" of a PlainTextObject uses these

	//virtual Resource                   **ResourceInstances() { return NULL; }
	virtual Imposition                 **Impositions() { return NULL; }
	//virtual Config                   **Configs() { return NULL; } //tool settings and global Laidout config
	virtual DrawableObject             **ObjectInstances() { return NULL; } //like scrapbook items

	virtual CalculatorModule  **CalculatorModules() { return NULL; }
	
	PluginBase();
	virtual ~PluginBase();
};


} //namespace Laiodut


#endif
