#pragma once
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <assert.h>
typedef unsigned char byte;
typedef unsigned int uint;
#define M_PI 3.14159265358979323846
#define LEN 512

const byte zigZagMap[] = {
		0,   1,  8, 16,  9,  2,  3, 10,
		17, 24, 32, 25, 18, 11,  4,  5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13,  6,  7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63
};

struct Block {
	union {
		int y[64] = { 0 };
		int r[64];
	};
	union {
		int cb[64] = { 0 };
		int g[64];
	};
	union {
		int cr[64] = { 0 };
		int b[64];
	};
	bool isbase = false;
	int* operator[](uint i) {
		switch (i) {
		case 0:
			return y;
		case 1:
			return cb;
		case 2:
			return cr;
		default:
			return nullptr;
		}
	}
};

struct HCUCycle
{
	byte H;
	byte W;
};

struct BMPImage 
{
	uint height = 0;
	uint width = 0;

	Block* blocks = nullptr;

	uint blockHeight = 0;
	uint blockWidth = 0;

	uint HCU_Height;
	uint HCU_Width;
	uint HCU_Count;
	HCUCycle* Cycles;
};

struct huffman_node {
	uint c = 0;
	uint freq = 0;
	char huffman_code[LEN]{};
	huffman_node* left_child, * right_child;
	bool leaf = false;
};



class HuffmanEncode
{

	std::map<uint, uint> huffman_table;
	huffman_node* root;
	uint* freq;
public:
	std::map<uint, std::string> huffman_code;
	HuffmanEncode(uint* f):freq(f), root(nullptr) {};
	void Encode();
	bool GetCode(uint coefflen, uint& code, uint& length);

private:
	void CalFreq();
	void CreateTree();
	void GenerateCode();
	void Serialize(huffman_node* node);
	void DestoryHuffmanTree(huffman_node* node);
};

class BitWriter {
private:
	//byte nextBit = 0;
	std::vector<byte>& data;
	uint bits_pos;
	byte pack_tmp;
public:
	BitWriter(std::vector<byte>& d) :
		data(d), bits_pos(0), pack_tmp(0)
	{}

	void writeBits(uint b, uint len);

	void flush();

	size_t GetDataSize();

	byte reverseBits(byte n);
};

struct freqzero
{
	uint numofzero;
	uint numofpadding;
	uint freq;

	bool operator<(const freqzero& rhs)
	{
		return freq < rhs.freq;
	}
};

struct JBPDImage
{
	BMPImage& _image;
	uint DC_freq[16]{};
	uint AC_freq[16]{};
	freqzero AC_zero[16]{};
	HuffmanEncode* DC;
	HuffmanEncode* AC;
	uint ACScanSize = 0;
	uint DCScanSize = 0;


	JBPDImage(BMPImage& in):_image(in)
	{
		DC = new HuffmanEncode(DC_freq);
		AC = new HuffmanEncode(AC_freq);
		for (uint i = 0; i < 16; ++i)
		{
			AC_zero[i].numofzero = i;
		}
	}

};

void reSampleBlock(BMPImage& image);
JBPDImage BuildTrees(BMPImage& image);
void DebugPrintPixels(JBPDImage& image);
bool GetScanData(JBPDImage& input, std::vector<byte>& bitWriter);
bool SortByFreq2(freqzero x, freqzero y);
