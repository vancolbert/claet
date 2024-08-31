/*!
 * \file
 * \ingroup misc_utils
 * \brief generating descriptive error messages and write them to specific targets.
 */
#ifndef __ERRORS_H__
#define __ERRORS_H__
#include <SDL_types.h>
#include "elloggingwrapper.h"
#ifdef __cplusplus
extern "C" {
#endif
/*!
 * \ingroup misc_utils
 * \brief       empties the connection_log.txt file
 *
 *      Clears the connection_log.txt file
 *
 */
void clear_conn_log();
/*!
 * \ingroup misc_utils
 * \brief       logs connection data to the connection_log.txt file.
 *
 *      Logs connection data to the connection_log.txt file.
 *
 * \param in_data           the data to write to the log
 * \param data_length       the length of \a in_data
 */
void log_conn(const Uint8 *in_data, Uint16 data_length);
#ifdef __cplusplus
} // extern "C"
#endif
#endif
