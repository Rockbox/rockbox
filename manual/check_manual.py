#!/usr/bin/python3
import glob
import os
import os.path
import re
import subprocess
import sys
import argparse
import collections
# gracefully handle the case where the package is not available
try:
    import colorama
    colorama.init()
    from termcolor import colored
except:
    # print message and stub colored()
    print("You need to install 'termcolor' and 'colorama' to have colors in the terminal")
    def colored(msg, col, *other):
        return msg

# TODO explain

def split_word_on_cap(word):
    """Split a CamelCaseWORD word into its subwords (Camel, Case, WORD)"""
    if len(word) == 0:
        return []
    # read as many upper case characters as we can (e.g. FMStopX -> read FMS)
    index = 0
    while index < len(word) and word[index].isupper():
        index += 1
    # if the entire word is upper case, return it
    if index == len(word):
        return [word]
    # if there are more than one upper case character, split on last upper case
    # e.g. FMStop -> FM
    if index > 1:
        return [word[0:index-1]] + split_word_on_cap(word[index-1:])
    # otherwise read as many lower case characters as we can
    # e.g. PrevRepeat -> Prev
    while index < len(word) and word[index].islower():
        index += 1
    return [word[0:index]] + split_word_on_cap(word[index:])

## Library
def debug_print(lvl, msg):
    """Print a debug message.

    Args:
        lvl (int): Verbosity level of the message
        msg (str): Message to print
    """
    if g_args.verbose >= lvl:
        print(msg)

def die(msg):
    """Print an error message and quit.

    Args:
        msg (str): Message to print
    """
    print(msg)
    exit(1)

def print_file(filename):
    with open(filename, "r") as fp:
        for line in fp:
            print(line.rstrip())

def check_dict(d, list, msg):
    """Check that a dictionary contains all given keys, die if a key is missing.

    Args:
        d (dict): dictionary to check
        list (list): list of keys to check
        msg (str): error message (interpolated with missing key)
    """
    for key in list:
        if key not in d:
            die(msg % key)

def preprocess_file(file, deflist):
    """Preprocess a file with a define list: it only gets rid of all #if and friends

    Args:
        file (str): path to file to preprocess
        deflist (dict): define list

    Returns:
        array of lines (str)
    """
    try:
        f = open(filename, 'r')
    except IOError as err:
        debug_print(1, err)
        die("Cannot open '%s' for preprocessing" % filename)
    # lines that we keep in the file
    res = []
    # some preprocess lines can span multiple lines, we accumulate into this variable
    current_preproc_inst = None # instruction (#if, #elif, ...)
    current_preproc_line = "" # everything but instruction
    parsing_inst = False # true if last line was a preprocessor instruction that finished with \
    current_preproc_value = True # value of last instruction (or True if not in instruction)
    inst_stack = [] # stack of instructions that are conditionals, each entry is (inst,value)
    for line in f:
        line = line.strip()
    f.close()
    return d
    

def parse_makefile(filename):
    """Parse Makefile created by configure

    Returns:
        dict. A dictionary containing useful information:
            'target': the target define (e.g. CREATIVE_ZENXFI3)
            'model': the target configure name (e.g. creativezenxfi3)
            'manualdev': the target configure name (e.g. creativezenxfi3)
    """
    try:
        f = open(filename, 'r')
    except IOError as err:
        debug_print(1, err)
        die("Cannot open '%s', you must run this script from the build directory" % filename)
    # extract defines
    d = dict()
    for line in f:
        m = re.search('export TARGET=-D(\w*)', line)
        if m is not None: d['target'] = m.group(1)
        m = re.search('export MODELNAME=(\w*)', line)
        if m is not None: d['modelname'] = m.group(1)
        m = re.search('export MANUALDEV=(\w*)', line)
        if m is not None: d['manualdev'] = m.group(1)
        m = re.search('export MANUALDIR=([\w/]*)', line)
        if m is not None: d['manualdir'] = m.group(1)
        m = re.search('export ROOTDIR=([\w/]*)', line)
        if m is not None: d['rootdir'] = m.group(1)
        m = re.search('export EXTRA_DEFINES=(.*)\Z', line.strip())
        if m is not None: d['extra_defines'] = m.group(1)
    check_dict(d, ['target', 'modelname', 'manualdev', 'manualdir', 'rootdir', 'extra_defines'],
        "Could not extract %s from Makefile")
    f.close()
    return d

def parse_keyval_file(filename):
    """Parse a file with format 'key: val' and return a dictionary"""
    try:
        f = open(filename, 'r')
    except IOError as err:
        debug_print(1, err)
        die("Cannot open '%s" % filename)
    # extract defines
    d = dict()
    for line in f:
        m = re.match('(.*):(.*)', line.strip())
        if m is not None: d[m.group(1).strip()] = m.group(2).strip()
    f.close()
    return d

def parse_deflist_file(filename):
    """Parse the output of gcc -dM -E to get the list of defines and values

    Args:
        filename: path to file

    Returns:
        dict. A map, with potentially empty values for defines without values
    """
    try:
        f = open(filename, 'r')
    except IOError as err:
        debug_print(1, err)
        die("Cannot open '%s', something is wrong" % filename)
    # parse file
    def_list = dict()
    for line in f:
        m = re.match('#define (\w*)(.*)', line.strip())
        if m is not None: def_list[m.group(1)] = m.group(2).strip()
    f.close()
    return def_list

def parse_latex_platform(filename):
    """Parse latex platform file

    Args:
        filename (str): path to the the keymap file

    Returns:
        (set,str). A set of options defined for this target, and the name of
        the keymap file
    """
    try:
        f = open(filename, 'r')
    except IOError as err:
        debug_print(1, err)
        die("Cannot open '%s', something is wrong" % filename)
    # extract macros
    options = set()
    keymap_file = None
    for line in f:
        m = re.search('\\\\def\\\\UseOption{(\w*)}', line)
        if m is not None: options.add(m.group(1))
        m = re.search('\\\\edef\\\\UseOption{\\\\UseOption,(\w*)}', line)
        if m is not None: options.add(m.group(1))
        m = re.search('\\\\input{platform/(keymap-\w*\.tex)}', line)
        if m is not None: keymap_file = m.group(1)
    if keymap_file is None:
        die("Could not extract keymap filename from platform file")
    return (options, keymap_file)

# LatexButtonCombo describes a button combination:
#   raw (str): raw string representation (latex or C code)
#   button (list): the set of buttons that must be pressed simulteanously
#   long (bool): True if long press
# If the set of button is empty, it means it could not be parsed and the raw
# representation is all whaht is available
LatexButtonCombo = collections.namedtuple('LatexButtonCombo', ['raw', 'buttons', 'long'])

