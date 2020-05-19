/*PGR-GNU*****************************************************************
File: combinations_input.c

Copyright (c) 2015 Celia Virginia Vergara Castillo
vicky_vergara@hotmail.com

------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 ********************************************************************PGR-GNU*/

#include "c_common/edges_input.h"

#include <math.h>
#include <float.h>
#include <limits.h>
/* for size-t */
#ifdef __cplusplus
#   include <cstddef>
#else
#   include <stddef.h>
#endif

#include "c_types/column_info_t.h"

#include "c_common/debug_macro.h"
#include "c_common/get_check_data.h"
#include "c_common/time_msg.h"

static
void fetch_basic_edge(
    HeapTuple *tuple,
    TupleDesc *tupdesc,
    Column_info_t info[5],
    int64_t *default_id,
    pgr_basic_edge_t *edge,
    size_t *valid_edges) {
    if (column_found(info[0].colNumber)) {
        edge->id = pgr_SPI_getBigInt(tuple, tupdesc, info[0]);
    } else {
        edge->id = *default_id;
        ++(*default_id);
    }

    edge->source = pgr_SPI_getBigInt(tuple, tupdesc, info[1]);
    edge->target = pgr_SPI_getBigInt(tuple, tupdesc, info[2]);
    edge->going = pgr_SPI_getFloat8(tuple, tupdesc, info[3]) > 0 ?
        true : false;
    edge->coming = (column_found(info[4].colNumber)
            && pgr_SPI_getFloat8(tuple, tupdesc, info[4]) > 0) ?
        true : false;

    (*valid_edges)++;
}

static
void fetch_edge(
        HeapTuple *tuple,
        TupleDesc *tupdesc,
        Column_info_t info[2],
        pgr_combination_t *combination,
        size_t *valid_combinations) {
    combination->source = pgr_SPI_getBigInt(tuple, tupdesc, info[0]);
    combination->target = pgr_SPI_getBigInt(tuple, tupdesc, info[1]);

    *valid_edges = *valid_edges + 1;
}



static
void
get_combinations_2_columns(
        char *sql,
        pgr_combinations_t **combinations,
        size_t *totalTuples) {
    clock_t start_t = clock();

    const int tuple_limit = 1000000;

    size_t total_tuples;
    size_t valid_edges;

    Column_info_t info[2];

    int i;
    for (i = 0; i < 2; ++i) {
        info[i].colNumber = -1;
        info[i].type = 0;
        info[i].strict = true;
        info[i].eType = ANY_INTEGER;
    }
    info[0].name = "source";
    info[1].name = "target";

    void *SPIplan;
    SPIplan = pgr_SPI_prepare(sql);

    Portal SPIportal;
    SPIportal = pgr_SPI_cursor_open(SPIplan);


    bool moredata = true;
    (*totalTuples) = total_tuples = valid_combinations = 0;


    int64_t default_id = 0;
    while (moredata == true) {
        SPI_cursor_fetch(SPIportal, true, tuple_limit);
        if (total_tuples == 0)
            pgr_fetch_column_info(info, 2);

        size_t ntuples = SPI_processed;
        total_tuples += ntuples;

        if (ntuples > 0) {
            if ((*combinations) == NULL)
                (*combinations) = (pgr_combination_t *)
                    palloc0(total_tuples * sizeof(pgr_combination_t));
            else
                (*combinations) = (pgr_combination_t *)
                    repalloc((*combinations), total_tuples * sizeof(pgr_combination_t));

            if ((*combinations) == NULL) {
                elog(ERROR, "Out of memory");
            }

            size_t t;
            SPITupleTable *tuptable = SPI_tuptable;
            TupleDesc tupdesc = SPI_tuptable->tupdesc;
            for (t = 0; t < ntuples; t++) {
                HeapTuple tuple = tuptable->vals[t];
                fetch_combination(&tuple, &tupdesc, info,
                                  &(*combinations)[total_tuples - ntuples + t],
                                  &valid_combinations);
            }
            SPI_freetuptable(tuptable);
        } else {
            moredata = false;
        }
    }

    SPI_cursor_close(SPIportal);

    if (total_tuples == 0 || valid_combinations == 0) {
        PGR_DBG("No combinations found");
    }

    (*totalTuples) = total_tuples;
    PGR_DBG("Reading %ld combinations", total_tuples);
    time_msg("reading combinations", start_t, clock());
}

/* select source, target */
void
pgr_get_combinations(
        char *combinations_sql,
        pgr_combination_t **combinations,
        size_t *total_combinations) {
    get_combinations_2_columns(combinations_sql, combinations, total_combinations);
}
