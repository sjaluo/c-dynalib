/*
 * This program tries to generate an appropriate header file for
 * inclusion with cdecl.c or cdecl3.c.
 */

#define main notmain
#include <EXTERN.h>
#include <perl.h>
#undef main
#undef fprintf
#undef fopen
#undef fclose

#include <stdio.h>

#ifdef INCLUDE_ALLOCA
#include <alloca.h>
#endif
#ifdef INCLUDE_MALLOC
#include <malloc.h>
#endif

#ifndef SIGSEGV
#include <signal.h>
#endif

#ifdef VERBOSE
#define debprintf(x) printf x
#else
#define debprintf(x)
#endif

/*          0003709d,0009675a,003acacb,359c46d6,05261433,
            00fca2d6,00016fcf,004aa9c6,04a2cd24 */
I32 a[] = { 225437, 616282, 3853003, 899434198, 86381619,
	    16556758, 94159, 4893126, 77778212 };

int grows_downward;
int reverse = 0;
int stack_reserve = 0; /* cdecl3: private data on top of call stack added later, 
			  cannot not be overwritten with alloca.
			  New gcc ABI places in the 4 ptrs 3 unwritable ptr here, 
			  on 32+64bit */
int stack_align;       /* diff between two alloca's */
int one_by_one = 1;    /* stack_align = 1 pointer */
int arg_align = sizeof(I32); /* size of an I32 arg on the call stack. 4 or 8 */
int do_reverse = 0;
int args_size[2];
int adjust[] = {0, 0};
int do_adjust = 0;      /* write beyond that from the alloca pointer */

int *which;

void handler(int sig) {
  printf("abnormal exit 1\n");
  /*if (!stack_reserve)*/
    exit(1);
}

int test(b0, b1, b2, b3, b4, b5, b6, b7, b8)
     I32 b0, b1, b2, b3, b4, b5, b6, b7, b8;
{
  int i;

  debprintf(("test: do_adjust=%d, stack_reserve=%d\n",
	     do_adjust, stack_reserve)); 
  if (b0 == a[0]
      && b1 == a[1]
      && b2 == a[2]
      && b3 == a[3]
      && b4 == a[4]
      && b5 == a[5]
      && b6 == a[6]
      && b7 == a[7]
      && b8 == a[8]
      ) {
    debprintf(("test ok\n"));
      return 1;
  }
  debprintf(("b0-8: %08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\n", 
	     b0,b1,b2,b3,b4,b5,b6,b7,b8));
  /* cdecl3 tests: */
  /* gcc-3.4 amd64: do_adjust=-24, stack_reserve=3 (3 ptrs) */
  if ((sizeof(int*) == 8) && (a[0] == b6) && (a[2] == b7)) {
    *which = -24;
    arg_align = 8;
#ifdef __GNUC__
    stack_reserve = 3;
#endif
    debprintf(("test arg_align=4->8, adjust=-24, stack_reserve=%d (0-6, 2-8)\n",
	       stack_reserve));
  }
  for (i = 0; i < 9; i++) {
    if (a[i] == b4) {
      if ((i == 0 || a[i-1] == b3) && (i == 8 || a[i+1] == b5)) {
	/* gcc-3.4 x86: do_adjust=-16, stack_reserve=3 (alloca cannot write beyond) */
	*which = (i - 4) * sizeof (I32);
	if (i>0 && b2 != a[i-2]) stack_reserve = i-1;
	debprintf(("test %d,do_adjust=%d,stack_reserve=%d => which=%d\n",
		   i-4, do_adjust, stack_reserve, *which));
	break;
      }
      else if ((i == 0 || a[i-1] == b5) && (i == 8 || a[i+1] == b3)) {
	reverse = 1;
	*which = (i - 4) * sizeof (I32);
	debprintf(("test %d,do_adjust=%d => which=%d,reverse=1\n", 
		   i-4, do_adjust, *which));
	break;
      }
    }
  }
  return 0;
}

int do_one_arg(x)
  char *x;
{
  char *arg;
  int i;
  void *d1,*d2,*d3;
  d1 = d2 = d3 = NULL;

  args_size[0] = sizeof x;
  which = &adjust[0];
  if (one_by_one) {
    for (i = 8; i >= 0; i--) {
      arg = (char *) alloca(sizeof (I32));
      Copy(&a[do_reverse ? 8-i : i], arg, sizeof (I32), char);
    }
  }
  else {
    arg = (char *) alloca(9*arg_align);
    arg += do_adjust;
    /* if (stack_reserve) arg = (void*)arg - stack_reserve; */
    for (i = 0; i < 9; i++) {
      Copy(&a[do_reverse ? 8-i : i], arg, sizeof(I32), char);
      arg += arg_align;
    }
  }
  if (stack_reserve == 3)
    return ((int (*)()) test)(d1,d2,d3);
  else
    return ((int (*)()) test)();
}

int do_three_args(x, y, z)
  int x;
  char *y;
  double z;
{
  char *arg;
  int i;
  void *d1,*d2,*d3;

  args_size[1] = sizeof x + sizeof y + sizeof z;
  which = &adjust[1];
  if (one_by_one) {
    for (i = 8; i >= 0; i--) {
      arg = (char *) alloca(sizeof (I32));
      Copy(&a[do_reverse ? 8-i : i], arg, sizeof (I32), char);
    }
  }
  else {
    arg = (char *) alloca(9*arg_align);
    arg += do_adjust;
    /*if (stack_reserve) arg = (void*)arg - stack_reserve;*/
    for (i = 0; i < 9; i++) {
      Copy(&a[do_reverse ? 8-i : i], arg, sizeof(I32), char);
      arg += arg_align;
    }
  }
  if (stack_reserve == 3)
    return ((int (*)()) test)(d1,d2,d3);
  else
    return ((int (*)()) test)();
}

