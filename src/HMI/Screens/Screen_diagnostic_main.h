#ifndef SCREEN_DIAGNOSTIC_MAIN_H
#define SCREEN_DIAGNOSTIC_MAIN_H

// Diagnostic screen functions
// This screen shows:
// - Real-time motor information when motors are active
// - System error list when no motors are active but errors exist
// - "No active motors" when no motors are active and no errors exist
// The screen automatically appears when motors start or system errors occur
void Init_diagnostic_main_screen(void *p);
bool Are_any_motors_active(void);

#endif
