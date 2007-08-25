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
// Copyright (C) 2007 by Tom Lechner
//

#include "language.h"
#include "utils.h"
#include <lax/strmanip.h>
#include <lax/fileutils.h>

#include <cctype>
#include <cstdlib>


#include <iostream>
#define DBG

using namespace LaxFiles;
using namespace std;





////! Find out the type of Laidout file this is, if any.
//int laidout_file_type(const char *file, char **version, char **typ)


	
//! Check if the file is a Laidout type typ, with a version [minversion,maxversion].
/*! \ingroup misc
 * Return 0 for file ok, else nonzero.
 *
 * \todo implement the version check
 */
int laidout_file_type(const char *file, char *minversion, char *maxversion, char *typ)
{
	if (file_exists(file,1,NULL)!=S_IFREG) return 1;

	FILE *f=fopen(file,"r");
	if (!f) return 2;
	
	char first100[100];
	int n=fread(first100,1,100,f);
	first100[n-1]='\0';
	int err=1;
	if (!strncmp(first100,"#Laidout ",9)) {
		char *version=first100+9;
		int c=9,c2=0,c3;
		while (c<n && isspace(*version) && *version!='\n') { version++; c++; }
		while (c<n && !isspace(version[c2])) { c2++; c++; }
		
		 //now the laidout version of the file is in version[0..c2)
		//*** check!

		c3=c2;
		while (c<n && isspace(version[c3])) { c3++; c++; }
		if (!strncmp(version+c3,typ,strlen(typ)) && isspace(version[c3+strlen(typ)])) err=0;
	}

	fclose(f);
	return err;
}

/*! \ingroup misc
 * Versions should be something like 0.05.1, or 0.05.custom.
 * This function merely checks the first part of the version string that can be converted
 * to a floating point number. If the version number cannot be so parsed, then 0 will be returned.
 *
 * Otherwise, 1 will be returned if version within bounds, else 0.
 */
int laidout_version_check(const char *version, const char *minversion, const char *maxversion)
{
	float v,min,max;
	char *end;
	
	v=strtof(version,&end);
	if (end==version) return 0;

	min=strtof(minversion,&end);
	if (end==minversion) return 0;

	max=strtof(maxversion,&end);
	if (end==maxversion) return 0;

	return v>=min && v<=max;
}

//! Simplify opening any sort of file for writing, does basic error checking, such as for existence, and writability.
/*! \ingroup misc
 *
 * If nooverwrite, then do not overwrite existing files.
 *
 * Returns the opened file, or NULL if file cannot be opened for writing,
 * and error_ret gets set to a proper error message if *error_ret was NULL,
 * or appended to error_ret if *error_ret!=NULL. Beware of this!! If you start
 * with a blank error, then remember to set it to NULL before calling this function!
 */
FILE *open_file_for_writing(const char *file,int nooverwrite,char **error_ret)
{
	int exists=file_exists(file,1,NULL);
	if (exists && exists!=S_IFREG) {
		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot write to %s."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}
	
	if (exists && nooverwrite) {
		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot overwrite %s."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}

	FILE *f=fopen(file,"w");

	if (!f) {
		DBG cerr <<"**** cannot load, "<<(file?file:"(nofile)")<<" cannot be opened for writing."<<endl;

		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot write to %s."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}

	return f;
}
	
//! Simplify opening any sort of file for reading, does basic error checking, such as for existence, and readability.
/*! \ingroup misc
 *
 * Returns the opened file, or NULL if file cannot be opened for reading,
 * and error_ret gets set to a proper error message if *error_ret was NULL,
 * or appended to error_ret if *error_ret!=NULL. Beware of this!! If you start
 * with a blank error, then remember to set it to NULL before calling this function!
 */
FILE *open_file_for_reading(const char *file,char **error_ret)
{
	if (file_exists(file,1,NULL)!=S_IFREG) {
		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot read the file %s. Wrong type."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}
	
	FILE *f=fopen(file,"r");

	if (!f) {
		DBG cerr <<"**** cannot load, "<<(file?file:"(nofile)")<<" cannot be opened for reading."<<endl;

		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot read file %s."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}

	return f;
}
	

//! Simplify opening Laidout specific files, does basic error checking, such as for existence, and readability.
/*! \ingroup misc
 *
 * what is the type of file, for instance "Project" or "Document".	
 * Returns the opened file, or NULL if file cannot be opened for reading,
 * and error_ret gets set to a proper error message if *error_ret was NULL,
 * or appended to error_ret if *error_ret!=NULL. Beware of this!! If you start
 * with a blank error, then remember to set it to NULL before calling this function!
 */
FILE *open_laidout_file_to_read(const char *file,const char *what,char **error_ret)
{
	FILE *f=open_file_for_reading(file,error_ret);
	if (!f) return NULL;

	 // make sure it is a laidout file!!
	char first100[100];
	int n=fread(first100,1,100,f);
	first100[n-1]='\0';
	int err=1;
	if (!strncmp(first100,"#Laidout ",9)) {
		char *version=first100+9;
		int c=9,c2=0,c3;
		while (c<n && isspace(*version) && *version!='\n') { version++; c++; }
		while (c<n && !isspace(version[c2])) { c2++; c++; }
		 //now the laidout version of the file is in version[0..c2)
		c3=c2;
		while (c<n && isspace(version[c3])) { c3++; c++; }
		if (!strncmp(version+c3,what,strlen(what)) && isspace(version[c3+strlen(what)])) err=0;
	}
	if (err) {
		if (error_ret) {
			*error_ret=new char[strlen(file)+strlen(what)+100];//****this definite 100 might cause problems!!
			sprintf(*error_ret, _("%s does not appear to be a Laidout %s file."), file, what);
		}
		fclose(f);
		return NULL;
	}
	rewind(f);
	return f;
}