def print_button_combo(self):
    # for unparsable buttons, return raw
    if len(self.buttons) == 0:
        return "'%s'" % self.raw
    s = ""
    if self.long:
        s = "Long "
    first = True
    # sort buttons to have stable hashing
    for btn in sorted(self.buttons):
        if not first:
            s += " + "
        first = False
        s += btn
    return s

def hash_latex_combo(self):
    # hash the case-insensitive version of the representation
    return hash(print_rb_context(self))

def eq_latex_combo(self, other):
    return print_button_combo(self) == print_button_combo(other)

def ne_latex_combo(self, other):
    return not ne_latex_combo(self, other)

# override LatexButtonCombo string representation and hashing function
# for comparison and hash, make sure to sort the array
LatexButtonCombo.__repr__ = print_button_combo
LatexButtonCombo.__hash__ = hash_latex_combo
LatexButtonCombo.__eq__ = eq_latex_combo
LatexButtonCombo.__ne__ = ne_latex_combo

def parse_latex_button_combo(raw):
    """Parse a latex button combo (e.g. Long \ButtonSelect{} or \ButtonRight)

    Args:
        raw (str): raw latex code

    Returns:
        list of LatexButtonCombo.
    """
    # handle empty string to avoid useless warnings
    if len(raw.strip()) == 0:
        return []
    # split on ' or '
    combos = []
    for sub_raw in  raw.split(" or "):
        buttons = []
        long = False
        btn_str = sub_raw.strip()
        # detect 'Long ' prefix
        if btn_str.startswith('Long '):
            long = True
            btn_str = btn_str[5:].strip()
        # split on ' + '
        error = False
        for btn_txt in btn_str.split("+"):
            # match again \ButtonXXXXX or \ButtonXXXXX{}
            btn_txt = btn_txt.strip()
            m = re.match('\\\\(Button\w*)({})?', btn_txt)
            if m is None:
                debug_print(2, "Warning: could not parse latex button '%s'" % btn_txt)
                error = True
            else:
                buttons.append(m.group(1))
        # any error cancels everything
        if error:
            buttons = []
        combos.append(LatexButtonCombo(sub_raw, buttons, long))
    return combos

def pretty_print_latex_keymap(level, keymap):
    """Pretty print the latex keymap at specified debug level"""
    for action, combo in keymap.items():
        debug_print(level, "%s: %s" % (action, ' or '.join([str(c) for c in combo])))

def process_latex_keymap_forward(keymap):
    """Post-process keymap to resolve buttons combos that refer to other actions

    For example: \newcommand{\ActionFMSettingsDec}{\ActionSettingDec}
    """
    # Rewrite until no change (since there could be chains of rewrites)
    new_keymap = dict()
    for action,combos in keymap.items():
        # rewrite until done
        done = False
        while not done:
            new_combos = []
            done = True
            for combo in combos:
                # match against combo's raw
                m = re.match('\\\\(Action\w*)', combo.raw)
                if m is not None:
                    debug_print(2, "forward: %s => %s" % (action, m.group(1)))
                    new_combos.extend(keymap[m.group(1)])
                    done = False
                else:
                    new_combos.append(combo)
            combos = new_combos
        new_keymap[action] = combos
    return new_keymap

def parse_latex_keymap(filename):
    """Parse latex keymap as much as possible

    Args:
        filename (str): path to the the keymap file

    Returns:
        (keymap,btnlist,pluginmap) where
        keymap (dict): A dictionary containing for each action macro the set of button combo
            that achieve this action (each one a LatexButtonCombo)
        btnlist (list): The list of button macro names (without \)
        pluginmap (dict): Same as keymap but for plugion action lib
    """
    try:
        f = open(filename, 'r')
    except IOError as err:
        debug_print(1, err)
        die("Cannot open '%s', something is wrong" % filename)
    # extract macros
    keymap = dict()
    btnlist = []
    pluginmap = dict()
    for line in f:
        m = re.match('\s*\\\\newcommand{\\\\(Action\w*)}{(.*)}[^}]*', line.strip())
        if m is not None: keymap[m.group(1)] = parse_latex_button_combo(m.group(2))
        m = re.match('\s*\\\\newcommand{\\\\(Button\w*)}{(.*)}[^}]*', line.strip())
        if m is not None: btnlist.append(m.group(1))
        m = re.match('\s*\\\\newcommand{\\\\(Plugin\w*)}{(.*)}[^}]*', line.strip())
        if m is not None: pluginmap[m.group(1)] = parse_latex_button_combo(m.group(2))
    f.close()
    # postprocess keymap for forwarding
    keymap = process_latex_keymap_forward(keymap)
    return (keymap,btnlist,pluginmap)

def get_rb_action_and_context_lists(action_h):
    """Extract action and context list from action.h header

    Args:
        action_h (str): path to action.h

    Returns:
        (list,list): The first list is the list of action name, the second of context
        names.
    """
    try:
        f = open(action_h, 'r')
    except IOError as err:
        debug_print(1, err)
        die("Cannot open '%s', something is wrong" % action_h)
    # extract macros
    actions = []
    contexts = []
    for line in f:
        m = re.match('\s*(ACTION_\w*)(\s*=.*)?,.*', line.strip())
        if m is not None: actions.append(m.group(1))
        m = re.match('\s*(CONTEXT_\w*)(\s*=.*)?,.*', line.strip())
        if m is not None: contexts.append(m.group(1))
    f.close()
    return (actions, contexts)

our_makefile_content = \
"""\
# include configure generated Makefile to get all the goodies
include Makefile
# extract the filename of the keymap source
KEYMAP_SRC=$(filter $(APPSDIR)/keymaps/keymap-%.c, $(call preprocess, $(APPSDIR)/SOURCES))
# checkmanual subdirectory
CM_SUBDIR={cm_subdir}
# this seems like a HACK because the build system does not need it but somehow I do...
CFLAGS+=$(OTHER_INC)

# find keymap file
.PHONY: check_manual check_manual_keymap
check_manual_keymap:
\techo "keymap-filename: $(KEYMAP_SRC)" > $(CM_SUBDIR)check_manual.txt

check_manual_deflist: $(CM_SUBDIR)check_manual.c
\t$(CC) $(CFLAGS) -dM -E $^ > $(CM_SUBDIR)check_manual_deflist.txt

# -fdirectives-only -E: only directives, do not expand
# NOTE: this will keep comments, so we re-run to remove comments:
# -fpreprocessed -dD: disable all processing (except comments), output #define and all
CM_PREPROCESS_STEP1=$(CPP) $(CFLAGS) -fdirectives-only -E
CM_PREPROCESS_STEP2=$(CPP) $(CFLAGS) -fpreprocessed -dD
# we may need some generated headers, so make sure we put them as dependencies
CM_PREPROCESS_DEP=$(BUILDDIR)/sysfont.h $(BMPHFILES)
{cm_preprocess_commands}

check_manual_preprocess: {cm_preprocess_target_list}

check_manual: check_manual_keymap check_manual_deflist check_manual_preprocess
"""

