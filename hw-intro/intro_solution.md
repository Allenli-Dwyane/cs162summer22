# Counting Words

The solution can be found in [main.c](./words/main.c) and [word_count.c](./words/word_count.c)

# User Limits 

Run the 

```bash
$ man getrlimt
```

to see the manual page of getrlimt. And we can get the following:

```c
SYNOPSIS
       #include <sys/time.h>
       #include <sys/resource.h>

       int getrlimit(int resource, struct rlimit *rlim);
```

We can see that the 'getrlimit' function requires two arguments, one is the 'resource' and another is the 'rlim'.

* struct rlimit *rlim:

    The rlimit structure has the following definition:

    ```c
    struct rlimit{
        rlim_t rlim_cur; // soft limit
        rlim_t rlim_max; // hard limit
    };
    ```

    The  soft  limit  is  the value that the kernel enforces for the corresponding resource.  The hard limit acts  as  a  ceiling  for  the  soft limit:  an  unprivileged process may set only its soft limit to a value in the range from 0 up to the hard limit, and (irreversibly) lower  its hard   limit.

* int resources:

    From the manual page of getrlimit, we can tell the requirements for the resource argument.

    ```
    The resource argument must be one of:

       RLIMIT_AS
              This  is  the  maximum size of the process's virtual memory (address space). 

       RLIMIT_CORE
              This  is  the maximum size of a core file (see core(5)) in bytes
              that the process may dump.
       ...        
    ```

    The useful resource argument for this homework are:
    1. RLIMIT_STACK
    2. RLIMIT_NPROC
    3. RLIMIT_NOFILE

Moreover, the 'prlimit' syscall can achieve the functions of both 'getrlimit' and 'setrlimit'.

```c
int prlimit(pid_t pid, int resource, const struct rlimit *new_limit, struct rlimit *old_limit);
```

* The resource argument has the same meaning as for setrlimit() and getrlimit().

* The new_limit argument and the old_limit argument: If new_limit is NULL, then the rlimit  structure  to which  it points is used to set new values for the soft and hard limits for resource. f the old_limit argument is a not NULL, then a successful  call to prlimit() places the previous soft and hard limits for resource in the rlimit structure pointed to by old_limit.

* The pid argument specifies the ID of the process on which the  call  is to operate. If pid is 0, then the call applies to the calling process.

Therefore, the example of retrieving user limits is as follows:

```c
if(prlimit(pid, RLIMIT_*, NULL, &old_rlimit) == -1)
    error();
```

The solution can be found in [limits.c](./limits.c).

# GDB basics

1. 
   ```bash
    gdb map
   ```

2. 
    ```bash
    (gdb) b main
    Breakpoint 1 at 0x699: file map.c, line 16.
    ```

3. 
    ```bash
    (gdb) r
    Starting program: /home/vagrant/code/personal/hw-intro/map

    Breakpoint 1, main (argc=1, argv=0x7fffffffdec8) at map.c:16
    ```

4. 
    ```bash
    (gdb) p &argv
    $1 = (char ***) 0x7fffffffddb0
    ```