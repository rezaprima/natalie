#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Regexp_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    if (NAT_TYPE(args[0]) == NAT_VALUE_REGEXP) {
        return regexp(env, args[0]->regexp_str);
    } else {
        NAT_ASSERT_TYPE(args[0], NAT_VALUE_STRING, "String");
        return regexp(env, args[0]->str);
    }
}

NatObject *Regexp_eqeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_REGEXP);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) == NAT_VALUE_REGEXP && strcmp(self->regexp_str, arg->regexp_str) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Regexp_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_REGEXP);
    NatObject *out = string(env, "/");
    string_append(env, out, self->regexp_str);
    string_append_char(env, out, '/');
    return out;
}

NatObject *Regexp_eqtilde(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_REGEXP);
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_STRING, "String");
    NatObject *matchdata = Regexp_match(env, self, argc, args, block);
    if (NAT_TYPE(matchdata) == NAT_VALUE_NIL) {
        return matchdata;
    } else {
        assert(matchdata->matchdata_region->num_regs > 0);
        return integer(env, matchdata->matchdata_region->beg[0]);
    }
}

NatObject *Regexp_match(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_REGEXP);
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_STRING, "String");
    NatObject *str_obj = args[0];
    unsigned char *str = (unsigned char *)str_obj->str;
    int result;
    OnigRegion *region = onig_region_new();
    unsigned char *end = str + strlen((char *)str);
    unsigned char *start = str;
    unsigned char *range = end;
    result = onig_search(self->regexp, str, end, start, range, region, ONIG_OPTION_NONE);
    if (result >= 0) {
        env->caller->match = matchdata(env, region, str_obj);
        return env->caller->match;
    } else if (result == ONIG_MISMATCH) {
        env->caller->match = NULL;
        onig_region_free(region, true);
        return NAT_NIL;
    } else {
        env->caller->match = NULL;
        onig_region_free(region, true);
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result);
        NAT_RAISE(env, "RuntimeError", (char *)s);
    }
}

}
