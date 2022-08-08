#pragma once
#include "jbpd.h"

// IDCT scaling factors
const float m0 = 2.0 * std::cos(1.0 / 16.0 * 2.0 * M_PI);
const float m1 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * M_PI);
const float m3 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * M_PI);
const float m5 = 2.0 * std::cos(3.0 / 16.0 * 2.0 * M_PI);
const float m2 = m0 - m5;
const float m4 = m0 + m5;

const float s0 = std::cos(0.0 / 16.0 * M_PI) / std::sqrt(8);
const float s1 = std::cos(1.0 / 16.0 * M_PI) / 2.0;
const float s2 = std::cos(2.0 / 16.0 * M_PI) / 2.0;
const float s3 = std::cos(3.0 / 16.0 * M_PI) / 2.0;
const float s4 = std::cos(4.0 / 16.0 * M_PI) / 2.0;
const float s5 = std::cos(5.0 / 16.0 * M_PI) / 2.0;
const float s6 = std::cos(6.0 / 16.0 * M_PI) / 2.0;
const float s7 = std::cos(7.0 / 16.0 * M_PI) / 2.0;

struct QuantizationTable
{
	uint table[64] = { 0 };
	bool set = false;
};

const QuantizationTable qTableY100 = {
		{
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1
		},
		true
};

const QuantizationTable qTableCbCr100 = {
		{
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1
		},
		true
};

const QuantizationTable qTableYCMV = {
		{
			0x06, 0x04, 0x04, 0x06, 0x0A, 0x10, 0x14, 0x18,
			0x05, 0x05, 0x06, 0x08, 0x0A, 0x17, 0x18, 0x16,
			0x06, 0x05, 0x06, 0x0A, 0x10, 0x17, 0x1C, 0x16,
			0x06, 0x07, 0x09, 0x0C, 0x14, 0x23, 0x20, 0x19,
			0x07, 0x09, 0x0F, 0x16, 0x1B, 0x2C, 0x29, 0x1F,
			0x0A, 0x0E, 0x16, 0x1A, 0x20, 0x2A, 0x2D, 0x25,
			0x14, 0x1A, 0x1F, 0x23, 0x29, 0x30, 0x30, 0x28,
			0x1D, 0x25, 0x26, 0x27, 0x2D, 0x28, 0x29, 0x28
		},
		true
};

const QuantizationTable qTableCbCrCMV = {
		{
			0x07, 0x07, 0x0A, 0x13, 0x28, 0x28, 0x28, 0x28,
			0x07, 0x08, 0x0A, 0x1A, 0x28, 0x28, 0x28, 0x28,
			0x0A, 0x0A, 0x16, 0x28, 0x28, 0x28, 0x28, 0x28,
			0x13, 0x1A, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
			0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
			0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
			0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
			0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28
		},
		true
};

const QuantizationTable* const qTables100[] = { &qTableY100, &qTableCbCr100, &qTableCbCr100 };
const QuantizationTable* const qTablesCMV[] = { &qTableYCMV, &qTableCbCrCMV, &qTableCbCrCMV };

BMPImage readImage(const std::wstring& filename);
void RGBToYCbCr(const BMPImage& image);
void forwardDCT(const BMPImage& image);
void quantize(const BMPImage& image);