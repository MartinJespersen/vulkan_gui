# Things to do:

* create simple build scripts using linking of each modules (ui and base) so that they compile in the correct order
* Improve allocation of objects with scratch arenas and especially what to do when resetting a temp arena and the values above the current stack pointer is no longer zero initialized. 
* Adding new glyph atlases should be made dynamic
* Find out a way to handle errors
* remove std libraries
* error handling in hot reloading