int main(argc, argv)
  int argc;
  char **argv;
{
  FILE *fp;
  int one_arg, three_args;
  int *p1, *p2;

#ifdef SIGSEGV
  signal(SIGSEGV, handler);
#endif
#ifdef SIGILL
  signal(SIGILL, handler);
#endif
  if (argc > 1) {
    do_adjust = atoi(argv[1]);
  }
  p1 = (int *) alloca(sizeof *p1);
  p2 = (int *) alloca(sizeof *p2); /* p1 - 0x20: stack-align=32 since gcc-4 */
  grows_downward = (p1 - p2 > 0 ? 1 : 0);
  one_by_one = (p1 - p2 == (grows_downward ? 1 : -1)); /* stack-align=4 */
  stack_align = abs((char*)p1 - (char*)p2);

  debprintf(("stack_align=%d,do_adjust=%d,grows_downward=%d,one_by_one=%d\n",
	     stack_align,do_adjust,grows_downward,one_by_one));
  debprintf(("a0-8: %08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\n", 
	     a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]));

  /* compute adjust[0], stack_reserve, reverse */
  one_arg = do_one_arg(NULL);
  do_adjust = adjust[0];
  debprintf(("one_arg=%d,reverse=%d,adjust=[%d,%d]\n",
	     one_arg,reverse,adjust[0],adjust[1]));
  if (!one_arg) {
    if (reverse) {
      do_reverse = reverse ^ (one_by_one ? grows_downward : 0);
      /* try with computed adjust[0] and reverse */
      one_arg = do_one_arg(NULL);
      do_adjust = adjust[0];
      debprintf(("one_arg=%d,do_reverse=%d,adjust=[%d,%d]\n",
		 one_arg,do_reverse,adjust[0],adjust[1]));
    }
    else if (stack_reserve || do_adjust) {
      /* try with computed adjust[0] and stack_reserve */
      one_arg = do_one_arg(NULL);
      debprintf(("one_arg=%d,stack_reserve=%d,adjust=[%d,%d]\n",
		 one_arg,stack_reserve,adjust[0],adjust[1]));
    }
  }

  /* verify and compute adjust[1] for more args */
  three_args = do_three_args(0, NULL, 0.0);
  debprintf(("three_args=%d,adjust=[%d,%d]\n",three_args,adjust[0],adjust[1]));
  /* adjust[1] maybe different? */
  if (! one_arg || ! three_args) {
    if (adjust[0] != 0) {
      do_adjust = adjust[1];
      one_arg = do_one_arg(NULL);
      debprintf(("one_arg=%d,adjust=[%d,%d],do_adjust=%d\n",
		 one_arg,adjust[0],adjust[1],do_adjust));
      three_args = do_three_args(0, NULL, 0.0);
      debprintf(("three_args=%d,adjust=[%d,%d],do_adjust=%d\n",
		 three_args,adjust[0],adjust[1],do_adjust));
    }
  }
  /* try it now with aligned stack */
  if (!one_by_one && (! one_arg || ! three_args)) {
    do_adjust = ((char*)p2 - (char*)p1) / 2;
    debprintf(("try again adjust=%d\n",do_adjust));
    one_arg = do_one_arg(NULL);
    debprintf(("one_arg=%d,adjust=[%d,%d],reverse=%d\n",
	       one_arg,adjust[0],adjust[1],reverse));
    three_args = do_three_args(0, NULL, 0.0);
    debprintf(("three_args=%d,adjust=[%d,%d],reverse=%d\n",
	      three_args,adjust[0],adjust[1],reverse));
  }
  if (one_arg && three_args) {
    fp = fopen("cdecl.h", "w");
    if (fp == NULL) {
      return 1;
    }
    fprintf(fp, "/*\n"
	    " * cdecl.h -- configuration parameters for the cdecl%s calling convention\n"
	    " *\n"
	    " * Generated automatically by %s.\n", 
	    stack_reserve>0 ? "3" : "", argv[0]);
#ifndef __FILE__
#define __FILE__ "testcall.c"
#endif
    fprintf(fp, " * Do not edit this file.  Edit %s and/or Makefile.PL instead.\n",
	    __FILE__);
    fprintf(fp, " */\n\n");
#ifdef INCLUDE_ALLOCA
    fprintf(fp, "#include <alloca.h>\n");
#endif
#ifdef INCLUDE_MALLOC
    fprintf(fp, "#include <malloc.h>\n");
#endif
    fprintf(fp, "#define CDECL_ONE_BY_ONE %d\n",  one_by_one);
    fprintf(fp, "#define CDECL_ADJUST %d\n",      do_adjust);
    fprintf(fp, "#define CDECL_ARG_ALIGN %d\n",   arg_align);
    fprintf(fp, "#define CDECL_STACK_RESERVE %d\n", stack_reserve);
    fprintf(fp, "#define CDECL_REVERSE %d\n",     do_reverse);
    fclose(fp);
    debprintf(("cdecl%s ok\n", stack_reserve>0 ? "3" : ""));
    return 0;
  }
  debprintf(("cdecl failed\n"));
  return 1;
}
