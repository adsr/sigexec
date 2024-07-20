# sigexec

sigexec forks a process and then waits for SIGUSR1 before exec-ing the given
command. I find this useful occasionally.

### Synopsis

```console
$ ./sigexec sh -c 'echo start; sleep 1; echo done'
sigexec: Forked 1283853
^Z
[1]+  Stopped                 ./sigexec sh -c 'echo start; sleep 1; echo done'
$ bg
[1]+ ./sigexec sh -c 'echo start; sleep 1; echo done' &
$ # Do whatever to child proc 1283853
$ # Start child proc by sending SIGUSR1 to parent
$ kill -USR1 %1 && fg
./sigexec sh -c 'echo start; sleep 1; echo done'
start
done
$ 
```
