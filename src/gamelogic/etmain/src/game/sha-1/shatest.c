/*
 *  shatest.c
 *
 *	Copyright (C) 1998
 *	Paul E. Jones <paulej@arid.us>
 *	All Rights Reserved
 *
 *****************************************************************************
 *	$Id: shatest.c,v 1.2 2004/03/27 18:00:33 paulej Exp $
 *****************************************************************************
 *
 *  Description:
 *      This file will exercise the SHA1 class and perform the three
 *      tests documented in FIPS PUB 180-1.
 *
 *  Portability Issues:
 *      None.
 *
 */

#include <stdio.h>
#include <string.h>
#include "sha1.h"

/*
 *  Define patterns for testing
 */
#define TESTA   "abc"
#define TESTB_1 "abcdbcdecdefdefgefghfghighij"
#define TESTB_2 "hijkijkljklmklmnlmnomnopnopq"
#define TESTB   TESTB_1 TESTB_2
#define TESTC   "a"

int main()
{
    SHA1Context sha;
    int i;

    /*
     *  Perform test A
     */
    printf("\nTest A: 'abc'\n");

    SHA1Reset(&sha);
    SHA1Input(&sha, (const unsigned char *) TESTA, strlen(TESTA));

    if (!SHA1Result(&sha))
    {
        fprintf(stderr, "ERROR-- could not compute message digest\n");
    }
    else
    {
        printf("\t");
        for(i = 0; i < 5 ; i++)
        {
            printf("%X ", sha.Message_Digest[i]);
        }
        printf("\n");
        printf("Should match:\n");
        printf("\tA9993E36 4706816A BA3E2571 7850C26C 9CD0D89D\n");
    }

    /*
     *  Perform test B
     */
    printf("\nTest B:\n");

    SHA1Reset(&sha);
    SHA1Input(&sha, (const unsigned char *) TESTB, strlen(TESTB));

    if (!SHA1Result(&sha))
    {
        fprintf(stderr, "ERROR-- could not compute message digest\n");
    }
    else
    {
        printf("\t");
        for(i = 0; i < 5 ; i++)
        {
            printf("%X ", sha.Message_Digest[i]);
        }
        printf("\n");
        printf("Should match:\n");
        printf("\t84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1\n");
    }

    /*
     *  Perform test C
     */
    printf("\nTest C: One million 'a' characters\n");

    SHA1Reset(&sha);
    for(i = 1; i <= 1000000; i++) {
        SHA1Input(&sha, (const unsigned char *) TESTC, 1);
    }

    if (!SHA1Result(&sha))
    {
        fprintf(stderr, "ERROR-- could not compute message digest\n");
    }
    else
    {
        printf("\t");
        for(i = 0; i < 5 ; i++)
        {
            printf("%X ", sha.Message_Digest[i]);
        }
        printf("\n");
        printf("Should match:\n");
        printf("\t34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F\n");
    }

    return 0;
}
