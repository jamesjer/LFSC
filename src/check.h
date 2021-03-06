#ifndef SC2_CHECK_H
#define SC2_CHECK_H

#include "expr.h"
#include "trie.h"

#ifdef _MSC_VER
#include <stdio.h>
#include <hash_map>
#else
#include <unordered_map>
#endif

#include <cstddef>
#include <iosfwd>
#include <map>
#include <stack>
#include <string>

// see the help message in main.cpp for explanation
typedef struct args
{
  std::vector<std::string> files;
  bool show_runs;
  bool no_tail_calls;
  bool compile_scc;
  bool compile_scc_debug;
  bool run_scc;
  bool use_nested_app;
  bool compile_lib;
} args;

extern int check_time;

class sccwriter;
class libwriter;

void init();

void check_file(const char *_filename,
                args a,
                sccwriter *scw = NULL,
                libwriter *lw = NULL);

void check_file(std::istream& in,
                const std::string& filename,
                args a,
                sccwriter* scw = NULL,
                libwriter* lw = NULL);

void cleanup();

extern char our_getc_c;

void report_error(const std::string &);

extern int linenum;
extern int colnum;
extern const char *filename;
extern std::istream* curfile;

inline void our_ungetc(char c)
{
  if (our_getc_c != 0) report_error("Internal error: our_ungetc buffer full");
  our_getc_c = c;
  if (c == '\n')
  {
    linenum--;
    colnum = -1;
  }
  else
    colnum--;
}

inline char our_getc()
{
  char c;
  if (our_getc_c > 0)
  {
    c = our_getc_c;
    our_getc_c = 0;
  }
  else
  {
    c = curfile->get();
  }
  switch (c)
  {
    case '\n': linenum++;
#ifdef DEBUG_LINES
      std::cout << "line " << linenum << "." << std::endl;
#endif
      colnum = 1;
      break;
    case std::istream::traits_type::eof(): break;
    default: colnum++;
  }

  return c;
}

// return the next character that is not whitespace
inline char non_ws()
{
  char c;
  while (isspace(c = our_getc()))
    ;
  if (c == ';')
  {
    // comment to end of line
    while ((c = our_getc()) != '\n' && c != std::istream::traits_type::eof())
      ;
    return non_ws();
  }
  return c;
}

inline void eat_char(char expected)
{
  if (non_ws() != expected)
  {
    char tmp[80];
    sprintf(tmp, "Expecting a \'%c\'", expected);
    report_error(tmp);
  }
}

extern int IDBUF_LEN;
extern char idbuf[];

/**
 * Parses an identifier.
 *
 * @param skip_ws If true, skips the whitespace before the identifier. Expects
 * the identifier to start at the current position otherwise.
 * @return Pointer to the buffer holding the identifier string
 */
inline const char *prefix_id(bool skip_ws = true)
{
  int i = 0;
  char c = idbuf[i++] = skip_ws ? non_ws() : our_getc();
  while (!isspace(c) && static_cast<unsigned>(c) < 256 && c != '(' && c != ')'
         && c != ';' && c != char(EOF))
  {
    if (i == IDBUF_LEN) report_error("Identifier is too long");
    idbuf[i++] = c = our_getc();
  }
  if (static_cast<unsigned>(c) >= 256)
  {
    report_error("Extended characters are not allowed identifiers.");
  }
  our_ungetc(c);
  idbuf[i - 1] = 0;
  return idbuf;
}

#ifdef _MSC_VER
typedef std::hash_map<std::string, Expr *> symmap;
typedef std::hash_map<std::string, SymExpr *> symmap2;
#else
typedef std::unordered_map<std::string, Expr *> symmap;
typedef std::unordered_map<std::string, SymExpr *> symmap2;
#endif
extern symmap2 progs;
extern std::vector<Expr *> ascHoles;

extern Trie<std::pair<Expr *, Expr *> > *symbols;

extern std::map<SymExpr *, int> mark_map;

extern std::vector<std::pair<std::string, std::pair<Expr *, Expr *> > >
    local_sym_names;

extern Expr *statMpz;
extern Expr *statMpq;
extern Expr *statType;

/**
 * Given a type, `e`, computes its kind.
 *
 * In particular, this will be `statType` if the kind is that of proper types.
 *
 * Since our system doen't draw a clear distinction between kinding and typing,
 * this function doesn't either. It could also be regarded as a function that
 * computes the type of a term. While values technically have no kind, this
 * function would return their type instead of an error.
 *
 * It is different from "check" in that it computes the kind/type of in-memory
 * terms, not serialized ones.
 *
 * Given these declarations:
 *
 *     (declare bool type)
 *     (declare sort type)
 *     (declare Real sort)
 *     (declare term (! s sort type))
 *
 * Examples of proper types:
 *
 *     bool
 *     sort
 *     (term Real)
 *
 * Example of non-proper types:
 *
 *   * term (it has kind `(! s sort type)`)
 *   * Real (it is not a type, and does not have a kind)
 *          (this function would return `sort`)
 *
 * Bibliography:
 *   * _Advanced Topics in Types and Programming Languages_
 */
Expr* compute_kind(Expr* e);

#endif
