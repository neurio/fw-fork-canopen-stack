/******************************************************************************
* (c) by Embedded Office GmbH & Co. KG, http://www.embedded-office.com
*------------------------------------------------------------------------------
* This file is part of CANopenStack, an open source CANopen Stack.
* Project home page is <https://github.com/MichaelHillmann/CANopenStack.git>.
* For more information on CANopen see < http ://www.can-cia.org/>.
*
* CANopenStack is free and open source software: you can redistribute
* it and / or modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation, either version 2 of the
* License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
******************************************************************************/

/******************************************************************************
* INCLUDES
******************************************************************************/

#include "co_para.h"

#include "co_core.h"

/******************************************************************************
* PRIVATE DEFINES
******************************************************************************/

#define CO_PARA_STORE_SIG    0x65766173     /*!< store parameter signature   */
#define CO_PARA_RESTORE_SIG  0x64616F6c     /*!< restore parameter signature */

/******************************************************************************
* EXTERNAL FUNCTIONS
******************************************************************************/

extern int16_t COParaLoad(CO_PARA *pg);
extern int16_t COParaSave(CO_PARA *pg);
extern int16_t COParaDefault(CO_PARA *pg);

/******************************************************************************
* GLOBAL CONSTANTS
******************************************************************************/

/*! \brief OBJECT TYPE PARAMETER
*
*    This object type specializes the general handling of objects for the
*    object directory entries 0x1010 and 0x1011. These entries are designed
*    to provide the store and restore feature of a configurable parameter
*    group.
*/
const CO_OBJ_TYPE COTPara = { 0, 0, 0, 0, COTypeParaRead, COTypeParaWrite };

/******************************************************************************
* FUNCTIONS
******************************************************************************/

/*
* see function definition
*/
void COParaStore(CO_PARA *pg, CO_NODE *node)
{
    int16_t err;

    /* argument chekcs */
    if ((pg == 0) || (node == 0)) {
        return;
    }
    /* call save callback function */
    if ((pg->Value & CO_PARA___E) != 0) {
        err = COParaSave(pg);
        if (err != CO_ERR_NONE) {
            node->Error = CO_ERR_PARA_STORE;
        }
    }
}

/*
* see function definition
*/
void COParaRestore(CO_PARA *pg, CO_NODE *node)
{
    int16_t err;

    /* argument checks */
    if ((pg == 0) || (node == 0)) {
        return;
    }

    /* call default callback function */
    if ((pg->Value & CO_PARA___E) != 0) {
        err = COParaDefault(pg);
        if (err != CO_ERR_NONE) {
            node->Error = CO_ERR_PARA_RESTORE;
        }
    }
}

/*
* see function definition
*/
int16_t COParaCheck(CO_OBJ* obj, void *buf, uint32_t size)
{
    uint32_t signature;
    int16_t  result = CO_ERR_PARA_IDX;
    CO_DIR  *cod;
    int16_t  err;
    uint8_t  num;
    uint8_t  sub;

    /* argument checks */
    if ((obj == 0) || (buf == 0) || (size != 4)) {
        return (CO_ERR_BAD_ARG);
    }
    
    cod = obj->Type->Dir;

    /* store parameter */
    if (CO_GET_IDX(obj->Key) == 0x1010) {
        err = CODirRdByte(cod, CO_DEV(0x1010,0), &num);
        if (err != CO_ERR_NONE) {
            cod->Node->Error = CO_ERR_CFG_1010_0;
            return (err);
        }

        if (num > 0x7F) {
            return (CO_ERR_CFG_1010_0);
        }
        sub = CO_GET_SUB(obj->Key);
        if ((sub < 1) || (sub > num)) {
            return (CO_ERR_BAD_ARG);
        }
        signature = *((uint32_t *)buf);
        if (signature != CO_PARA_STORE_SIG) {
            return (CO_ERR_OBJ_ACC);
        }
        result = CO_ERR_NONE;
    }
    
    /* restore parameters */
    if (CO_GET_IDX(obj->Key) == 0x1011) {
        err = CODirRdByte(cod, CO_DEV(0x1011,0), &num);
        if (err != CO_ERR_NONE) {
            cod->Node->Error = CO_ERR_CFG_1011_0;
            return (err);
        }
        if (num > 0x7F) {
            return (CO_ERR_CFG_1011_0);
        }
        sub = CO_GET_SUB(obj->Key);
        if ((sub < 1) || (sub > num)) {
            return (CO_ERR_BAD_ARG);
        }
        signature = *((uint32_t *)buf);
        if (signature != CO_PARA_RESTORE_SIG) {
            return (CO_ERR_OBJ_ACC);
        }
        result = CO_ERR_NONE;
    }

    /* bad parameter index */
    if (result != CO_ERR_NONE) {
        cod->Node->Error = CO_ERR_PARA_IDX;
    }

    return (result);
}

/*
* see function definition
*/
int16_t COTypeParaRead(CO_OBJ* obj, void *buf, uint32_t size)
{
    CO_PARA *pg;

    /* argument checks */
    if ((obj == 0) || (buf == 0)) {
        return (CO_ERR_BAD_ARG);
    }
    if (size != CO_LONG) {
        return (CO_ERR_BAD_ARG);
    }
    pg = (CO_PARA *)obj->Data;
    if (pg == 0) {
        return (CO_ERR_OBJ_READ);
    }

    /* read parameter */
    *(uint32_t *)buf = pg->Value;

    return (CO_ERR_NONE);
}

/*
* see function definition
*/
int16_t COTypeParaWrite(CO_OBJ* obj, void *buf, uint32_t size)
{
    CO_DIR  *cod;
    CO_OBJ  *pwo;
    CO_PARA *pg;
    int16_t  select;
    uint16_t idx;
    uint8_t  num;
    uint8_t  sub;
    
    /* check parameter and configuration */
    select = COParaCheck(obj, buf, size);
    if (select != CO_ERR_NONE) {
        return (select);
    }
    cod = obj->Type->Dir;
    idx = CO_GET_IDX(obj->Key);
    sub = CO_GET_SUB(obj->Key);
    (void)CODirRdByte(cod, CO_DEV(idx,0), &num);

    /* all parameter groups */
    if ((sub == 1) && (num > 1)) {
        for (sub = 2; sub < num; sub++) {
            pwo = CODirFind(cod, CO_DEV(idx, sub));
            if (pwo != 0) {
                pg = (CO_PARA *)pwo->Data;
                if (idx == 0x1011) {
                    COParaRestore(pg, cod->Node);
                } else {
                    COParaStore(pg, cod->Node);
                }
            }
        }
    } else {
        /* save addressed para group */
        pg = (CO_PARA *)obj->Data;
        if (idx == 0x1011) {
            COParaRestore(pg, cod->Node);
        } else {
            COParaStore(pg, cod->Node);
        }
    }
    return (CO_ERR_NONE);
}
