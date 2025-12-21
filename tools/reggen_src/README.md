# RegGen: register definition generator

RegGen is a utility that generates C headers files for accessing
memory-mapped hardware registers in SoCs and microcontrollers.

RegGen uses a custom description format designed to be easy to
read & maintain by hand. It includes various features aimed at
reducing unnecessary repetition in register descriptions.

The output of RegGen is comparable to ARM's CMSIS headers, but
unlike CMSIS it does not include any C code, only preprocessor
definitions. This allows RegGen output to be used in assembly,
or any other format amenable to the C preprocessor. RegGen is
also not limited to a single processor architecture.

While RegGen's output can be used directly in C or C++ code,
it's intended to be used with a set of helper macros which make
register manipulation less verbose. These helper macros are not
part of the generated output, and are instead written by hand.

## Examples

Examples can be found for the STM32H743 and Ingenic X1000
processors in the `regs` directory. Run `make example` to
generate sample C header outputs in the `out` directory.

## Input format

RegGen input files should have the extension `.regs`. They may
contain both _type definitions_ and _instance definitions_ at
the top level.

### Whitespace and comments

Whitespace is not significant, and is only needed to separate
tokens. Both C-style line comments `//` and multi-line comments
`/* ... */` are supported and can occur anywhere in the input.

### Type definitions

There are three kinds of types: enums, registers, and blocks.
Enums are similar to C enums and are used to describe special
values of register fields. Registers are used to describe the
layout of fields in a machine register.

Blocks act as containers for registers, which are defined with
instances in the body of the block. These instances specify the
offset of a register relative to the block's base address. It's
also possible to define sub-blocks of registers inside a block,
by giving an instance a block type instead of a register type.

#### General syntax

The general form of a type definition is:

```
type NAME {
    // body of type goes here
}
```

where `type` is either `enum`, `reg`, or `block` depending on
the kind of type being declared. `NAME` is an identifier which
specifies the type's name.

#### Enum definitions

Enums are defined with the following syntax:

```
enum SWCLK {
    0 = HSI
    1 = CSI
    2 = HSE
    3 = PLL1P
}
```

Each enumerator has the value on the left hand side, which
must be a non-negative integer, and the name on the right
hand side of the `=`, unlike C.

#### Register definitions

Registers are defined with the following syntax:

```
reg REGISTER {
    // Untyped field occupying bits 15 through 20, inclusive.
    20 15 FIELD1

    // Field with an enum type 'TYPE', which must be previously defined.
    14 10 FIELD2 : TYPE

    // Field with inline enum; the 'enum' keyword is optional.
    09 08 FIELD3 : { 0 = E1; 1 = E2 }
    07 06 FIELD4 : enum { ... }

    // Single bit fields, occupying bits 5 and 4 only.
    // The '--' is only used to align the field names nicely.
    -- 05 FIELD5
    04 -- FIELD6

    // Single bit fields can be declared with a single number
    03 FIELD7

    // Leading zeros are only needed for alignment
    2 0 FIELD8
}
```

Each register field is defined by specifying its most significant
bit (MSB), least significant bit (LSB), name, and optionally an
enum type following the colon (`:`). Assigning a type which is not
an enum is an error.

Because many enums are only used for a single field it is allowed
to define an enum inline, shown in the definitions of `FIELD3` and
`FIELD4` above. An inline enum is equivalent to declaring an enum
separately with the same name as the field, like this:

```
reg REGISTER {
    enum FIELD3 { 0 = E1; 1 = E2 }

    09 08 FIELD3 : FIELD3
}
```

Field names only need to be unique within their register and may
have the same name as a type.

The examples in `FIELD5` to `FIELD7` show how to declare a single
bit register field. It is also possible to manually set the MSB
and LSB equal as in `04 04 FIELD6`, but this is error-prone so
should be avoided.

Fields must occupy a contiguous range of bits and not overlap any
other fields in the same register. They must also stay within the
bounds of the register word type. A register declared with the
`reg` keyword uses the machine word type specified on the RegGen
command line, but it is possible to use the alternative keywords
`reg8`, `reg16`, `reg32`, or `reg64` to explicitly set the width
of the register between 8- and 64-bits.

