#include <iostream>

#define MAX_SIZE 1024 // Maximum number of characters in the text

using namespace std;

typedef unsigned long long bits64; // 64-bit number (8 bytes)
typedef unsigned int bits32; // 32-bit number (4 bytes)
typedef unsigned char byte; // 8-bit number (1 byte)

// Replacement nodes defined by RFC 4357 Identifier: id-Gost28147-89-CryptoPro-A-ParamSet
byte S[8][16] = { {0x9, 0x6, 0x3, 0x2, 0x8, 0xB, 0x1, 0x7, 0xA, 0x4, 0xE, 0xF, 0xC, 0x0, 0xD, 0x5},
				{0xE, 0x4, 0x6, 0x2, 0xB, 0x3, 0xD, 0x8, 0xC, 0xF, 0x5, 0xA, 0x0, 0x7, 0x1, 0x9},
				{0x3, 0x7, 0xE, 0x9, 0x8, 0xA, 0xF, 0x0, 0x5, 0x2, 0x6, 0xC, 0xB, 0x4, 0xD, 0x1},
				{0xE, 0x7, 0xA, 0xC, 0xD, 0x1, 0x3, 0x9, 0x0, 0x2, 0xB, 0x4, 0xF, 0x8, 0x5, 0x6},
				{0xB, 0x5, 0x1, 0x9, 0x8, 0xD, 0xF, 0x0, 0xE, 0x4, 0x2, 0x3, 0xC, 0x7, 0xA, 0x6},
				{0x3, 0xA, 0xD, 0xC, 0x1, 0x2, 0x0, 0xB, 0x7, 0x5, 0x9, 0x4, 0x8, 0xF, 0xE, 0x6},
				{0x1, 0xD, 0x2, 0x9, 0x7, 0xA, 0x6, 0x0, 0x8, 0xC, 0x4, 0x5, 0xF, 0x3, 0xB, 0xE},
				{0xB, 0xA, 0xF, 0x5, 0x0, 0xC, 0xE, 0x8, 0x6, 0x2, 0x3, 0x9, 0x1, 0x7, 0xD, 0x4} };

// Rotate left by 11 bits
bits32 LeftShift11(bits32 value)
{
	bits32 right = value >> 1;
	right &= 0x7FFFFFFF;
	right >>= 20;

	return (value << 11) | right;
}

// Data block class
template <typename T>
class DataBlocks
{
	int length;
	T* data;

	int max(int a, int b)
	{
		if (a > b)
			return a;
		return b;
	}

public:

	// Constructor by length
	DataBlocks(int length)
	{
		this->length = length;
		data = new T[length];
		for (int i = 0; i < length; i++)
			data[i] = 0;
	}

	~DataBlocks()
	{
		delete data;
	}

	int Length()
	{
		return this->length;
	}

	// Slicing data into blocks
	DataBlocks(DataBlocks<byte>* str, int minLength = 0)
	{
		int bytes = sizeof(T);
		int sLength = str->Length();

		length = max(sLength / bytes + (sLength % bytes > 0 ? 1 : 0), minLength);
		data = new T[length];

		for (int i = 0; i < length; i++)
			data[i] = 0;

		for (int i = 0, j; i < sLength; i++)
		{
			j = i / bytes;

			data[j] = (data[j] << 8) | (*str)[i]; // Big-Endian
			//data[j] += ((T)(*str)[i]) << (8 * (i % bytes)); // Little-Endian
		}
	}

	T& operator[](const int index)
	{
		return data[index];
	}
};

// Get an array of bytes from a string of text
DataBlocks<byte>* GetBytes(char* str)
{
	int length = strlen(str);
	DataBlocks<byte>* blocks = new DataBlocks<byte>(length);
	for (int i = 0; i < length; i++)
		(*blocks)[i] = str[i];

	return blocks;
}

// Get an array of bytes from a number
template <typename T>
DataBlocks<byte>* GetBytes(T number)
{
	int bytes = sizeof(number);
	DataBlocks<byte>* blocks = new DataBlocks<byte>(bytes);
	for (int i = bytes - 1; i >= 0; i--)
	{
		(*blocks)[i] = number & 0x0FF;
		number >>= 8;
	}
	return blocks;
}

