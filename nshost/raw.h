#if !defined NSP_RAW_H
#define NSP_RAW_H

#include "ncb.h"

extern nsp_status_t raw_txn(ncb_t *ncb, void *p);
extern nsp_status_t raw_tx(ncb_t *ncb);

#endif
