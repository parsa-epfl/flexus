#!/usr/bin/env python

import lex, yacc
import os, re, string, sys

test_only = 0

#### LEX

tokens = ('COMMENT', 'COLON', 'COMMA', 'ID', 'LBRAC', 'LINECOMMENT', 'LPAR',
          'OBJECT', 'RAWEND', 'RAWSTART', 'RBRAC', 'RPAR', 'STRING', 'TYPE' )

t_LPAR     = r'\('
t_RPAR     = r'\)'
t_LBRAC    = r'{'
t_RBRAC    = r'}'
t_COLON    = r':'
t_COMMA    = r','
t_RAWEND   = r']'
t_RAWSTART = r'\[R'
t_STRING   = r'""|".*?[^\\]"'

# use functions for most tokens to keep parse order

def t_COMMENT(t):
    r'/\*.*?\*/'
    pass

def t_LINECOMMENT(t):
    r'\#.*\n'
    pass

def t_ID(t):
    r'(\.|[A-Za-z0-9+/_-])+'
    if t.value in ['OBJECT', 'TYPE']:
        t.type = t.value
    return t

def t_newline(t):
    r'\n+'
    t.lineno += len(t.value)

t_ignore  = ' \t'

def t_error(t):
    print "Illegal character '%s'" % t.value[0]
    t.skip(1)

#### YACC

def p_configuration(p):
    'configuration : objects'
    p[0] = p[1]

def p_objects(p):
    'objects : objects object'
    p[0] = p[1] + [p[2]]

def p_objects_empty(p):
    'objects : empty'
    p[0] = []

def p_object(p):
    'object : OBJECT ID TYPE ID LBRAC attributes RBRAC'
    p[0] = [p[2], p[4], p[6]]

def p_attributes(p):
    'attributes : attributes attribute'
    p[0] = p[1] + [p[2]]

def p_attributes_empty(p):
    'attributes : empty'
    p[0] = []

def p_attribute(p):
    'attribute : ID COLON value'
    p[0] = [p[1], p[3]]

def p_value_id(p):
    'value : ID'
    p[0] = p[1]

def p_value_string(p):
    'value : STRING'
    p[0] = p[1]

def p_value_dict(p):
    'value : LBRAC dictlist RBRAC'
    p[0] = p[2]

def p_dictlist(p):
    'dictlist : dictlist COMMA STRING COLON value'
    p[0] = { p[3]: p[5] }
    p[0].update(p[1])

def p_dictlist_value(p):
    'dictlist : STRING COLON value'
    p[0] = { p[1]: p[3] }

def p_value_raw(p):
    'value : RAWSTART ID STRING ID RAWEND'
    p[0] = (p[2], p[3], p[4])

def p_value_list(p):
    'value : LPAR valuelist RPAR'
    p[0] = p[2]

def p_valuelist(p):
    'valuelist : valuelist COMMA value'
    p[0] = p[1] + [p[3]]

def p_valuelist_value(p):
    'valuelist : value'
    p[0] = [p[1]]

def p_valuelist_empty(p):
    'valuelist : empty'
    p[0] = []

def p_empty(p):
    'empty :'
    pass

# dummy rule to avoid unused warnings
def p_dummy(p):
    'configuration : COMMENT LINECOMMENT'
    pass

def p_error(p):
    print "Syntax error in input:", p
    sys.exit(1)

####

def init_parser():
    lex.lex(debug = 0, optimize = 1)
    yacc.yacc(debug = 0, optimize = 1)

def attr_string(a):
    if type(a) == type(()):
        return '[R %s "%s" %s ]' % (a[0], rawfile, a[2])
    if type(a) == type([]):
        val = "("
        first = ""
        for aa in a:
            val += first + attr_string(aa)
            first = ", "
        return val + ")"
    if type(a) == type({}):
        val = "{"
        first = ""
        for k, v in a.items():
            val += first + attr_string(k) + ':' + attr_string(v)
            first = ", "
        return val + "}"
    return a