check_manual_c_src =\
"""\
/* include some headers so that we can dump the defines */
#include "config.h"
"""

def get_rb_info(builddir, cm_subdir, preprocess_list):
    """Get rockbox information

    Args:
        builddir (str): path to build directory
        cm_subdir(str): check_manual subdirectory where to put files
        preprocess_list (list): list of files to preprocess (see below)

    To obtain the keymap, we create an extra makefile that includes Makefile and
    contains our own target. This target uses the stuff from Rockbox makefiles to
    preprocess apps/SOURCES and find the keymap file.
    We also run the preprocess in a very special mode on files listed in
    preprocess_list: we run the preprocessor so that it #include files and
    process #if/#elif/#endif but does not perform macro expansion. The format
    of preprocess_list is a list of
        (full_path_to_file_to_process, relative_filename_of_output)
    """
    # create an extra makefile
    our_makefile = os.path.join(builddir, 'check_manual.make')
    try:
        f = open(our_makefile, 'w')
    except IOError as err:
        debug_print(1, err)
        die("Cannot create '%s', something is wrong" % our_makefile)
    pp_list = []
    pp_cmd = []
    for (full_in_file, out_file) in preprocess_list:
        full_out_file = os.path.join(builddir, cm_subdir, out_file)
        pp_list.append(full_out_file)
        pp_cmd.append("")
        pp_cmd.append("%s: %s $(CM_PREPROCESS_DEP)" % (full_out_file, full_in_file))
        pp_cmd.append("\t$(CM_PREPROCESS_STEP1) $< > $@.i")
        pp_cmd.append("\t$(CM_PREPROCESS_STEP2) $@.i > $@")
    make_fmt = {
        'cm_subdir': cm_subdir + "/",
        'cm_preprocess_target_list': ' '.join(pp_list),
        'cm_preprocess_commands': '\n'.join(pp_cmd)
    }
    f.write(our_makefile_content.format(**make_fmt))
    f.close()
    # create check_manual.c
    check_manual_c = os.path.join(builddir, cm_subdir, 'check_manual.c')
    try:
        f = open(check_manual_c, 'w')
    except IOError as err:
        debug_print(1, err)
        die("Cannot create '%s', something is wrong" % check_manual_c)
    f.write(check_manual_c_src)
    f.close()
    # run make -C <path to build dir> -f <path to our makefile>
    output_log = os.path.join(builddir, cm_subdir, 'check_manual.log')
    output_log_fp = open(output_log, "w")
    cmd = ["make" ]
    if len(builddir) > 0:
        cmd.extend(["-C", builddir])
    cmd.extend(["-f", our_makefile, "check_manual"])
    make_res = subprocess.run(cmd, stdout = output_log_fp, stderr = subprocess.STDOUT)
    output_log_fp.close()
    if make_res.returncode != 0:
        if g_args.verbose >= 1:
            print_file(output_log)
        die("An error occured when compile an auxiliary program (use -v do print or check %s)" % output_log)
    # parse the output check_manual.txt to extract information
    output = parse_keyval_file(os.path.join(builddir, cm_subdir, 'check_manual.txt'))
    check_dict(output, ['keymap-filename'],
        "Could not extract %s from check_manual.txt")
    # parse deflist
    deflist = parse_deflist_file(os.path.join(builddir, cm_subdir, 'check_manual_deflist.txt'))
    return (output, deflist)

# RockboxLatexButtonCombo describes a button combination as used in rockbox keymap
#   buttons (list): list of buttons names (empty if BUTTON_NONE)
#   repeat (bool): True if BUTTON_REPEAT is set
#   release (bool): True if BUTTON_REL is set
# RockboxActionDesc describes an action combination as used in rockbox keymap
#   action (str): name of the action
#   combo (RockboxLatexButtonCombo): button combo that triggers the action
#   pre_combo (RockboxLatexButtonCombo): button combo required as precondition
# RockboxContext describes a rockbox context
#   combo (list): list of ORed contexts
RockboxLatexButtonCombo = collections.namedtuple('RockboxLatexButtonCombo', ['buttons', 'repeat', 'release'])
RockboxActionDesc = collections.namedtuple('RockboxActionDesc', ['action', 'combo', 'pre_combo'])
RockboxContext = collections.namedtuple('RockboxContext', ['ctx'])

def print_rb_button_combo(self):
    # special case for BUTTON_NONE
    if len(self.buttons) == 0:
        return 'none'
    s = "|".join(self.buttons)
    if self.repeat:
        s = "repeat(%s)" % s
    if self.release:
        s = "release(%s)" % s
    return s

# override RockboxLatexButtonCombo string representation
RockboxLatexButtonCombo.__repr__ = print_rb_button_combo

def print_rb_action_desc(self):
    return "(%s,%s) -> %s" % (self.pre_combo, self.combo, self.action)

# override RockboxLatexButtonCombo string representation
RockboxActionDesc.__repr__ = print_rb_action_desc

def print_rb_context(self):
    return "|".join(sorted(self.ctx))

def hash_rb_context(self):
    return hash(print_rb_context(self))

def eq_rb_context(self, other):
    return sorted(self.ctx) == sorted(other.ctx)

def ne_rb_context(self, other):
    return not eq_rb_context(self, other)

# override RockboxContext string representation and hashing function
# for comparison and hash, make sure to sort the array
RockboxContext.__repr__ = print_rb_context
RockboxContext.__hash__ = hash_rb_context
RockboxContext.__eq__ = eq_rb_context
RockboxContext.__ne__ = ne_rb_context

def parse_rb_context(combo):
    """Parse a rockbox context OR and turn it into a RockboxContext"""
    ctx_list = [btn.strip() for btn in combo.split("|")]
    return RockboxContext(ctx_list)

def parse_rb_button_combo(combo):
    """Parse rockbox button OR and turn it into a RockboxLatexButtonCombo or None"""
    btn_list = [btn.strip() for btn in combo.split("|")]
    # ignore empty list (should not happen)
    if len(btn_list) == 0:
        return None
    # handle repeat
    repeat = ('BUTTON_REPEAT' in btn_list)
    release = ('BUTTON_REL' in btn_list)
    # remove BUTTON_{NONE, REL, REPEAT} from the list
    for x in ['BUTTON_NONE', 'BUTTON_REPEAT', 'BUTTON_REL']:
        if x in btn_list:
            btn_list.remove(x)
    debug_print(2, btn_list)
    return RockboxLatexButtonCombo(btn_list, repeat, release)

def parse_rb_action_line(action, combo, pre_combo):
    """Parse rockbox keymap action line and turn it into a RockboxActionDesc or None"""
    debug_print(2, [action, combo, pre_combo])
    combo = parse_rb_button_combo(combo)
    pre_combo = parse_rb_button_combo(pre_combo)
    if combo is None or pre_combo is None:
        return None
    return RockboxActionDesc(action, combo, pre_combo)

