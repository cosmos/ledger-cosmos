/*******************************************************************************
*   (c) 2016 Ledger
*   (c) 2018 ZondaX GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#pragma once
#include <bagl.h>

#define UI_CENTER11PX       BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER
#define UI_CENTER11PX_BOLD  BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER
#define DEFAULT_FONT        BAGL_FONT_OPEN_SANS_LIGHT_16px | BAGL_FONT_ALIGNMENT_LEFT

#define UI_FillRectangle(id, x, y, w, h, fgcolor, bgcolor)  \
{                                                           \
    {                                                       \
        BAGL_RECTANGLE, /* type     */                      \
        id,           /* usedid   */                        \
        x,              /* x        */                      \
        y,              /* y        */                      \
        w,              /* width    */                      \
        h,              /* height   */                      \
        0,              /* stroke   */                      \
        0,              /* radius   */                      \
        BAGL_FILL,      /* fill     */                      \
        fgcolor,        /* fgcolor  */                      \
        bgcolor,        /* bgcolor  */                      \
        0,              /* font_id  */                      \
        0               /* icon_id  */                      \
    },                                                      \
        NULL,   /* text             */                      \
        0,      /* touch_area_brim  */                      \
        0,      /* overfgcolor      */                      \
        0,      /* overbgcolor      */                      \
        NULL,   /* tap              */                      \
        NULL,   /* out              */                      \
        NULL,   /* over             */                      \
}

#define UI_LabelLine(id, x, y, w, h, fgcolor, bgcolor, text)    \
{                                                               \
    {                                                           \
        BAGL_LABELINE, /* type     */                           \
        id,           /* usedid   */                            \
        x,              /* x        */                          \
        y,              /* y        */                          \
        w,              /* width    */                          \
        h,              /* height   */                          \
        0,              /* stroke   */                          \
        0,              /* radius   */                          \
        0,              /* fill     */                          \
        fgcolor,        /* fgcolor  */                          \
        bgcolor,        /* bgcolor  */                          \
        UI_CENTER11PX,  /* font_id  */                          \
        0               /* icon_id  */                          \
    },                                                          \
        text,   /* text             */                          \
        0,      /* touch_area_brim  */                          \
        0,      /* overfgcolor      */                          \
        0,      /* overbgcolor      */                          \
        NULL,   /* tap              */                          \
        NULL,   /* out              */                          \
        NULL,   /* over             */                          \
}

#define UI_LabelLineScrolling(id, x, y, w, h, fgcolor, bgcolor, text)    \
{                                                               \
    {                                                           \
        BAGL_LABELINE, /* type     */                           \
        id,           /* usedid   */                            \
        x,              /* x        */                          \
        y,              /* y        */                          \
        w,              /* width    */                          \
        h,              /* height   */                          \
        5 | BAGL_STROKE_FLAG_ONESHOT, /* stroke | scr pause */ \
        0,              /* radius   */                          \
        0,              /* fill     */                          \
        fgcolor,        /* fgcolor  */                          \
        bgcolor,        /* bgcolor  */                          \
        UI_CENTER11PX,  /* font_id  */                          \
        50 /* icon_id / scroll speed  */ \
    },                                                          \
        text,   /* text             */                          \
        0,      /* touch_area_brim  */                          \
        0,      /* overfgcolor      */                          \
        0,      /* overbgcolor      */                          \
        NULL,   /* tap              */                          \
        NULL,   /* out              */                          \
        NULL,   /* over             */                          \
}

#define UI_Icon(id, x, y, w, h, icon)                       \
{                                                           \
    {                                                       \
        BAGL_ICON,      /* type     */                      \
        id,           /* usedid   */                        \
        x,              /* x        */                      \
        y,              /* y        */                      \
        w,              /* width    */                      \
        h,              /* height   */                      \
        0,              /* stroke   */                      \
        0,              /* radius   */                      \
        0,              /* fill     */                      \
        0xFFFFFF,       /* fgcolor  */                      \
        0x000000,       /* bgcolor  */                      \
        0,              /* font_id  */                      \
        icon            /* icon_id  */                      \
    },                                                      \
        NULL,   /* text             */                      \
        0,      /* touch_area_brim  */                      \
        0,      /* overfgcolor      */                      \
        0,      /* overbgcolor      */                      \
        NULL,   /* tap              */                      \
        NULL,   /* out              */                      \
        NULL,   /* over             */                      \
}
