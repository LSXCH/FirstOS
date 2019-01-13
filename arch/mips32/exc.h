#ifndef _EXC_H
#define _EXC_H

#include <driver/vga.h>
#include <zjunix/pc.h>
#include <zjunix/utils.h>
#include <zjunix/slab.h>
#include <zjunix/page.h>
#include <driver/ps2.h>

typedef void (*exc_fn)(unsigned int, unsigned int, context*);

extern exc_fn exceptions[32];

void do_exceptions(unsigned int status, unsigned int cause, context* pt_context, unsigned int bad_addr);
void register_exception_handler(int index, exc_fn fn);
void init_exception();
void tlb_refill(unsigned int bad_addr);
void refill_after(pgd_t* pgd);
#endif