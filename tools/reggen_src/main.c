/*
 * This file is part of RegGen -- register definition generator
 * Copyright (C) 2025 Aidan MacDonald
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "parse.h"
#include "validate.h"
#include "generate.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>

enum opt_type
{
    OPTTYPE_FLAG,
    OPTTYPE_STRING,
    OPTTYPE_INT,
    OPTTYPE_POSARG,
};

#define OPT_FOUND      1
#define OPT_NOT_FOUND  0
#define OPT_ERROR      (-1)

int opt_get(int *argc_p, char ***argv_p,
            struct context *ctx,
            enum opt_type opttype,
            const char *shortopt,
            const char *longopt,
            void *output)
{
    int argc = *argc_p;
    char **argv = *argv_p;
    bool force_posarg = false;

    for (int i = 1; i < argc; ++i)
    {
        char *opt_val = NULL;
        int num_args = 1;

        /*
         * Accept arguments of the form "--opt arg", "--opt=arg", or "-o arg".
         * Positional arguments can be interleaved with options. A positional
         * argument starting with a dash is only allowed after "--" indicating
         * the end of options.
         */
        if (!strcmp(argv[i], "--"))
        {
            force_posarg = true;
            continue;
        }

        if (opttype == OPTTYPE_POSARG)
        {
            if (!force_posarg && argv[i][0] == '-')
                continue;

            opt_val = argv[i];
        }
        else if (!force_posarg && longopt && strstr(argv[i], longopt) == argv[i])
        {
            size_t n = strlen(longopt);
            if (opttype != OPTTYPE_FLAG && argv[i][n] == '=')
                opt_val = &argv[i][n+1];
            else if (argv[i][n] != '\0')
                continue;
        }
        else if (!force_posarg && shortopt && strcmp(argv[i], shortopt))
        {
            continue;
        }

        if (opttype != OPTTYPE_FLAG && opt_val == NULL)
        {
            if (i + 1 >= argc)
            {
                context_error(ctx, NULL, "missing argument for option %s", argv[i]);
                return OPT_ERROR;
            }

            opt_val = argv[i + 1];
            num_args++;
        }

        /* Return output value for option */
        char *endptr = NULL;
        switch (opttype)
        {
        case OPTTYPE_FLAG:
            *(bool *)output = true;
            break;

        case OPTTYPE_INT:
            *(long *)output = strtol(opt_val, &endptr, 0);
            if (*opt_val == '\0' || *endptr != '\0')
            {
                context_error(ctx, NULL, "invalid argument for option %s: %s", argv[i], opt_val);
                return OPT_ERROR;
            }

            break;

        case OPTTYPE_STRING:
        case OPTTYPE_POSARG:
            *(char **)output = opt_val;
            break;

        default:
            return OPT_NOT_FOUND;
        }

        /* Remove the parsed option from the argv array */
        memmove(&argv[i], &argv[i + num_args], (argc - i - num_args) * sizeof(*argv));
        *argc_p -= num_args;
        return OPT_FOUND;
    }

    return OPT_NOT_FOUND;
}

/* Initial context with default configuration */
struct context g_ctx = {
    .config = {
        .machine_word_width                 = WIDTH_32,
        .address_word_width                 = WIDTH_UNSPECIFIED,
        .include_guard_prefix               = "INCLUDE",
        .reggen_header_name                 = NULL,
        .implicit_type_prefix               = "",
        .register_iotype_macro_prefix       = "RTYPE_",
        .field_pos_macro_prefix             = "BP_",
        .field_mask_macro_prefix            = "BM_",
        .field_enum_macro_prefix            = "BV_",
        .field_value_macro_prefix           = "BF_",
        .field_valmask_macro_prefix         = "BFM_",
        .instance_address_macro_prefix      = "ITA_",
        .instance_offset_macro_prefix       = "ITO_",
        .instance_name_address_macro_prefix = "ITNA_",
        .instance_name_offset_macro_prefix  = "ITNO_",
        .instance_type_address_macro_prefix = "ITTA_",
        .instance_type_offset_macro_prefix  = "ITTO_",
        .output_directory                   = NULL,
    },
};

int handle_word_width_option(int width, enum word_width *dest)
{
    switch (width)
    {
    case 8:  *dest = WIDTH_8;  break;
    case 16: *dest = WIDTH_16; break;
    case 32: *dest = WIDTH_32; break;
    case 64: *dest = WIDTH_64; break;
    default: return -1;
    }

    return 0;
}

