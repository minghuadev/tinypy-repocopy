
tp_vm *_tp_init(void) {
    int i;
    tp_vm *tp = tp_malloc(sizeof(tp_vm)); 
    tp->cur = 0;
    tp->jmp = 0;
    tp->ex = None;
    tp->root = tp_list(0);
    for (i=0; i<256; i++) { tp->chars[i][0]=i; }
    tp_gc_init(tp);
    tp->_regs = tp_list(tp);
    for (i=0; i<TP_REGS; i++) { tp_set(tp,tp->_regs,None,None); }
    tp->builtins = tp_dict(tp);
    tp->modules = tp_dict(tp);
    tp->_params = tp_list(tp);
    for (i=0; i<TP_FRAMES; i++) { tp_set(tp,tp->_params,None,tp_list(tp)); }
    tp_set(tp,tp->root,None,tp->builtins);
    tp_set(tp,tp->root,None,tp->modules);
    tp_set(tp,tp->root,None,tp->_regs);
    tp_set(tp,tp->root,None,tp->_params);
    tp_set(tp,tp->builtins,tp_string("MODULES"),tp->modules);
    tp_set(tp,tp->modules,tp_string("BUILTINS"),tp->builtins);
    tp_set(tp,tp->builtins,tp_string("BUILTINS"),tp->builtins);
    tp->regs = tp->_regs.list.val->items;
    tp_full(tp);
    return tp;
}

void tp_deinit(TP) {
    while (tp->root.list.val->len) {
        _tp_list_pop(tp,tp->root.list.val,0,"tp_deinit");
    }
    tp_full(tp); tp_full(tp);
    tp_delete(tp,tp->root);
    tp_gc_deinit(tp);
    tp_free(tp);
}


// tp_frame_ 
void tp_frame(TP,tp_obj globals,tp_code *codes,tp_obj *ret_dest) {
    tp_frame_ f;
    f.globals = globals;
    f.codes = codes;
    f.cur = f.codes;
    f.jmp = 0;
//     fprintf(stderr,"tp->cur: %d\n",tp->cur);
    f.regs = (tp->cur <= 0?tp->regs:tp->frames[tp->cur].regs+tp->frames[tp->cur].cregs);
    f.ret_dest = ret_dest;
    f.lineno = 0;
    f.line = tp_string("");
    f.name = tp_string("?");
    f.fname = tp_string("?");
    f.cregs = 0;
//     return f;
    if (f.regs+256 >= tp->regs+TP_REGS || tp->cur >= TP_FRAMES-1) { tp_raise(,"tp_frame: stack overflow %d",tp->cur); }
    tp->cur += 1;
    tp->frames[tp->cur] = f; 
}

void _tp_raise(TP,tp_obj e) {
    if (!tp || !tp->jmp) {
        printf("\nException:\n%s\n",STR(e));
        exit(-1);
        return;
    }
    if (e.type != TP_NONE) { tp->ex = e; }
    tp_grey(tp,e);
    longjmp(tp->buf,1);
}

void tp_print_stack(TP) {
    printf("\n");
    int i;
    for (i=0; i<=tp->cur; i++) {
        if (!tp->frames[i].lineno) { continue; }
        printf("File \"%s\", line %d, in %s\n  %s\n",
            STR(tp->frames[i].fname),tp->frames[i].lineno,STR(tp->frames[i].name),STR(tp->frames[i].line));
    }
    printf("\nException:\n%s\n",STR(tp->ex));
}



void tp_handle(TP) {
    int i;
    for (i=tp->cur; i>=0; i--) {
        if (tp->frames[i].jmp) { break; }
    }
    if (i >= 0) {
        tp->cur = i;
        tp->frames[i].cur = tp->frames[i].jmp;
        tp->frames[i].jmp = 0;
        return;
    }
    tp_print_stack(tp);
    exit(-1);
}