def get_rb_keymap(keymap_file):
    """Get rockbox keymap from the source code

    Args:
        keymap_file (str): path to file

    Returns:
        dict. A mapping from context name to list (order is important). Each
        item in the list is a RockboxActionDesc. Next list is handle by
        creating a RockboxActionDesc(next_ctx, None, None). Context names are
        guessed by parsing the function get_context_mapping()
    """
    try:
        f = open(keymap_file, 'r')
    except IOError as err:
        debug_print(1, err)
        die("Cannot open '%s', something is wrong" % keymap_file)
    # map from array name to list of RockboxActionDesc
    arr_to_action_list = dict()
    last_arr_name = None
    # map from action name to array name
    ctx_to_arr = dict()
    last_ctx_case = [] # there might be several (e.g. case X: case Y: return bla)
    for line in f:
        m = re.search('struct\s+button_mapping\s+(button_context_\w*)\s*\[\]\s*=', line)
        if m is not None:
            debug_print(2, m.group(1))
            last_arr_name = m.group(1)
            arr_to_action_list[last_arr_name] = []
        m = re.match('\s*{\s*(ACTION_\w*)\s*,([^,]*),([^}]*)}.*', line)
        if m is not None:
            act = parse_rb_action_line(m.group(1).strip(), m.group(2).strip(), m.group(3).strip())
            # we must know in which array we are
            if act is not None and last_arr_name is not None:
                debug_print(2, act)
                arr_to_action_list[last_arr_name].append(act)
        m = re.search("LAST_ITEM_IN_LIST__NEXTLIST\(([\w|]*)\)", line)
        if m is not None:
            ctx = parse_rb_context(m.group(1))
            # we must know in which array we are
            if last_arr_name is not None:
                debug_print(2, ctx)
                arr_to_action_list[last_arr_name].append(RockboxActionDesc(
                    ctx, None, None))
            last_arr_name = None
        m = re.search("LAST_ITEM_IN_LIST", line)
        if m is not None:
            last_arr_name = None
        m = re.search('case (CONTEXT_[\w|]*):', line)
        if m is not None:
            debug_print(2, m.group(1))
            last_ctx_case.append(parse_rb_context(m.group(1)))
        m = re.search('default:', line)
        if m is not None:
            debug_print(2, 'default')
            last_ctx_case.append(RockboxContext(['default']))
        m = re.search('return\s+(button_context_\w*)\s*;', line)
        if m is not None:
            # add entry for all context that apply
            debug_print(2, m.group(1).strip())
            for ctx in last_ctx_case:
                ctx_to_arr[ctx] = m.group(1).strip()
            last_ctx_case = []
    f.close()
    # debug
    debug_print(2, arr_to_action_list)
    debug_print(2, ctx_to_arr)
    # build the final map
    keymap = dict()
    for key,val in ctx_to_arr.items():
        if val not in arr_to_action_list:
            print("Warning: %s -> %s -> ? (no array found)" % (key, val))
        else:
            keymap[key] = arr_to_action_list[val]
    return keymap

def get_rb_plugin_keymap(filename):
    """Get rockbox keymap from the source code

    Args:
        keymap_file (str): path to file

    Returns:
        (keymap,maplist). First a mapping from context name to list (order is important). Each
        item in the list is a RockboxActionDesc. Next list is handle by
        creating a RockboxActionDesc(next_ctx, None, None). Context names are the
        raw array names (e.g. pla_main_ctx). Second a mapping from names to list
        of contexts names (order is important), e.g. plugin_contexts
    """
    try:
        f = open(filename, 'r')
    except IOError as err:
        debug_print(1, err)
        die("Cannot open '%s', something is wrong" % filename)
    # map from array name to list of RockboxActionDesc
    arr_to_action_list = dict()
    arr_to_arr_names = dict()
    last_arr_name = None
    last_listarr_name = None
    for line in f:
        m = re.search('struct\s+button_mapping\s+(\w+)\s*\[\]\s*=', line)
        if m is not None:
            debug_print(2, m.group(1))
            last_arr_name = m.group(1)
            arr_to_action_list[last_arr_name] = []
        m = re.match('\s*{\s*(\w+)\s*,\s*(BUTTON_[^,]*),\s*(BUTTON_[^}]*)}.*', line)
        if m is not None:
            act = parse_rb_action_line(m.group(1).strip(), m.group(2).strip(), m.group(3).strip())
            # we must know in which array we are
            if act is not None and last_arr_name is not None:
                debug_print(2, act)
                arr_to_action_list[last_arr_name].append(act)
        m = re.search("LAST_ITEM_IN_LIST__NEXTLIST\(([\w|]*)\)", line)
        if m is not None:
            ctx = parse_rb_context(m.group(1))
            # we must know in which array we are
            if last_arr_name is not None:
                debug_print(2, ctx)
                arr_to_action_list[last_arr_name].append(RockboxActionDesc(
                    ctx, None, None))
            last_arr_name = None
        m = re.search("LAST_ITEM_IN_LIST", line)
        if m is not None:
            last_arr_name = None
        # annoyingly, there are two cases of map lists:
        #
        # const struct button_mapping *plugin_contexts[] =
        # {
        #  .....
        #
        # static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
        #
        # This regex handles the first case
        m = re.search('struct\s*button_mapping\s+\*(\w+)\s*\[\]\s*=', line)
        if m is not None:
            last_listarr_name = m.group(1)
            arr_to_arr_names[last_listarr_name] = []
            print("START %s" % last_listarr_name)
        # and this one the second
        m = re.search('struct\s*button_mapping\s+\*(\w+)\s*\[\]\s*=\s*{\s*(\w*)\s*}', line)
        if m is not None:
            arr_to_arr_names[m.group(1)] = [m.group(2)]
        # the list will typically look like this:
        #
        # const struct button_mapping *plugin_contexts[] =
        # {
        #   pla_main_ctx,
        #   iriver_syncaction,
        # };
        #
        # This is tricky because we need to read very unformated lines without errors...
        # to avoid errors, we stop the current array as soon as a line contains a }
        m = re.search('}\s*;', line)
        if m is not None:
            last_listarr_name = None
        # assume no more than one per line
        m = re.match("(\w+)\s*,?\Z", line.strip())
        if m is not None and last_listarr_name is not None:
            print("ADD %s" % line.strip())
            arr_to_arr_names[last_listarr_name].append(m.group(1))
    f.close()
    # debug
    debug_print(2, arr_to_action_list)
    debug_print(2, arr_to_arr_names)
    return (arr_to_action_list,arr_to_arr_names)

