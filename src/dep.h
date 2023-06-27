#pragma once

/* this component use to manage all modules which loaded by config parsed
 * from json file, and it also provide some interfaces to invoke module
 * we use key-value pair to store module name and handle by redis dict
 */

extern void *dep_open_library(const char *name);
extern void dep_close_library(const char *name);
