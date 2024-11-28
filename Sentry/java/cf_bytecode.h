/* ==============================================================================================================
 * Author: CXX
 * Date: 2022-04-14
 * Version:
 * Copyright (C) CXX, All rights reserved
 * Description:
 * History:
 * 20220414: C header file, be created
 * ==============================================================================================================
*/

#ifndef _CF_BYTECODE_H_
#define _CF_BYTECODE_H_

#include "classfile_constants.h"

#include "classfile_util.h"

#pragma pack(push, 4)


// opcode
// according to the JVMS
typedef enum
{
    CF_OPC_nop                 = 0,
	/* Constants */
    CF_OPC_aconst_null         = 1,
    CF_OPC_iconst_m1           = 2,
    CF_OPC_iconst_0            = 3,
    CF_OPC_iconst_1            = 4,
    CF_OPC_iconst_2            = 5,
    CF_OPC_iconst_3            = 6,
    CF_OPC_iconst_4            = 7,
    CF_OPC_iconst_5            = 8,
    CF_OPC_lconst_0            = 9,
    CF_OPC_lconst_1            = 10,
    CF_OPC_fconst_0            = 11,
    CF_OPC_fconst_1            = 12,
    CF_OPC_fconst_2            = 13,
    CF_OPC_dconst_0            = 14,
    CF_OPC_dconst_1            = 15,
    CF_OPC_bipush              = 16,
    CF_OPC_sipush              = 17,
    CF_OPC_ldc                 = 18,
    CF_OPC_ldc_w               = 19,
    CF_OPC_ldc2_w              = 20,
	/* Loads */
    CF_OPC_iload               = 21,
    CF_OPC_lload               = 22,
    CF_OPC_fload               = 23,
    CF_OPC_dload               = 24,
    CF_OPC_aload               = 25,
    CF_OPC_iload_0             = 26,
    CF_OPC_iload_1             = 27,
    CF_OPC_iload_2             = 28,
    CF_OPC_iload_3             = 29,
    CF_OPC_lload_0             = 30,
    CF_OPC_lload_1             = 31,
    CF_OPC_lload_2             = 32,
    CF_OPC_lload_3             = 33,
    CF_OPC_fload_0             = 34,
    CF_OPC_fload_1             = 35,
    CF_OPC_fload_2             = 36,
    CF_OPC_fload_3             = 37,
    CF_OPC_dload_0             = 38,
    CF_OPC_dload_1             = 39,
    CF_OPC_dload_2             = 40,
    CF_OPC_dload_3             = 41,
    CF_OPC_aload_0             = 42,
    CF_OPC_aload_1             = 43,
    CF_OPC_aload_2             = 44,
    CF_OPC_aload_3             = 45,
    CF_OPC_iaload              = 46,
    CF_OPC_laload              = 47,
    CF_OPC_faload              = 48,
    CF_OPC_daload              = 49,
    CF_OPC_aaload              = 50,
    CF_OPC_baload              = 51,
    CF_OPC_caload              = 52,
    CF_OPC_saload              = 53,
	/* Stores */
    CF_OPC_istore              = 54,
    CF_OPC_lstore              = 55,
    CF_OPC_fstore              = 56,
    CF_OPC_dstore              = 57,
    CF_OPC_astore              = 58,
    CF_OPC_istore_0            = 59,
    CF_OPC_istore_1            = 60,
    CF_OPC_istore_2            = 61,
    CF_OPC_istore_3            = 62,
    CF_OPC_lstore_0            = 63,
    CF_OPC_lstore_1            = 64,
    CF_OPC_lstore_2            = 65,
    CF_OPC_lstore_3            = 66,
    CF_OPC_fstore_0            = 67,
    CF_OPC_fstore_1            = 68,
    CF_OPC_fstore_2            = 69,
    CF_OPC_fstore_3            = 70,
    CF_OPC_dstore_0            = 71,
    CF_OPC_dstore_1            = 72,
    CF_OPC_dstore_2            = 73,
    CF_OPC_dstore_3            = 74,
    CF_OPC_astore_0            = 75,
    CF_OPC_astore_1            = 76,
    CF_OPC_astore_2            = 77,
    CF_OPC_astore_3            = 78,
    CF_OPC_iastore             = 79,
    CF_OPC_lastore             = 80,
    CF_OPC_fastore             = 81,
    CF_OPC_dastore             = 82,
    CF_OPC_aastore             = 83,
    CF_OPC_bastore             = 84,
    CF_OPC_castore             = 85,
    CF_OPC_sastore             = 86,
	/* Stack */
    CF_OPC_pop                 = 87,
    CF_OPC_pop2                = 88,
    CF_OPC_dup                 = 89,
    CF_OPC_dup_x1              = 90,
    CF_OPC_dup_x2              = 91,
    CF_OPC_dup2                = 92,
    CF_OPC_dup2_x1             = 93,
    CF_OPC_dup2_x2             = 94,
    CF_OPC_swap                = 95,
	/* Math */
    CF_OPC_iadd                = 96,
    CF_OPC_ladd                = 97,
    CF_OPC_fadd                = 98,
    CF_OPC_dadd                = 99,
    CF_OPC_isub                = 100,
    CF_OPC_lsub                = 101,
    CF_OPC_fsub                = 102,
    CF_OPC_dsub                = 103,
    CF_OPC_imul                = 104,
    CF_OPC_lmul                = 105,
    CF_OPC_fmul                = 106,
    CF_OPC_dmul                = 107,
    CF_OPC_idiv                = 108,
    CF_OPC_ldiv                = 109,
    CF_OPC_fdiv                = 110,
    CF_OPC_ddiv                = 111,
    CF_OPC_irem                = 112,
    CF_OPC_lrem                = 113,
    CF_OPC_frem                = 114,
    CF_OPC_drem                = 115,
    CF_OPC_ineg                = 116,
    CF_OPC_lneg                = 117,
    CF_OPC_fneg                = 118,
    CF_OPC_dneg                = 119,
    CF_OPC_ishl                = 120,
    CF_OPC_lshl                = 121,
    CF_OPC_ishr                = 122,
    CF_OPC_lshr                = 123,
    CF_OPC_iushr               = 124,
    CF_OPC_lushr               = 125,
    CF_OPC_iand                = 126,
    CF_OPC_land                = 127,
    CF_OPC_ior                 = 128,
    CF_OPC_lor                 = 129,
    CF_OPC_ixor                = 130,
    CF_OPC_lxor                = 131,
    CF_OPC_iinc                = 132,
	/* Conversions */
    CF_OPC_i2l                 = 133,
    CF_OPC_i2f                 = 134,
    CF_OPC_i2d                 = 135,
    CF_OPC_l2i                 = 136,
    CF_OPC_l2f                 = 137,
    CF_OPC_l2d                 = 138,
    CF_OPC_f2i                 = 139,
    CF_OPC_f2l                 = 140,
    CF_OPC_f2d                 = 141,
    CF_OPC_d2i                 = 142,
    CF_OPC_d2l                 = 143,
    CF_OPC_d2f                 = 144,
    CF_OPC_i2b                 = 145,
    CF_OPC_i2c                 = 146,
    CF_OPC_i2s                 = 147,
	/* Comparisons */
	CF_OPC_CMP_Begin           = 148,
    CF_OPC_lcmp                = 148,
    CF_OPC_fcmpl               = 149,
    CF_OPC_fcmpg               = 150,
    CF_OPC_dcmpl               = 151,
    CF_OPC_dcmpg               = 152,
	//
	CF_OPC_IF_Begin            = 153,
    CF_OPC_ifeq                = 153,
    CF_OPC_ifne                = 154,
    CF_OPC_iflt                = 155,
    CF_OPC_ifge                = 156,
    CF_OPC_ifgt                = 157,
    CF_OPC_ifle                = 158,
    CF_OPC_if_icmpeq           = 159,
    CF_OPC_if_icmpne           = 160,
    CF_OPC_if_icmplt           = 161,
    CF_OPC_if_icmpge           = 162,
    CF_OPC_if_icmpgt           = 163,
    CF_OPC_if_icmple           = 164,
    CF_OPC_if_acmpeq           = 165,
    CF_OPC_if_acmpne           = 166,
	CF_OPC_IF_End              = 166,
	CF_OPC_CMP_End             = 166,
	/* Controls */
	CF_OPC_CTRL_Begin          = 167,
    CF_OPC_goto                = 167,
    CF_OPC_jsr                 = 168,
    CF_OPC_ret                 = 169,
    CF_OPC_tableswitch         = 170,
    CF_OPC_lookupswitch        = 171,
    CF_OPC_ireturn             = 172,
    CF_OPC_lreturn             = 173,
    CF_OPC_freturn             = 174,
    CF_OPC_dreturn             = 175,
    CF_OPC_areturn             = 176,
    CF_OPC_return              = 177,
	CF_OPC_CTRL_End            = 177,
	/* References */
    CF_OPC_getstatic           = 178,
    CF_OPC_putstatic           = 179,
    CF_OPC_getfield            = 180,
    CF_OPC_putfield            = 181,
    CF_OPC_invokevirtual       = 182,
    CF_OPC_invokespecial       = 183,
    CF_OPC_invokestatic        = 184,
    CF_OPC_invokeinterface     = 185,
    CF_OPC_invokedynamic       = 186,
    CF_OPC_new                 = 187,
    CF_OPC_newarray            = 188,
    CF_OPC_anewarray           = 189,
    CF_OPC_arraylength         = 190,
    CF_OPC_athrow              = 191,
    CF_OPC_checkcast           = 192,
    CF_OPC_instanceof          = 193,
    CF_OPC_monitorenter        = 194,
    CF_OPC_monitorexit         = 195,
	/* Extended */
    CF_OPC_wide                = 196,
    CF_OPC_multianewarray      = 197,
    CF_OPC_ifnull              = 198,
    CF_OPC_ifnonnull           = 199,
    CF_OPC_goto_w              = 200,  /* also is Control */
    CF_OPC_jsr_w               = 201,  /* also is Control */

    CF_OPC_MAX                 = 201,

	/* Reserved */
	CF_OPC_breakpoint          = 253,
	CF_OPC_impdep1             = 254,
	CF_OPC_impdep2             = 255
}CF_OPC;

