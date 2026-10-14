#!/usr/bin/env perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright (C) 2026 Marcin Bukat
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
# Minimal ZSP500 assembler.
#
# Encodings come from zsp500.isa, which was recovered from the disassembler
# in the vendor's ZSP debugger DLL (abizsp.dll). The accepted syntax is
# exactly what that disassembler prints, with free spacing, e.g.
#
#     start:  movhw a7,hi(RESULT)
#             movlw a7,lo(RESULT)
#             mov r0,0x1234         ; 32-bit F800 form
#             st r0,a7
#             bz start
#     loop:   br loop
#
# Directives: .org .equ (or NAME = expr) .word/.dw .long .space .align .include
#             .section imem|dmem  switch between program and data memory
#             .reserve n          skip n halfwords without emitting data (bss)
#             .export a, b        symbols written to the C header (--header)
# Expressions: + - * / % << >> & | ^ ~ ( ), hi(x), lo(x), '.' = current address.
# A '.l' suffix on the mnemonic forces the 32-bit form (e.g. 'bz.l label').
#
# Outputs: raw IMEM image (-o), listing (-l), loadable image with IMEM and
# DMEM sections (--image, format at image_bytes()), the same image as a C
# array (--c), and a C header with the exported symbols (--header).
#
# Addresses count 16-bit halfwords by default (--addr-unit hw); --addr-unit
# byte counts bytes instead. Branch offsets are relative to the branch's own
# address, as in the abizsp.dll disassembler.

use strict;
use warnings;
use integer;
use File::Basename;
use File::Spec;
use Getopt::Long qw(:config no_ignore_case bundling);

my $DEFAULT_ISA = File::Spec->catfile(dirname(File::Spec->rel2abs($0)), 'zsp500.isa');

# Errors: die with "message\n". An operand that does not fit its field dies
# with \"range" so that the encoder can try the next form.

sub sext {
    my ($v, $bits) = @_;
    $v &= (1 << $bits) - 1;
    return $v >> ($bits - 1) ? $v - (1 << $bits) : $v;
}

# Python-style (floor) division and modulo
sub fdiv {
    my ($a, $b) = @_;
    die "division by zero\n" unless $b;
    my $q = $a / $b;
    $q-- if $q * $b != $a && (($a < 0) != ($b < 0));
    return $q;
}

sub fmod { my ($a, $b) = @_; return $a - $b * fdiv($a, $b); }

# "0x" and hex digits of $v, at least $width of them (a negative value
# gets a '-' after the 0x, counted in the width)
sub hexfmt {
    my ($width, $v) = @_;
    return sprintf('0x%0*x', $width, $v) if $v >= 0;
    return sprintf('0x-%0*x', $width > 1 ? $width - 1 : 1, -$v);
}

sub strip { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s; }

# ---------------------------------------------------------------- ISA spec

my $PH_RE = qr/\{(?<mul>[24]?)(?<f>[a-z])(?<add>\+\d+)?(?::(?<kind>\w+(?:\+\d+)?))?\}/;

# literal syntax text -> regex text: a space needs whitespace, punctuation
# allows it around
sub lit_re {
    my $out = '';
    for my $ch (split //, shift) {
        if ($ch eq ' ') {
            $out .= '\s+' unless $out =~ /\\s\*\z/;
        } elsif (index(',(){}!', $ch) >= 0) {
            $out =~ s/\\s\+\z//;
            $out .= '\s*' . quotemeta($ch) . '\s*';
        } else {
            $out .= quotemeta($ch);
        }
    }
    return $out;
}

