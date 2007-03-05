/*
 * Debugging object format (used to debug object format module interface)
 *
 *  Copyright (C) 2001  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <util.h>
/*@unused@*/ RCSID("$Id$");

#define YASM_LIB_INTERNAL
#include <libyasm.h>


typedef struct yasm_objfmt_dbg {
    yasm_objfmt_base objfmt;	    /* base structure */

    FILE *dbgfile;
} yasm_objfmt_dbg;

yasm_objfmt_module yasm_dbg_LTX_objfmt;


static yasm_objfmt *
dbg_objfmt_create(yasm_object *object)
{
    yasm_objfmt_dbg *objfmt_dbg = yasm_xmalloc(sizeof(yasm_objfmt_dbg));

    objfmt_dbg->objfmt.module = &yasm_dbg_LTX_objfmt;

    objfmt_dbg->dbgfile = tmpfile();
    if (!objfmt_dbg->dbgfile) {
	fprintf(stderr, N_("could not open temporary file"));
	return 0;
    }
    fprintf(objfmt_dbg->dbgfile, "create()\n");
    return (yasm_objfmt *)objfmt_dbg;
}

static void
dbg_objfmt_output(yasm_object *object, FILE *f, int all_syms,
		  yasm_errwarns *errwarns)
{
    yasm_objfmt_dbg *objfmt_dbg = (yasm_objfmt_dbg *)object->objfmt;
    char buf[1024];
    size_t i;

    /* Copy temp file to real output file */
    rewind(objfmt_dbg->dbgfile);
    while ((i = fread(buf, 1, 1024, objfmt_dbg->dbgfile))) {
	if (fwrite(buf, 1, i, f) != i)
	    break;
    }

    /* Reassign objfmt debug file to output file */
    fclose(objfmt_dbg->dbgfile);
    objfmt_dbg->dbgfile = f;

    fprintf(objfmt_dbg->dbgfile, "output(f, object->\n");
    yasm_object_print(object, objfmt_dbg->dbgfile, 1);
    fprintf(objfmt_dbg->dbgfile, "%d)\n", all_syms);
    fprintf(objfmt_dbg->dbgfile, " Symbol Table:\n");
    yasm_symtab_print(object->symtab, objfmt_dbg->dbgfile, 1);
}

static void
dbg_objfmt_destroy(yasm_objfmt *objfmt)
{
    yasm_objfmt_dbg *objfmt_dbg = (yasm_objfmt_dbg *)objfmt;
    fprintf(objfmt_dbg->dbgfile, "destroy()\n");
    yasm_xfree(objfmt);
}

static yasm_section *
dbg_objfmt_add_default_section(yasm_object *object)
{
    yasm_objfmt_dbg *objfmt_dbg = (yasm_objfmt_dbg *)object->objfmt;
    yasm_section *retval;
    int isnew;

    retval = yasm_object_get_general(object, ".text",
	yasm_expr_create_ident(yasm_expr_int(yasm_intnum_create_uint(200)), 0),
	0, 0, 0, &isnew, 0);
    if (isnew) {
	fprintf(objfmt_dbg->dbgfile, "(new) ");
	yasm_symtab_define_label(object->symtab, ".text",
	    yasm_section_bcs_first(retval), 1, 0);
	yasm_section_set_default(retval, 1);
    }
    return retval;
}

static /*@observer@*/ /*@null@*/ yasm_section *
dbg_objfmt_section_switch(yasm_object *object, yasm_valparamhead *valparams,
			  /*@unused@*/ /*@null@*/
			  yasm_valparamhead *objext_valparams,
			  unsigned long line)
{
    yasm_objfmt_dbg *objfmt_dbg = (yasm_objfmt_dbg *)object->objfmt;
    yasm_valparam *vp;
    yasm_section *retval;
    int isnew;

    fprintf(objfmt_dbg->dbgfile, "section_switch(headp, ");
    yasm_vps_print(valparams, objfmt_dbg->dbgfile);
    fprintf(objfmt_dbg->dbgfile, ", ");
    yasm_vps_print(objext_valparams, objfmt_dbg->dbgfile);
    fprintf(objfmt_dbg->dbgfile, ", %lu), returning ", line);

    if ((vp = yasm_vps_first(valparams)) && !vp->param && vp->val != NULL) {
	retval = yasm_object_get_general(object, vp->val,
	    yasm_expr_create_ident(yasm_expr_int(yasm_intnum_create_uint(200)),
				   line), 0, 0, 0, &isnew, line);
	if (isnew) {
	    fprintf(objfmt_dbg->dbgfile, "(new) ");
	    yasm_symtab_define_label(object->symtab, vp->val,
		yasm_section_bcs_first(retval), 1, line);
	}
	yasm_section_set_default(retval, 0);
	fprintf(objfmt_dbg->dbgfile, "\"%s\" section\n", vp->val);
	return retval;
    } else {
	fprintf(objfmt_dbg->dbgfile, "NULL\n");
	return NULL;
    }
}

