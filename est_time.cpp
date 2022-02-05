/**
* @file est_time.cpp
* @brief Module used for estimated remaining time printing
* @author Tomas Hladky <xhladk15@stud.fit.vutbr.cz>
* @author Martin Peresini <iperesini@fit.vut.cz>
* @date 2021 - 2022
*/
/*
* Copyright (C) BUT Security@FIT, 2021 - 2022
*
* LICENSE TERMS
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided with the
*    distribution.
* 3. Neither the name of the Company nor the names of its contributors
*    may be used to endorse or promote products derived from this
*    software without specific prior written permission.
*
* ALTERNATIVELY, provided that this notice is retained in full, this
* product may be distributed under the terms of the GNU General Public
* License (GPL) version 2 or later, in which case the provisions
* of the GPL apply INSTEAD OF those given above.
*
* This software is provided ``as is'', and any express or implied
* warranties, including, but not limited to, the implied warranties of
* merchantability and fitness for a particular purpose are disclaimed.
* In no event shall the company or contributors be liable for any
* direct, indirect, incidental, special, exemplary, or consequential
* damages (including, but not limited to, procurement of substitute
* goods or services; loss of use, data, or profits; or business
* interruption) however caused and on any theory of liability, whether
* in contract, strict liability, or tort (including negligence or
* otherwise) arising in any way out of the use of this software, even
* if advised of the possibility of such damage.
*
*/

#include <cstdio>
#include <ctime>

#include "est_time.h"
#include "log.h"

void print_diff_time(time_t time_diff)
{
    if (time_diff < 60) {
        // seconds
        log_progress("ETA: %lds", time_diff);
    } else if (time_diff >= 60 && time_diff < 3600) {
        // minutes
        log_progress("ETA: %ldm:%02lds", time_diff / 60, time_diff % 60);
    } else if (time_diff >= 3600 && time_diff < 86400) {
        // hours
        log_progress("ETA: %ldh:%02ldm:%02lds", time_diff / 3600, time_diff % 3600 / 60,
               time_diff % 3600 % 60);
    } else {
        // days
        log_progress("ETA: %ld days, %ldh:%02ldm:%02lds", time_diff / 86400,
               time_diff % 86400 / 3600, time_diff % 86400 % 3600 / 60,
               time_diff % 86400 % 3600 % 60);
    }
}
