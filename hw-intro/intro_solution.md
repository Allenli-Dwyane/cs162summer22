# Counting Words

The solution can be found in [main.c](./words/main.c) and [word_count.c](./words/word_count.c)

# User Limits 


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