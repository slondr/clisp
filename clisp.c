#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define debug(m,e) printf("%s:%d: %s",__FILE__,__LINE__,m); print_obj(e,1); puts("");

typedef struct List {
  struct List *next;
  void *data;
} List;

//global head of the symbols list
List *symbols = 0;

// \look ahead character
static int look;
//array of symbols
#define SYMBOL_MAX 32
static char token[SYMBOL_MAX];

//check if char is whitespace
int is_space(char cur){
  return cur == ' ' || cur == '\n' || cur == '\0';
}

//check if char is parens
int is_parens(char cur) {
  return cur == '(' || cur == ')';
}

//reads characters from stdin and determines what has been discovered
static void get_token() {
  int index = 0;

  while(is_space(look)) {
    look = getchar();
  }

  if(is_parens(look)) {
    token[index++] = look;
    look = getchar();
  } else {
    while(index < SYMBOL_MAX - 1 && look != EOF && !is_space(look) && !is_parens(look)) {
      token[index++] = look;
      look = getchar();
    }
  }

  token[index] = '\0';
}

//macro check if something is atomic, or another list
#define is_pair(x) (((uintptr_t)x & 0x1) == 0x1)
#define is_atom(x) (((uintptr_t)x & 0x1) == 0x0)
#define untag(x) ((uintptr_t)x & ~0x1)
#define tag(x) ((uintptr_t)x | 0x1)

//car - head of the list
#define car(x) (((List*)untag(x))->data)
//cdr - tail of list
#define cdr(x) (((List*)untag(x))->next)

//true and false defintions 
#define e_true cons(intern("quote"), cons(intern("t"), 0))
#define e_false 0

//cons - constructs a pair
List * cons(void *_car, void *_cdr) {
  List *_pair = calloc(1, sizeof(List));
  _pair->data = _car;
  _pair->next = _cdr;
  return (List*) tag(_pair);
}

//retreive symbol from global list of symbols
//add it if not found 
void *intern(char *sym) {
  List *_pair = symbols;
  
  for(; _pair; _pair = cdr(_pair)) {
    if(strncmp(sym, (char*) car(_pair), 32) == 0) return car(_pair);
  }

  symbols = cons(strdup(sym), symbols);
  return car(symbols);
}

//forward declation
List *get_list();

//checks if first char is ( if so calls get list
//otherwise token is a symbol
void *get_obj() {
  if(token[0] == '(') return get_list();
  return intern(token);
}

//reads next token from input
//if valid add to tokens
List *get_list() {
  List *tmp;
  get_token();
  if (token[0] == ')') return 0;
  tmp = get_obj();
  return cons(tmp, get_list());
}

//prints symbol/list to stdout
void print_obj(List *ob, int list_head) {
  if(!is_pair(ob)) {
    printf("%s", ob ? (char*) ob : "null");
  } else {
    if(list_head) printf("(");

    print_obj(car(ob), 1);

    if(cdr(ob) != 0) {
      printf(" ");
      print_obj(cdr(ob), 0);
    } else printf(")");
  }
}

//primitive lisp functions
List *fcons(List *a) {
  return cons(car(a), car(cdr(a)));
}

List *fcar(List *a) {
  return car(car(a));
}

List *fcdr(List *a) {
  return cdr(car(a));
}

List *feq(List *a) {
  return car(a) == car(cdr(a)) ? e_true : e_false;
}

List *fpair(List *a) {
  return is_pair(car(a)) ? e_true : e_false;
}

List *fatom(List *a) {
  return is_atom(car(a)) ? e_true : e_false;
}

List *fnull(List *a) {
  return car(a) == 0 ? e_true : e_false;
}

List *freadobj(List *a) {
  look = getchar();

  get_token();
  return get_obj();
}

List *fwriteobj(List *a) {
  print_obj(car(a), 1);
  puts("");
  return e_true;
}

//forward declaration
List *eval(List *exp, List *env);


//evaluates a list
//maintains order while evaluating each item
List *eval_list(List *list, List *env) {
  //parallel list with eval'd eles in same order
  List *head = 0, **args = &head;

  for(; list; list = cdr(list)) {
    *args = cons(eval(car(list), env), 0);
    //c is annoying sometimes
    args = &((List*) untag(*args))->next;
  }

  return head;
}

//casts l_fn to a pointer to a func that takes a list and returns a list
//yayyy c
List *apply_primitive(void *l_fn, List *args) {
  return ((List * (*) (List *)) l_fn)  ( args );
}

List *eval(List *exp, List *env) {
  if(is_atom(exp)) {

    for(; env != 0; env = cdr(env)) {
      if(exp == car(car(env))) return car(cdr(car(env)));
    }

    return 0;

  } else if(is_atom(car(exp))) {
    //other cases

    if(car(exp) == intern("quote")) {
      return car(cdr(exp));
    } else if(car(exp) == intern("if")) {

      if(eval(car(cdr(exp)), env) != 0) {
        return eval(car(cdr(cdr(exp))), env);
      } else {
        return eval(car(cdr(exp)), env);
      }

    } else if(car(exp) == intern("lambda")) {
      //needs work
      return exp;
    } else if(car(exp) == intern("apply")) {
      //applies prim fn
      List *args = eval_list(cdr(cdr(exp)), env);
      args = car(args);
      return apply_primitive(eval(car(cdr(exp)), env), args);

    } else {
      //fn call
      List *primfn = eval(car(exp), env);

      if(is_pair(primfn)) {
        //user defined lambda
        return eval(cons(primfn, cdr(exp)), env);
      } else if(primfn) {
        //prim fn
        return apply_primitive(primfn, eval_list(cdr(exp), env));
      }

    }

  } else if(car(car(exp)) == intern("lambda")) {
    //binds names into env for future usage
    List *extenv = env, *names = car(cdr(car(exp))), *vars = cdr(exp);

    for(; names; names = cdr(names), vars = cdr(vars)) {
      extenv = cons(cons(car(names), cons(eval(car(vars), env), 0)), extenv);
    }

    return eval(car(cdr(cdr(car(exp)))), extenv);
  }

  puts("Expression cannot be evaluated.");
  return 0;
}

int main(int argc, char *argv[]) {
  List *env = cons(cons(intern("car"), cons((void *)fcar, 0)),
		   cons(cons(intern("cdr"), cons((void *)fcdr, 0)),
			cons(cons(intern("cons"), cons((void *)fcons, 0)),
			     cons(cons(intern("eq?"), cons((void *)feq, 0)),
				  cons(cons(intern("pair?"), cons((void *)fpair, 0)),
				       cons(cons(intern("symbol?"), cons((void *)fatom, 0)),
					    cons(cons(intern("null?"), cons((void *)fnull, 0)),
						 cons(cons(intern("read"), cons((void *)freadobj, 0)),
						      cons(cons(intern("write"), cons((void *)fwriteobj, 0)),
							   cons(cons(intern("null"), cons(0,0)), 0))))))))));


  while(look != EOF) {
    look = getchar();
    get_token();
    print_obj(eval(get_obj(), env), 1 );
    printf("\n");
  }
  return 0;
}