Unused bits in the register word are allowed, and remain anonymous.
There is no need to create dummy fields for padding bits.

#### Block definitions

Blocks are defined with the following syntax:

```
block BLOCK {
    // Single instance
    INST1 @ 0x00 : TYPE

    // Instance with inline register definition
    REG @ 0x04 : reg {
        15 00 FIELD
        // ...
    }

    // Instance with anonymous register
    ANON_REG @ 0x08 : reg

    // Instance with inline block definition
    INNER_BLOCK @ 0x10 : block {
        // Instances within the sub-block are relative
        // to the sub-block, not the outer block, so this
        // register will be at offset 0x10 + 0x04 = 0x14
        // relative to the outer block
        SUBREG @ 0x04 : reg
    }

    // Arrayed instance
    ARRAYINST @ 0x80 [4; 0x100] : TYPE
}
```

The name of an instance is given on the left hand side of the
at-sign (`@`) and the address offset is given on the right hand
side. The type assignment after the colon (`:`) is required, and
the type must be a previously defined register or block type.

##### Singular instances

_Singular instances_ are the most common, and have the form:

```
NAME @ OFFSET : TYPE
```

All the examples above except for `ARRAYINST` are singular
instances. Declaring a singular instance means that if you
have an pointer to `BLOCK`, then you can get a pointer to
whatever is referred to by `TYPE` by adding `OFFSET` to the
block pointer. This may be a pointer to a register, in which
case it can be dereferenced as a compatible integer type
(eg. `uint32_t`), or it may be another block, in which case
the pointer is only useful for further address calculations.

The type can be defined inline, similarly to inline definitions
of enums in register fields. This is again identical to creating
a separate type definition with the same name as the instance.
The type keyword is also required because the parser otherwise
can't determine whether the inline type is a register or block.

The `ANON_REG` example shows an anonymous register definition.
These behave a bit differently than a normal inline register
definition like `REG`: an anonymous register doesn't generate
any type name. It is also possible to use an explicit width
suffix on anonymous registers (`reg32`, etc.) to set the word
type of the register. Anonymous registers have no fields and
can only be accessed as bare machine words.

##### Arrayed instances

The `ARRAYINST` example shows an _arrayed instance_, which is
used to create instances of the same register or block type at
multiple related addresses. In the generated code the instance
macros will accept an index parameter to select which instance
is used. The general form for an arrayed instance is:

```
NAME @ OFFSET [COUNT; STRIDE] : TYPE
```

which declares an array of `COUNT` instances, the first one
at offset `OFFSET` and the n'th one at `OFFSET + (n) * STRIDE`.
There is no requirement that all of the instances actually be
implemented in hardware.

It is not possible to declare arrays with a non-uniform stride.
If you have several instances at unrelated addresses, then you
must declare multiple singular instances.

##### Block instance rules

Blocks don't perform any checking of instances, except that
the name of instances must be unique within the block. It is
allowed to have multiple instances at overlapping addresses.

This is sometimes useful when the hardware being described
maps multiple incompatible registers at the same address
(usually which one is mapped is controlled by a separate
register).

However, this lack of checking can be a source of errors, and
it is up to the user to avoid mistakes.

#### Nested type definitions

It is possible to define types within the body of another type.
This is only for convenience; all types exist in single global
namespace. All types are assigned a _fully qualified name_ based
on the following rules:

1. A type named `N` defined at top level has the fully qualified
   name `N`.
2. A type named `N` defined in the body of another type has
   the name `U_N`, where `U` is the fully qualified name of
   the enclosing type.

For example, in the snippet below:

```
block BLK {
    reg REG {
        enum ENUM {
            ...
        }
    }
}
```

the fully qualified name of each type is:

- `block BLK` is `BLK`
- `reg REG` is `BLK_REG`
- `enum ENUM` is `BLK_REG_ENUM`

Note that the same thing can be achieved by writing every type
at top level:

```
enum BLK_REG_ENUM { ... }
reg BLK_REG { ... }
block BLK { ... }
```

The fully qualified name of every type must be globally unique
in the input file. If this is not the case, RegGen will exit
and print an error message.

#### Type name resolution

Types can be referred to by name in various parts of the input
file, examples of which will be given below. However, it would
be cumbersome to write the fully qualified name of each type
every time you refer to it, so when a type is referred to within
the body of another type the parser will try several fully
qualified names automatically, ranging from most-specific to
least-specific.

The intuitive idea is that when you refer to a type with the
name `N`, the parser looks in the body of each syntactically
enclosing type starting from the innermost type, and searches
for the type named `N` until it finds a match.

The exact algorithm is as follows: if `N` is the identifier
used to refer to the type, let `X1`, `X2`, ..., `Xn` be the
syntactically enclosing types, as in the example below:

```
type X1 {
    type X2 {
        /* ... */
        ... {
            type Xn {
                // 'N' is used here
            }
        }
    }
}
```

The parser will construct the fully qualified name `X1_X2_..._Xn_N`
and look for a matching type. If no match is found, it will
strip off the innermost enclosing type and try again. For n=3
this search would look like:

1. `X1_X2_X3_N`
2. `X1_X2_N`
3. `X1_N`

The search stops once the parser finds a match. If no match is
found then `N` is tried directly.

#### Inclusion

To reduce repetition it is possible to include the contents of
one type in another. For example, sometimes several registers
will share most of their fields but have one or two unique ones
not present in the other registers -- copying and pasting the
same fields in multiple locations is tiresome and can be avoided
by using includes.

The syntax for an include is:

```
type TYPE {
    include OTHERTYPE
}
```

The identifier `OTHERTYPE` names the type whose contents will
be copied to `TYPE`. `OTHERTYPE` must be the same kind of type
as `TYPE`. Attempting to include a different kind of type, for
example trying to include an enum in a register, is an error.

The inclusion of other members is semantic, not syntactic or
textual: the resolved types of included members are unchanged,
and won't be re-resolved in the context of the including type.

### Root instance definitions

Instances that are defined at the top level, outside of any
block, are called _root instances_. They specify an absolute
memory address where a block or register exists.

Usually, a register description for a particular machine
will define a block type for each peripheral and use root
instances to define the address of each peripheral block.

The general form of a root instance definition is:

```
// Single instance
NAME @ ADDRESS : TYPE

// Arrayed instance
NAME @ ADDRESS [COUNT; STRIDE] : TYPE
```

The meaning is that if you cast the absolute address `ADDRESS`
to a pointer, then it points to whatever is defined by `TYPE`
(either a block or register). The address calculation for the
n'th member of an arrayed instance is `ADDRESS + (n) * STRIDE`.

## Output format

RegGen generates multiple C headers as outputs. One header
is generated for each type referenced by a root instance.
The header filename is the name of the type converted to
lowercase.

Types are then generated by recursing into each block type
accessible through an instance. Instances that point to a
register type will cause macros for that register type to
be generated.

Only types referenced by an instance will be generated.
"Free-floating" types are ignored, as no valid address
can be derived for them and thus they are treated as not
accessible.

Instances also generate several macros to define their
addresses/offsets, and their register type if any.

### Register macros

Each generated register will emit the following macro:

```
// Register word type
#define RTYPE_{REGNAME} uintN_t
```

The string `{REGNAME}` is replaced by the fully qualified
name of the register type. The `uintN_t` type is selected
to match the register width.

### Field macros

Each field in a generated register will emit several macros.

```
// Field mask
#define BM_{REGNAME}_{FIELDNAME} field_mask

// Field position
#define BP_{REGNAME}_{FIELDNAME} field_lsb

// Field value
#define BF_{REGNAME}_{FIELDNAME}(x)  ((x << field_lsb) & field_mask)

// Field value mask; argument 'x' is ignored
#define BFM_{REGNAME}_{FIELDNAME}(x) field_mask
```