// Divide an array of 64-bit numbers into 1 array of 32-bit ones
DataBlocks<bits32>* Split64To32(DataBlocks<bits64>* blocks)
{
	int length = blocks->Length();
	DataBlocks<bits32>* result = new DataBlocks<bits32>(length * 2);
	for (int i = 0; i < length; i++)
	{
		bits32 left = (bits32)(((*blocks)[i] & 0xFFFFFFFF00000000) >> 32);
		bits32 right = (bits32)((*blocks)[i] & 0x00000000FFFFFFFF);

		(*result)[2 * i] = left;
		(*result)[2 * i + 1] = right;
	}

	return result;
}

// Layout of 32-bit blocks into left and right
void Linkage32(DataBlocks<bits32>* blocks, DataBlocks<bits32>* left, DataBlocks<bits32>* right)
{
	int length = blocks->Length() / 2;
	for (int i = 0; i < length; i++)
	{
		(*left)[i] = (*blocks)[2 * i];
		(*right)[i] = (*blocks)[2 * i + 1];
	}

	return;
}

// Concatenation of 2 arrays of 32-bit numbers
DataBlocks<bits64>* Join32To64(DataBlocks<bits32>* left, DataBlocks<bits32>* right)
{
	int length = left->Length();

	DataBlocks<bits64>* result = new DataBlocks<bits64>(length);

	for (int i = 0; i < length; i++)
	{
		(*result)[i] = (((bits64)(*left)[i]) << 32) + ((bits64)(*right)[i]);
	}

	return result;
}

// Concatenate an array of 32-bit numbers into an array of 64-bit numbers
DataBlocks<bits64>* Join32To64(DataBlocks<bits32>* blocks32)
{
	int length = blocks32->Length() / 2;
	DataBlocks<bits64>* blocks = new DataBlocks<bits64>(length);

	for (int i = 0; i < length; i++)
	{
		(*blocks)[i] += (bits64)(*blocks32)[2 * i] << 32;
		(*blocks)[i] += (*blocks32)[2 * i + 1];
	}

	return blocks;
}

// Subkeys of encryption/decryption
DataBlocks<bits32>* Subkeys(DataBlocks<byte>* key, bool isEncrypt)
{
	DataBlocks<bits32>* K = new DataBlocks<bits32>(key, 8);
	DataBlocks<bits32>* X = new DataBlocks<bits32>(32);
	if (isEncrypt) // Subkeys of encryption
	{
		for (int i = 0; i < 24; i++)
			(*X)[i] = (*K)[i % 8];

		for (int i = 0; i < 8; i++)
			(*X)[i + 24] = (*K)[7 - i];
	}

	else // Subkeys of decription
	{
		for (int i = 0; i < 8; i++)
			(*X)[i] = (*K)[i % 8];

		for (int i = 0; i < 24; i++)
			(*X)[i + 8] = (*K)[7 - i % 8];
	}

	return X;
}

// Function f
bits32 F(bits32 data, bits32 subKey)
{
	bits32 result = ((bits64)data + (bits64)subKey) % 4294967296; // Сложение по модулю 2^32
	byte arr[8] = {};
	for (int i = 0; i < 8; i++) // Замены через S-блок
	{
		arr[i] = S[i][result % 16];
		result >>= 4;
	}

	result = 0;

	for (int i = 0; i < 8; i++)
	{
		result <<= 4;
		result |= arr[i];
	}

	return LeftShift11(result); // Циклический сдвиг влево на 11 бит
}

// Blocks of data from hex values
DataBlocks<bits32>* FromHex(DataBlocks<byte>* hexStr)
{
	int strLength = hexStr->Length();
	int length = strLength / 8;
	char buffer[9] = {};
	DataBlocks<bits32>* blocks = new DataBlocks<bits32>(length);

	for (int i = 0; i < length; i++)
	{
		for (int j = 0; j < 8; j++)
			buffer[j] = (*hexStr)[i * 8 + j];

		buffer[8] = '\0';
		(*blocks)[i] = strtoul(buffer, NULL, 16);
	}

	return blocks;
}