// opcode length
// 0 means variable-length
#define CF_OPC_LEN_INITIALIZER { \
   1,   /* nop */                       \
   1,   /* aconst_null */               \
   1,   /* iconst_m1 */                 \
   1,   /* iconst_0 */                  \
   1,   /* iconst_1 */                  \
   1,   /* iconst_2 */                  \
   1,   /* iconst_3 */                  \
   1,   /* iconst_4 */                  \
   1,   /* iconst_5 */                  \
   1,   /* lconst_0 */                  \
   1,   /* lconst_1 */                  \
   1,   /* fconst_0 */                  \
   1,   /* fconst_1 */                  \
   1,   /* fconst_2 */                  \
   1,   /* dconst_0 */                  \
   1,   /* dconst_1 */                  \
   2,   /* bipush */                    \
   3,   /* sipush */                    \
   2,   /* ldc */                       \
   3,   /* ldc_w */                     \
   3,   /* ldc2_w */                    \
   2,   /* iload */                     \
   2,   /* lload */                     \
   2,   /* fload */                     \
   2,   /* dload */                     \
   2,   /* aload */                     \
   1,   /* iload_0 */                   \
   1,   /* iload_1 */                   \
   1,   /* iload_2 */                   \
   1,   /* iload_3 */                   \
   1,   /* lload_0 */                   \
   1,   /* lload_1 */                   \
   1,   /* lload_2 */                   \
   1,   /* lload_3 */                   \
   1,   /* fload_0 */                   \
   1,   /* fload_1 */                   \
   1,   /* fload_2 */                   \
   1,   /* fload_3 */                   \
   1,   /* dload_0 */                   \
   1,   /* dload_1 */                   \
   1,   /* dload_2 */                   \
   1,   /* dload_3 */                   \
   1,   /* aload_0 */                   \
   1,   /* aload_1 */                   \
   1,   /* aload_2 */                   \
   1,   /* aload_3 */                   \
   1,   /* iaload */                    \
   1,   /* laload */                    \
   1,   /* faload */                    \
   1,   /* daload */                    \
   1,   /* aaload */                    \
   1,   /* baload */                    \
   1,   /* caload */                    \
   1,   /* saload */                    \
   2,   /* istore */                    \
   2,   /* lstore */                    \
   2,   /* fstore */                    \
   2,   /* dstore */                    \
   2,   /* astore */                    \
   1,   /* istore_0 */                  \
   1,   /* istore_1 */                  \
   1,   /* istore_2 */                  \
   1,   /* istore_3 */                  \
   1,   /* lstore_0 */                  \
   1,   /* lstore_1 */                  \
   1,   /* lstore_2 */                  \
   1,   /* lstore_3 */                  \
   1,   /* fstore_0 */                  \
   1,   /* fstore_1 */                  \
   1,   /* fstore_2 */                  \
   1,   /* fstore_3 */                  \
   1,   /* dstore_0 */                  \
   1,   /* dstore_1 */                  \
   1,   /* dstore_2 */                  \
   1,   /* dstore_3 */                  \
   1,   /* astore_0 */                  \
   1,   /* astore_1 */                  \
   1,   /* astore_2 */                  \
   1,   /* astore_3 */                  \
   1,   /* iastore */                   \
   1,   /* lastore */                   \
   1,   /* fastore */                   \
   1,   /* dastore */                   \
   1,   /* aastore */                   \
   1,   /* bastore */                   \
   1,   /* castore */                   \
   1,   /* sastore */                   \
   1,   /* pop */                       \
   1,   /* pop2 */                      \
   1,   /* dup */                       \
   1,   /* dup_x1 */                    \
   1,   /* dup_x2 */                    \
   1,   /* dup2 */                      \
   1,   /* dup2_x1 */                   \
   1,   /* dup2_x2 */                   \
   1,   /* swap */                      \
   1,   /* iadd */                      \
   1,   /* ladd */                      \
   1,   /* fadd */                      \
   1,   /* dadd */                      \
   1,   /* isub */                      \
   1,   /* lsub */                      \
   1,   /* fsub */                      \
   1,   /* dsub */                      \
   1,   /* imul */                      \
   1,   /* lmul */                      \
   1,   /* fmul */                      \
   1,   /* dmul */                      \
   1,   /* idiv */                      \
   1,   /* ldiv */                      \
   1,   /* fdiv */                      \
   1,   /* ddiv */                      \
   1,   /* irem */                      \
   1,   /* lrem */                      \
   1,   /* frem */                      \
   1,   /* drem */                      \
   1,   /* ineg */                      \
   1,   /* lneg */                      \
   1,   /* fneg */                      \
   1,   /* dneg */                      \
   1,   /* ishl */                      \
   1,   /* lshl */                      \
   1,   /* ishr */                      \
   1,   /* lshr */                      \
   1,   /* iushr */                     \
   1,   /* lushr */                     \
   1,   /* iand */                      \
   1,   /* land */                      \
   1,   /* ior */                       \
   1,   /* lor */                       \
   1,   /* ixor */                      \
   1,   /* lxor */                      \
   3,   /* iinc */                      \
   1,   /* i2l */                       \
   1,   /* i2f */                       \
   1,   /* i2d */                       \
   1,   /* l2i */                       \
   1,   /* l2f */                       \
   1,   /* l2d */                       \
   1,   /* f2i */                       \
   1,   /* f2l */                       \
   1,   /* f2d */                       \
   1,   /* d2i */                       \
   1,   /* d2l */                       \
   1,   /* d2f */                       \
   1,   /* i2b */                       \
   1,   /* i2c */                       \
   1,   /* i2s */                       \
   1,   /* lcmp */                      \
   1,   /* fcmpl */                     \
   1,   /* fcmpg */                     \
   1,   /* dcmpl */                     \
   1,   /* dcmpg */                     \
   3,   /* ifeq */                      \
   3,   /* ifne */                      \
   3,   /* iflt */                      \
   3,   /* ifge */                      \
   3,   /* ifgt */                      \
   3,   /* ifle */                      \
   3,   /* if_icmpeq */                 \
   3,   /* if_icmpne */                 \
   3,   /* if_icmplt */                 \
   3,   /* if_icmpge */                 \
   3,   /* if_icmpgt */                 \
   3,   /* if_icmple */                 \
   3,   /* if_acmpeq */                 \
   3,   /* if_acmpne */                 \
   3,   /* goto */                      \
   3,   /* jsr */                       \
   2,   /* ret */                       \
   0,   /* tableswitch */               \
   0,   /* lookupswitch */              \
   1,   /* ireturn */                   \
   1,   /* lreturn */                   \
   1,   /* freturn */                   \
   1,   /* dreturn */                   \
   1,   /* areturn */                   \
   1,   /* return */                    \
   3,   /* getstatic */                 \
   3,   /* putstatic */                 \
   3,   /* getfield */                  \
   3,   /* putfield */                  \
   3,   /* invokevirtual */             \
   3,   /* invokespecial */             \
   3,   /* invokestatic */              \
   5,   /* invokeinterface */           \
   5,   /* invokedynamic */             \
   3,   /* new */                       \
   2,   /* newarray */                  \
   3,   /* anewarray */                 \
   1,   /* arraylength */               \
   1,   /* athrow */                    \
   3,   /* checkcast */                 \
   3,   /* instanceof */                \
   1,   /* monitorenter */              \
   1,   /* monitorexit */               \
   0,   /* wide */                      \
   4,   /* multianewarray */            \
   3,   /* ifnull */                    \
   3,   /* ifnonnull */                 \
   5,   /* goto_w */                    \
   5,   /* jsr_w */                     \
   -1, -1, -1, -1, -1, -1,              \
   -1, -1, -1, -1, -1, -1,              \
   -1, -1, -1, -1, -1, -1,              \
   -1, -1, -1, -1, -1, -1,              \
   -1, -1, -1, -1, -1, -1,              \
   -1, -1, -1, -1, -1, -1,              \
   -1, -1, -1, -1, -1, -1,              \
   -1, -1, -1, -1, -1, -1,              \
   -1, -1, -1, -1, -1, -1               \
}

