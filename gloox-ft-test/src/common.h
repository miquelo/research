/*
 * common.h 12/01/29
 *
 * Miquel Ferran <mikilaguna@gmail.com>
 */
 
#ifndef COMMON_H
#define COMMON_H

int sender_main(const char* s_uid, const char* s_pass, const char* r_uid,
		const char* filename, const char* p_port);
int receiver_main(const char* r_uid, const char* r_pass);

#endif // COMMON_H