def search_rb_keymap(keymap, ctx, match_fn):
    """Search Rockbox keymap for the first action match. Search starts in given context
    but will follow the next list pointers to other contexts. Returns list of matches.

    Args:
        keymap (dict): rockbox keymap (dict of list of RockboxActionDesc)
        ctx (str): first context
        match_fn (function): matching function

    Returns:
        list of matching RockboxActionDesc
    """
    match_list = []
    while True:
        debug_print(3, "Searching in %s" % str(ctx))
        if ctx not in keymap:
            print("Warning: no context '%s' in keymap, this may indicate a parsing failure" % str(ctx))
            return None
        stop = True
        for act in keymap[ctx]:
            debug_print(3, "Try %s" % str(act))
            # handle next links
            if act.combo is None and act.pre_combo is None:
                ctx = act.action
                debug_print(3, "follow next to %s" % ctx)
                stop = False
                break
            # check action
            if match_fn(act):
                match_list.append(act)
        if stop:
            break
    return match_list

def pretty_print_rockbox_keymap(level, keymap):
    """Pretty print the rockbox keymap at specified debug level"""
    for ctx, act_list in keymap.items():
        debug_print(level, "%s:" % str(ctx))
        for act in act_list:
            debug_print(level, "  %s" % str(act))

# action list
#
# this list gives for each latex action the corresponding context and action in
# the rockbox keymap.
#
# Example:
#   ActionStdPrevRepeat -> (CONTEXT_STD, ACTION_STD_PREVREPEAT)
#
# This mapping can be constructed automatically for certain actions following a
# pattern. Currently, the list is thus constructed as follows:
#   final_list = auto-list + manual-list
# where the manual list as precedence over the automatic list in case of duplicate.
# The automatic list is built using the following rule:
#   ActionXxxYyyZzzz -> (CONTEXT_XXX, ACTION_XXX_YYY_ZZZ)
#
# Example:
#   ActionStdOk -> (CONTEXT_STD, ACTION_STD_OK)
#
# In order to make the automatic mapping more useful, we also a small database
# of word remapping
#
# Example:
#   CONTEXT_REC -> CONTEXT_RECSCREEN
#   will make the automatic mapping
#   ActionRecMenu -> (CONTEXT_REC,ACTION_REC_MENU) -> (CONTEXT_RECSCREEN,ACTION_REC_MENU)
#
# The manual list may specific contexts as string or as RockboxContext. A string
# s is converted to RockboxContext([s])
#
g_latex_to_rb_map_manual = {
    'ActionYesNoAccept': ('CONTEXT_YESNOSCREEN', 'ACTION_YESNO_ACCEPT')
}

g_rb_map_auto_rewrite_db = {
    # context rewrites
    'CONTEXT_REC': 'CONTEXT_RECSCREEN',
    'CONTEXT_PS': 'CONTEXT_PITCHSCREEN',
    'CONTEXT_KBD': 'CONTEXT_KEYBOARD',
    'CONTEXT_SETTING': 'CONTEXT_SETTINGS',
    'CONTEXT_QUICK': 'CONTEXT_QUICKSCREEN',
    'CONTEXT_BM': 'CONTEXT_BOOKMARKSCREEN',
    'CONTEXT_ALARM': 'CONTEXT_SETTINGS',
    # full action rewrites
    'ACTION_REC_SETTINGS_DEC': 'ACTION_SETTINGS_DEC',
    'ACTION_REC_SETTINGS_INC': 'ACTION_SETTINGS_INC',
    'ACTION_REC_PREV': 'ACTION_STD_PREV',
    'ACTION_REC_NEXT': 'ACTION_STD_NEXT',
    'ACTION_REC_EXIT': 'ACTION_STD_CANCEL',
    'ACTION_REC_F_TWO': 'ACTION_REC_F2',
    'ACTION_REC_F_THREE': 'ACTION_REC_F3',
    'ACTION_REC_MENU': 'ACTION_STD_MENU',
    'ACTION_STD_NEXT_REPEAT': 'ACTION_STD_NEXTREPEAT',
    'ACTION_STD_PREV_REPEAT': 'ACTION_STD_PREVREPEAT',
    'ACTION_STD_QUICK_SCREEN': 'ACTION_STD_QUICKSCREEN',
    'ACTION_FM_SETTINGS_DEC': 'ACTION_SETTINGS_DEC',
    'ACTION_FM_SETTINGS_INC': 'ACTION_SETTINGS_INC',
    'ACTION_FM_NEXT': 'ACTION_STD_NEXT',
    'ACTION_FM_PREV': 'ACTION_STD_PREV',
    'ACTION_SETTING_INC': 'ACTION_SETTINGS_INC',
    'ACTION_SETTING_DEC': 'ACTION_SETTINGS_DEC',
    'ACTION_KBD_BACK_SPACE': 'ACTION_KBD_BACKSPACE',
    'ACTION_WPS_VOL_UP': 'ACTION_WPS_VOLUP',
    'ACTION_WPS_VOL_DOWN': 'ACTION_WPS_VOLDOWN',
    'ACTION_WPS_SKIP_NEXT': 'ACTION_WPS_SKIPNEXT',
    'ACTION_WPS_SKIP_PREV': 'ACTION_WPS_SKIPPREV',
    'ACTION_WPS_SEEK_BACK': 'ACTION_WPS_SEEKBACK',
    'ACTION_WPS_SEEK_FWD': 'ACTION_WPS_SEEKFWD',
    'ACTION_WPS_QUICK_SCREEN': 'ACTION_WPS_QUICKSCREEN',
    'ACTION_WPS_ID_THREE_SCREEN': 'ACTION_WPS_ID3SCREEN',
    'ACTION_WPS_PITCH_SCREEN': 'ACTION_WPS_PITCHSCREEN',
    'ACTION_WPS_AB_RESET': 'ACTION_WPS_ABRESET',
    'ACTION_WPS_AB_SET_B_NEXT_DIR': 'ACTION_WPS_ABSETB_NEXTDIR',
    'ACTION_WPS_AB_SET_A_PREV_DIR': 'ACTION_WPS_ABSETA_PREVDIR',
    'ACTION_WPS_PLAYLIST': 'ACTION_WPS_VIEW_PLAYLIST',
    'ACTION_BM_DELETE': 'ACTION_BMS_DELETE',
    'ACTION_ALARM_SET': 'ACTION_STD_OK',
    'ACTION_ALARM_HOURS_DEC': 'ACTION_STD_PREV',
    'ACTION_ALARM_HOURS_INC': 'ACTION_STD_NEXT',
    'ACTION_ALARM_CANCEL': 'ACTION_STD_CANCEL',
    'ACTION_QUICK_SCREEN_EXIT': 'ACTION_STD_CANCEL',
    'ACTION_TREE_ENTER': 'ACTION_STD_OK',
    'ACTION_TREE_PARENT_DIRECTORY': 'ACTION_STD_CANCEL',
    'ACTION_TREE_QUICK_SCREEN': 'ACTION_STD_QUICKSCREEN',
}