def write_configuration(conf, filename):
    try:
        file = open(filename, "w")
    except Exception, msg:
        print "Failed opening output file '%s': %s" % (filename, msg)
        raise Exception
    for o in conf:
        file.write("OBJECT %s TYPE %s {\n" % (o[0], o[1]))
        for a in o[2]:
            file.write("\t%s: %s\n" % (a[0], attr_string(a[1])))
        file.write("}\n")
    file.close()

def read_configuration(filename):
    try:
        file = open(filename)
    except:
        print "Failed opening checkpoint file '%s'" % filename
        raise Exception
    try:
        return yacc.parse(string.join(file.readlines()))
    except Exception, msg:
        print "Failed parsing checkpoint file '%s'" % filename
        print "Error: %s" % msg
        sys.exit(1)

def get_attr(conf, o, a):
    try:
        [obj] = [x for x in conf if x[0] == o]
        [attr] = [x for x in obj[2] if x[0] == a]
        return attr[1]
    except:
        return None

def set_attr(conf, o, a, v):
    try:
        [obj] = [x for x in conf if x[0] == o]
        [attr] = [x for x in obj[2] if x[0] == a]
        attr[1] = v
    except:
        pass

def file_exists(filename):
    try:
        os.stat(filename)
        return filename
    except:
        return None


def run_command(cmd):
    print 'Executing:', cmd
    if not test_only:
      res = os.system(cmd)
    else:
      res = 0
    if res != 0:
        print
        print "Command returned an error, checkpoint not merged!"
        print "Command string: %s" % cmd
        print
        sys.exit(1)

def expand_checkpoint_path(f, path):
    f = f.strip('"')
    match = re.match("%([0-9]*)%(.*)", f)
    if match:
        return "%s%s" % (path[int(match.group(1))], match.group(2))
    else:
        return f

# Tom's stuff starts Here

def fix_consoles(conf):
    consoles = [o[0] for o in conf if o[1] == "text-console"]
    for con in consoles:
      set_attr(conf, con, "output_file", '""')

def fix_freq(conf):
    cpus = [o[0] for o in conf if o[1] == "ultrasparc-iii"]
    for cpu in cpus:
      set_attr(conf, cpu, "freq_mhz", 1000)

def fix_simics_path(conf):
    paths = get_attr(conf, "sim", "simics_path")
    set_paths = set( paths )
    paths = [ x for x in set_paths ]
    set_attr(conf, "sim", "simics_path", paths)

#def fix_timing_model(conf):
#    "Strip timing_model attributes from all cpus
#    set_paths = sets.Set().union( paths )
#    paths = [ x for x in set_paths ]
#    set_attr(conf, "sim", "simics_path", paths)

def getCheckpointPaths(conf):
    return get_attr(conf, "sim", "checkpoint_path")

def markPaths(paths):
    "Checks assumptions on checkpoint paths for phase jobs"
    marked_paths = []
    n = 0
    phasere = re.compile('.*/phase_(...)/simics')
    for path in paths[0:]:
      print 'base=', path
      match = phasere.match(path)
      if match:
        print 'Last: ', path
        break
      marked_paths.append( [n, 'b' , 0 ] )
      n = n + 1
    for path in paths[n:]:
      print '1path=', path
      match = phasere.match(path)
      if not match:
        # Found last phase, better be a flexpoint next
        print 'Last: ', path
        break
      marked_paths.append( [n, 'p' , match.group(1) ] )
      n = n + 1
    j = 1 
    print 'n=', n
    for path in paths[n:]:  
      if not path.endswith( '/flexpoint_%(#)03d/simics' % { '#': j } ):
        print 'Path:', path, 'does not appear to be a phase or flexpoint in an appropriate sequence'
        sys.exit(1)
      marked_paths.append( [ n, 'f' , j ] )
      j = j + 1
      n = n + 1
    print 'Checkpoint path sequence appears valid'
    i = 0
    for path in paths:
      print path, marked_paths[i]
      i = i + 1
    return marked_paths