static yasm_symrec *
dbg_objfmt_extern_declare(yasm_object *object, const char *name,
			  /*@unused@*/ /*@null@*/
			  yasm_valparamhead *objext_valparams,
			  unsigned long line)
{
    yasm_objfmt_dbg *objfmt_dbg = (yasm_objfmt_dbg *)object->objfmt;
    yasm_symrec *sym;

    fprintf(objfmt_dbg->dbgfile, "extern_declare(\"%s\", ", name);
    yasm_vps_print(objext_valparams, objfmt_dbg->dbgfile);
    fprintf(objfmt_dbg->dbgfile, ", %lu), returning sym\n", line);

    sym = yasm_symtab_declare(object->symtab, name, YASM_SYM_EXTERN, line);
    return sym;
}

static yasm_symrec *
dbg_objfmt_global_declare(yasm_object *object, const char *name,
			  /*@unused@*/ /*@null@*/
			  yasm_valparamhead *objext_valparams,
			  unsigned long line)
{
    yasm_objfmt_dbg *objfmt_dbg = (yasm_objfmt_dbg *)object->objfmt;
    yasm_symrec *sym;

    fprintf(objfmt_dbg->dbgfile, "global_declare(\"%s\", ", name);
    yasm_vps_print(objext_valparams, objfmt_dbg->dbgfile);
    fprintf(objfmt_dbg->dbgfile, ", %lu), returning sym\n", line);

    sym = yasm_symtab_declare(object->symtab, name, YASM_SYM_GLOBAL, line);
    return sym;
}

static yasm_symrec *
dbg_objfmt_common_declare(yasm_object *object, const char *name,
			  /*@only@*/ yasm_expr *size,
			  /*@unused@*/ /*@null@*/
			  yasm_valparamhead *objext_valparams,
			  unsigned long line)
{
    yasm_objfmt_dbg *objfmt_dbg = (yasm_objfmt_dbg *)object->objfmt;
    yasm_symrec *sym;

    assert(objfmt_dbg->dbgfile != NULL);
    fprintf(objfmt_dbg->dbgfile, "common_declare(\"%s\", ", name);
    yasm_expr_print(size, objfmt_dbg->dbgfile);
    fprintf(objfmt_dbg->dbgfile, ", ");
    yasm_vps_print(objext_valparams, objfmt_dbg->dbgfile);
    fprintf(objfmt_dbg->dbgfile, ", %lu), returning sym\n", line);
    yasm_expr_destroy(size);


    sym = yasm_symtab_declare(object->symtab, name, YASM_SYM_COMMON, line);
    return sym;
}

static int
dbg_objfmt_directive(yasm_object *object, const char *name,
		     /*@null@*/ yasm_valparamhead *valparams,
		     /*@null@*/ yasm_valparamhead *objext_valparams,
		     unsigned long line)
{
    yasm_objfmt_dbg *objfmt_dbg = (yasm_objfmt_dbg *)object->objfmt;
    fprintf(objfmt_dbg->dbgfile, "directive(\"%s\", ", name);
    yasm_vps_print(valparams, objfmt_dbg->dbgfile);
    fprintf(objfmt_dbg->dbgfile, ", ");
    yasm_vps_print(objext_valparams, objfmt_dbg->dbgfile);
    fprintf(objfmt_dbg->dbgfile, ", %lu), returning 0 (recognized)\n", line);
    return 0;	    /* dbg format "recognizes" all directives */
}


/* Define valid debug formats to use with this object format */
static const char *dbg_objfmt_dbgfmt_keywords[] = {
    "null",
    NULL
};

/* Define objfmt structure -- see objfmt.h for details */
yasm_objfmt_module yasm_dbg_LTX_objfmt = {
    "Trace of all info passed to object format module",
    "dbg",
    "dbg",
    32,
    dbg_objfmt_dbgfmt_keywords,
    "null",
    dbg_objfmt_create,
    dbg_objfmt_output,
    dbg_objfmt_destroy,
    dbg_objfmt_add_default_section,
    dbg_objfmt_section_switch,
    dbg_objfmt_extern_declare,
    dbg_objfmt_global_declare,
    dbg_objfmt_common_declare,
    dbg_objfmt_directive
};
