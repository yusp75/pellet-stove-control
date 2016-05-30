/******************************************************************************/
/* This file has been generated by the uGFX-Studio                            */
/*                                                                            */
/* http://ugfx.org                                                            */
/******************************************************************************/

#ifndef _GUI_H_
#define _GUI_H_

#include "gfx.h"

// GListeners
extern GListener gl;

// GHandles
extern GHandle ghContainerHome;
extern GHandle lbl_temp;
extern GHandle lbl_status;
extern GHandle lbl_day;
extern GHandle lbl_time;

// Function Prototypes
void guiCreate(void);
void guiShowPage(unsigned pageIndex);
void guiEventLoop(void);

#endif /* _GUI_H_ */

