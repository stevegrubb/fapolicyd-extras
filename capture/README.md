# facapture Quick Start Guide

This guide explains how to use `facapture` to record the last portion of `fapolicyd --debug` output whenever a specific string (for example `deny`) appears. The utility is intended to be invoked from the top-level repository directory as `./facapture`.

## Prerequisites

- `fapolicyd` built with debug support, so that `fapolicyd --debug` emits the desired diagnostic stream. The daemon's debug output is written to **stderr**, so be sure to pipe both stdout and stderr when feeding `facapture`.
- Python 3 is required because `facapture` is implemented as a Python script.

## Basic Usage

```
fapolicyd --debug |& ./facapture -n <buffer-size> -m <match-string> <output-file>
```

- `-n/--buffer-size <buffer-size>`: The number of lines kept in the ring buffer. Only the most recent `N` lines are retained, so choose a size large enough to capture the context you need.
- `-m/--match <match-string>`: Plain string that is searched for in the debug output **after** ANSI color codes have been stripped. Matching only begins once `facapture` sees the line `Starting to listen for events`, allowing `fapolicyd` to finish initializing before the watch is armed.
- `<output-file>`: Path to the file that will receive the buffered log lines once the match is detected (plus two additional lines after the match). The lines are written oldest-to-newest and contain no ANSI color sequences.

`facapture` tees all incoming output to your terminal immediately. When the match fires, it prints a `[facapture]` notification followed by the stripped match line so you can distinguish the triggering moment from ongoing daemon output.

## Keeping fapolicyd Running After a Match

By default `facapture` exits as soon as it captures the match plus two follow-up lines. If the tool is sitting in a pipeline directly connected to `fapolicyd --debug`, that exit will close the pipe and cause the daemon to stop. (SIGPIPE when unhandled means TERM.) To keep `fapolicyd` running, add `--keepalive` and make sure `facapture` is reading from a pipe:

```
fapolicyd --debug |& ./facapture -n 200 -m deny capture.log --keepalive
```

With `--keepalive`, once the buffer snapshot is finalized `facapture` hands the stream off to a background `cat` process so that `fapolicyd` can continue emitting debug data until you manually stop it. The saved log file remains frozen at the moment the trigger fired.

## Inspecting the Captured Log

After `facapture` terminates (or immediately when `--keepalive` is used), review the captured lines:

```
cat capture.log
```

The file will contain the final `N` lines captured before the match, the match line, and the two lines that followed, all without ANSI color codes. This makes it suitable for sharing in tickets or parsing with other tools. It also saves you from having GB sized files with an interesting event somewhere in there.

## Triggering on a specific rule

If you have a specic rule that you want to capture, then you would want to do the following. Use fapolicyd-cli to list the rules. Find the rule number you want to match on. And use it in the facapture command.
```
fapolicyd-cli --list
-> %languages=application/x-bytecode.ocaml,application/x-bytecode.python,application/java-archive,text/x-java,application/x-java-applet,application/javascript,text/javascript,text/x-awk,text/x-gawk,text/x-lisp,application/x-elc,text/x-lua,text/x-m4,text/x-nftables,text/x-perl,text/x-php,text/x-python,text/x-R,text/x-ruby,text/x-script.guile,text/x-tcl,text/x-luatex,text/x-systemtap
1. allow perm=any uid=0 : dir=/var/tmp/
2. allow perm=any uid=0 trust=1 : all
3. allow perm=open exe=/usr/bin/rpm : all
4. allow perm=open exe=/usr/bin/python3.13 comm=dnf : all
5. deny_audit perm=any pattern=ld_so : all
...
fapolicyd --debug |& ./facapture -n 100 -m 'rule=5' capture.log --keepalive
```

## Troubleshooting Tips

- If you never see `Starting to listen for events`, ensure `fapolicyd --debug` is starting cleanly and not failing during initialization.
- If you request `--keepalive` but are running `facapture` directly (no piped input), the flag is ignored because there is no upstream process to keep alive.
- When using a small buffer size, it is possible that older context lines are overwritten before the match occurs. Increase `-n` if you need a longer history.
