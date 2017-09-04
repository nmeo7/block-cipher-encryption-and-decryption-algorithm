#include <iostream>
#include <string>
#include <cmath>
#include <fstream>
#include <Windows.h>
#include <locale.h>

// gamma - key stream

using namespace std;

unsigned __int64 ecb_compl(unsigned __int64 block, unsigned int n)
{
	unsigned __int64 mask = 1;
	mask = (mask << (n * 8)) - 1; // this is the mask of what we read.
	unsigned __int64 mask1 = 48 + n;

	for (int i = 1; i < 8; i++)
		mask1 = (mask1 << 8) | rand();

	mask1 = mask | mask1;
	mask = ~mask;
	mask = mask | block;

	return mask1 & mask;
}

unsigned int ecb_compl1(unsigned __int64 block)
{
	unsigned n = (block >> 56) - 48;
	return n;
}

unsigned __int64 round1(unsigned __int64 _block, unsigned __int32 subkey, short s_block[][16])
{
	unsigned __int64 block = _block;
	unsigned __int32 right;
	unsigned __int32 left = block;


	right = (block + subkey);

	// substitution through s-blocks. 
	for (int i = 0; i < 8; i++)
	{
		unsigned __int8 fragment = (right >> (4 * i)) % 16;
		fragment = s_block[7 - i][fragment];

		unsigned __int32 mask = right;
		mask = ((mask >> (4 * i)) >> 4);
		mask <<= 4;
		mask = (mask | fragment) << (4 * i);
		right = (right << (32 - (4 * i)));
		right = (right >> (32 - (4 * i))) | mask;
	}

	// must rotate to the left by 11 bits.
	unsigned __int32 mask = right << 11;
	
	right = (right >> 21) | mask;

	// add the result to the left part from the last round, and the left round becomes the new right round
	right = right ^ (block >> 32);

	// block = left;
	// block <<= 32;
	block = (_block << 32) | right;

	return block;
}

// s-block - 8. each has 16 values of 4 bits each. you can put them in a 8x16 matrix.
// prostoi zameny. cipher each gangsta individually.
unsigned __int64 ecb(unsigned __int64 _block, unsigned __int32 *key, short s_block[][16]) // prostoi zameny
{
	unsigned __int64 block = _block;

	for (int i = 0; i < 24; i++)
		block = round1(block, key[i % 8], s_block); // the input is the previous output.

	for (int i = 7; i >= 0; i--)
		block = round1(block, key[i], s_block);

	block = (block << 32) | (block >> 32); // undo the last switch of the left and right sides

	return block;
}

unsigned __int64 ecb1(unsigned __int64 _block, unsigned __int32 *key, short s_block[][16]) // prostoi zameny
{
	unsigned __int64 block = _block;

	for (int i = 0; i < 8; i++)
		block = round1(block, key[i], s_block);

	for (int i = 23; i >= 0; i--)
		block = round1(block, key[i % 8], s_block); // the input is the previous output.

	block = (block << 32) | (block >> 32); // undo the last switch of the left and right sides

	return block;
}

void cfb(unsigned __int64 *gamma, unsigned __int64 *block, unsigned int *key, short s_block[][16], int regime) // гаммирование с обратной связью
{
	*gamma = ecb(*gamma, key, s_block);
	*block ^= *gamma;
}

void xor_cipher(unsigned __int64 *gamma, unsigned __int64 *block, unsigned int *key, short s_block[][16], int regime) // гаммирование
{
	// xor the gamma with the block
	// encrypt the gamma
	unsigned __int32 n1 = *gamma ^ 0x1010101;
	unsigned __int32 n2 = 0;
	n2 = ((*gamma >> 32) ^ 0x1010104) % (~n2);
	*gamma = (n2 << 32) | n1;

	// if (regime == 0)
		*block = *block ^ ecb(*gamma, key, s_block);
	// else
		// *block = *block ^ ecb1(*gamma, key, s_block);
}

BOOL readblock(HANDLE f, unsigned __int64 *block, unsigned int *n)
{
	DWORD i = 0;

	ReadFile(f, block, 8, &i, NULL); // try to read more bytes
	*n = i;

	if (*n < 8)
		return FALSE; // this was the last byte if no more bytes could be read further

	// SetFilePointer( f, -1, NULL, FILE_CURRENT);
	return TRUE;
}

BOOL readblock1(HANDLE f, unsigned __int64 *block, unsigned int *n)
{
	DWORD i = 0;

	ReadFile(f, block, 9, &i, NULL); // try to read more bytes
	*n = i;

	if (*n < 9)
		return FALSE; // this was the last byte if no more bytes could be read further
	SetFilePointer(f, -1, NULL, FILE_CURRENT);
	*n = 8;
	return TRUE;
}