// Simple replacement mode
DataBlocks<bits64>* ECB(DataBlocks<byte>* text, DataBlocks<byte>* key, bool isEncrypt)
{
	int length;
	DataBlocks<bits32>* blocks32;
	if (isEncrypt) // For encryption
	{
		DataBlocks<bits64>* blocks64 = new DataBlocks<bits64>(text);
		blocks32 = Split64To32(blocks64);
		length = blocks64->Length();
	}
	else // For decryption
	{
		blocks32 = FromHex(text);
		length = blocks32->Length() / 2;
	}

	DataBlocks<bits32>* A = new DataBlocks<bits32>(length);
	DataBlocks<bits32>* B = new DataBlocks<bits32>(length);

	// Layout of 32-bit blocks into left and right
	Linkage32(blocks32, A, B);

	DataBlocks<bits32>* X = Subkeys(key, isEncrypt); // Generate subkeys
	bits32 A_, B_; // Values of the previous round

	for (int i = 0; i < length; i++)
	{
		for (int j = 0; j < 32; j++) // 32 GOST rounds
		{
			A_ = (*B)[i] ^ F((*A)[i], (*X)[j]);
			B_ = (*A)[i];

			(*A)[i] = A_;
			(*B)[i] = B_;
		}
	}

	return Join32To64(B, A); // Combine 32-bit numbers into result
}

// Gamma Mode
DataBlocks<bits64>* CTR(DataBlocks<byte>* text, DataBlocks<byte>* key, DataBlocks<byte>* IV, bool isEncrypt)
{
	// Encryption of the sync message
	DataBlocks<bits64>* SP = ECB(IV, key, true);
	// Split into 32-bit numbers
	DataBlocks<bits32>* SP32 = Split64To32(SP);

	bits64 left = ((bits64)(*SP32)[0] + 0x1010104) % 4294967295; // Сложение левой части с C1 по модулю 2^32 - 1
	bits64 right = ((bits64)(*SP32)[1] + 0x1010101) % 4294967296; // Сложение правой части с C2 по модулю 2^32
	bits64 gamma = (*ECB(GetBytes((left << 32) + right), key, true))[0]; // Полученная гамма шифрования
	DataBlocks<bits64>* blocks;
	if (isEncrypt) // For encryption
		blocks = new DataBlocks<bits64>(text);
	else // For decryption
	{
		DataBlocks<bits32>* blocks32 = FromHex(text);
		blocks = Join32To64(blocks32);
	}

	// XOR input with gamma
	for (int i = 0; i < blocks->Length(); i++)
		(*blocks)[i] ^= gamma;

	return blocks;
}

// Gamma mode with feedback
DataBlocks<bits64>* CFB(DataBlocks<byte>* text, DataBlocks<byte>* key, DataBlocks<byte>* IV, bool isEncrypt)
{
	// Encryption of the sync message
	DataBlocks<bits64>* SP = ECB(IV, key, true);
	// Split into 32-bit numbers
	DataBlocks<bits32>* SP32 = Split64To32(SP);

	bits64 left = (bits64)(*SP32)[0];
	bits64 right = (bits64)(*SP32)[1];
	bits64 gamma = (*ECB(GetBytes((left << 32) + right), key, true))[0]; // Полученная гамма шифрования

	DataBlocks<bits64>* blocks;
	if (isEncrypt) // For encryption
		blocks = new DataBlocks<bits64>(text);
	else // For decryption
	{
		DataBlocks<bits32>* blocks32 = FromHex(text);
		blocks = Join32To64(blocks32);
	}

	DataBlocks<bits64>* result = new DataBlocks<bits64>(blocks->Length());

	for (int i = 0; i < blocks->Length(); i++)
	{
		(*result)[i] = gamma ^ (*blocks)[i];
		if (isEncrypt)
			// Redefining the gamma when encrypting
			gamma = (*ECB(GetBytes((*result)[i]), key, true))[0];
		else
			// Redefine gamma when decrypting
			gamma = (*ECB(GetBytes((*blocks)[i]), key, true))[0];
	}

	return result;
}

// Imitation insert mode
DataBlocks<bits64>* MAC(DataBlocks<byte>* text, DataBlocks<byte>* key)
{
	DataBlocks<bits64>* blocks64 = new DataBlocks<bits64>(text);
	DataBlocks<bits32>* blocks32 = Split64To32(blocks64);
	int length = blocks64->Length();

	DataBlocks<bits32>* A = new DataBlocks<bits32>(length);
	DataBlocks<bits32>* B = new DataBlocks<bits32>(length);

	// Layout of 32-bit blocks into left and right
	Linkage32(blocks32, A, B);

	DataBlocks<bits32>* X = Subkeys(key, true); // Generate subkeys
	bits32 A_, B_; // Values of the previous round

	for (int i = 0; i < length; i++)
	{
		for (int j = 0; j < 16; j++) // 16 GOST rounds
		{
			A_ = (*B)[i] ^ F((*A)[i], (*X)[j]);
			B_ = (*A)[i];

			(*A)[i] = A_;
			(*B)[i] = B_;
		}

		if (i < length - 1)
		{
			// Modulo 2 addition with the next block
			(*B)[i + 1] ^= (*B)[i];
			(*A)[i + 1] ^= (*A)[i];
		}
	}

	DataBlocks<bits64>* result = new DataBlocks<bits64>(1);
	(*result)[0] = (((bits64)(*B)[length - 1]) << 32) | (*A)[length - 1];
	return result;
}

