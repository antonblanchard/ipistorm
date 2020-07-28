# ipistorm

Create lots of IPIs! Lock up your box!

## Building

```make```

## Examples

Use `vmstat 1` to measure the interrupt rate. When finished you need to
remove the module before trying again. If you want to oops your kernel
you can remove the module before it finishes.

Doorbells to neighbouring small core on a POWER9 big core box:

```insmod ./ipistorm.ko timeout=10 single=1 offset=1 mask=7```

Doorbells to same small core on a POWER9 big core box:

```insmod ./ipistorm.ko timeout=10 single=1 offset=2 mask=7```

Doorbells to same small core on a POWER9 small core box:

```insmod ./ipistorm.ko timeout=10 single=1 offset=1 mask=3```

XIVE interrupts:

```# insmod ./ipistorm.ko timeout=10 single=1 offset=8```