int get_context_options(struct context *ctx, int *argcp, char ***argvp)
{
    int width;
    int rc = opt_get(argcp, argvp, ctx, OPTTYPE_INT, "-mw", "--machine-word-width", &width);
    if (rc == OPT_ERROR)
        return -1;
    if (rc == OPT_FOUND)
    {
        if (handle_word_width_option(width, &ctx->config.machine_word_width))
        {
            context_error(ctx, NULL, "invalid machine word width: %d", width);
            return -1;
        }
    }

    rc = opt_get(argcp, argvp, ctx, OPTTYPE_INT, "-aw", "--address-word-width", &width);
    if (rc == OPT_ERROR)
        return -1;
    if (rc == OPT_FOUND)
    {
        if (handle_word_width_option(width, &ctx->config.address_word_width))
        {
            context_error(ctx, NULL, "invalid address word width: %d", width);
            return -1;
        }
    }

    struct {
        const char *shortopt;
        const char *longopt;
        const char **dest;
    } str_opt_map[] = {
        {"-ig",   "--include-guard-prefix",               &ctx->config.include_guard_prefix},
        {"-rh",   "--reggen-header-name",                 &ctx->config.reggen_header_name},
        {"-itp",  "--implicit-type-prefix",               &ctx->config.implicit_type_prefix},
        {"-rim",  "--register-iotype-macro-prefix",       &ctx->config.register_iotype_macro_prefix},
        {"-fpm",  "--field-pos-macro-prefix",             &ctx->config.field_pos_macro_prefix},
        {"-fmm",  "--field-mask-macro-prefix",            &ctx->config.field_mask_macro_prefix},
        {"-fem",  "--field-enum-macro-prefix",            &ctx->config.field_enum_macro_prefix},
        {"-fvm",  "--field-value-macro-prefix",           &ctx->config.field_value_macro_prefix},
        {"-fvmm", "--field-valmask-macro-prefix",         &ctx->config.field_valmask_macro_prefix},
        {"-iam",  "--instance-address-macro-prefix",      &ctx->config.instance_address_macro_prefix},
        {"-iom",  "--instance-offset-macro-prefix",       &ctx->config.instance_offset_macro_prefix},
        {"-inam", "--instance-name-address-macro-prefix", &ctx->config.instance_name_address_macro_prefix},
        {"-inom", "--instance-name-offset-macro-prefix",  &ctx->config.instance_name_offset_macro_prefix},
        {"-itam", "--instance-type-address-macro-prefix", &ctx->config.instance_type_address_macro_prefix},
        {"-itom", "--instance-type-offset-macro-prefix",  &ctx->config.instance_type_offset_macro_prefix},
        {"-o",    "--output-directory",                   &ctx->config.output_directory},
        {NULL, NULL, NULL},
    };

    for (size_t i = 0; str_opt_map[i].dest != NULL; ++i)
    {
        rc = opt_get(argcp, argvp, ctx, OPTTYPE_STRING,
                     str_opt_map[i].shortopt,
                     str_opt_map[i].longopt,
                     str_opt_map[i].dest);
        if (rc == OPT_ERROR)
            return -1;
    }

    if (!ctx->config.output_directory)
    {
        context_error(ctx, NULL, "output directory not specified");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    struct context *ctx = &g_ctx;

    /* Option parsing */
    int rc;
    char *input_filename = NULL;

    rc = opt_get(&argc, &argv, ctx, OPTTYPE_POSARG, NULL, NULL, &input_filename);
    if (rc != OPT_FOUND)
    {
        context_error(ctx, NULL, "missing input filename");
        return 1;
    }

    /* Load options for context */
    if (get_context_options(ctx, &argc, &argv))
        return 1;

    /* Check for unknown options, ignoring "--" which may be left over */
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--"))
        {
            context_error(ctx, NULL, "unknown option: %s", argv[i]);
            return 1;
        }
    }

    FILE *input_file = NULL;

    /* Open input file */
    input_file = fopen(input_filename, "rb");
    if (!input_file)
    {
        context_error(ctx, NULL, "unable to open input file: %s", input_filename);
        goto exit;
    }

    /* Parse the input */
    rc = parse(ctx, input_filename, input_file);
    if (rc != PARSE_OK)
    {
        context_error(ctx, NULL, "failed to parse input file: %s", input_filename);
        goto exit;
    }

    /* Validate input to check for errors */
    validate(ctx);

exit:
    if (ctx->num_errors == 0)
        generate_output(ctx);

    if (input_file)
        fclose(input_file);

    if (ctx->num_errors > 0)
        rc = 1;
    else
        rc = 0;

    context_free(ctx);
    return rc;
}
