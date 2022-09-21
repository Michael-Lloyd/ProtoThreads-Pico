/*
 *  Author:     Michael Lloyd (micl.dev)
 *  Version:    1.0.0
 *  Variant:    A
 *  Date:       21/09/22
 *
 *      __   ___.--'_`. 
 *     ( _`.'. -   'o` ) 
 *     _\.'_'      _.-'  
 *     ( `. )    //`    
 *     \_`-'`---'\\__,   
 *      \\`        \`--    
 *
 */

/*     ============================================================================================
 *     This file contains an extraction and refactoring of the BRL4 additions to the ProtoThreads
 *     header, from Cornell university. 
 *
 *     The original source files implemented everything primarily in the header file that is 
 *     distributed on the ECE4760 website, which does not allow for reliable abstraction or 
 *     modification. 
 *
 *     As it stands, this is not a modification, but there may be one here in the future. 
 *     - micl 
 *     ============================================================================================
 */

#include "ProtoThreads.h" 

//=====================================================================
//=== BRL4 additions for rp2040 =======================================
//=====================================================================

//====================================================================
// IMPROVED SCHEDULER 
// === thread structures ===
// thread control structs

// === extended structure for scheduler ===============
// an array of task structures

// see https://github.com/edartuz/c-ptx/tree/master/src
// and the license above
// add an entry to the thread list
//struct ptx *pt_add( char (*pf)(struct pt *pt), int rate) {
int pt_add( char (*pf)(struct pt *pt)) {
    if (pt_task_count < (MAX_THREADS)) {
        // get the current thread table entry 
        struct ptx *ptx = &pt_thread_list[pt_task_count];
        // enter the tak data into the thread table
        ptx->num   = pt_task_count;
        // function pointer
        ptx->pf    = pf;
        //
        PT_INIT( &ptx->pt );
        // count of number of defined threads
        pt_task_count++;
        // return current entry
        return pt_task_count-1;
    }
    return 0;
}

// core 1 -- add an entry to the thread list
//struct ptx *pt_add( char (*pf)(struct pt *pt), int rate) {
int pt_add1( char (*pf)(struct pt *pt)) {
    if (pt_task_count1 < (MAX_THREADS)) {
        // get the current thread table entry 
        struct ptx *ptx = &pt_thread_list1[pt_task_count1];
        // enter the tak data into the thread table
        ptx->num   = pt_task_count1;
        // function pointer
        ptx->pf    = pf;
        //
        PT_INIT( &ptx->pt );
        // count of number of defined threads
        pt_task_count1++;
        // return current entry
        return pt_task_count1-1;
    }
    return 0;
}

/* Scheduler
Copyright (c) 2014 edartuz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// === Scheduler Thread =================================================
// update a 1 second tick counter
// schedulser code was almost copied from
// https://github.com/edartuz/c-ptx
// see license above

// choose schedule method
static PT_THREAD (protothread_sched(struct pt *pt)) {   
    PT_BEGIN(pt);
    static int i, rate;

    if (pt_sched_method==SCHED_ROUND_ROBIN) {
        while(1) {
            // test stupid round-robin 
            // on all defined threads
            struct ptx *ptx = &pt_thread_list[0];
            // step thru all defined threads
            // -- loop can have more than one initialization or increment/decrement, 
            // -- separated using comma operator. But it can have only one condition.
            for (i=0; i<pt_task_count; i++, ptx++ ) {
                // call thread function
                (pt_thread_list[i].pf)(&ptx->pt); 
            }
            // Never yields! 
            // NEVER exit while!
        } // END WHILE(1)
    } //end if (pt_sched_method==RR)       

    PT_END(pt);
} // scheduler thread

// ================================================
// === second core scheduler
static PT_THREAD (protothread_sched1(struct pt *pt)) {   
    PT_BEGIN(pt);

    static int i, rate;

    if (pt_sched_method==SCHED_ROUND_ROBIN ) {
        while(1) {
            // test stupid round-robin 
            // on all defined threads
            struct ptx *ptx = &pt_thread_list1[0];
            // step thru all defined threads
            // -- loop can have more than one initialization or increment/decrement, 
            // -- separated using comma operator. But it can have only one condition.
            for (i=0; i<pt_task_count1; i++, ptx++ ) {
                // call thread function
                (pt_thread_list1[i].pf)(&ptx->pt); 
            }
            // Never yields! 
            // NEVER exit while!
        } // END WHILE(1)
    } // end if(pt_sched_method==SCHED_ROUND_ROBIN)      

    PT_END(pt);
} // scheduler1 thread

// === serial input thread ================================
// serial buffers
//
static PT_THREAD (pt_serialin_polled(struct pt *pt)) {

    PT_BEGIN(pt);
    static uint8_t ch;
    static int pt_current_char_count;

    // clear the string
    memset(pt_serial_in_buffer, 0, pt_buffer_size);
    pt_current_char_count = 0;

    // clear uart fifo
    while(uart_is_readable(UART_ID)) {
        uart_getc(UART_ID);
    }

    // build the output string
    while(pt_current_char_count < pt_buffer_size) {   

        PT_YIELD_UNTIL(pt, (int)uart_is_readable(UART_ID));

        //get the character and echo it back to terminal
        // NOTE this assumes a human is typing!!
        ch = uart_getc(UART_ID);
        PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID));
        uart_putc(UART_ID, ch);

        // check for <enter> or <backspace>
        if (ch == '\r' ) {
            // <enter>> character terminates string,
            // advances the cursor to the next line, then exits
            pt_serial_in_buffer[pt_current_char_count] = 0;
            PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID));
            uart_putc(UART_ID, '\n');
            break; 
        }
        // check fo ,backspace>
        else if (ch == pt_backspace) {
            PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID));
            uart_putc(UART_ID, ' ');
            PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID));
            uart_putc(UART_ID, pt_backspace);
            //uart_putc(UART_ID, ' ');
            // wipe a character from the output
            pt_current_char_count--;
            if (pt_current_char_count<0) {pt_current_char_count = 0;
            }

        }
        // must be a real character
        else {
            // build the output string
            pt_serial_in_buffer[pt_current_char_count++] = ch;
        }

    } // END WHILe
    
    // kill this input thread, to allow spawning thread to execute
    PT_EXIT(pt);
    PT_END(pt);

} // serial input thread

// ================================================================
// === serial output thread
int pt_serialout_polled(struct pt *pt) {

    static int num_send_chars;
    PT_BEGIN(pt);
    num_send_chars = 0;

    while (pt_serial_out_buffer[num_send_chars] != 0) {

        PT_YIELD_UNTIL(pt, (int)uart_is_writable(UART_ID));
        uart_putc(UART_ID, pt_serial_out_buffer[num_send_chars]);
        num_send_chars++;

    }

    // kill this output thread, to allow spawning thread to execute
    PT_EXIT(pt);
    // and indicate the end of the thread
    PT_END(pt);

}

