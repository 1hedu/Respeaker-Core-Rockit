/*
 * arpeggiator.h
 *
 * Created: 1/02/2012 11:36:01 AM
 *  Author: Administrator
 */ 


#ifndef ARPEGGIATOR_H_
#define ARPEGGIATOR_H_

#define ARPEGGIATOR_COUNTER_MAX		512

void
arpeggiator_reset_current_active_note(void);

void
arpeggiator_reset_timer(void);

void
arpeggiator(g_setting *p_global_setting);

void
initialize_arpeggiator(void);


#endif /* ARPEGGIATOR_H_ */