def map_latex_to_rb(action):
    """Convert a latex action name in tuple (context, rb_action)

    Example:
        'ActionStdOk' -> ('CONTEXT_STD', 'ACTION_STD_OK')
    """
    # manual list has priority
    if action in g_latex_to_rb_map_manual:
        return g_latex_to_rb_map_manual[action]
    # apply heuristic
    word_list = split_word_on_cap(action)
    debug_print(3, "%s -> %s" % (action, word_list))
    # if it does not start with Action, print a warning and return (None, None)
    if len(word_list) == 0:
        print("Warning: ignoring empty (!) action name, this might indicate a problem")
        return (None, None)
    if word_list[0] != "Action":
        print("Warning: ignoring latex action that does not start with 'Action': %s" % action)
        return (None, None)
    if len(word_list) == 1:
        print("Warning: ignoring latex action that is too short: %s" % action)
        return (None, None)
    # convert using our heuristic
    ctx = 'CONTEXT_' + word_list[1].upper()
    # rewrite if needed
    if ctx in g_rb_map_auto_rewrite_db:
        ctx = g_rb_map_auto_rewrite_db[ctx]
    # rewrite word list if necessary
    for i in range(len(word_list)):
        w = word_list[i].upper()
        if w in g_rb_map_auto_rewrite_db:
            w = g_rb_map_auto_rewrite_db[w]
        word_list[i] = w
    act = '_'.join([word for word in word_list])
    # rewrite entire word?
    if act in g_rb_map_auto_rewrite_db:
        act = g_rb_map_auto_rewrite_db[act]
    return (ctx, act)

# button list
#
# this list maps every rockbox button to a Latex button
#
# Examples:
#   BUTTON_UP -> ButtonUp
#   BUTTON_BOTTOMRIGHT -> ButtonBottomRight
#
# For simplicity, the final button comparison between latex names will be case-insensitive
# so that ButtonBottomRight matches ButtonBottomright for example. Currently
# the list is constructed as follows:
#   final_list = auto-list + manual-list
# where the manual list as precedence over the automatic list in case of duplicate.
# The automatic list is built using the following rule:
#   BUTTON_XXX_YYY_ZZZ -> ButtonXxxyyyZzz
#
# Example:
#   BUTTON_VOL_UP -> ButtonVolUp
#
# Note that the manual list is target specific
g_sansafuzeplus_latex_btn_manual = {
    'BUTTON_PLAYPAUSE': 'ButtonPlay'
}

g_rb_to_latex_btn_manual = {
    'SANSA_FUZEPLUS': g_sansafuzeplus_latex_btn_manual
}

def convert_rb_button_to_latex(rb_btn, latex_btnlist):
    """Convert a rockbox button (e.g. BUTTON_UP) to a latex button (ButtonUp).
    The produced name is then matched (case-insensitive) against the list of
    latex buttons. If there is no match, a warning is produce and the program
    continues with this name, otherwise the name is replace with the match (so
    that the name has the correct case)
    """
    # check manual database
    target = g_makefile_db['target']
    if target in g_rb_to_latex_btn_manual and rb_btn in g_rb_to_latex_btn_manual[target]:
        name = g_rb_to_latex_btn_manual[target][rb_btn]
        debug_print(2, "Convert %s to %s manually (%s)" % (rb_btn, name, target))
        return name
    # automatic translation: don't care about the case since we match case-insensitive
    name = "".join(rb_btn.split('_'))
    # now match against latex list
    for latex_btn in latex_btnlist:
        if latex_btn.lower() == name.lower():
            debug_print(2, "Convert %s to %s automatically" % (rb_btn, latex_btn))
            return latex_btn
    msg = "could not find button %s in keymap-*.tex, this may" % name
    print("%s: %s" % (colored("Warning", 'cyan', attrs=['bold']), msg))
    print("         indicate a parsing problem, or you need to add a manual entry for the")
    print("         rockbox -> latex mapping, rockbox button was %s" % rb_btn)
    return rb_btn

def convert_rb_action_to_combo(action_list, latex_btnlist):
    """Convert a list of RockboxActionDesc to a list of LatexButtonCombo as best as possible"""
    combo_list = []
    for act in action_list:
        # we keep as a raw form the pre_combo and combo (but not action but it is
        # not relevant in this representation)
        raw = "%s then %s" % (act.pre_combo, act.combo)
        # for analysis purpose, we assume that if pre_combo is not BUTTON_NONE
        # then the same buttons will appear in combo and pre_combo (this is how
        # it should be), thus we ignore BUTTON_NONE
        buttons = [convert_rb_button_to_latex(btn, latex_btnlist) for btn in act.combo.buttons]
        long = act.combo.repeat or act.pre_combo.repeat
        combo_list.append(LatexButtonCombo(raw, buttons, long))
    return combo_list

def simulate_rb_action(keymap, rb_ctx, pre_combo, combo):
    debug_print(3, "Simulating context %s" % rb_ctx)
    # make sure context exists
    if rb_ctx not in keymap:
        debug_print(3, "context %s not found in keymap" % rb_ctx)
        return None
    # check all entries of the context keymap
    for act in keymap[rb_ctx]:
        debug_print(3, "Match (%s,%s) againt (%s,%s)" % (pre_combo, combo, act.pre_combo, act.combo))
        # if pre_combo/combo is compatible with this action, this is the one that
        # Rockbox would select
        if act.pre_combo == pre_combo and act.combo == combo:
            return act.action
        # handle context fallback
        if act.pre_combo is None and act.combo is None:
            debug_print(3, "fallback to context %s" % act.action)
            return simulate_rb_action(keymap, act.action, pre_combo, combo)
    return None

def filter_rb_actions(keymap, rb_ctx, filt_act, action_list):
    """Filter a list of Rockbox actions, only keeping those that produce the requested
    action in the requested context

    Args:
        rb_ctx (RockboxContext): context in which the actions are performed
        rb_act (str): expected resulting action
        action_list (list of RockboxActionDesc): actions (the .action is ignored, only (pre)combos is used)

    Returns:
        sublist of action_list
    """
    new_list = []
    for act in action_list:
        res_act = simulate_rb_action(keymap, rb_ctx, act.pre_combo, act.combo)
        debug_print(2, "Simulate (%s,%s) in %s gives %s, match against %s"
                    % (act.pre_combo, act.combo, rb_ctx, res_act, filt_act))
        if res_act == filt_act:
            new_list.append(act)
    return new_list

def compare_latex_combos_one_sided(latex_combos, rb_combos):
    """Compare lists of combos and return sets of unmatched combos for both latex and rockbox"""
    unmatched_rb = []
    for rb_combo in rb_combos:
        is_unmatched = True
        # see if we can find the same combo in latex list
        if rb_combo in latex_combos:
            is_unmatched = False
        if not g_args.nomerge:
            # if this is a repeat action, and we can find the action without
            # repeat in both list, it is not unmatched
            if rb_combo.long:
                new_act = rb_combo._replace(long = False)
                if new_act in rb_combos and new_act in latex_combos:
                    is_unmatched = False
        # do merging if asked
        if is_unmatched:
            unmatched_rb.append(rb_combo)
    return unmatched_rb