// opcode symbols/mnemonics
#define CF_OPC_SYM_INITIALIZER { \
   "nop",                       \
   "aconst_null",               \
   "iconst_m1",                 \
   "iconst_0",                  \
   "iconst_1",                  \
   "iconst_2",                  \
   "iconst_3",                  \
   "iconst_4",                  \
   "iconst_5",                  \
   "lconst_0",                  \
   "lconst_1",                  \
   "fconst_0",                  \
   "fconst_1",                  \
   "fconst_2",                  \
   "dconst_0",                  \
   "dconst_1",                  \
   "bipush",                    \
   "sipush",                    \
   "ldc",                       \
   "ldc_w",                     \
   "ldc2_w",                    \
   "iload",                     \
   "lload",                     \
   "fload",                     \
   "dload",                     \
   "aload",                     \
   "iload_0",                   \
   "iload_1",                   \
   "iload_2",                   \
   "iload_3",                   \
   "lload_0",                   \
   "lload_1",                   \
   "lload_2",                   \
   "lload_3",                   \
   "fload_0",                   \
   "fload_1",                   \
   "fload_2",                   \
   "fload_3",                   \
   "dload_0",                   \
   "dload_1",                   \
   "dload_2",                   \
   "dload_3",                   \
   "aload_0",                   \
   "aload_1",                   \
   "aload_2",                   \
   "aload_3",                   \
   "iaload",                    \
   "laload",                    \
   "faload",                    \
   "daload",                    \
   "aaload",                    \
   "baload",                    \
   "caload",                    \
   "saload",                    \
   "istore",                    \
   "lstore",                    \
   "fstore",                    \
   "dstore",                    \
   "astore",                    \
   "istore_0",                  \
   "istore_1",                  \
   "istore_2",                  \
   "istore_3",                  \
   "lstore_0",                  \
   "lstore_1",                  \
   "lstore_2",                  \
   "lstore_3",                  \
   "fstore_0",                  \
   "fstore_1",                  \
   "fstore_2",                  \
   "fstore_3",                  \
   "dstore_0",                  \
   "dstore_1",                  \
   "dstore_2",                  \
   "dstore_3",                  \
   "astore_0",                  \
   "astore_1",                  \
   "astore_2",                  \
   "astore_3",                  \
   "iastore",                   \
   "lastore",                   \
   "fastore",                   \
   "dastore",                   \
   "aastore",                   \
   "bastore",                   \
   "castore",                   \
   "sastore",                   \
   "pop",                       \
   "pop2",                      \
   "dup",                       \
   "dup_x1",                    \
   "dup_x2",                    \
   "dup2",                      \
   "dup2_x1",                   \
   "dup2_x2",                   \
   "swap",                      \
   "iadd",                      \
   "ladd",                      \
   "fadd",                      \
   "dadd",                      \
   "isub",                      \
   "lsub",                      \
   "fsub",                      \
   "dsub",                      \
   "imul",                      \
   "lmul",                      \
   "fmul",                      \
   "dmul",                      \
   "idiv",                      \
   "ldiv",                      \
   "fdiv",                      \
   "ddiv",                      \
   "irem",                      \
   "lrem",                      \
   "frem",                      \
   "drem",                      \
   "ineg",                      \
   "lneg",                      \
   "fneg",                      \
   "dneg",                      \
   "ishl",                      \
   "lshl",                      \
   "ishr",                      \
   "lshr",                      \
   "iushr",                     \
   "lushr",                     \
   "iand",                      \
   "land",                      \
   "ior",                       \
   "lor",                       \
   "ixor",                      \
   "lxor",                      \
   "iinc",                      \
   "i2l",                       \
   "i2f",                       \
   "i2d",                       \
   "l2i",                       \
   "l2f",                       \
   "l2d",                       \
   "f2i",                       \
   "f2l",                       \
   "f2d",                       \
   "d2i",                       \
   "d2l",                       \
   "d2f",                       \
   "i2b",                       \
   "i2c",                       \
   "i2s",                       \
   "lcmp",                      \
   "fcmpl",                     \
   "fcmpg",                     \
   "dcmpl",                     \
   "dcmpg",                     \
   "ifeq",                      \
   "ifne",                      \
   "iflt",                      \
   "ifge",                      \
   "ifgt",                      \
   "ifle",                      \
   "if_icmpeq",                 \
   "if_icmpne",                 \
   "if_icmplt",                 \
   "if_icmpge",                 \
   "if_icmpgt",                 \
   "if_icmple",                 \
   "if_acmpeq",                 \
   "if_acmpne",                 \
   "goto",                      \
   "jsr",                       \
   "ret",                       \
   "tableswitch",               \
   "lookupswitch",              \
   "ireturn",                   \
   "lreturn",                   \
   "freturn",                   \
   "dreturn",                   \
   "areturn",                   \
   "return",                    \
   "getstatic",                 \
   "putstatic",                 \
   "getfield",                  \
   "putfield",                  \
   "invokevirtual",             \
   "invokespecial",             \
   "invokestatic",              \
   "invokeinterface",           \
   "invokedynamic",             \
   "new",                       \
   "newarray",                  \
   "anewarray",                 \
   "arraylength",               \
   "athrow",                    \
   "checkcast",                 \
   "instanceof",                \
   "monitorenter",              \
   "monitorexit",               \
   "wide",                      \
   "multianewarray",            \
   "ifnull",                    \
   "ifnonnull",                 \
   "goto_w",                    \
   "jsr_w",                     \
   "*bad", "*bad", "*bad", "*bad", "*bad", "*bad",      \
   "*bad", "*bad", "*bad", "*bad", "*bad", "*bad",      \
   "*bad", "*bad", "*bad", "*bad", "*bad", "*bad",      \
   "*bad", "*bad", "*bad", "*bad", "*bad", "*bad",      \
   "*bad", "*bad", "*bad", "*bad", "*bad", "*bad",      \
   "*bad", "*bad", "*bad", "*bad", "*bad", "*bad",      \
   "*bad", "*bad", "*bad", "*bad", "*bad", "*bad",      \
   "*bad", "*bad", "*bad", "*bad", "*bad", "*bad",      \
   "*bad", "*bad", "*bad",     \
   "breakpoint",               \
   "impdep1",                  \
   "impdep2"                   \
}