where `field_mask` is an integer constant the same width
as the register type, with all bits in the field set to 1,
and all other bits set to 0. For example, a field occupying
bits 11 to 8 will have a mask of `0xf00`.

The `field_lsb` is the least significant bit number in
the field. A field occupying bits 11 to 8 would have the
value 8.

The string `{FIELDNAME}` is replaced by the name of the
field being generated.

If the field is assigned an enum type, then each member of
of the enum is emitted according to the following pattern:

```
// Field enum member
#define BV_{REGNAME}_{FIELDNAME}_{MEMBERNAME} member_value
```

`{MEMBERNAME}` is replaced by the name of the enum member
and `member_value` is its corresponding integer value. The
type name of the enum is ignored.

Fields assigned an enum type will also have a couple of
extra macros emitted:

```
// Field enum value
#define BF_{REGNAME}_{FIELDNAME}_V(e) \
    (BF_{REGNAME}_{FIELDNAME}(BV_{REGNAME}_{FIELDNAME}_##e))

// Field enum value mask; argument 'e' is ignored
#define BFM_{REGNAME}_{FIELDNAME}_V(e) \
    BM_{REGNAME}_{FIELDNAME}
```

thus the expression `BF_{REGNAME}_{FIELDNAME}_V(memb)` expands
to the value of the enum member `memb` shifted into the field's
position.

### Instance macros

Each instance reachable from the root instance will generate
a set of macros. Root instances will generate address macros,
while instances in blocks generate offset macros.
```
// Singular instance address
#define ITA_{INSTNAME} address

// Singular instance offset
#define ITO_{INSTNAME} offset

// Arrayed instance address
#define ITA_{INSTNAME}(i) (address + (i) * stride)

// Arrayed instance offset
#define ITO_{INSTNAME}(i) (offset + (i) * stride)

// Instance type name; only defined if instance has register type.
// Has same arguments (or lack thereof) as the 'ITA_' / 'ITO_' macros.
// Not defined if the instance register is anonymous.
#define ITNA_{INSTNAME}(...) {TYPENAME}
#define ITNO_{INSTNAME}(...) {TYPENAME}

// Instance access type; only defined if instance has register type.
// Has same arguments (or lack thereof) as the 'ITA_' / 'ITO_' macros.
// If the instance register is anonymous, then expands directly to
// a uintN_t type matching the width of the anonymous register.
#define ITTA_{INSTNAME}(...) RTYPE_{TYPENAME}
#define ITTO_{INSTNAME}(...) RTYPE_{TYPENAME}
```

The string `{INSTNAME}` is replaced by the instance name.
For root instances, this is the name used when the instance
is defined while for block instances this is prefixed by the
string `{BLOCKNAME}_`, where `{BLOCKNAME}` is the fully
qualified name of the block type containing the instance.
(For example instance `I` in block `B` has `{INSTNAME} = B_I`).

The string `{TYPENAME}` is replaced by the fully qualified
name of the instance type.

An instance defined in a block will also generate an address
macro if it is reachable through a unique path from a root
instance. That is, it must have an address that is calculated
by applying successive offsets from a single root instance.
For the purposes of this check, an arrayed instance counts as
a single node in the path, even though it generates multiple
addresses. An instance which is accessible in this way has an
absolute address that is calculable using only integer
multiplication and integer constants, without further input.

This means that the `ITA_{INSTNAME}` macro may have more
than one index argument for instances defined in sub-blocks.
One index argument is required for each arrayed instance in
the unique path. The order of arguments goes from the root
down, ie. the first index argument is used to address the
first arrayed instance in the path, nearest to the root.

## License

Copyright (C) 2025 Aidan MacDonald.

The RegGen utility source code is distributed under the
terms of the GNU GPLv3, or at your option, any later version;
see `LICENSE-GPLv3.txt`.

Register definition files in the `regs` directory are
distributed under the terms of the Creative Commons CC0
V1.0 license; see `LICENSE-CC0.txt`.

For avoidance of doubt, the source code license does *not*
apply to any C source code emitted by RegGen, only to the
source code of the RegGen utility itself.