def collapse(ckpt_paths, dirname, newest, preceeding):
    newest = os.path.basename(newest.strip('"'))
    full_preceeding =  map( lambda x: expand_checkpoint_path(x,ckpt_paths), preceeding)
    print 'ckpt_paths=',ckpt_paths,'\ndirname=',dirname,'\nnewest=',newest,'\npreceeding=',preceeding,'\nfull_preceeding=',full_preceeding
    run_command('if [[ ! -e ' + os.path.join(dirname, 'orig.' + newest) + ' ]] ; then mv ' + os.path.join(dirname, newest) + ' ' + os.path.join(dirname, 'orig.' + newest) + ' ; fi ')
    run_command('craff -o ' + os.path.join(dirname, newest)  + ' ' + ' '.join(full_preceeding) + ' ' + os.path.join(dirname, 'orig.' + newest))

def getIndex(f):
    f = f.strip('"')
    match = re.match("%([0-9]*)%(.*)", f)
    if match:
      if f.find(basename) != -1: # MF: added this if() because above re.match is always true at EPFL
        return 1000              #     which caused nothing to be collapsed (because 1000 was never returned)
      return int( match.group(1))
    else:
      return 1000 # Really big number for files that belong to the current flexpoint

def doFlexpointImages(conf, ckpt_paths, marks, this_flexpoint, dirname):
    images = [o[0] for o in conf if o[1] == "image"]
    for i in images:
      files = get_attr(conf, i, "files")
      if not files:
        continue
      # Filter out corner cases
      if len(files) <= 1:
        continue
      if getIndex(files[-2][0]) == 1000:
        continue # Second to last file is not from a phase or flexpoint.  Don't touch this image

      print '\n-------------- NEXT --------------\n'
      print 'Image:', i
      print 'Old files:', files
      new_files_r = []
      collapse_files_r = []
      last_idx = getIndex(files[-1][0])

      collapse_every_n_flexpoints = 5
      if last_idx == 1000:
        # New file created this flexpoint.
        preserve_range = range( this_flexpoint - (this_flexpoint % collapse_every_n_flexpoints),this_flexpoint)
        collapse_range = None
        preserved_a_file = 0
        preserved_a_phase = 0
        file_this_flexpoint = files[-1][0]

        files.reverse()
        for file in files:
          idx = getIndex(file[0])
          if idx == 1000:
            print 'File: ', file, ' idx: ', idx
            # Preserve the new file
            new_files_r.append(file)
          elif marks[idx][1] == 'b':
            print 'File: ', file, ' idx: ', idx, ' b'
            # Preserve base images
            new_files_r.append(file)
          elif marks[idx][1] == 'p':
            print 'File: ', file, ' idx: ', idx, ' p'
            # Preserve the first phase we encounter.
            #if not preserved_a_phase:
            #  preserved_a_phase = 1
            #  new_files_r.append(file)
            new_files_r.append(file)
          elif marks[idx][1] == 'f':
            print 'File: ', file, ' idx: ', idx, ' fp_idx: ', marks[idx][2]
            fp_idx = marks[idx][2]
            if fp_idx in preserve_range:
              preserved_a_file = 1 # Implies we won't have to do any collapsing
              new_files_r.append(file)
            elif preserved_a_file == 0:
              if collapse_range == None:
                # This flexpoint is the first flexpoint in its range to produce
                # output, so we need to collapse in from the previous range.
                # Lets determine collapse_range
                collapse_range = range( fp_idx - (fp_idx % collapse_every_n_flexpoints), fp_idx )
                collapse_files_r.append( file[0] )
              elif fp_idx in collapse_range:
                collapse_files_r.append( file[0] )
              else:
                # A file that is older than collapse range.  Drop it.
                pass
            else:
              # We have already preserved a file, so we can drop files that
              # are not in preserve_range
              pass
          else:
            assert 0

        if len( collapse_files_r ) > 0:
          print 'collapse: '
          collapse_files_r.reverse()
          collapse( ckpt_paths, dirname, file_this_flexpoint, collapse_files_r)
        new_files_r.reverse()
        print 'New files', new_files_r
        set_attr(conf, i, "files", new_files_r)
      else:
        print 'last index is not 1000: '
        # No new file created this flexpoint
        preserve_range = None
        preserved_a_phase = 0

        files.reverse()
        for file in files:
          idx = getIndex(file[0])
          if idx == 1000:
            # Preserve non-ckpt-path images
            new_files_r.append(file)
          elif marks[idx][1] == 'b':
            # Preserve base images
            new_files_r.append(file)
          elif marks[idx][1] == 'p':
            # Preserve the first phase we encounter.
            #if not preserved_a_phase:
            #  preserved_a_phase = 1
            #  new_files_r.append(file)
						new_files_r.append(file)  # BTG changed -- was dropping phases that weren't collapsed ??
          elif marks[idx][1] == 'f':
            fp_idx = marks[idx][2]
            if preserve_range == None:
              preserve_range = range( fp_idx - (fp_idx % collapse_every_n_flexpoints), fp_idx )
              new_files_r.append(file)
            elif fp_idx in preserve_range:
              new_files_r.append(file)
            else:
              # We have already preserved a file, so we can drop files that
              # are not in preserve_range
              pass
          else:
            assert 0
        new_files_r.reverse()
        print 'New files', new_files_r
        set_attr(conf, i, "files", new_files_r)