// stack growth
#define CF_OPC_STACK_GROWTH_INITIALIZER { \
   0,    /* nop */                       \
   1,    /* aconst_null */               \
   1,    /* iconst_m1 */                 \
   1,    /* iconst_0 */                  \
   1,    /* iconst_1 */                  \
   1,    /* iconst_2 */                  \
   1,    /* iconst_3 */                  \
   1,    /* iconst_4 */                  \
   1,    /* iconst_5 */                  \
   2,    /* lconst_0 */                  \
   2,    /* lconst_1 */                  \
   1,    /* fconst_0 */                  \
   1,    /* fconst_1 */                  \
   1,    /* fconst_2 */                  \
   2,    /* dconst_0 */                  \
   2,    /* dconst_1 */                  \
   1,    /* bipush */                    \
   1,    /* sipush */                    \
   1,    /* ldc */                       \
   1,    /* ldc_w */                     \
   2,    /* ldc2_w */                    \
   1,    /* iload */                     \
   2,    /* lload */                     \
   1,    /* fload */                     \
   2,    /* dload */                     \
   1,    /* aload */                     \
   1,    /* iload_0 */                   \
   1,    /* iload_1 */                   \
   1,    /* iload_2 */                   \
   1,    /* iload_3 */                   \
   2,    /* lload_0 */                   \
   2,    /* lload_1 */                   \
   2,    /* lload_2 */                   \
   2,    /* lload_3 */                   \
   1,    /* fload_0 */                   \
   1,    /* fload_1 */                   \
   1,    /* fload_2 */                   \
   1,    /* fload_3 */                   \
   2,    /* dload_0 */                   \
   2,    /* dload_1 */                   \
   2,    /* dload_2 */                   \
   2,    /* dload_3 */                   \
   1,    /* aload_0 */                   \
   1,    /* aload_1 */                   \
   1,    /* aload_2 */                   \
   1,    /* aload_3 */                   \
   -1,   /* iaload */                    \
   0,    /* laload */                    \
   -1,   /* faload */                    \
   0,    /* daload */                    \
   -1,   /* aaload */                    \
   -1,   /* baload */                    \
   -1,   /* caload */                    \
   -1,   /* saload */                    \
   -1,   /* istore */                    \
   -2,   /* lstore */                    \
   -1,   /* fstore */                    \
   -2,   /* dstore */                    \
   -1,   /* astore */                    \
   -1,   /* istore_0 */                  \
   -1,   /* istore_1 */                  \
   -1,   /* istore_2 */                  \
   -1,   /* istore_3 */                  \
   -2,   /* lstore_0 */                  \
   -2,   /* lstore_1 */                  \
   -2,   /* lstore_2 */                  \
   -2,   /* lstore_3 */                  \
   -1,   /* fstore_0 */                  \
   -1,   /* fstore_1 */                  \
   -1,   /* fstore_2 */                  \
   -1,   /* fstore_3 */                  \
   -2,   /* dstore_0 */                  \
   -2,   /* dstore_1 */                  \
   -2,   /* dstore_2 */                  \
   -2,   /* dstore_3 */                  \
   -1,   /* astore_0 */                  \
   -1,   /* astore_1 */                  \
   -1,   /* astore_2 */                  \
   -1,   /* astore_3 */                  \
   -3,   /* iastore */                   \
   -4,   /* lastore */                   \
   -3,   /* fastore */                   \
   -4,   /* dastore */                   \
   -3,   /* aastore */                   \
   -3,   /* bastore */                   \
   -3,   /* castore */                   \
   -3,   /* sastore */                   \
   -1,   /* pop */                       \
   -2,   /* pop2 */                      \
   1,    /* dup */                       \
   1,    /* dup_x1 */                    \
   1,    /* dup_x2 */                    \
   2,    /* dup2 */                      \
   2,    /* dup2_x1 */                   \
   2,    /* dup2_x2 */                   \
   0,    /* swap */                      \
   -1,   /* iadd */                      \
   -2,   /* ladd */                      \
   -1,   /* fadd */                      \
   -2,   /* dadd */                      \
   -1,   /* isub */                      \
   -2,   /* lsub */                      \
   -1,   /* fsub */                      \
   -2,   /* dsub */                      \
   -1,   /* imul */                      \
   -2,   /* lmul */                      \
   -1,   /* fmul */                      \
   -2,   /* dmul */                      \
   -1,   /* idiv */                      \
   -2,   /* ldiv */                      \
   -1,   /* fdiv */                      \
   -2,   /* ddiv */                      \
   -1,   /* irem */                      \
   -2,   /* lrem */                      \
   -1,   /* frem */                      \
   -2,   /* drem */                      \
   0,    /* ineg */                      \
   0,    /* lneg */                      \
   0,    /* fneg */                      \
   0,    /* dneg */                      \
   -1,   /* ishl */                      \
   -1,   /* lshl */                      \
   -1,   /* ishr */                      \
   -1,   /* lshr */                      \
   -1,   /* iushr */                     \
   -1,   /* lushr */                     \
   -1,   /* iand */                      \
   -2,   /* land */                      \
   -1,   /* ior */                       \
   -2,   /* lor */                       \
   -1,   /* ixor */                      \
   -2,   /* lxor */                      \
   0,    /* iinc */                      \
   1,    /* i2l */                       \
   0,    /* i2f */                       \
   1,    /* i2d */                       \
   -1,   /* l2i */                       \
   -1,   /* l2f */                       \
   0,    /* l2d */                       \
   0,    /* f2i */                       \
   1,    /* f2l */                       \
   1,    /* f2d */                       \
   -1,   /* d2i */                       \
   0,    /* d2l */                       \
   -1,   /* d2f */                       \
   0,    /* i2b */                       \
   0,    /* i2c */                       \
   0,    /* i2s */                       \
   -3,   /* lcmp */                      \
   -1,   /* fcmpl */                     \
   -1,   /* fcmpg */                     \
   -3,   /* dcmpl */                     \
   -3,   /* dcmpg */                     \
   -1,   /* ifeq */                      \
   -1,   /* ifne */                      \
   -1,   /* iflt */                      \
   -1,   /* ifge */                      \
   -1,   /* ifgt */                      \
   -1,   /* ifle */                      \
   -2,   /* if_icmpeq */                 \
   -2,   /* if_icmpne */                 \
   -2,   /* if_icmplt */                 \
   -2,   /* if_icmpge */                 \
   -2,   /* if_icmpgt */                 \
   -2,   /* if_icmple */                 \
   -2,   /* if_acmpeq */                 \
   -2,   /* if_acmpne */                 \
   0,    /* goto */                      \
   1,    /* jsr */                       \
   0,    /* ret */                       \
   -1,   /* tableswitch */               \
   -1,   /* lookupswitch */              \
   -1,   /* ireturn */                   \
   -2,   /* lreturn */                   \
   -1,   /* freturn */                   \
   -2,   /* dreturn */                   \
   -1,   /* areturn */                   \
   0,    /* return */                    \
   0x10000,    /* getstatic */                 \
   0x10000,    /* putstatic */                 \
   0x10000,    /* getfield */                  \
   0x10000,    /* putfield */                  \
   0x10000,    /* invokevirtual */             \
   0x10000,    /* invokespecial */             \
   0x10000,    /* invokestatic */              \
   0x10000,    /* invokeinterface */           \
   0x10000,    /* invokedynamic */             \
   1,    /* new */                       \
   0,    /* newarray */                  \
   0,    /* anewarray */                 \
   0,    /* arraylength */               \
   0,    /* athrow */                    \
   0,    /* checkcast */                 \
   0,    /* instanceof */                \
   -1,   /* monitorenter */              \
   -1,   /* monitorexit */               \
   0,    /* wide */                      \
   0x10000,    /* multianewarray */            \
   -1,   /* ifnull */                    \
   -1,   /* ifnonnull */                 \
   0,    /* goto_w */                    \
   1,    /* jsr_w */                     \
   0, 0, 0, 0, 0, 0,              \
   0, 0, 0, 0, 0, 0,              \
   0, 0, 0, 0, 0, 0,              \
   0, 0, 0, 0, 0, 0,              \
   0, 0, 0, 0, 0, 0,              \
   0, 0, 0, 0, 0, 0,              \
   0, 0, 0, 0, 0, 0,              \
   0, 0, 0, 0, 0, 0,              \
   0, 0, 0, 0, 0, 0               \
}


