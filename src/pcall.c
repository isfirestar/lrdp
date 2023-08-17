#include "lobj.h"

#include "zmalloc.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#pragma pack(push, 8)
union va_type_comb {
    int i;
    unsigned int ui;
    long l;
    unsigned long ul;
    long long ll;
    unsigned long long ull;
    double d;
    long double ld;
    void *p;
};
#pragma pack(pop)

#define VA_TYPE_COMB_INDEX_INT 5
#define VA_TYPE_COMB_INDEX_UINT 6
#define VA_TYPE_COMB_INDEX_LONG 7
#define VA_TYPE_COMB_INDEX_ULONG 8
#define VA_TYPE_COMB_INDEX_LONGLONG 9
#define VA_TYPE_COMB_INDEX_ULONGLONG 10
#define VA_TYPE_COMB_INDEX_DOUBLE 12
#define VA_TYPE_COMB_INDEX_LONGDOUBLE 13
#define VA_TYPE_COMB_INDEX_VOID 14
#define VA_TYPE_COMB_INDEX_VOIDP 15

// #define SWITCH_TO_COMB_TYPE(idx, comb)  
//     ((VA_TYPE_COMB_INDEX_INT == (idx)) ? (comb).i : 
//     ((VA_TYPE_COMB_INDEX_UINT == (idx)) ? (comb).ui : 
//     (((VA_TYPE_COMB_INDEX_LONG == (idx)) ? (comb).l : 
//     (((VA_TYPE_COMB_INDEX_ULONG == (idx)) ? (comb).ul : 
//     (((VA_TYPE_COMB_INDEX_LONGLONG == (idx)) ? (comb).ll : 
//     (((VA_TYPE_COMB_INDEX_ULONGLONG == (idx)) ? (comb).ull : 
//     (comb).p))))))))))