sub new_spec {
    my ($pattern, $syntax, $tables, $lineno) = @_;
    (my $bits = $pattern) =~ s/ //g;
    my $len = length $bits;
    die "isa line $lineno: bad pattern\n" unless $len == 16 || $len == 32;
    my $s = { size => $len / 16, syntax => $syntax, mask => 0, val => 0,
              fields => {}, phs => [] };
    for my $i (0 .. $len - 1) {
        my $ch = substr($bits, $i, 1);
        # chars 0..15 -> inst bits 15..0, chars 16..31 -> inst bits 31..16
        my $bit = $i < 16 ? 15 - $i : 31 - ($i - 16);
        if ($ch eq '0' || $ch eq '1') {
            $s->{mask} |= 1 << $bit;
            $s->{val} |= $ch << $bit;
        } elsif ($ch ne 'x') {
            push @{$s->{fields}{$ch}}, $bit;
        }
    }
    $s->{mnemonic} = $syntax =~ /^\{/ ? '' : lc((split ' ', $syntax)[0]);
    my $rx = '';
    my $pos = 0;
    while ($syntax =~ /$PH_RE/g) {
        my ($start, $end) = ($-[0], $+[0]);
        my %m = %+;
        my $lit = substr($syntax, $pos, $start - $pos);
        $lit =~ s/0x\z//;               # hex prefix belongs to the number
        $rx .= lit_re($lit);
        my $kind = $m{kind} // '';
        my $isreg = length($lit) >= 1 && index('rang', substr($lit, -1)) >= 0 &&
                    (length($lit) == 1 || substr($lit, -2, 1) !~ /[A-Za-z0-9]/) &&
                    $kind eq '';
        if ($isreg) {
            $rx .= '(\d+)';
        } elsif ($kind =~ /^rel/ || $kind eq '' || $kind eq 's') {
            $rx .= '([^,{}]+?)';
        } else {
            $rx .= '(%?[\w.\-+]+)';
        }
        push @{$s->{phs}}, { f => $m{f}, mul => $m{mul}, add => $m{add} // '',
                             kind => $kind, isreg => $isreg };
        $pos = $end;
    }
    $rx .= lit_re(substr($syntax, $pos));
    $s->{regex} = qr/^\s*$rx\s*$/i;
    for my $ph (@{$s->{phs}}) {
        die "isa line $lineno: field $ph->{f} not in pattern\n"
            unless $s->{fields}{$ph->{f}};
        my $k = $ph->{kind};
        if ($k ne '' && $k !~ /^rel/ && $k ne 's') {
            (my $t = $k) =~ s/\+.*//;
            die "isa line $lineno: unknown table $t\n" unless $tables->{$t};
        }
    }
    return $s;
}

# value -> field bits, MSB first in pattern order
sub spec_put {
    my ($s, $inst, $f, $v) = @_;
    my $bits = $s->{fields}{$f};
    my $n = @$bits;
    die \"range" if $v < 0 || $v >= (1 << $n);
    for my $i (0 .. $n - 1) {
        $inst |= 1 << $bits->[$i] if ($v >> ($n - 1 - $i)) & 1;
    }
    return $inst;
}

sub load_isa {
    my $path = shift;
    open my $fh, '<', $path or die "cannot open $path: $!\n";
    my @lines = <$fh>;
    close $fh;
    chomp @lines;
    my %tables;
    for (@lines) {
        $tables{$1} = [split ' ', $2] if strip($_) =~ /^@(\w+)\s*=\s*(.*)$/;
    }
    my @specs;
    for my $i (0 .. $#lines) {
        my $ln = strip($lines[$i]);
        next if $ln eq '' || $ln =~ /^[#@]/;
        my ($pat, $syn) = map { strip($_) } split /\|/, $ln, 2;
        push @specs, new_spec($pat, $syn, \%tables, $i + 1);
    }
    # 16-bit forms first, otherwise file order
    my @order = sort { $specs[$a]{size} <=> $specs[$b]{size} || $a <=> $b } 0 .. $#specs;
    return ([@specs[@order]], \%tables);
}

# ---------------------------------------------------------------- expressions

# Recursive descent with Python's precedence: | ^ & << >> + - * / % unary
# Returns the value, or undef when a symbol is still undefined (only outside
# the final pass, $strict false).

my %FUNCS = (hi => sub { ($_[0] >> 16) & 0xffff }, lo => sub { $_[0] & 0xffff });
my @PREC = (['|'], ['^'], ['&'], ['<<', '>>'], ['+', '-'], ['*', '/', '%', '//']);

sub expr_eval {
    my ($syms, $text, $pc, $strict) = @_;
    my $t = strip($text);
    my @tok;
    while (length $t) {
        if ($t =~ s/^\s+//) {
        } elsif ($t =~ s/^0[xX]([0-9a-fA-F_]+)h?(?![\w])//) {
            (my $h = $1) =~ s/_//g;
            push @tok, ['n', hex $h];
        } elsif ($t =~ s/^0[bB]([01_]+)(?![\w])//) {
            (my $b = $1) =~ s/_//g;
            push @tok, ['n', oct "0b$b"];
        } elsif ($t =~ s/^0[oO]([0-7_]+)(?![\w])//) {
            (my $o = $1) =~ s/_//g;
            push @tok, ['n', oct $o];
        } elsif ($t =~ s/^(\d[\d_]*)(?![\w.])//) {
            (my $d = $1) =~ s/_//g;
            push @tok, ['n', 0 + $d];
        } elsif ($t =~ s/^([A-Za-z_]\w*)//) {
            push @tok, ['id', $1];
        } elsif ($t =~ s/^\.(?![\w])//) {
            push @tok, ['pc'];
        } elsif ($t =~ s/^(<<|>>|\/\/|[-+*\/%&|^~()])//) {
            push @tok, ['op', $1];
        } else {
            die "bad expression: $text\n";
        }
    }
    my $i = 0;
    my $undef = 0;
    my $peek = sub { $i < @tok && $tok[$i][0] eq 'op' ? $tok[$i][1] : '' };
    my ($binary, $unary);
    $binary = sub {
        my $lvl = shift;
        return $unary->() if $lvl > $#PREC;
        my $v = $binary->($lvl + 1);
        while (grep { $_ eq $peek->() } @{$PREC[$lvl]}) {
            my $op = $tok[$i++][1];
            my $r = $binary->($lvl + 1);
            if    ($op eq '|')  { $v |= $r }
            elsif ($op eq '^')  { $v ^= $r }
            elsif ($op eq '&')  { $v &= $r }
            elsif ($op eq '<<') { $v <<= $r }
            elsif ($op eq '>>') { $v >>= $r }
            elsif ($op eq '+')  { $v += $r }
            elsif ($op eq '-')  { $v -= $r }
            elsif ($op eq '*')  { $v *= $r }
            elsif ($op eq '%')  { $v = fmod($v, $r) }
            else                { $v = fdiv($v, $r) }
        }
        return $v;
    };
    $unary = sub {
        my $op = $peek->();
        if ($op eq '-' || $op eq '+' || $op eq '~') {
            $i++;
            my $v = $unary->();
            return $op eq '-' ? -$v : $op eq '+' ? $v : -$v - 1;
        }
        die "bad expression: $text\n" if $i >= @tok;
        my $tk = $tok[$i++];
        if ($tk->[0] eq 'n') {
            return $tk->[1];
        } elsif ($tk->[0] eq 'pc') {
            return $pc;
        } elsif ($tk->[0] eq 'op' && $tk->[1] eq '(') {
            my $v = $binary->(0);
            die "bad expression: $text\n" unless $peek->() eq ')';
            $i++;
            return $v;
        } elsif ($tk->[0] eq 'id') {
            my $name = $tk->[1];
            if ($peek->() eq '(') {
                die "unsupported expression element\n" unless $FUNCS{$name};
                $i++;
                my $v = $binary->(0);
                die "bad expression: $text\n" unless $peek->() eq ')';
                $i++;
                return $FUNCS{$name}->($v);
            }
            return $syms->{$name} if exists $syms->{$name};
            die "undefined symbol: $name\n" if $strict;
            $undef = 1;
            return 0;
        }
        die "bad expression: $text\n";
    };
    my $v = $binary->(0);
    die "bad expression: $text\n" if $i < @tok;
    return $undef ? undef : $v;
}

# ---------------------------------------------------------------- encoder

my ($SPECS, $TABLES, %BY_MN, $UNIT);

sub candidates {
    my $text = shift;
    my @w = split ' ', $text;
    my $mn = @w ? lc $w[0] : '';
    my $force32 = 0;
    if ($mn =~ /\.l\z/ && !$BY_MN{$mn}) {
        $force32 = 1;
        my ($first, $rest) = split ' ', $text, 2;
        $text = substr($first, 0, -2) . (defined $rest ? " $rest" : '');
        $mn = substr($mn, 0, -2);
    }
    my @c = (@{$BY_MN{$mn} || []}, @{$BY_MN{''} || []});
    @c = grep { $_->{size} == 2 } @c if $force32;
    return ($text, @c);
}

# $ev->(expr text) -> value or undef. Returns the halfwords.
sub encode {
    my ($text, $pc, $ev) = @_;
    my @cands;
    ($text, @cands) = candidates($text);
    my @errs;
    for my $s (@cands) {
        my @g = $text =~ $s->{regex} or next;
        @g = () unless @{$s->{phs}};
        my $inst = eval { fill($s, \@g, $pc, $ev) };
        if (!defined $inst) {
            my $e = $@;
            push @errs, ref $e ? "$s->{syntax}: operand out of range"
                               : "$s->{syntax}: " . ($e =~ s/\n\z//r);
            next;
        }
        return $s->{size} == 1 ? ($inst & 0xffff) : ($inst & 0xffff, ($inst >> 16) & 0xffff);
    }
    die 'no encoding fits: ' . join('; ', @errs[0 .. ($#errs < 2 ? $#errs : 2)]) . "\n" if @errs;
    die 'unknown instruction: ' . strip($text) . "\n";
}

sub fill {
    my ($s, $g, $pc, $ev) = @_;
    my $inst = $s->{val};
    my ($cond, $rel);
    for my $k (0 .. $#{$s->{phs}}) {
        my $ph = $s->{phs}[$k];
        my $txt = strip($g->[$k]);
        my ($f, $kind) = ($ph->{f}, $ph->{kind});
        if ($kind =~ /^rel/) {
            $rel = [$f, $kind, $txt];
            next;
        }
        if ($kind ne '' && $kind ne 's') {
            my ($tname, $off) = split /\+/, $kind, 2;
            my $idx = lookup($TABLES->{$tname}, $txt, $ev);
            $cond = $idx if $tname eq 'br_cond';
            $inst = spec_put($s, $inst, $f, $idx - ($off // 0));
            next;
        }
        my $v = $ph->{isreg} ? 0 + $txt : ($ev->($txt) // 0);
        my $n = @{$s->{fields}{$f}};
        if ($ph->{mul} ne '') {
            die "register r$v not aligned to $ph->{mul}\n" if $v % $ph->{mul};
            $v /= $ph->{mul};
        }
        $v -= substr($ph->{add}, 1) if $ph->{add} ne '';
        if ($kind eq 's') {
            my ($lo, $hi) = (-(1 << ($n - 1)), 1 << ($n - 1));
            if ($v >= $lo && $v < $hi) {
                $v &= (1 << $n) - 1;
            } elsif ($v >= 0 && $v <= 0xffff && sext($v, 16) >= $lo && sext($v, 16) < $hi) {
                $v = sext($v, 16) & ((1 << $n) - 1);
            } elsif (!($n == 16 && $v >= 0 && $v <= 0xffff)) {
                die \"range";
            }
        }
        $inst = spec_put($s, $inst, $f, $v);
    }
    if ($rel) {
        my ($f, $kind, $txt) = @$rel;
        my $tgt = $ev->($txt);
        # targets wrap at 24 bits, as in the disassembler
        my $off = defined $tgt ? sext(fdiv($tgt - $pc, $UNIT), 24) : 0;
        die "misaligned branch target\n" if defined $tgt && fmod($tgt - $pc, $UNIT);
        my $v;
        if ($kind eq 'rel8') {
            if (defined $cond && $cond <= 5) {
                # agnX: always backward, offset - 256
                die "agn target must be 1..256 back\n"
                    if defined $tgt && !($off >= -256 && $off <= -1);
                $v = ($off + 256) & 0xff;
            } else {
                die "branch out of range\n" unless $off >= -128 && $off <= 127;
                $v = $off & 0xff;
            }
        } else {
            my $c = (($inst & 0xffff) >> 4) & 0xf;
            if ($c >= 2 && $c <= 5) {           # agn0..agn3
                die "agn only branches backward\n"
                    if defined $tgt && !($off >= -65536 && $off <= -1);
                $v = ($off + 65536) & 0xffff;
            } else {
                die "branch out of range\n" unless $off >= -32768 && $off <= 32767;
                $v = $off & 0xffff;
            }
        }
        $inst = spec_put($s, $inst, $f, $v);
    }
    return $inst;
}

sub lookup {
    my ($tab, $txt, $ev) = @_;
    my $t = lc $txt;
    for my $i (0 .. $#$tab) {
        return $i if $tab->[$i] ne '-' && lc($tab->[$i]) eq $t;
    }
    my $v = eval { $ev->($txt) };
    if (defined $v) {
        for my $i (0 .. $#$tab) {
            return $i if $tab->[$i] =~ /^-?\d+\z/ && $tab->[$i] == $v;
        }
    }
    die "\"$txt\" not valid here\n";
}

# ---------------------------------------------------------------- assembler

my (@INCDIRS, @EXPORTS, %DMEM);

sub find_include {
    my ($name, $here) = @_;
    for my $d (dirname($here), @INCDIRS) {
        my $p = File::Spec->catfile($d, $name);
        return $p if -e $p;
    }
    die "cannot find include file $name\n";
}

# -> list of [path, line number, raw line, line without comment]
sub read_src {
    my ($path, $out, $depth) = @_;
    die "include nesting too deep\n" if $depth > 8;
    open my $fh, '<', $path or die "cannot open $path: $!\n";
    my @raw = <$fh>;
    close $fh;
    for my $i (0 .. $#raw) {
        (my $raw = $raw[$i]) =~ s/\n\z//;
        my ($line) = split /;|\/\//, $raw, 2;
        $line //= '';
        if ($line =~ /^\s*\.include\s+"([^"]+)"/) {
            read_src(find_include($1, $path), $out, $depth + 1);
            next;
        }
        push @$out, [$path, $i + 1, $raw, $line];
    }
    return $out;
}

# split on commas outside parentheses
sub split_args {
    my $s = shift;
    my ($depth, $cur, @out) = (0, '');
    for my $ch (split //, $s) {
        $depth++ if $ch eq '(';
        $depth-- if $ch eq ')';
        if ($ch eq ',' && $depth == 0) {
            push @out, $cur;
            $cur = '';
        } else {
            $cur .= $ch;
        }
    }
    push @out, $cur;
    return map { strip($_) } @out;
}

# 'pst <store>': the store, then two stores of the same register to
# a7 + STORE_DUMMY, if STORE_LAG is non-zero (default 0). With the DSP
# peripheral block not reset (RSTCFG bit 5), the RK27xx DSP writes each
# store's data to the address of the store two before it; the dummy stores
# push a store out. With STORE_LAG = 0 'pst' is a plain store.
sub pst {
    my ($store, $pc, $ev, $syms) = @_;
    $store =~ /^(\w+)(?:\.l)?\s+([rna]\d+)/i or die "pst needs a store instruction\n";
    my ($mn, $reg) = (lc $1, $2);
    my $dummy = $mn =~ /^(std|pushd|popd)/ ? 'std' : 'st';
    my @words = encode($store, $pc, $ev);
    if ($syms->{STORE_LAG}) {
        for (1 .. 2) {
            push @words, encode("$dummy $reg,a7,STORE_DUMMY", $pc + @words * $UNIT, $ev);
        }
    }
    return @words;
}

sub same_labels {
    my ($a, $b) = @_;
    return 0 unless keys %$a == keys %$b;
    for (keys %$a) {
        return 0 unless exists $b->{$_} && $a->{$_} == $b->{$_};
    }
    return 1;
}

# one pass over the source -> (imem hash, listing, labels)
sub asm_pass {
    my ($lines, $syms, $strict) = @_;
    my $sec = 'imem';
    my %pcs = (imem => 0, dmem => 0);
    my %mems = (imem => {}, dmem => {});
    my $pc = 0;
    my $mem = $mems{$sec};
    my (@lst, %labels);
    @EXPORTS = ();
    for my $l (@$lines) {
        my ($path, $ln, $raw, $line) = @$l;
        eval {
            my $label = $line =~ /^\s*([A-Za-z_]\w*):(.*)$/;
            if ($label) {
                $labels{$1} = $pc;
                $syms->{$1} = $pc;
                $line = $2;
            }
            if ($line !~ /\S/) {
                push @lst, [$pc, [], $raw, $sec] if $label;
                return;
            }
            if ($line =~ /^\s*([A-Za-z_]\w*)\s*=\s*(.+)$/ ||
                $line =~ /^\s*\.equ\s+([A-Za-z_]\w*)\s*,?\s*(.+)$/) {
                my $name = $1;
                my $v = expr_eval($syms, $2, $pc, $strict);
                if (defined $v) {
                    $syms->{$name} = $v;
                    $labels{$name} = $v;
                }
                return;
            }
            my @words;
            my $ev = sub { expr_eval($syms, $_[0], $pc, $strict) };
            if ($line =~ /^\s*(\.\w+)\s*(.*)$/) {
                my ($name, $arg) = (lc $1, $2);
                my @args = $arg =~ /\S/ ? split_args($arg) : ();
                if ($name eq '.section') {
                    die "unknown section $args[0]\n" unless exists $pcs{$args[0]};
                    $pcs{$sec} = $pc;
                    $sec = $args[0];
                    ($pc, $mem) = ($pcs{$sec}, $mems{$sec});
                    return;
                } elsif ($name eq '.export') {
                    push @EXPORTS, @args;
                    return;
                } elsif ($name eq '.reserve') {
                    $pc += ($ev->($args[0]) // 0) * $UNIT;
                    return;
                } elsif ($name eq '.org') {
                    $pc = $ev->($args[0]);
                    return;
                } elsif ($name eq '.align') {
                    my $a = $ev->($args[0]);
                    while (fmod($pc, $a)) {
                        $mem->{$pc} = 0;
                        $pc += $UNIT;
                    }
                    return;
                } elsif ($name eq '.word' || $name eq '.dw' || $name eq '.hword') {
                    @words = map { ($ev->($_) // 0) & 0xffff } @args;
                } elsif ($name eq '.long') {
                    for (@args) {
                        my $v = $ev->($_) // 0;
                        push @words, $v & 0xffff, ($v >> 16) & 0xffff;
                    }
                } elsif ($name eq '.space') {
                    @words = (0) x $ev->($args[0]);
                } else {
                    die "unknown directive $name\n";
                }
            } elsif ($sec ne 'imem') {
                die "instructions are only allowed in the imem section\n";
            } elsif ($line =~ /^\s*pst\s/) {
                @words = pst(strip(substr(strip($line), 3)), $pc, $ev, $syms);
            } else {
                @words = encode(strip($line), $pc, $ev);
            }
            push @lst, [$pc, [@words], $raw, $sec];
            for my $w (@words) {
                die sprintf("overlapping code at 0x%X\n", $pc) if exists $mem->{$pc} && $strict;
                $mem->{$pc} = $w;
                $pc += $UNIT;
            }
            1;
        } or do {
            my $e = $@;
            if ($e) {
                $e = "operand out of range\n" if ref $e;
                die basename($path) . ":$ln: $e";
            }
        };
    }
    %DMEM = %{$mems{dmem}};
    return ($mems{imem}, \@lst, \%labels);
}

sub assemble {
    my ($path, $defines) = @_;
    my $lines = read_src($path, [], 0);
    my %prev;
    for my $npass (0 .. 19) {
        my %syms = (%$defines, %prev);
        my (undef, undef, $labels) = asm_pass($lines, \%syms, 0);
        if ($npass && same_labels($labels, \%prev)) {
            %syms = (%$defines, %$labels);
            return asm_pass($lines, \%syms, 1);
        }
        %prev = %$labels;
    }
    die "labels do not converge\n";
}

# flat halfword list from address 0 (gaps zero-filled)
sub image {
    my $mem = shift;
    return () unless %$mem;
    my ($top) = sort { $b <=> $a } keys %$mem;
    return map { $mem->{$_ * $UNIT} // 0 } 0 .. $top / $UNIT;
}

# contiguous runs of emitted halfwords: [start halfword, [halfwords]]
sub runs {
    my $mem = shift;
    my @out;
    for my $a (sort { $a <=> $b } keys %$mem) {
        my $hw = $a / $UNIT;
        if (@out && $out[-1][0] + @{$out[-1][1]} == $hw) {
            push @{$out[-1][1]}, $mem->{$a};
        } else {
            push @out, [$hw, [$mem->{$a}]];
        }
    }
    return @out;
}

# Loadable image, little-endian:
#   char magic[4] = "ZSP1"; u16 version = 1; u16 nsect; u32 reserved = 0;
#   nsect x { u16 kind (0 IMEM, 1 DMEM); u16 reserved; u32 start (halfwords);
#             u32 count (halfwords); u16 data[count]; padding to 4 bytes }
# The core starts at IMEM 0. DMEM not covered by a section is undefined.
sub image_bytes {
    my ($imem, $dmem) = @_;
    my @sects = ((map { [0, @$_] } runs($imem)), (map { [1, @$_] } runs($dmem)));
    my $out = 'ZSP1' . pack('vvV', 1, scalar @sects, 0);
    for (@sects) {
        my ($kind, $st, $d) = @$_;
        $out .= pack('vvVV', $kind, 0, $st, scalar @$d) . pack('v*', @$d);
        $out .= "\0" x (4 - length($out) % 4) if length($out) % 4;
    }
    return $out;
}

sub write_file {
    my ($path, $data, $binary) = @_;
    open my $fh, '>', $path or die "cannot write $path: $!\n";
    binmode $fh if $binary;
    print $fh $data;
    close $fh or die "cannot write $path: $!\n";
}

sub c_array {
    my ($name, $data, $header) = @_;
    my @words = unpack('V*', $data);
    my $out = "/* Generated by tools/zsp/zspasm.pl - do not edit. */\n";
    $out .= sprintf("#include \"%s\"\n", basename($header)) if $header;
    $out .= "#include <stdint.h>\n#include <stddef.h>\n\n";
    $out .= sprintf("const uint32_t %s[%d] __attribute__((aligned(4))) = {\n", $name, scalar @words);
    for (my $i = 0; $i < @words; $i += 6) {
        my $e = $i + 5 < $#words ? $i + 5 : $#words;
        $out .= '    ' . join(', ', map { sprintf '0x%08x', $_ } @words[$i .. $e]) . ",\n";
    }
    $out .= sprintf("};\nconst size_t %s_size = %d;\n", $name, length $data);
    return $out;
}

sub c_header {
    my ($path, $name, $prefix, $labels) = @_;
    (my $guard = uc basename($path)) =~ s/\W/_/g;
    my $out = "/* Generated by tools/zsp/zspasm.pl - do not edit. */\n";
    $out .= "#ifndef $guard\n#define $guard\n\n#include <stdint.h>\n#include <stddef.h>\n\n";
    $out .= "extern const uint32_t ${name}[];\nextern const size_t ${name}_size;\n\n" if defined $name;
    for my $e (@EXPORTS) {
        die "exported symbol $e is not defined\n" unless exists $labels->{$e};
        $out .= sprintf("#define %s%s %s\n", $prefix, $e, hexfmt(4, $labels->{$e}));
    }
    return $out . "\n#endif\n";
}

sub usage {
    print STDERR <<"EOF";
usage: $0 [options] SRC
  -o, --bin FILE      raw little-endian IMEM image starting at address 0
  -l, --lst FILE      listing file
      --lua FILE      Lua file returning {halfwords}
      --image FILE    loadable IMEM+DMEM image (see image_bytes())
      --c FILE        the loadable image as a C array
      --c-name NAME   array name for --c (default zsp_image)
      --header FILE   C header with the .export symbols
      --prefix P      macro prefix for --header (default ZSP_)
  -I DIR              include directory
  -D NAME[=VALUE]     define a symbol (default 1)
  -q, --quiet
      --isa FILE      encoding table (default zsp500.isa next to this script)
      --addr-unit hw|byte
EOF
    exit 2;
}

my %o = ('c-name' => 'zsp_image', prefix => 'ZSP_', isa => $DEFAULT_ISA, 'addr-unit' => 'hw');
my @defs;
GetOptions(\%o, 'bin|o=s', 'lst|l=s', 'lua=s', 'image=s', 'c=s', 'c-name=s', 'header=s',
           'prefix=s', 'I=s' => \@INCDIRS, 'D=s' => \@defs, 'quiet|q', 'isa=s',
           'addr-unit=s') or usage();
usage() unless @ARGV == 1 && $o{'addr-unit'} =~ /^(hw|byte)\z/;
my $src = $ARGV[0];

my %defines;
for (@defs) {
    my ($k, $v) = split /=/, $_, 2;
    $v = 1 unless defined $v && length $v;
    $defines{$k} = $v =~ /^0[xXbBoO]/ ? oct($v =~ s/^0[oO]/0/r) : 0 + $v;
}

my ($mem, $lst, $labels);
eval {
    ($SPECS, $TABLES) = load_isa($o{isa});
    push @{$BY_MN{$_->{mnemonic}}}, $_ for @$SPECS;
    $UNIT = $o{'addr-unit'} eq 'hw' ? 1 : 2;
    ($mem, $lst, $labels) = assemble($src, \%defines);
    1;
} or do { print STDERR "$src: error: $@"; exit 1; };

my @img = image($mem);
(my $base = $src) =~ s/\.[^.\/]*\z//;
my $only_c = $o{c} || $o{image} || $o{header};
if ($o{bin} || !$only_c) {
    write_file($o{bin} || "$base.bin", pack('v*', @img), 1);
}
if ($o{lst} || !$only_c) {
    my $out = '';
    for (@$lst) {
        my ($pc, $words, $raw, $sec) = @$_;
        if (@$words || $raw =~ /\S/) {
            $out .= sprintf("%s%06X  %-10s %s\n", $sec eq 'dmem' ? 'D' : ' ', $pc,
                            join(' ', map { sprintf '%04X', $_ } @$words), $raw);
        } else {
            $out .= "\n";
        }
    }
    $out .= "\n; symbols\n";
    $out .= sprintf("; %-20s %s\n", $_, uc(hexfmt(1, $labels->{$_})) =~ s/^0X/0x/r) for sort keys %$labels;
    write_file($o{lst} || "$base.lst", $out);
}
my $data = image_bytes($mem, \%DMEM);
write_file($o{image}, $data, 1) if $o{image};
if ($o{header}) {
    my $h = eval { c_header($o{header}, $o{c} ? $o{'c-name'} : undef, $o{prefix}, $labels) };
    unless (defined $h) { print STDERR "$src: error: $@"; exit 1; }
    write_file($o{header}, $h);
}
write_file($o{c}, c_array($o{'c-name'}, $data, $o{header})) if $o{c};
if ($o{lua}) {
    my $out = "return {\n";
    for (my $i = 0; $i < @img; $i += 8) {
        my $e = $i + 7 < $#img ? $i + 7 : $#img;
        $out .= '  ' . join(', ', map { sprintf '0x%04X', $_ } @img[$i .. $e]) . ",\n";
    }
    write_file($o{lua}, $out . "}\n");
}
printf "%s: %d IMEM halfwords, %d DMEM halfwords\n", $src, scalar @img, scalar keys %DMEM
    unless $o{quiet};
