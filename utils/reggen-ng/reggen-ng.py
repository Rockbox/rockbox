#!/usr/bin/python3
#
# Tool to generate XML files for Rockbox regtools. The syntax is pretty simple
# and much less verbose than XML; see the '.reggen' files next to this script.
#
# Currently this tool expects a 'reggen' file on standard input and writes the
# equivalent XML to standard output.
#
# TODO: document this better and improve the command line usage

from lxml import etree as ET
from lxml.builder import E
import functools

class Soc:
    def __init__(self):
        self.name = None
        self.title = None
        self.desc = None
        self.isa = None
        self.version = None
        self.authors = []
        self.nodes = []

    def gen(self):
        node = E("soc", version="2")
        if self.name is not None:
            node.append(E("name", self.name))
        if self.desc is not None:
            node.append(E("desc", self.desc))
        if self.isa is not None:
            node.append(E("isa", self.isa))
        if self.isa is not None:
            node.append(E("version", self.version))
        for a in self.authors:
            node.append(E("author", a))
        for n in self.nodes:
            node.append(n.gen())
        return node

class Node:
    def __init__(self, name):
        self.name = name
        self.title = None
        self.desc = None
        self.instances = []
        self.children = []
        self.register_width = None
        self.fields = []

    def gen(self):
        node = E("node", E("name", self.name))

        if self.title is not None:
            node.append(E("title", self.title))

        if self.desc is not None:
            node.append(E("desc", self.desc))

        for i in self.instances:
            node.append(i.gen())

        for c in self.children:
            node.append(c.gen())

        if self.register_width is not None:
            reg = E("register",
                    E("width", self.register_width))
            for f in self.fields:
                reg.append(f.gen())
            node.append(reg)

        return node

class Instance:
    def __init__(self, name, addr, stride=None, count=None):
        self.name = name
        self.address = addr
        self.stride = stride
        self.count = count

    def gen(self):
        node = E("instance", E("name", self.name))

        if self.stride is not None:
            node.append(E("range",
                          E("first", "0"),
                          E("count", self.count),
                          E("base", self.address),
                          E("stride", self.stride)))
        else:
            node.append(E("address", self.address))

        return node

class Field:
    def __init__(self, name, msb, lsb):
        self.name = name
        self.desc = None
        self.msb = int(msb)
        self.lsb = int(lsb)
        self.enums = []

    def gen(self):
        node = E("field",
                 E("name", self.name),
                 E("position", "%d" % self.lsb))

        if self.desc is not None:
            node.append(E("desc", self.desc))

        if self.msb != self.lsb:
            node.append(E("width", str(self.msb - self.lsb + 1)))

        for n, v in self.enums:
            node.append(E("enum", E("name", n), E("value", v)))

        return node

class Variant:
    def __init__(self, ty, offset):
        self.type = ty
        self.offset = offset

    def gen(self):
        return E("variant",
                 E("type", self.type),
                 E("offset", self.offset))