static const struct stdc_type_dict {
    char *type;
    int size;
    int isfloat;
    int ispointer;
    int va_type_comb_index;
} __ctype_dict[] = {
    {"char", sizeof(char), 0, 0, VA_TYPE_COMB_INDEX_INT},
    {"unsigned char", sizeof(unsigned char), 0, 0, VA_TYPE_COMB_INDEX_INT},
    {"short", sizeof(short), 0, 0, VA_TYPE_COMB_INDEX_INT},
    {"unsigned short", sizeof(unsigned short), 0, 0, VA_TYPE_COMB_INDEX_INT},
    {"int", sizeof(int), 0, 0, VA_TYPE_COMB_INDEX_INT},
    {"unsigned int", sizeof(unsigned int), 0, 0, VA_TYPE_COMB_INDEX_UINT},
    {"long", sizeof(long), 0, 0, VA_TYPE_COMB_INDEX_LONG},
    {"unsigned long", sizeof(unsigned long), 0, 0, VA_TYPE_COMB_INDEX_ULONG},
    {"long long", sizeof(long long), 0, 0, VA_TYPE_COMB_INDEX_LONGLONG},
    {"unsigned long long", sizeof(unsigned long long), 0, 0, VA_TYPE_COMB_INDEX_ULONGLONG},
    {"float", sizeof(float), 1, 0, VA_TYPE_COMB_INDEX_DOUBLE},
    {"double", sizeof(double), 1, 0, VA_TYPE_COMB_INDEX_DOUBLE},
    {"long double", sizeof(long double), 1, 0, VA_TYPE_COMB_INDEX_LONGDOUBLE},
    {"void", 0, 0, 0, VA_TYPE_COMB_INDEX_VOID},
    {"char *", sizeof(char *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"unsigned char *", sizeof(char *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"void *", sizeof(void *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"int *", sizeof(int *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"unsigned int *", sizeof(unsigned int *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"long *", sizeof(long *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"unsigned long *", sizeof(unsigned long *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"long long *", sizeof(long long *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"unsigned long long *", sizeof(unsigned long long *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"float *", sizeof(float *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"double *", sizeof(double *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {"long double *", sizeof(long double *), 0, 1, VA_TYPE_COMB_INDEX_VOIDP},
    {NULL, 0, 0, 0}
};

static const struct stdc_type_dict *__pcall_legal_ctype(const char *typestring)
{
    int i;

    i = 0;
    while (__ctype_dict[i].type) {
        if (strcmp(typestring, __ctype_dict[i].type) == 0) {
            return &__ctype_dict[i];
        }
        i++;
    }
    return NULL;
}

static char *__pcall_trim_head(char *str)
{
    char *cursor;

    if (!str) {
        return NULL;
    }

    cursor = str;
    while (*cursor == ' ') {
        cursor++;
    }

    return cursor;
}

static const struct stdc_type_dict *__pcall_get_returntype(char *prototype, char **next)
{
    char *head, *cursor, *tail;
    const struct stdc_type_dict *type_item;

    cursor = __pcall_trim_head(prototype);
    if (!cursor) {
        return NULL;
    }

    do {
        head = cursor;
        tail = strchr(cursor, '(');
        if (!tail) {
            break;
        }
        *tail = '\0';
        cursor = tail + 1;
        tail--;

        // trimed the space in the tail of the return type
        while (*tail == ' ' && tail > head) {
            *tail = '\0';
            tail--;
        }
        // illegal return type
        if (tail == head) {
            break;
        }
        type_item = __pcall_legal_ctype(head);
        if (!type_item) {
            break;
        }

        *next = cursor;
        return type_item;
    } while (0);
    
   return NULL;
}

static char *__pcall_get_funcname(char *cursor, char **next)
{
    char *head, *tail;

    if (!cursor) {
        return NULL;
    }

    head = strchr(cursor, '*');
    if (!head) {
        return NULL;
    }
    head++;
    head = __pcall_trim_head(head);

    tail = strchr(head, ')');
    if (!tail) {
        return NULL;
    }
    *next = tail + 1;
    *tail = '\0';
    return head;
}

static int __pcall_iterate_parameter(va_list ap, char *cursor, char **next, union va_type_comb *comb)
{
    char *head, *tail, *begin;
    const struct stdc_type_dict *type_item;

    if (!cursor) {
        return -1;
    }

    if (!cursor[0]) {
        return 0;
    }

    head = __pcall_trim_head(cursor);
    if (!head) {
        return -1;
    }

    // the begin symbol of paramter table
    begin = strchr(head, '(');
    if (begin) {
        head = begin + 1;
    }

    tail = strchr(head, ',');
    if (!tail) {
        tail = strchr(head, ')');
    }
    if (!tail) {
        return -1;
    }

    *next = tail + 1;
    *tail = '\0';
    type_item = __pcall_legal_ctype(head);
    if (!type_item) {
        return -1;
    }

    switch (type_item->va_type_comb_index) {
        case VA_TYPE_COMB_INDEX_INT:
            comb->i = va_arg(ap, int);
            break;
        case VA_TYPE_COMB_INDEX_UINT:
            comb->ui = va_arg(ap, unsigned int);
            break;
        case VA_TYPE_COMB_INDEX_LONG:
            comb->l = va_arg(ap, long);
            break;
        case VA_TYPE_COMB_INDEX_ULONG:
            comb->ul = va_arg(ap, unsigned long);
            break;
        case VA_TYPE_COMB_INDEX_LONGLONG:
            comb->ll = va_arg(ap, long long);
            break;
        case VA_TYPE_COMB_INDEX_ULONGLONG:
            comb->ull = va_arg(ap, unsigned long long);
            break;
        case VA_TYPE_COMB_INDEX_DOUBLE:
            comb->d = va_arg(ap, double);
            break;
        case VA_TYPE_COMB_INDEX_LONGDOUBLE:
            comb->ld = va_arg(ap, long double);
            break;
        case VA_TYPE_COMB_INDEX_VOIDP:
            comb->p = va_arg(ap, void *);
            break;
        default:
            return -1;
    }
    return type_item->va_type_comb_index;
}

/* this function @lop_portable_call use portable way to invoke a specific function
 * parameter @prototype: the prototype of the function to be invoked
 *
 * below text are describe the prototype format:
 *  the prototype format is like the function pointer type/variable definitionin standard C language, for example: int (*foo)(int, char *)
 *  It can be summarized as : [return_type] (*[function_name])([parameter_type1], [parameter_type2], ...)
 * 
 * the 1st step is to parse the prototype string, and get the return type, function name, parameter type list
 * the 2nd step is to get the function pointer by the function name
 * the 3rd step is to invoke the function pointer with the parameter list, use va_list to get the parameter list from input parameter
 */
int lobj_portable_call(lobj_pt lop, const char *prototype, void *returnptr, ...) 
{
    char *next, *cursor, *dup, *func_name;
    va_list ap;
    long (*pfn)();
    union va_type_comb combs[10];
    int par_count, i, idx[10], pcallret;
    const struct stdc_type_dict *ctype_return_item;
    long retl;
 
    if (!prototype) {
        return -1;
    }

    dup = (char *)ztrymalloc(strlen(prototype) + 1);
    if (!dup) {
        return -1;
    }
    strcpy(dup, prototype);

    pcallret = -1;
    do {
        cursor = dup;

        // get the return type
        ctype_return_item = __pcall_get_returntype(cursor, &next);
        if (!ctype_return_item) {
            break;
        }
        cursor = next;

        // get the function name and load the function pointer
        func_name = __pcall_get_funcname(cursor, &next);
        if (!func_name) {
            break;
        }
        cursor = next;
        pfn = (long (*)())lobj_dlsym(lop, func_name);
        if (!pfn) {
            break;
        }

        // traverse the parameter list and get the parameter value
        par_count = 0;
        i = 0;
        va_start(ap, returnptr);
        while ((idx[i] = __pcall_iterate_parameter(ap, cursor, &next, &combs[i])) > 0) {
            cursor = next;
            i++;
            par_count++;
        }
        va_end(ap);

        pcallret = 0;
        switch (par_count) {
            case 0:
                retl = pfn();
                break;
            case 1:
                retl = pfn(combs[0].ll);
                break;
            case 2:
                retl = pfn(combs[0].ll, combs[1].ll);
                break;
            case 3:
                retl = pfn(combs[0].ll, combs[1].ll, combs[2].ll);
                break;
            case 4:
                retl = pfn(combs[0].ll, combs[1].ll, combs[2].ll, combs[3].ll);
                break;
            case 5:
                retl = pfn(combs[0].ll, combs[1].ll, combs[2].ll, combs[3].ll, combs[4].ll);
                break;
            case 6:
                retl = pfn(combs[0].ll, combs[1].ll, combs[2].ll, combs[3].ll, combs[4].ll, combs[5].ll);
                break;
            default:
                pcallret = -1;
                break;
        }

        if (0 == pcallret && ctype_return_item->size > 0 && returnptr) {
            if (ctype_return_item->ispointer) {
                memcpy(returnptr, (const void *)retl, ctype_return_item->size);
            } else {
                memcpy(returnptr, &retl, ctype_return_item->size);
            }
        }
        
    } while (0);
    
    zfree(dup);
    return pcallret;
}