void _tp_call(TP,tp_obj *dest, tp_obj fnc, tp_obj params) {
    if (fnc.type == TP_DICT) {
        return _tp_call(tp,dest,tp_get(tp,fnc,tp_string("__call__")),params);
    }
    if (fnc.type == TP_FNC && !(fnc.fnc.ftype&1)) {
        *dest = _tp_tcall(tp,fnc);
        tp_grey(tp,*dest);
        return;
    }
    if (fnc.type == TP_FNC) {
        tp_frame(tp,fnc.fnc.val->globals,fnc.fnc.fval,dest);
        if ((fnc.fnc.ftype&2)) {
            tp->frames[tp->cur].regs[0] = params;
            _tp_list_insert(tp,params.list.val,0,fnc.fnc.val->self);
        } else {
            tp->frames[tp->cur].regs[0] = params;
        }
        return;
    }
    tp_params_v(tp,1,fnc); tp_print(tp);
    tp_raise(,"tp_call: %s is not callable",STR(fnc));
}


void tp_return(TP, tp_obj v) {
    tp_obj *dest = tp->frames[tp->cur].ret_dest;
    if (dest) { *dest = v; tp_grey(tp,v); }
//     memset(tp->frames[tp->cur].regs,0,TP_REGS_PER_FRAME*sizeof(tp_obj));
//     fprintf(stderr,"regs:%d\n",(tp->frames[tp->cur].cregs+1));
    memset(tp->frames[tp->cur].regs,0,tp->frames[tp->cur].cregs*sizeof(tp_obj));
    tp->cur -= 1;
}

enum {
    TP_IEOF,TP_IADD,TP_ISUB,TP_IMUL,TP_IDIV,TP_IPOW,TP_IAND,TP_IOR,TP_ICMP,TP_IGET,TP_ISET,
    TP_INUMBER,TP_ISTRING,TP_IGGET,TP_IGSET,TP_IMOVE,TP_IDEF,TP_IPASS,TP_IJUMP,TP_ICALL,
    TP_IRETURN,TP_IIF,TP_IDEBUG,TP_IEQ,TP_ILE,TP_ILT,TP_IDICT,TP_ILIST,TP_INONE,TP_ILEN,
    TP_ILINE,TP_IPARAMS,TP_IIGET,TP_IFILE,TP_INAME,TP_INE,TP_IHAS,TP_IRAISE,TP_ISETJMP,
    TP_IMOD,TP_ILSH,TP_IRSH,TP_IITER,TP_IDEL,TP_IREGS,
    TP_ITOTAL
};

// char *tp_strings[TP_ITOTAL] = {
//     "EOF","ADD","SUB","MUL","DIV","POW","AND","OR","CMP","GET","SET","NUM",
//     "STR","GGET","GSET","MOVE","DEF","PASS","JUMP","CALL","RETURN","IF","DEBUG",
//     "EQ","LE","LT","DICT","LIST","NONE","LEN","LINE","PARAMS","IGET","FILE",
//     "NAME","NE","HAS","RAISE","SETJMP","MOD","LSH","RSH","ITER","DEL","REGS",
// };

#define VA ((int)e.regs.a)
#define VB ((int)e.regs.b)
#define VC ((int)e.regs.c)
#define RA regs[e.regs.a]
#define RB regs[e.regs.b]
#define RC regs[e.regs.c]
#define UVBC (unsigned short)(((VB<<8)+VC))
#define SVBC (short)(((VB<<8)+VC))
#define GA tp_grey(tp,RA)
#define SR(v) f->cur = cur; return(v);

