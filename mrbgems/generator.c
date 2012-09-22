#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mrbconf.h>
 
static int
one (const struct dirent *unused)
{
  return 1;
}

/*
 * Does a directory exist?
 *   yes => TRUE
 *   no => FALSE
 *   fs error => FALSE
 *
 */
static int
directory_exists(char path[4096]) {
  DIR* dir = opendir(path);
  if (dir)
    return TRUE;
  else
    return FALSE;
}

/*
 * Template generator for each GEM
 *
 * Arguments:
 *   before:
 *     String before each GEM template
 *   after:
 *     String after each GEM template
 *   start: 
 *     String at the start of the template
 *   end:
 *     String at the end of the template
 *   skip_if_src_not_exist:
 *     TRUE => skip template for GEMs with SRC directory
 *     FALSE => template for all GEMs
 *
 */
static char
*for_each_gem (char before[1024], char after[1024],
               char start[1024], char end[1024],
               char dir_to_skip[1024])
{
  struct dirent **eps;
  int n;
  char gemname[1024] = "";
  char gemname_path[4096] = "";
  char complete_line[4096] = "";
  char src_path[4096] = "";
  struct stat attribut;

  strcat(complete_line, start);

  n = scandir("./g", &eps, one, alphasort);
  if (n >= 0) {
    int cnt;
    for (cnt = 0; cnt < n; ++cnt) {
      strcpy(gemname, eps[cnt]->d_name);
      strcpy(gemname_path, "./g/");
      strcat(gemname_path, gemname);
      strcpy(src_path, gemname_path);
      strcat(src_path, "/src");

      if (strcmp(gemname, ".") == 0)
        continue;
      if (strcmp(gemname, "..") == 0)
        continue;

      stat(gemname_path, &attribut);
      if (S_ISDIR(attribut.st_mode) == 0) {
        continue;
      }

      if (strcmp(dir_to_skip, "") != 0) {
        strcpy(src_path, gemname_path);
        strcat(src_path, "/");
        strcat(src_path, dir_to_skip);

        if (directory_exists(src_path) != TRUE)
          continue;
      }

      strcat(complete_line, before);
      strcat(complete_line, gemname);
      strcat(complete_line, after);
    }
  }
  else {
    perror("Error while scanning the directory.");
  }

  strcat(complete_line, end);
  return complete_line;
}

/*
 * Gem Makefile Generator
 *
 */
void
make_gem_makefile()
{
  char *gem_check = "";
  int gem_empty;
  int gem_c_empty;
  int gem_ruby_empty;

  puts("CFLAGS := -I. -I../../include -I../../src");
  puts("");
  puts("ifeq ($(OS),Windows_NT)");
  puts("MAKE_FLAGS = --no-print-directory CC=$(CC) LL=$(LL) ALL_CFLAGS='$(ALL_CFLAGS)'");
  puts("else");
  puts("MAKE_FLAGS = --no-print-directory CC='$(CC)' LL='$(LL)' ALL_CFLAGS='$(ALL_CFLAGS)'");
  puts("endif");
  puts("");

  gem_check = for_each_gem("", "", "", "", "");
  if (strcmp(gem_check, "") == 0)
    gem_empty = TRUE;
  else
    gem_empty = FALSE;

  gem_check = for_each_gem("", "", "", "", "src");
  if (strcmp(gem_check, "") == 0)
    gem_c_empty = TRUE;
  else 
    gem_c_empty = FALSE;

  gem_check = for_each_gem("", "", "", "", "mrblib");
  if (strcmp(gem_check, "") == 0)
    gem_ruby_empty = TRUE;
  else 
    gem_ruby_empty = FALSE;

  puts(".PHONY : all");
  if (gem_empty) {
    puts("all :");
    puts("");
  }
  else {
    if (gem_c_empty) {
      puts("all : mrblib_gem.o");
      puts("\t$(AR) rs ../../lib/libmruby.a mrblib_gem.o");
    }
    else if (gem_ruby_empty) {
      puts("all : all_gems");
    }
    else {
      puts("all : all_gems mrblib_gem.o");
      puts("\t$(AR) rs ../../lib/libmruby.a mrblib_gem.o");
    }

    puts("");

    // Rule for building all C extensions of each Gem
    if (!gem_c_empty) {
      puts("all_gems :");
      puts(for_each_gem("\t@$(MAKE) -C ", " $(MAKE_FLAGS)\n", "", "", ""));
      puts("");
    }

    // Rule for building all Ruby Extension of each Gem
    if (!gem_ruby_empty) {
      puts("mrblib_gem.o : mrblib_gem.c");
      puts("");

      puts("mrblib_gem.c : mrblib_gem.ctmp");
      puts("\tcat $< > $@");
      puts("");

      puts("mrblib_gem.ctmp : mrblib_gem.rbtmp");
      puts("\t../../bin/mrbc -Bmrblib_gem_irep -o$@ $<");
      puts("");

      puts("mrblib_gem.rbtmp :");
      puts(for_each_gem(" ", "/mrblib/*.rb", "\tcat", "> mrblib_gem.rbtmp", "mrblib"));
      puts("");
    }
  }

  puts(".PHONY : prepare-test");
  puts("prepare-test : mrbgemtest.ctmp");
  puts("");

  puts("mrbgemtest.ctmp : mrbgemtest.rbtmp");
  puts("\t../../bin/mrbc -Bmrbgemtest_irep -omrbgemtest.ctmp mrbgemtest.rbtmp");
  puts("");

  puts("mrbgemtest.rbtmp :");

  if (!gem_empty)
    puts(for_each_gem(" ", "/test/*.rb ", "\tcat", " > mrbgemtest.rbtmp", ""));
  else
    puts("\t../generator rbtmp > mrbgemtest.rbtmp");
    
  puts("");

  puts(".PHONY : clean");
  puts("clean :");
  puts("\t$(RM) *.c *.d *.rbtmp *.ctmp *.o mrbtest");

  if (!gem_empty)
    puts(for_each_gem("\t@$(MAKE) clean -C ", " $(MAKE_FLAGS)\n", "", "", ""));
}

