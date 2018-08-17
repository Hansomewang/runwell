/*
 * utils_mackey.h
 *
 * utilities function using MAC address to generate S/N and Activate Key
 */
#ifndef UTILS_MACKEY_INCLUDED
#define UTILS_MACKEY_INCLUDED

void ifaddr2sn( unsigned char *ifaddr, unsigned short *sn );
void sn2ifaddr( unsigned short *sn, unsigned char *ifaddr );
void id2key( const unsigned char* id, char* key );
int key2id( const char* key_in, unsigned char * id );
const char* dashed_key( const char* key );
void GetMACCode( unsigned char *ifaddr, unsigned char *MACCode, int nType );
void EncodeMACCode( unsigned char *code );

#endif