class Parser:
    def __init__(self, f):
        self.input = f
        self.line = 1
        self.eolstop = False
        self.next()

    def parse(self):
        self.skipws()
        results = self.parse_block_inner({
            'name': (self.parse_string, 1),
            'title': (self.parse_string, 1),
            'desc': (self.parse_string, 1),
            'isa': (self.parse_string, 1),
            'version': (self.parse_string, 1),
            'author': self.parse_string,
            'node': self.parse_node,
            'reg': self.parse_register,
        })

        ret = Soc()

        if 'name' in results:
            ret.name = results['name'][0]
        else:
            self.err('missing "name" statement at toplevel')

        if 'title' in results:
            ret.title = results['title'][0]
        if 'desc' in results:
            ret.desc = results['desc'][0]
        if 'isa' in results:
            ret.isa = results['isa'][0]
        if 'version' in results:
            ret.version = results['version'][0]
        if 'author' in results:
            ret.authors += results['author']
        if 'node' in results:
            ret.nodes += results['node']
        if 'reg' in results:
            ret.nodes += results['reg']

        return ret

    def parse_node(self):
        name = self.parse_ident()
        ret, results = self.parse_node_block(name, {
            'title': (self.parse_string, 1),
            'node': self.parse_node,
            'reg': self.parse_register,
        })

        if 'title' in results:
            ret.title = results['title'][0]
        if 'node' in results:
            ret.children += results['node']
        if 'reg' in results:
            ret.children += results['reg']

        return ret

    def parse_register(self):
        words, has_block = self.parse_wordline(True)
        name = words[0]
        width = "32"
        addr = None

        if len(words) == 1:
            # reg NAME
            pass
        elif len(words) == 2:
            # reg NAME ADDR
            addr = words[1]
        elif len(words) == 3:
            # reg WIDTH NAME ADDR
            name = words[1]
            width = words[0]
            addr = words[2]
        else:
            self.err('malformed register statement')

        if has_block:
            ret, results = self.parse_node_block(name, {
                'field': self.parse_field,
                'fld': self.parse_field,
                'bit': self.parse_field,
                'variant': self.parse_variant,
            })

            if 'field' in results:
                ret.fields += results['field']
            if 'fld' in results:
                ret.fields += results['fld']
            if 'bit' in results:
                ret.fields += results['bit']
            if 'variant' in results:
                ret.fields += results['variant']
        else:
            ret = Node(name)

        if len(ret.instances) == 0:
            if addr is None:
                self.err("no address specified for register")
            ret.instances.append(Instance(ret.name, addr))
        elif addr:
            self.err("duplicate address specification for register")

        ret.register_width = width
        return ret

    def parse_node_block(self, name, extra_parsers):
        parsers = {
            'desc': (self.parse_string, 1),
            'addr': (self.parse_printable, 1),
            'instance': functools.partial(self.parse_instance, name),
        }

        parsers.update(extra_parsers)
        results = self.parse_block(parsers)
        ret = Node(name)

        if 'desc' in results:
            ret.desc = results['desc'][0]
        if 'addr' in results:
            ret.instances.append(Instance(ret.name, results['addr'][0]))
        if 'instance' in results:
            if 'addr' in results:
                self.err('instance statement not allowed when addr is given')

            ret.instances += results['instance']

        return ret, results

    def parse_instance(self, default_name):
        words = self.parse_wordline(False)[0]

        if len(words) == 1:
            # instance ADDR
            return Instance(default_name, words[0])
        elif len(words) == 2:
            # instance NAME ADDR
            return Instance(words[0], words[1])
        elif len(words) == 3:
            # instance ADDR STRIDE COUNT
            return Instance(default_name, words[0], words[1], words[2])
        elif len(words) == 4:
            # instance NAME ADDR STRIDE COUNT
            return Instance(words[0], words[1], words[2], words[3])
        else:
            self.err('malformed instance statement')

    def parse_field(self):
        words, has_block = self.parse_wordline(True)
        name = None
        msb = None
        lsb = None

        if len(words) == 2:
            # field BIT NAME
            lsb = msb = words[0]
            name = words[1]
        elif len(words) == 3:
            # field MSB LSB NAME
            msb = words[0]
            lsb = words[1]
            name = words[2]
        else:
            self.err('malformed field statement')

        try:
            int(msb)
            int(lsb)
        except:
            self.err('field MSB/LSB must be integers')

        ret = Field(name, msb, lsb)
        if has_block:
            results = self.parse_block({
                'desc': self.parse_string,
                'enum': self.parse_enum,
            })

            if 'desc' in results:
                if len(results['desc']) > 1:
                    self.err("only one description allowed")
                ret.desc = results['desc'][0]

            if 'enum' in results:
                ret.enums += results['enum']

        return ret

    def parse_wordline(self, allow_block=False):
        words = []
        self.eolstop = True
        while(self.chr != '{' and self.chr != '}' and
              self.chr != '\n' and self.chr != ';'):
            words.append(self.parse_printable())
        self.eolstop = False

        if len(words) == 0:
            self.err('this type of statement cannot be empty')

        if self.chr == '{':
            if not allow_block:
                self.err('this type of statement does not accept blocks')
            has_block = True
        else:
            has_block = False
            self.skipws()

        return words, has_block

    def parse_variant(self):
        ty = self.parse_printable()
        off = self.parse_printable()
        return Variant(ty, off)

    def parse_enum(self):
        name = self.parse_printable()
        value = self.parse_printable()
        return (name, value)

    def parse_block(self, parsers):
        if self.chr != '{':
            self.err("expected '{'")
        self.next()
        self.skipws()

        ret = self.parse_block_inner(parsers)

        assert self.chr == '}'
        self.next()
        self.skipws()
        return ret

    def parse_block_inner(self, parsers):
        ret = {}
        cnt = {}
        while self.chr != '}' and self.chr != '\0':
            kwd = self.parse_ident()
            if kwd not in parsers:
                self.err("invalid keyword for block")

            if kwd not in ret:
                ret[kwd] = []
                cnt[kwd] = 0

            pentry = parsers[kwd]
            if type(pentry) is tuple and len(pentry) == 2:
                pfunc = pentry[0]
                max_cnt = pentry[1]
            else:
                pfunc = pentry
                max_cnt = 0

            if max_cnt > 0 and cnt[kwd] == max_cnt:
                if max_cnt == 1:
                    self.err("at most one '%s' statement allowed in this block" % kwd)
                else:
                    self.err("at most %d '%s' statements allowed in this block" % (max_cnt, kwd))

            ret[kwd].append(pfunc())
            cnt[kwd] += 1

        return ret

    def parse_ident(self):
        ident = ''
        while self.chr.isalnum() or self.chr == '_':
            ident += self.chr
            self.next()

        if len(ident) == 0:
            self.err("expected identifier")

        self.skipws()
        return ident

    def parse_string(self):
        if self.chr != '"':
            self.err("expected string")
        self.next()

        s = ''
        while self.chr != '"':
            s += self.chr
            self.next()
        self.next()

        self.skipws()
        return s

    def parse_printable(self):
        s = ''
        while not self.chr.isspace() and self.chr != ';':
            s += self.chr
            self.next()

        self.skipws()
        return s

    def skipws(self):
        while True:
            if self.chr == '#':
                while self.chr != '\n':
                    self.next()
                if self.eolstop:
                    break
                self.next()
            elif self.chr == '\n' or self.chr == ';':
                if self.eolstop:
                    break
                self.next()
            elif self.chr.isspace():
                self.next()
            else:
                break

    def next(self):
        self.chr = self.input.read(1)
        if len(self.chr) == 0:
            self.chr = '\0'
        if self.chr == '\n':
            self.line += 1
        return self.chr

    def err(self, msg):
        raise ValueError("on line %d: %s" % (self.line, msg))

if __name__ == '__main__':
    import sys
    p = Parser(sys.stdin)
    r = p.parse()
    print('<?xml version="1.0"?>')
    ET.dump(r.gen())