int tp_step(TP) {
    tp_frame_ *f = &tp->frames[tp->cur];
    tp_obj *regs = f->regs;
    tp_code *cur = f->cur;
    while(1) {
    tp_code e = *cur;
//     fprintf(stderr,"%2d.%4d: %-6s %3d %3d %3d\n",tp->cur,cur-f->codes,tp_strings[e.i],VA,VB,VC);
//     int i; for(i=0;i<16;i++) { fprintf(stderr,"%d: %s\n",i,STR(regs[i])); }
    switch (e.i) {
        case TP_IEOF: tp_return(tp,None); SR(0); break;
        case TP_IADD: RA = tp_add(tp,RB,RC); break;
        case TP_ISUB: RA = tp_sub(tp,RB,RC); break;
        case TP_IMUL: RA = tp_mul(tp,RB,RC); break;
        case TP_IDIV: RA = tp_div(tp,RB,RC); break;
        case TP_IPOW: RA = tp_pow(tp,RB,RC); break;
        case TP_IAND: RA = tp_and(tp,RB,RC); break;
        case TP_IOR:  RA = tp_or(tp,RB,RC); break;
        case TP_IMOD:  RA = tp_mod(tp,RB,RC); break;
        case TP_ILSH:  RA = tp_lsh(tp,RB,RC); break;
        case TP_IRSH:  RA = tp_rsh(tp,RB,RC); break;
        case TP_ICMP: RA = tp_number(tp_cmp(tp,RB,RC)); break;
        case TP_INE: RA = tp_number(tp_cmp(tp,RB,RC)!=0); break;
        case TP_IEQ: RA = tp_number(tp_cmp(tp,RB,RC)==0); break;
        case TP_ILE: RA = tp_number(tp_cmp(tp,RB,RC)<=0); break;
        case TP_ILT: RA = tp_number(tp_cmp(tp,RB,RC)<0); break;
        case TP_IPASS: break;
        case TP_IIF: if (tp_bool(tp,RA)) { cur += 1; } break;
        case TP_IGET: RA = tp_get(tp,RB,RC); GA; break;
        case TP_IITER:
            if (RC.number.val < tp_len(tp,RB).number.val) {
                RA = tp_iter(tp,RB,RC); GA;
                RC.number.val += 1;
                cur += 1;
            }
            break;
        case TP_IHAS: RA = tp_has(tp,RB,RC); break;
        case TP_IIGET: tp_iget(tp,&RA,RB,RC); break;
        case TP_ISET: tp_set(tp,RA,RB,RC); break;
        case TP_IDEL: tp_del(tp,RA,RB); break;
        case TP_IMOVE: RA = RB; break;
        case TP_INUMBER:
            RA = tp_number(*(tp_num*)(*++cur).string.val);
            cur += sizeof(tp_num)/4;
            continue;
        case TP_ISTRING:
            RA = tp_string_n((*(cur+1)).string.val,UVBC);
            cur += (UVBC/4)+1;
            break;
        case TP_IDICT: RA = tp_dict_n(tp,VC/2,&RB); break;
        case TP_ILIST: RA = tp_list_n(tp,VC,&RB); break;
        case TP_IPARAMS: RA = tp_params_n(tp,VC,&RB); break;
        case TP_ILEN: RA = tp_len(tp,RB); break;
        case TP_IJUMP: cur += SVBC; continue; break;
        case TP_ISETJMP: f->jmp = cur+SVBC; break;
        case TP_ICALL: _tp_call(tp,&RA,RB,RC); cur++; SR(0); break;
        case TP_IGGET:
            if (!tp_iget(tp,&RA,f->globals,RB)) {
                RA = tp_get(tp,tp->builtins,RB); GA;
            }
            break;
        case TP_IGSET: tp_set(tp,f->globals,RA,RB); break;
        case TP_IDEF:
            RA = tp_def(tp,(*(cur+1)).string.val,f->globals);
            cur += SVBC; continue;
            break;
        case TP_IRETURN: tp_return(tp,RA); SR(0); break;
        case TP_IRAISE: _tp_raise(tp,RA); SR(0); break;
        case TP_IDEBUG:
            tp_params_v(tp,3,tp_string("DEBUG:"),tp_number(VA),RA); tp_print(tp);
            break;
        case TP_INONE: RA = None; break;
        case TP_ILINE:
            f->line = tp_string_n((*(cur+1)).string.val,VA*4-1);
//             fprintf(stderr,"%7d: %s\n",UVBC,f->line.string.val);
            cur += VA; f->lineno = UVBC;
            break;
        case TP_IFILE: f->fname = RA; break;
        case TP_INAME: f->name = RA; break;
        case TP_IREGS: f->cregs = VA; break;
        default: tp_raise(0,"tp_step: invalid instruction %d",e.i); break;
    }
    cur += 1;
    }
    SR(0);
}

void tp_run(TP,int cur) {
    if (tp->jmp) { tp_raise(,"tp_run(%d) called recusively",cur); }
    tp->jmp = 1; if (setjmp(tp->buf)) { tp_handle(tp); }
    while (tp->cur >= cur && tp_step(tp) != -1);
    tp->cur = cur-1; tp->jmp = 0;
}