// instruction location
typedef struct
{
	int    pos;    // position of instruction
	int    len;    // length of instruction
	int    jmpto;  // position of transfer target
	int    ext;    // total length of extended space(bytes)
	int    last_opc_pos;
	juint8 opc;    // opcode
	juint8 same;   // only for switch
	int16 add;    // number of additive(bytes) at an opcode
}CF_InsLoc;


// only for middle process
typedef struct
{
	int start_pc;
	int end_pc;
	int handler_pc;
	int start_off;
	int end_off;
	int handler_off;
}CF_ExpOff;

//
typedef struct
{
	int cp_off;
	int btsmtd_off;
	const char *mtd_name;
	const char *mtd_desc;

	//
	int entry_pc;
	int exit_pc;
	int catch_pc;
	int code_pc;
	int code_ln;

	int catch_beg_pc;
	int catch_end_pc;


	juint16 entry_last_opc_off;
	juint16 entry_last_opc_len;
	juint16 exit_last_opc_off;
	juint16 exit_last_opc_len;
	juint16 catch_last_opc_off;
	juint16 catch_last_opc_len;
	juint16 code_last_opc_off;
	juint16 code_last_opc_len;

	juint16 exit_ret_off;

	juint8 stk_add;
	juint8 var_add;
	juint8 ret_tp_idx;
	juint8 ret_slots;

	//cf_idx_t return_type;
	cf_idx_t catch_type;

	CF_InsLoc *ins_locs;
	int ins_locs_num;

	CF_InsLoc ins_locs2[10];	// for stub's
	int ins_locs_num2;		// for stub's

	CF_SignExt *this_var;
	CF_SignExt *mtd_sign_tab;
	int mtd_sign_tab_len;
}CF_CodeTransformInfo;

