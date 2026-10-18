#include "chaon_script.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

enum { CHAON_MAX_LINE = 128, CHAON_MAX_DELAY_MS = 10000 };

static char *trim(char *s)
{
    while (isspace((unsigned char)*s)) ++s;
    size_t len = strlen(s);
    while (len && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    return s;
}

static chaon_parse_result_t parse_type(const char *args, chaon_instruction_t *step)
{
    if (*args++ != '"') return CHAON_PARSE_SYNTAX;
    size_t len = 0;
    bool closed = false;
    while (*args) {
        unsigned char ch = (unsigned char)*args++;
        if (ch == '"') { closed = true; break; }
        if (ch == '\\') {
            ch = (unsigned char)*args++;
            if (ch != '\\' && ch != '"') return CHAON_PARSE_SYNTAX;
        }
        if (ch < 32 || ch > 126 || len >= CHAON_MAX_TEXT) return CHAON_PARSE_BOUNDS;
        step->text[len++] = (char)ch;
    }
    while (*args && isspace((unsigned char)*args)) ++args;
    if (!closed || *args) return CHAON_PARSE_SYNTAX;
    step->text[len] = '\0';
    step->op = CHAON_OP_TYPE;
    return CHAON_PARSE_OK;
}

static chaon_parse_result_t parse_delay(const char *args, chaon_instruction_t *step)
{
    if (!isdigit((unsigned char)*args)) return CHAON_PARSE_SYNTAX;
    uint32_t duration = 0;
    for (; *args; ++args) {
        if (!isdigit((unsigned char)*args)) return CHAON_PARSE_SYNTAX;
        duration = duration * 10U + (uint32_t)(*args - '0');
        if (duration > CHAON_MAX_DELAY_MS) return CHAON_PARSE_BOUNDS;
    }
    if (!duration) return CHAON_PARSE_BOUNDS;
    step->op = CHAON_OP_DELAY;
    step->delay_ms = duration;
    return CHAON_PARSE_OK;
}

static chaon_parse_result_t parse_line(char *s, chaon_instruction_t *step)
{
    if (!strcmp(s, "ENTER")) { step->op = CHAON_OP_ENTER; return CHAON_PARSE_OK; }
    if (!strcmp(s, "TAB")) { step->op = CHAON_OP_TAB; return CHAON_PARSE_OK; }
    if (!strncmp(s, "TYPE", 4) && isspace((unsigned char)s[4]))
        return parse_type(trim(s + 4), step);
    if (!strncmp(s, "DELAY", 5) && isspace((unsigned char)s[5]))
        return parse_delay(trim(s + 5), step);
    return CHAON_PARSE_SYNTAX;
}

chaon_parse_result_t chaon_parse(const char *source, chaon_program_t *out,
                                chaon_parse_error_t *error)
{
    if (error) { error->code = CHAON_PARSE_INVALID_ARGUMENT; error->line = 0; }
    if (!source || !out) return CHAON_PARSE_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    size_t n = 0;
    while (n < CHAON_MAX_SOURCE && source[n]) ++n;
    if (n == CHAON_MAX_SOURCE) {
        if (error) error->code = CHAON_PARSE_BOUNDS;
        return CHAON_PARSE_BOUNDS;
    }
    chaon_program_t p = {0};
    chaon_parse_result_t result = CHAON_PARSE_OK;
    size_t line_num = 0;
    const char *cur = source;
    for (;;) {
        ++line_num;
        const char *end = cur;
        while (*end && *end != '\n') ++end;
        size_t len = (size_t)(end - cur);
        if (len >= CHAON_MAX_LINE) { result = CHAON_PARSE_BOUNDS; break; }
        char line[CHAON_MAX_LINE];
        memcpy(line, cur, len);
        line[len] = '\0';
        char *s = trim(line);
        if (*s && *s != '#') {
            if (p.count >= CHAON_MAX_STEPS) { result = CHAON_PARSE_BOUNDS; break; }
            result = parse_line(s, &p.steps[p.count]);
            if (result != CHAON_PARSE_OK) break;
            ++p.count;
        }
        if (!*end) break;
        cur = end + 1;
    }
    if (result == CHAON_PARSE_OK && !p.count) result = CHAON_PARSE_EMPTY;
    if (error) { error->code = result; error->line = result == CHAON_PARSE_OK ? 0 : line_num; }
    if (result == CHAON_PARSE_OK) *out = p;
    return result;
}