// Translating bytes from bits64 blocks into a string of text
char* Blocks64ToString(DataBlocks<bits64>* blocks)
{
	int length = blocks->Length();
	char* result = new char[length * 8];
	int k = 0;
	for (int i = 0; i < length; i++)
	{
		DataBlocks<byte>* buffer = GetBytes((*blocks)[i]);
		bool flag = true;
		for (int j = 0; j < buffer->Length(); j++)
		{
			if ((*buffer)[j] == 0 && flag)
			{
				continue;
			}

			result[k] = (*buffer)[j];
			k++;
			flag = false;
		}
	}
	result[k] = '\0';
	return result;
}

// Display blocks of data bits64
void PrintBlocks64(DataBlocks<bits64>* text, bool isEncrypt)
{
	int length = text->Length();
	if (isEncrypt)
	{
		cout << "Cipher text: ";
		for (int i = 0; i < length; i++)
		{
			cout.width(16);
			cout.fill('0');
			cout << hex << (*text)[i];
		}
	}
	else
		cout << "Plain text: " << Blocks64ToString(text);

	cout << endl;
	return;
}

// String comparison
bool StrCmp(const char* str1, const char* str2)
{
	int length1 = strlen(str1);
	int length2 = strlen(str2);
	if (length1 != length2)
		return false;

	for (int i = 0; i < length1; i++)
		if (str1[i] != str2[i])
			return false;

	return true;
}

void Error()
{
	cout << "Get away from the keyboard, animal!" << endl;
	system("pause");
	exit(1);
}

int main()
{
	system("chcp 1251");
	system("cls");
	char* str = new char[MAX_SIZE + 1];
	bool isEncrypt;
	enum modes { ecb, ctr, cfb, mac };
	modes mode;
	DataBlocks<byte>* text = new DataBlocks<byte>(0);
	DataBlocks<byte>* key = new DataBlocks<byte>(0);
	DataBlocks<byte>* IV = new DataBlocks<byte>(0);

	cout << "Enter the text: ";
	cin.getline(str, MAX_SIZE);
	text = GetBytes(str);

	cout << "Select encryption mode (e - encrypt, d - decrypt): ";
	cin.getline(str, 2);

	if (StrCmp(str, "e"))
		isEncrypt = true;
	else if (StrCmp(str, "d"))
		isEncrypt = false;
	else
		Error();

	if (isEncrypt)
		cout << "Select encryption mode (ECB, CTR, CFB or MAC): ";
	else
		cout << "Select decryption mode (ECB, CTR or CFB): ";

	cin.getline(str, 4);

	if (StrCmp(str, "ECB"))
		mode = ecb;
	else if (StrCmp(str, "CTR"))
		mode = ctr;
	else if (StrCmp(str, "CFB"))
		mode = cfb;
	else if (StrCmp(str, "MAC"))
		mode = mac;
	else
		Error();

	if (mode == mac && !isEncrypt)
		Error();

	cout << "Enter the key (max 32 bytes): ";
	cin.getline(str, 33);
	key = GetBytes(str);

	if (mode == ctr || mode == cfb)
	{
		cout << "Enter the initialization vector (64-bit number): ";
		bits64 number;
		cin >> number;
		IV = GetBytes(number);
	}

	switch (mode)
	{
	case ecb:
		PrintBlocks64(ECB(text, key, isEncrypt), isEncrypt);
		break;
	case ctr:
		PrintBlocks64(CTR(text, key, IV, isEncrypt), isEncrypt);
		break;
	case cfb:
		PrintBlocks64(CFB(text, key, IV, isEncrypt), isEncrypt);
		break;
	case mac:
		PrintBlocks64(MAC(text, key), isEncrypt);
		break;
	default: break;
	}

	delete str;
	delete text;
	delete key;
	delete IV;
	system("pause");
	return 0;
}