//
typedef struct
{
	int pos;
	int len;
	juint8 *ptr;
}CF_InsertCode;


#pragma pack(pop)



#ifdef __cplusplus
extern "C" {
#endif

CF_NOEXPORT const juint8 CF_OPC_LEN[256];
CF_NOEXPORT const char CF_OPC_SYM[256][24];
CF_NOEXPORT const jint CF_OPC_STK_GROW[256];
CF_NOEXPORT const juint8 CF_IDX_TO_OPC_RETURN[16];
CF_NOEXPORT const juint8 CF_SMFVT_LEN[9];


// Function: test opcode is xreturn or athrow
inline static int cf_opc_is_return_or_throw( uint8 opc )
{
	return ((opc >= CF_OPC_ireturn) && (opc <= CF_OPC_return)) || (opc == CF_OPC_athrow);
}


// Function: calculates length of a instruction/opcode
inline static int cf_opc_len_calcu( const juint8 *code, int pos )
{
	switch( code[0] )
	{
	case CF_OPC_tableswitch:{
		int p = 1 + ((-1-pos) & 3);  // 1 + ((4 - ((pos + 1) & 3)) & 3); // opcode + padding
		// 4 * (high - low + 1) + 12 + p
		return 4 * (_MAKE_INT32_(code[p+8], code[p+9], code[p+10], code[p+11])
					- _MAKE_INT32_(code[p+4], code[p+5], code[p+6], code[p+7]) + 1)
					+ 12 + p;
	}
	case CF_OPC_lookupswitch:{
		int p = 1 + ((-1-pos) & 3);  // 1 + ((4 - ((pos + 1) & 3)) & 3); // opcode + padding
		// 8 * npairs + 8 + p
		return 8 * _MAKE_INT32_(code[p+4], code[p+5], code[p+6], code[p+7]) + 8 + p;
	}
	case CF_OPC_wide:
		/*return code[1] == CF_OPC_iinc ? 6 :
				 (code[1]>=CF_OPC_iload & code[1]<=CF_OPC_aload)
				|(code[1]>=CF_OPC_istore & code[1]<=CF_OPC_astore)
				|(code[1] == CF_OPC_ret) ? 4 : -1;
		*/
		return (6 & -(code[1]==CF_OPC_iinc)) ||
			   (4 & -((code[1]>=CF_OPC_iload & code[1]<=CF_OPC_aload)
						|(code[1]>=CF_OPC_istore & code[1]<=CF_OPC_astore)
						|(code[1] == CF_OPC_ret)));
	}

	return CF_OPC_LEN[code[0]];
}


// Function: check validation of stub byte-code(NOT ADOPTED)
inline static int cf_check_stubcode( const juint8 *byte, int stublen, const juint8 *blkptr[], int *blklen, cf_idx_t *catch_type )
{
	// stub code layout:
	// | impdep1 | ENTER_LEN | impdep1 | EXIT_LEN | impdep1 | CATCH_LEN | CATCH_TYPE | ENTRY BYTECODE | EXIT BYTECODE | CATCH BYTECODE | CRC32 |
	// 0         1           3         4          6         7           9            11               N               M                X       X+4
	if( stublen <= 15 ||
		byte[0] != CF_OPC_impdep1 ||
		byte[3] != CF_OPC_impdep1 ||
		byte[6] != CF_OPC_impdep1 )
		return -1;

	blklen[0] = _MAKE_INT16_( byte[1], byte[2] );
	blklen[1] = _MAKE_INT16_( byte[4], byte[5] );
	blklen[2] = _MAKE_INT16_( byte[7], byte[8] );

	// check length
	if( blklen[0] + blklen[1] + blklen[2] + 15 != stublen )
		return -1;

	blkptr[0] = byte + 11;  			// ENTRY
	blkptr[1] = blkptr[0] + blklen[0];	// EXIT
	blkptr[2] = blkptr[1] + blklen[1];	// CATCH

	*catch_type = _MAKE_INT16_( byte[9], byte[10] );

	// check CRC
	return p_Make_CRC32( (UINT8*)byte, stublen, 0 );
}



// Function: seek local variables in byte code
CF_NOEXPORT int cf_seek_local_var( const juint8 *code, int len, cf_idx_t *rdvar, cf_idx_t *wrvar, int varmax  );

//
CF_NOEXPORT int cf_seek_opcode( const juint8 *code, int len, juint8 opc, int *opc_len, int *last_opc_len  );

//
CF_NOEXPORT int cf_seek_end_opcode( const juint8 *code, int len, int *opc_len  );

// Function: locate all CONTROL instructions
CF_NOEXPORT CF_InsLoc*  cf_locate_ctrl_instruction( const juint8 *code, int len, CF_InsLoc *loc_ptr, int *loc_len,
													int *last_opc_off, int *last_opc_len, CF_OPC ldc );

// Function: alter offset which as operand of CONTROL instructions
CF_NOEXPORT int cf_alter_instruction_offset( CF_InsLoc *locs, int num, int beg_pos, int exit_pos, CF_ExpOff *exptab, int expnum );

// Function: modify constant pool index used by specific opcode
CF_NOEXPORT int cf_modify_cp_index( juint8 *code, int len, int cp_off, int *last_opc_len );

// Function: scan constants used in specified code sequences
CF_NOEXPORT int cf_scan_constant_refer( juint8 *code, int len, cf_idx_t *mask, int cp_num, int *last_opc_len, int *last_opc_off );

// Function: re-assemble instruction sequences according to altered locations
CF_NOEXPORT
int cf_assemble_instruction( const juint8 *src, int srclen,
							 juint8 *dst, int dstsize,
							 CF_InsLoc *locs, int nlocs,
							 int dst_pos, int exit_pos,
							 int *end_opc_off, int *end_opc_len
							 );

// Function: blend source code and stub code as one integrated sequences
CF_NOEXPORT
int cf_bytecode_transform_v2( const juint8 *src_buf[],
						      const int    *src_len,
						      juint8      **dst_buf,
						      int           dst_sz,
						      CF_ExpInfo   *exp_tab,
						      int           exp_num,
						      CF_CodeTransformInfo *info,
							  uint32        flags,
							  int			cpi_tpix
						      );

CF_NOEXPORT
int cf_bytecode_transform_v3( const juint8 *src_buf[],
						      const int    *src_len,
						      juint8      **dst_buf,
						      int           dst_sz,
						      CF_ExpInfo   *exp_tab,
						      int           exp_num,
						      CF_CodeTransformInfo *info,
							  uint32        flags,
							  int			cpi_tpix
						      );

CF_NOEXPORT
int cf_bytecode_transform( const juint8 *src_buf[],
						   const int    *src_len,
						   juint8      **dst_buf,
						   int           dst_sz,
						   CF_ExpInfo   *exp_tab,
						   int           exp_num,
						   CF_CodeTransformInfo *info
						   );


// Function: translate byte-code in readable instruction sequences
CF_NOEXPORT void cf_bytecode_translate( const juint8 *code, int len, const CF_ClassFileMap *cfmap, FILE *fout );

// Function: build StackMapTables from byte stream
CF_NOEXPORT CF_Attr_StackMapFrame_Union** cf_build_stackmaptab( const CF_Attr_StackMapTable **tabs, int ntab, int *nfrm, int **frm_len_ptr );

// Function: alter stack map table
CF_NOEXPORT
int cf_alter_stackmaptab( CF_Attr_StackMapFrame_Union **smf_ptr, int nfrm, int *smf_len,
						  const CF_InsLoc *insLocs, int nlocs,
						  int code_shift, int cpool_shift, void **ext_buf, int *end_pos
						  );

CF_NOEXPORT int cf_count_smf_delta( CF_Attr_StackMapFrame_Union **smf, int num );

CF_NOEXPORT int cf_fill_smf_full_frame( juint16 delta, const CF_SignExt *locals, int local_num,
		const CF_SignExt *stacks, int stack_num, void *dst_buf, const CF_SignExt *this_var );

CF_NOEXPORT int cf_alter_smf_delta( CF_Attr_StackMapFrame_Union *smf, int shift, BOOL flip_endian );


#ifdef __cplusplus
}
#endif

#endif // _CF_BYTECODE_H_
