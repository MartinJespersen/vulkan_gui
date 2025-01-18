# Things to do:


* remove std libraries
* how to handle global null objects when hot reloading is enabled and the object gets a new address
* Improve allocation of objects with scratch arenas and especially what to do when resetting a temp arena and the values above the current stack pointer is no longer zero initialized. 
* Adding new glyph atlases should be made dynamic
* Find out a way to handle errors
* error handling in hot reloading
* what to do about profiler context