def compare_latex_combos(latex_combos, rb_combos):
    """Compare lists of combos and return sets of unmatched combos for both latex and rockbox"""
    unmatched_rb = compare_latex_combos_one_sided(latex_combos, rb_combos)
    unmatched_latex = compare_latex_combos_one_sided(rb_combos, latex_combos)
    return (unmatched_latex, unmatched_rb)

def pretty_print_match_result(title, tot_list, unmatch_set):
    s = "  " + colored(title, 'white', attrs=['bold'])
    s += ": ["
    arr = []
    for act in tot_list:
        if act in unmatch_set:
            btn = colored(str(act), 'red', attrs=['bold'])
        else:
            btn = colored(str(act), 'green')
        # add raw
        if g_args.raw:
            btn += " (" + colored(act.raw, 'blue') + ")"
        arr.append(btn)
    s += ", ".join(arr)
    s += "]"
    print(s)

def pretty_print_suggestion(latex_action, latex_combo, rb_btn_combo):
    # do not print a suggestion if there is something we could not parse (because
    # the mismatch could be caused by that and we cannot suggest anything)
    has_raw_btn = False
    raw_btn_example = None
    for combo in latex_combo:
        if len(combo.buttons) == 0:
            has_raw_btn = True
            raw_btn_example = combo.raw
    if has_raw_btn:
        desc = "disabled by raw button ('%s')" % raw_btn_example
    else:
        btn_list = []
        for combo in rb_btn_combo:
            desc = "Long " if combo.long else ""
            desc += " + ".join(["\\" + btn + "{}" for btn in combo.buttons])
            btn_list.append(desc)
        desc = "\\newcommand{\\%s}{%s}" % (latex_action, " or ".join(btn_list))
    # print
    print("  %s %s" %(colored("Suggestion:", 'blue', attrs=['bold']), desc))

## Plugins
#
# Plugins are a mess, so we need more information about how/what to parse.
# Each plugin desc has the following keys (mandatory unless mentioned):
# - latex_file: name of the latex file (relative to manual/plugins) that contains the keymap (btnmap environement)
# - rb_file: name of the file (relative to apps/plugins) that contains the keymap
g_plugin_2048 = {
    'latex_file': '2048.tex',
    'rb_file': '2048.c',
}

g_plugin_metronome = {
    'latex_file': 'metronome.tex',
    'rb_file': 'metronome.c',
}

g_plugin_pictureflow = {
    'latex_file': 'pictureflow.tex',
    'rb_file': os.path.join('pictureflow', 'pictureflow.c')
}

# Plugin list: 'name_of_the_plugin': plugin_desc
# NOTE: the name of the plugin MUST NOT CONTAIN SPACES
g_plugin_list = {
    '2048': g_plugin_2048,
    'pictureflow': g_plugin_pictureflow,
    'metronome': g_plugin_metronome,
}

## Latex parser (yeah I know this crazy)
class LatexExpr:
    def __init__(text, type, **kwargs):
        self.text = text
        self.type = type
        self.__prop = kwargs
        for (key, arg) in kwargs.items():
            self[key] = arg

    def __repr__(self):
        return "LatexExpr(%s,%s,%s)" % (text, type, __prop)

class LatexParser:
    def __init__(text):
        self.text = text

    def next_char(self):
        try:
            return self.next()
        except StopIteration:
            return None

    def _next_token(self):
        

    def next_token(self):
        self.cur_token = _next_token()

    def cur_token(self):
        return self.cur_token

    def _parse(self):

    def parse(self):
        self.text_iter = iter(text)
        next_token()
        return _parse()

## Main

# argument parsing
args_epilog = """\
When merging is enabled, if a Rockbox action (e.g. ACTION_WPS_VOLDOWN) is enabled
by a key combo (e.g. BUTTON_VOL_UP) and the same combo with repeat (e.g. BUTTON_VOL_UP|BUTTON_REPEAT)
then the two actions are merged into one (the repeat combo is ignore). This is because
in such a case, the repeat is not documented in the manual since it is obvious.
"""
g_parser = argparse.ArgumentParser(epilog = args_epilog)
g_parser.add_argument("-v", "--verbose", help="increase output verbosity", action="count", default=0)
g_parser.add_argument("-p", "--printall", help="print all keys (even when it matches)", action="store_true")
g_parser.add_argument("-r", "--raw", help="print raw forms of buttons", action="store_true")
g_parser.add_argument("-m", "--nomerge", help="no merging", action="store_true")
g_parser.add_argument("-d", "--directory", help="run in given directory", default='')
g_parser.add_argument("-s", "--nosuggestion", help="do not print suggestions to fix", action="store_true")
g_parser.add_argument("-u", "--plugins", help="only parse specified plugins (comma separated, special values: 'all' and 'none')", default='all')
g_args = g_parser.parse_args()
# convert plugin list to array
if g_args.plugins == 'all':
    g_args.plugins = g_plugin_list.keys()
elif g_args.plugins == 'none':
    g_args.plugins = []
else:
    # check names
    list = g_args.plugins.split(',')
    g_args.plugins = []
    for plugin_name in list:
        if plugin_name not in g_plugin_list:
            print(colored("Warning: ", 'cyan', attrs=['bold']) + "I do not know plugin "
                  + colored(plugin_name, 'red', attrs=['bold']))
            continue
        g_args.plugins.append(plugin_name)

# the script must be run from the build directory: check for autoconf.h
try:
    open(os.path.join(g_args.directory, 'autoconf.h'), 'r').close()
except IOError as err:
    debug_print(1, err)
    die("Cannot open 'autoconf.h', you must run this script from the build directory")

# name of the subdirectory where we create files
g_checkmanual_subdir = "checkmanual"
g_checkmanual_dir = os.path.join(g_args.directory, g_checkmanual_subdir)
def cm_path(filename):
    return os.path.join(g_checkmanual_dir, filename)
# create it
try:
    os.mkdir(g_checkmanual_dir)
except: pass

# parse Makefile
g_makefile_db = parse_makefile(os.path.join(g_args.directory, 'Makefile'))
debug_print(1, g_makefile_db)
# parse latex platform file (in manualdir/platform/<manualdev>.tex)
(g_latex_platform_options, g_latex_keymap_file) = parse_latex_platform(os.path.join(
    g_makefile_db['manualdir'], 'platform', '%s.tex' % g_makefile_db['manualdev']))
debug_print(1, g_latex_platform_options)
debug_print(1, g_latex_keymap_file)

g_preprocess_list = []
def add_to_preprocess_list(rb_file, output_file):
    g_preprocess_list.append((rb_file, output_file))
