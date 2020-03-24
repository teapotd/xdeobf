# xdeobf
Experimental deobfuscation plugin for IDA 7.2. It aims to reverse [control flow flattening](https://github.com/obfuscator-llvm/obfuscator/wiki/Control-Flow-Flattening) transformation that I encountered (probably a variation of [obfuscator-llvm](https://github.com/obfuscator-llvm/obfuscator/wiki/Control-Flow-Flattening)).
The plugin most likely won't work out of the box, as it was designed to handle a specific binary.

## Approach

1. Switch reconstruction - in LOCOPT phase
   - find dispatcher variable
   - find mapping from dispatcher variable values to blocks (switch cases)
   - copy blocks reachable from multiple switch cases (so that subgraphs for different cases don't overlap)
   - finally create NWAY block for recovered switch cases
2. Unflattening - in GLBOPT1 phase
   - find dispatcher switch and variable again
   - recover succesors for each switch case

xdeobf, unlike [HexRaysDeob](https://github.com/RolfRolles/HexRaysDeob), runs in two microcode optimization phases. Switch reconstruction in LOCOPT phase allows HexRays decompiler to optimize away conditions that change dispatcher variable better, before attempting to recover control flow in GLBOPT1 phase.

## References

- [HexRaysDeob](https://github.com/RolfRolles/HexRaysDeob) (and [blog post](https://www.hexblog.com/?p=1248)) by RolfRolles
