# List of Built-in Native Function

| Name | Description |
| --- | --- |
| `write(buffer: Mem_Addr, buffer_size: Unsigned_Int)` | Write a buffer of a particular size to the standard output |
| `external(ptr: Mem_Addr) -> External_Mem_Addr` | Convert internal Virtual Machine address to an External Pointer that can be passed to C function. |