# build list of files that need preprocessing
add_to_preprocess_list(os.path.join(g_makefile_db['rootdir'], 'apps', 'action.h'), 'action.i')
add_to_preprocess_list(os.path.join(g_makefile_db['rootdir'], 'apps', 'plugins', 'lib', 'pluginlib_actions.c'), 'pluginlib_action.i')
def plugin_file(plugin_name, filename):
    return plugin_name + '__' + filename
for plugin_name in g_args.plugins:
    plugin_desc = g_plugin_list[plugin_name]
    # preprocess rockbox file containing the keymap and standardize the name
    infile = os.path.join(g_makefile_db['rootdir'], 'apps', 'plugins', plugin_desc['rb_file'])
    add_to_preprocess_list(infile, plugin_file(plugin_name, 'rb_file.i'))

# extract rockbox keymap and preprocess files
(g_rb_info, g_rb_deflist) = get_rb_info(g_args.directory, g_checkmanual_subdir, g_preprocess_list)
debug_print(1, g_rb_info)
debug_print(2, g_rb_deflist)
g_rb_keymap = get_rb_keymap(g_rb_info['keymap-filename'])
debug_print(3, g_rb_keymap)
pretty_print_rockbox_keymap(1, g_rb_keymap)

# parse all keymaps files (in manualdir/platform/keymap-*.tex) to get the list
# of all possible actions
g_latex_list_of_actions = set()
for file in glob.glob(os.path.join(g_makefile_db['manualdir'], 'platform', 'keymap-*.tex')):
    (latex_map,_,_) = parse_latex_keymap(file)
    g_latex_list_of_actions.update(latex_map.keys())
    latex_map = None
debug_print(1, "list of all actions: %s" % g_latex_list_of_actions)
# parse action.h to get the complete list of contexts and actions
(g_rb_list_of_actions, g_rb_list_of_contexts) = get_rb_action_and_context_lists(
    cm_path('action.i'))
# parse pluginlib_action.c to get pluginlib keymap
(g_rb_plugnlib_keymap, _) = get_rb_plugin_keymap(cm_path('pluginlib_action.i'))
debug_print(2, g_rb_plugnlib_keymap)
pretty_print_rockbox_keymap(1, g_rb_plugnlib_keymap)
# parse latex keymap (in manualdir/platform/keymap-<manualdev>.tex)
(g_latex_keymap, g_latex_btnlist, g_latex_pluginlibmap) = parse_latex_keymap(os.path.join(
    g_makefile_db['manualdir'], 'platform', g_latex_keymap_file))
pretty_print_latex_keymap(1, g_latex_keymap)
debug_print(2, g_latex_keymap)
debug_print(1, g_latex_btnlist)
# make sure the latex keymap is complete (ie has entries for all possible keys)
# by adding empty entries
for act in g_latex_list_of_actions:
    if act not in g_latex_keymap:
        g_latex_keymap[act] = []
# now compare latex keymap and rockbox keymap
# sort by name so that the output is stable
are_there_mismatched = False
for (latex_act, latex_combo) in sorted(g_latex_keymap.items(), key=lambda x: x[0]):
    # map latex action name to rockbox (context,action)
    (rb_ctx, rb_act) = map_latex_to_rb(latex_act)
    debug_print(1, "%s -> (%s,%s)" % (latex_act, rb_ctx, rb_act))
    if rb_ctx is None:
        continue
    # sanity check: to warn the user about potential errors in conversion, we match
    # it against the list of all possible Rockbox context/actions
    if rb_ctx not in g_rb_list_of_contexts:
        msg = "could not find context %s in action.h, this may" % rb_ctx
        print("%s: %s" % (colored("Warning", 'cyan', attrs=['bold']), msg))
        print("         indicate a parsing problem, or you need to add a manual entry for the")
        print("         latex -> rockbox mapping, latex action was %s" % latex_act)
        continue
    if rb_act not in g_rb_list_of_actions:
        msg = "could not find action %s in action.h, this may" % rb_act
        print("%s: %s" % (colored("Warning", 'cyan', attrs=['bold']), msg))
        print("         indicate a parsing problem, or you need to add a manual entry for the")
        print("         latex -> rockbox mapping, latex action was %s" % latex_act)
        continue
    # promote ctx string to RockboxContext
    if isinstance(rb_ctx, str):
        rb_ctx = RockboxContext([rb_ctx])
    # use rockbox keymap to find corresponding key combo
    rb_btn_action = search_rb_keymap(g_rb_keymap, rb_ctx,
        lambda act_desc: act_desc.action == rb_act)
    debug_print(2, rb_btn_action)
    # we have a list of potential actions but some might not be feasible, for example
    # the WPS context may fallback to STD and have two combos for an action, but
    # the combo for STD is hidden by another in WPS. Thus for each combo, we need
    # to 'simulate' the combo to see if it really produces the action
    rb_btn_action = filter_rb_actions(g_rb_keymap, rb_ctx, rb_act, rb_btn_action)
    # convert each action in a LatexButtonCombo
    rb_btn_combo = convert_rb_action_to_combo(rb_btn_action, g_latex_btnlist)
    debug_print(1, "Rockbox: %s" % rb_btn_combo)
    debug_print(1, "Latex: %s" % latex_combo)
    # check if it matches
    (unmatched_latex, unmatched_rb) = compare_latex_combos(latex_combo, rb_btn_combo)
    match = len(unmatched_latex) == 0 and len(unmatched_rb) == 0
    if not match:
        are_there_mismatched = True
    # print result
    if g_args.printall or not match:
        desc = "%s (%s, %s)" % (colored(latex_act, 'white', attrs=['bold']), rb_ctx, rb_act)
        err = "Mismatch" if not match else ""
        print("%s: %s" % (desc, colored(err, 'red', attrs=['bold'])))
        pretty_print_match_result("Rockbox", rb_btn_combo, unmatched_rb)
        pretty_print_match_result("Latex", latex_combo, unmatched_latex)
        # if there is a mismathc, print a suggestion
        if not match and not g_args.nosuggestion:
            pretty_print_suggestion(latex_act, latex_combo, rb_btn_combo)
# do plugins
for plugin_name in g_args.plugins:
    plugin_desc = g_plugin_list[plugin_name]
    # parse the rockbox file in search for keymap
    (plugin_keymap, plugin_maplist) = get_rb_plugin_keymap(cm_path(plugin_file(plugin_name, 'rb_file.i')))
    debug_print(0, "%s: %s" % (plugin_name, plugin_keymap))
    debug_print(0, "%s: %s" % (plugin_name, plugin_maplist))
    print(colored("Plugin: %s" % plugin_name, 'yellow', attrs=['bold']))
# do not print nothing in case everything matches
if not are_there_mismatched:
    print(colored("No mismatches found", 'green', attrs=['bold']))