VOID writeblock1(HANDLE f, unsigned __int64 block, int n)
{
	if (n != 0)
		WriteFile(f, &block, n, NULL, NULL);
}

VOID writeblock(HANDLE f, unsigned __int64 block, int n)
{
	if (n != 0)
		WriteFile(f, &block, 8, NULL, NULL);
}

int main()
{
	setlocale(LC_ALL, "");
	char chars[2][64];
	char ch1, ch2;
	// int a;
	unsigned __int32 subkeys[8] = {1,3,5,6,2,7,9,12};
	unsigned __int64 iv = 1234567;

	cout << "input file: ";
	char file[64];
	cin >> file;
	HANDLE f_in = CreateFile(file, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	cout << "output file: ";
	cin >> file;
	HANDLE f_out = CreateFile(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	cout << "initial vector (eg. 1234567): ";
	cin >> iv;
	cout << "regime (0 - простая замена, 1 - гаммирование, 2 - с обратной связью): ";
	int regime1 = 2, regime2 = 1; // regime1 - the type, regime2 - decrypt or encrypt
	cin >> regime1;
	cout << "encryption (0)/ dercr (1): ";
	cin >> regime2;



	bool reader = true;
	unsigned int n;
	
	// the Central Bank of Russian Federation uses the following S-boxes
	short s_block[8][16] = { { 4, 10, 9, 2, 13, 8, 0, 14, 6, 11, 1, 12, 7, 15, 5, 3 }, { 14, 11, 4, 12, 6, 13, 15, 10, 2, 3, 8, 1, 0, 7, 5, 9 },
	{ 5, 8, 1, 13, 10, 3, 4, 2, 14, 15, 12, 7, 6, 0, 9, 11 }, { 7, 13, 10, 1, 0, 8, 9, 15, 14, 4, 6, 12, 11, 2, 5, 3 },
	{ 6, 12, 7, 1, 5, 15, 13, 8, 4, 10, 9, 14, 0, 3, 11, 2 }, { 4, 11, 10, 0, 7, 2, 1, 13, 3, 6, 8, 5, 9, 12, 15, 14 },
	{ 13, 11, 4, 1, 3, 15, 5, 9, 0, 10, 14, 7, 6, 8, 2, 12 }, { 1, 15, 13, 0, 5, 7, 10, 4, 9, 2, 3, 14, 6, 11, 8, 12 } };

	// bit_output(iv);
	unsigned __int64 a;


	if (regime2 == 0) // encrypting
	{
		switch (regime1)
		{
		case 0: // ecb
			while (reader) {
				unsigned __int64 buf;
				reader = readblock(f_in, &buf, &n);
				if (n < 8)
					buf = ecb_compl(buf, n);
				buf = ecb(buf, subkeys, s_block);
				writeblock(f_out, buf, n);
			}
			cout << "encryption successful." << endl;
			break;

		case 1:
			iv = ecb(iv, subkeys, s_block);
			while (reader) {
				unsigned __int64 buf;
				reader = readblock(f_in, &buf, &n);
				xor_cipher(&iv, &buf, subkeys, s_block, 0);
				writeblock1(f_out, buf, n);
			}
			SetEndOfFile(f_out);
			cout << "encryption successful." << endl;
			break;

		case 2:
			while (reader) {
				unsigned __int64 buf;
				reader = readblock(f_in, &buf, &n);
				cfb(&iv, &buf, subkeys, s_block, 0);
				writeblock1(f_out, buf, n);
			}
			cout << "encryption successful." << endl;
			break;
		default:
			break;
		}
	}
	else
	{
		switch (regime1)
		{
		case 0: // ecb
			while (reader) {
				unsigned __int64 buf;
				reader = readblock1(f_in, &buf, &n);
				buf = ecb1(buf, subkeys, s_block);
				if (!reader)
					n = ecb_compl1(buf);
				else
					n = 8;
				writeblock1(f_out, buf, n);
			}
			SetEndOfFile(f_out);
			cout << "decryption successful." << endl;
			break;

		case 1:
			iv = ecb(iv, subkeys, s_block);
			while (reader) {
				unsigned __int64 buf;
				reader = readblock(f_in, &buf, &n);
				xor_cipher(&iv, &buf, subkeys, s_block, 1);
				writeblock1(f_out, buf, n);
			}
			SetEndOfFile(f_out);
			cout << "decryption successful." << endl;
			break;

		case 2:
			while (reader) {
				unsigned __int64 buf;
				reader = readblock(f_in, &buf, &n);
				cfb(&iv, &buf, subkeys, s_block, 1);
				writeblock1(f_out, buf, n);
			}
			SetEndOfFile(f_out);
			cout << "decryption successful." << endl;
			break;
		default:
			break;
		}
	}

	CloseHandle(f_in);
	CloseHandle(f_out);
	system("pause");
	return 0;
}

