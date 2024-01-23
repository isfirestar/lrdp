#if !defined NSP_RAW_H
#define NSP_RAW_H

#include "ncb.h"

extern nsp_status_t raw_txn(ncb_t *ncb, void *p);
extern nsp_status_t raw_tx(ncb_t *ncb);
extern void raw_setattr_r(ncb_t *ncb, int attr);

#endif
