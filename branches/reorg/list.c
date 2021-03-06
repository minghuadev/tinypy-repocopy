void _tp_list_realloc(_tp_list *self,int len) {
    if (!len) { len=1; }
    self->items = tp_realloc(self->items,len*sizeof(tp_obj));
    self->alloc = len;
}

void _tp_list_set(TP,_tp_list *self,int k, tp_obj v, char *error) {
    if (k >= self->len) { tp_raise(,"%s: KeyError: %d\n",error,k); }
    self->items[k] = v;
    tp_grey(tp,v);
}
void _tp_list_free(_tp_list *self) {
    tp_free(self->items);
    tp_free(self);
}

tp_obj _tp_list_get(TP,_tp_list *self,int k,char *error) {
    if (k >= self->len) { tp_raise(None,"%s: KeyError: %d\n",error,k); }
    return self->items[k];
}
void _tp_list_insertx(TP,_tp_list *self, int n, tp_obj v) {
    if (self->len >= self->alloc) {
        _tp_list_realloc(self,self->alloc*2);
    }
    if (n < self->len) { memmove(&self->items[n+1],&self->items[n],sizeof(tp_obj)*(self->len-n)); }
    self->items[n] = v;
    self->len += 1;
}
void _tp_list_appendx(TP,_tp_list *self, tp_obj v) {
    _tp_list_insertx(tp,self,self->len,v);
}
void _tp_list_insert(TP,_tp_list *self, int n, tp_obj v) {
    _tp_list_insertx(tp,self,n,v);
    tp_grey(tp,v);
}
void _tp_list_append(TP,_tp_list *self, tp_obj v) {
    _tp_list_insert(tp,self,self->len,v);
}
tp_obj _tp_list_pop(TP,_tp_list *self, int n, char *error) {
    tp_obj r = _tp_list_get(tp,self,n,error);
    if (n != self->len-1) { memmove(&self->items[n],&self->items[n+1],sizeof(tp_obj)*(self->len-(n+1))); }
    self->len -= 1;
    return r;
}

int _tp_list_find(TP,_tp_list *self, tp_obj v) {
    int n;
    for (n=0; n<self->len; n++) {
        if (tp_cmp(tp,v,self->items[n]) == 0) {
            return n;
        }
    }
    return -1;
}

tp_obj tp_index(TP) {
    tp_obj self = TP_OBJ();
    tp_obj v = TP_OBJ();
    int i = _tp_list_find(tp,self.list.val,v);
    if (i < 0) { tp_raise(None,"tp_index(%s,%s) - item not found",STR(self),STR(v)); }
    return tp_number(i);
}

_tp_list *_tp_list_new(void) {
    return tp_malloc(sizeof(_tp_list));
}

tp_obj _tp_list_copy(TP, tp_obj rr) {
    _tp_list *o = rr.list.val;
    _tp_list *r = _tp_list_new(); *r = *o;
    r->items = tp_malloc(sizeof(tp_obj)*o->alloc);
    memcpy(r->items,o->items,sizeof(tp_obj)*o->alloc);
    return tp_track(tp,(tp_obj)(tp_list_){TP_LIST,r});
}

tp_obj tp_append(TP) {
    tp_obj self = TP_OBJ();
    tp_obj v = TP_OBJ();
    _tp_list_append(tp,self.list.val,v);
    return None;
}

tp_obj tp_pop(TP) {
    tp_obj self = TP_OBJ();
    return _tp_list_pop(tp,self.list.val,self.list.val->len-1,"pop");
}

tp_obj tp_insert(TP) {
    tp_obj self = TP_OBJ();
    int n = TP_NUM();
    tp_obj v = TP_OBJ();
    _tp_list_insert(tp,self.list.val,n,v);
    return None;
}

tp_obj tp_extend(TP) {
    tp_obj self = TP_OBJ();
    tp_obj v = TP_OBJ();
    int i;
    for (i=0; i<v.list.val->len; i++) {
        _tp_list_append(tp,self.list.val,v.list.val->items[i]);
    }
    return None;
}

tp_obj tp_list(TP) {
    tp_obj r = (tp_obj)(tp_list_){TP_LIST,_tp_list_new()};
    return tp?tp_track(tp,r):r;
}

tp_obj tp_list_n(TP,int n,tp_obj *argv) {
    tp_obj r = tp_list(tp); _tp_list_realloc(r.list.val,n);
    int i; for (i=0; i<n; i++) { _tp_list_append(tp,r.list.val,argv[i]); }
    return r;
}

int _tp_sort_cmp(tp_obj *a,tp_obj *b) {
    return tp_cmp(0,*a,*b);
}
tp_obj tp_sort(TP) {
    tp_obj self = TP_OBJ();
    qsort(self.list.val->items, self.list.val->len, sizeof(tp_obj), (int(*)(const void*,const void*))_tp_sort_cmp);
    return None;
}



//