def doPhaseImages(conf, ckpt_paths, dirname):
    images = [o[0] for o in conf if o[1] == "image"]
    for i in images:
      files = get_attr(conf, i, "files")
      if len(files) > 1:
        print
        print 'Image:', i
        print 'Old files:', files
        last_file = files[-1]
        if last_file[0].startswith('"%0%'):
          # Don't collapse or modify image objects where the last image is
          # relative to %0%
          continue
        elif not files[0][0].startswith('"%'):
          # Don't touch images whose first file is not relative to %
          print 'NO TOUCHING!!!'
          continue
        else:
          # Get all the %0% files in new_files
          new_files = [ f for f in files if f[0].startswith('"%0%') ]
          if last_file[0].startswith('"%'):
            # There is no new image for this phase, but some intermediate phase
            # did modify the image since %0%.  Preserve the last entry, drop
            # the rest
            new_files.append( last_file )
          else:
            # There is a new image for this phase.  See if the prior image
            # was not a base image
            if len(files) > 2:
              if files[-2][0].startswith('"%0%'):
                # This phase created an image, and the preceding image is a base.
                # We do not modify any files
                continue
              elif files[-2][0].startswith('"%'):
                # Collapse the preceeding file into this one.
                collapse( ckpt_paths, dirname, last_file[0], [ files[-2][0] ] )
                new_files.append( last_file )
              else:
                # This image does not match a pattern we recognize.
                # We do nothing
                continue
        print 'New files:', new_files
        set_attr(conf, i, "files", new_files)





def processPaths(conf, dirname):
    ckpt_paths = [ x.strip('"') for x in getCheckpointPaths(conf) ]
    marks = markPaths(ckpt_paths)
    is_flexpoint = basename.startswith('flexpoint_')
    number = int ( basename[-3:] )

    if not is_flexpoint:
      print 'Processing phase', number
      phases = [ x for x in marks if x[1] == 'p' ]
      if phases:
        doPhaseImages( conf, ckpt_paths, dirname )
    else:
      # Compute indices of first and last phases and flexpoints
      doFlexpointImages( conf, ckpt_paths, marks, number, dirname )



####

if len(sys.argv) < 2:
    print "Usage:"
    print
    print "%s <simics checkpoint file>" % sys.argv[0]
    print
    sys.exit(1)

if os.system("craff -h 2>&1 > /dev/null") != 0:
    print "\nCan't find craff utility in path.\n"
    sys.exit(1)

src = sys.argv[1]
basename = os.path.basename(src)
dirname = os.path.dirname(src)
rawfile = "%s.raw" % basename


init_parser()

conf = read_configuration(src)
run_command("if [[ ! -e %s.premunge ]] ; then cp %s %s.premunge ; fi" % (src, src, src))

# backup configuration file

processPaths(conf, dirname)
fix_consoles(conf)
#fix_freq(conf)
fix_simics_path(conf)

if test_only:
  write_configuration(conf, 'test_output')
else:
  write_configuration(conf, src)

print
print "Finished writing merged checkpoint: %s" % src
print "Try the new checkpoint before removing old ones."
print
sys.exit(0)
