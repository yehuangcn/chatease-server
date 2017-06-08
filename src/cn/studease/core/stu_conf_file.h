/*
 * stu_conf_file.h
 *
 *  Created on: 2017-6-7
 *      Author: Tony Lau
 */

#ifndef STU_CONF_FILE_H_
#define STU_CONF_FILE_H_

#include "stu_config.h"
#include "stu_core.h"

#define STU_CONF_FILE_MAX_SIZE  2048

typedef struct {
	stu_file_t  file;
	stu_buf_t  *buffer;
} stu_conf_file_t;

stu_int_t stu_conf_file_parse(u_char *name);

#endif /* STU_CONF_FILE_H_ */