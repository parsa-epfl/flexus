#!/usr/bin/python3

import os
import sys

import re
import shlex
import subprocess


def find(fn):
    if not (dn := os.path.dirname(fn)):
        dn = '.'

    if not os.path.isdir(dn):
        return None

    fn = os.path.abspath(os.path.join(dn, fn))

    if os.path.isfile(fn):
        return fn

    return None


def norm(env, fn):
    pat = re.compile(r'\$\{([^}]*)\}')

    while mat := pat.search(fn):
        sp = mat.span(0)
        fn = fn[:sp[0]] + env.get(mat.group(1), '') + fn[sp[1]:]

    if fn.find(os.sep) < 0:
        return fn

    old = fn.split(os.sep)
    ret = []

    # explicit root
    if fn.startswith(os.sep):
        ret.append('')

    for i in old:
        if i == '.':
            continue
        elif i == '..':
            if ret:
                ret.pop()
        elif i:
            ret.append(i)

    return os.sep.join(ret)


def test(env, cs, opts):
    last = cs[-1] if cs else True
    cond = False

    for op in opts:
        if op == 'OR':
            continue

        cond = cond or op in env

    cs.append(cond and last)


def kick(env, args):
    if 'expect' in env:
        import pexpect

        log = env.get('logfile', '')

        return pexpect.spawn(args[0],
                    args    = args[1:],
                    timeout = None,
                    logfile = open(log, 'wb') if log else None)

    out = env.get('stdout', '')
    err = env.get('stderr', '')
    fdo = 0
    fde = 0

    if out:
        fdo = os.open(out, os.O_RDWR | os.O_CREAT)
    if err == out or err == '-':
        fde = fdo
    elif err:
        fde = os.open(err, os.O_RDWR | os.O_CREAT)

    if fdo:
        os.dup2(fdo, 1)
    if fde:
        os.dup2(fde, 2)

    if 'daemon' in env:
        if (pid := os.fork()) < 0:
            sys.exit(f'ERROR: 1st fork failed')
        elif pid > 0:
            return

        os.setsid()

        if (pid := os.fork()) < 0:
            sys.exit(f'ERROR: 2nd fork failed')
        elif pid > 0:
            return

    if 'wait' in env:
        return subprocess.run(args, stdout = fdo, stderr = fde)
    else:
        os.execvp(args[0], args)


def runq(env, *opts):
    real = []

    # defaults
    env['smp'] = '1'

    for op in opts:
        if op.startswith('+'):
            if sp := op[1:].split('='):
                env[sp[0]] = sp[1] if len(sp) > 1 else '1'
        elif op:
            real.append(op)

    flag = not real or real[0].startswith('-')

    qemu = find('qemu')
    conf = find('qemu.cfg' if flag else real[0])

    if not conf or not qemu:
        return

    # always provided
    env['root'] = os.path.dirname(os.path.abspath(conf))

    args = [qemu]
    curr = []
    cond = []

    with open(conf) as fd:
        for cs in fd:
            if cs.startswith(('#', '//')):
                continue

            if cs.startswith('IF '):
                test(env, cond, cs.split()[1:])
                continue

            if cs.startswith('ENDIF'):
                if cond:
                    cond.pop()
                continue

            if cond and not cond[-1]:
                continue

            if not (sp := shlex.split(cs)):
                continue

            if cs.startswith((' ', '\t')):
                curr.append('='.join(list(map(lambda x: norm(env, x), sp[:2]))))

            else:
                if curr:
                    args.append(','.join(curr))
                    curr = []

                args.append('-{}'.format(sp[0]))

        if curr:
            args.append(','.join(curr))

    # especially for -S -s or -d ...
    args.extend(real[0 if flag else 1:])

    if 'dbg' in env:
        args = ['gdb', '--args'] + args

    if 'dry' in env:
        print(' '.join(args))
    else:
        return kick(env, args)


if __name__ == '__main__':
    env = {}
    env.update(os.environ)

    runq(env, *sys.argv[1:])