/*
 * init_gems.c Generator
 *
 */
void
make_init_gems()
{
  char *gem_check = "";
  int gem_empty;
  int gem_c_empty;
  int gem_ruby_empty;

  gem_check = for_each_gem("", "", "", "", "");
  if (strcmp(gem_check, "") == 0)
    gem_empty = TRUE;
  else
    gem_empty = FALSE;

  gem_check = for_each_gem("", "", "", "", "src");
  if (strcmp(gem_check, "") == 0)
    gem_c_empty = TRUE;
  else
    gem_c_empty = FALSE;

  gem_check = for_each_gem("", "", "", "", "mrblib");
  if (strcmp(gem_check, "") == 0)
    gem_ruby_empty = TRUE;
  else
    gem_ruby_empty = FALSE;

  puts("/*");
  puts(" * This file contains a list of all");
  puts(" * initializing methods which are");
  puts(" * necessary to bootstrap all gems.");
  puts(" *");
  puts(" * IMPORTANT:");
  puts(" *   This file was generated!");
  puts(" *   All manual changes will get lost.");
  puts(" */");

  puts("");
  puts("#include \"mruby.h\"");
  puts("#include \"mruby/irep.h\"");
  puts("#include \"mruby/dump.h\"");
  puts("#include \"mruby/string.h\"");
  puts("#include \"mruby/proc.h\"");
  puts("");

  if (!gem_c_empty)
    puts(for_each_gem("void mrb_", "_gem_init(mrb_state*);\n", "", "", "src"));

  if (!gem_ruby_empty)
    puts("extern const char mrblib_gem_irep[];");

  puts("");

  puts("void");
  puts("mrb_init_mrbgems(mrb_state *mrb) {");

  if (!gem_c_empty)
    puts(for_each_gem("  mrb_", "_gem_init(mrb);\n", "", "", "src"));

  if (!gem_ruby_empty) {
    puts("  int n = mrb_read_irep(mrb, mrblib_gem_irep);");
    puts("  mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_top_self(mrb));");
    puts("  if (mrb->exc) {");
    puts("    mrb_p(mrb, mrb_obj_value(mrb->exc));");
    puts("    exit(0);");
    puts("  }");
  }

  puts("}");
}

void
make_rbtmp()
{
  puts("");
}

int
main (int argc, char *argv[])
{
  if (argc == 2) {
    if (strcmp(argv[1], "makefile") == 0)
      make_gem_makefile();
    else if (strcmp(argv[1], "init_gems") == 0)
      make_init_gems();
    else if (strcmp(argv[1], "rbtmp") == 0)
      make_rbtmp();
    else
      return 1;
  }
  else {
    puts("Argument missing! Options: 'makefile', 'init_gems', 'rbtmp'");
    return 1;
  }

  return 0;
}