tp_obj tp_call(TP, char *mod, char *fnc, tp_obj params) {
    tp_obj tmp;
    tp_obj r = None;
    tmp = tp_get(tp,tp->modules,tp_string(mod));
    tmp = tp_get(tp,tmp,tp_string(fnc));
    _tp_call(tp,&r,tmp,params);
    tp_run(tp,tp->cur);
    return r;
}

tp_obj tp_import(TP,char *fname, char *name, void *codes) {
    if (!((fname && strstr(fname,".tpc")) || codes)) {
        return tp_call(tp,"py2bc","import_fname",tp_params_v(tp,2,tp_string(fname),tp_string(name)));
    }

    tp_obj code = None;
    if (!codes) {
        tp_params_v(tp,1,tp_string(fname));
        code = tp_load(tp);
        codes = code.string.val;
    } else {
        code = tp_data(tp,codes);
    }

    tp_obj g = tp_dict(tp);
    tp_set(tp,g,tp_string("__name__"),tp_string(name));
    tp_set(tp,g,tp_string("__code__"),code); 
    tp_frame(tp,g,codes,0);
    tp_set(tp,tp->modules,tp_string(name),g);
    
    if (!tp->jmp) { tp_run(tp,tp->cur); }
    
    return g;
}



tp_obj tp_exec_(TP) {
    tp_obj code = TP_OBJ();
    tp_obj globals = TP_OBJ();
    tp_frame(tp,globals,(void*)code.string.val,0);
    return None;
}


tp_obj tp_import_(TP) {
    tp_obj mod = TP_OBJ();

    if (tp_has(tp,tp->modules,mod).number.val) {
        return tp_get(tp,tp->modules,mod);
    }

    char *s = STR(mod);
    tp_obj r = tp_import(tp,STR(tp_add(tp,mod,tp_string(".tpc"))),s,0);
    return r;
}

#define TP_IB(a,b) tp_set(tp,ctx,tp_string(a),tp_fnc(tp,b))

tp_obj tp_builtins(TP) {
    tp_obj ctx = tp->builtins;
    TP_IB("print",tp_print); TP_IB("range",tp_range); TP_IB("min",tp_min);
    TP_IB("max",tp_max); TP_IB("bind",tp_bind); TP_IB("copy",tp_copy);
    TP_IB("import",tp_import_); TP_IB("len",tp_len_); TP_IB("assert",tp_assert);
    TP_IB("str",tp_str2); TP_IB("float",tp_float); TP_IB("system",tp_system);
    TP_IB("istype",tp_istype); TP_IB("chr",tp_chr); TP_IB("save",tp_save);
    TP_IB("load",tp_load); TP_IB("fpack",tp_fpack); TP_IB("abs",tp_abs);
    TP_IB("int",tp_int); TP_IB("exec",tp_exec_); TP_IB("exists",tp_exists);
    TP_IB("mtime",tp_mtime); TP_IB("number",tp_float); TP_IB("round",tp_round);
    TP_IB("ord",tp_ord); TP_IB("merge",tp_merge);
    return ctx;
}


void tp_args(TP,int argc, char *argv[]) {
    tp_obj self = tp_list(tp);
    int i;
    for (i=1; i<argc; i++) { _tp_list_append(tp,self.list.val,tp_string(argv[i])); }
    tp_set(tp,tp->builtins,tp_string("ARGV"),self);
}


tp_obj tp_main(TP,char *fname, void *code) {
    return tp_import(tp,fname,"__main__",code);
}
tp_obj tp_compile(TP, tp_obj text, tp_obj fname) {
    return tp_call(tp,"BUILTINS","compile",tp_params_v(tp,2,text,fname));
}

tp_obj tp_exec(TP,tp_obj code, tp_obj globals) {
    tp_obj r=None;
    tp_frame(tp,globals,(void*)code.string.val,&r);
    tp_run(tp,tp->cur);
    return r;
}

tp_obj tp_eval(TP, char *text, tp_obj globals) {
    tp_obj code = tp_compile(tp,tp_string(text),tp_string("<eval>"));
    return tp_exec(tp,code,globals);
}

tp_vm *tp_init(int argc, char *argv[]) {
    tp_vm *tp = _tp_init();
    tp_builtins(tp);
    tp_args(tp,argc,argv);
    tp_compiler(tp);
    return tp;
}
    

//
