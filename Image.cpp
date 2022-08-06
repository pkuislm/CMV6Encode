#include "Image.h"

#include <ximage.h>
#pragma comment(lib,"cximage.lib")

BMPImage readImage(const std::wstring& filename) {
	BMPImage image;

	// open file
	std::wcout << "Reading " << filename << "...\n";
	CxImage cximage;
	cximage.Load(filename.c_str(), CXIMAGE_FORMAT_PNG);
	cximage.Flip();
	//
	image.width = cximage.GetWidth();
	image.height = cximage.GetHeight();

	image.blockHeight = (image.height + 7) / 8;
	image.blockWidth = (image.width + 7) / 8;

	image.blocks = new (std::nothrow) Block[image.blockHeight * image.blockWidth];
	if (image.blocks == nullptr) {
		std::cout << "Error - Memory error\n";
		return image;
	}

	RGBQUAD px;
	for (int y = 0; y < image.height; ++y) {
		const uint blockRow = y / 8;
		const uint pixelRow = y % 8;
		for (uint x = 0; x < image.width; ++x) {
			px = cximage.GetPixelColor(x, y);
			const uint blockColumn = x / 8;
			const uint pixelColumn = x % 8;
			const uint blockIndex = blockRow * image.blockWidth + blockColumn;
			const uint pixelIndex = pixelRow * 8 + pixelColumn;
			image.blocks[blockIndex].cr[pixelIndex] = px.rgbBlue;
			image.blocks[blockIndex].cb[pixelIndex] = px.rgbGreen;
			image.blocks[blockIndex].y[pixelIndex] = px.rgbRed;
		}
	}
	cximage.Destroy();
	
	//¼ÆËãHCU²¢Ìî³ä
	image.HCU_Height = (image.blockHeight + 31) / 32;
	image.HCU_Width = (image.blockWidth + 31) / 32;
	image.Cycles = new HCUCycle[image.HCU_Height * image.HCU_Width + 1]{};
	image.HCU_Count = image.HCU_Height * image.HCU_Width;

	for (uint i = 0; i < image.HCU_Count; ++i)
	{
		if (((i + 1) % image.HCU_Width) == 0)
		{
			image.Cycles[i].W = (image.blockWidth / 2) % 16 == 0 ? 16 : (image.blockWidth / 2) % 16;
		}
		else
		{
			image.Cycles[i].W = 16;
		}

		if ((i / image.HCU_Width) == image.HCU_Height - 1)
		{
			image.Cycles[i].H = (image.blockHeight / 2) % 16 == 0 ? 16 : (image.blockHeight / 2) % 16;
		}
		else
		{
			image.Cycles[i].H = 16;
		}
	}

	return image;
}

// convert all pixels in a block from RGB color space to YCbCr
void RGBToYCbCrBlock(Block& block) {
	for (uint y = 0; y < 8; ++y) {
		for (uint x = 0; x < 8; ++x) {
			const uint pixel = y * 8 + x;
			int y = 0.2990 * block.r[pixel] + 0.5870 * block.g[pixel] + 0.1140 * block.b[pixel] - 128;
			int cb = -0.1687 * block.r[pixel] - 0.3313 * block.g[pixel] + 0.5000 * block.b[pixel];
			int cr = 0.5000 * block.r[pixel] - 0.4187 * block.g[pixel] - 0.0813 * block.b[pixel];
			if (y < -128) y = -128;
			if (y > 127) y = 127;
			if (cb < -128) cb = -128;
			if (cb > 127) cb = 127;
			if (cr < -128) cr = -128;
			if (cr > 127) cr = 127;
			block.y[pixel] = y;
			block.cb[pixel] = cb;
			block.cr[pixel] = cr;
		}
	}
}

// convert all pixels from RGB color space to YCbCr
void RGBToYCbCr(const BMPImage& image) {
	for (uint y = 0; y < image.blockHeight; ++y) {
		for (uint x = 0; x < image.blockWidth; ++x) {
			RGBToYCbCrBlock(image.blocks[y * image.blockWidth + x]);
		}
	}
}

// perform 1-D FDCT on all columns and rows of a block component
//   resulting in 2-D FDCT
void forwardDCTBlockComponent(int* const component) {
	for (uint i = 0; i < 8; ++i) {
		const float a0 = component[0 * 8 + i];
		const float a1 = component[1 * 8 + i];
		const float a2 = component[2 * 8 + i];
		const float a3 = component[3 * 8 + i];
		const float a4 = component[4 * 8 + i];
		const float a5 = component[5 * 8 + i];
		const float a6 = component[6 * 8 + i];
		const float a7 = component[7 * 8 + i];

		const float b0 = a0 + a7;
		const float b1 = a1 + a6;
		const float b2 = a2 + a5;
		const float b3 = a3 + a4;
		const float b4 = a3 - a4;
		const float b5 = a2 - a5;
		const float b6 = a1 - a6;
		const float b7 = a0 - a7;

		const float c0 = b0 + b3;
		const float c1 = b1 + b2;
		const float c2 = b1 - b2;
		const float c3 = b0 - b3;
		const float c4 = b4;
		const float c5 = b5 - b4;
		const float c6 = b6 - c5;
		const float c7 = b7 - b6;

		const float d0 = c0 + c1;
		const float d1 = c0 - c1;
		const float d2 = c2;
		const float d3 = c3 - c2;
		const float d4 = c4;
		const float d5 = c5;
		const float d6 = c6;
		const float d7 = c5 + c7;
		const float d8 = c4 - c6;

		const float e0 = d0;
		const float e1 = d1;
		const float e2 = d2 * m1;
		const float e3 = d3;
		const float e4 = d4 * m2;
		const float e5 = d5 * m3;
		const float e6 = d6 * m4;
		const float e7 = d7;
		const float e8 = d8 * m5;

		const float f0 = e0;
		const float f1 = e1;
		const float f2 = e2 + e3;
		const float f3 = e3 - e2;
		const float f4 = e4 + e8;
		const float f5 = e5 + e7;
		const float f6 = e6 + e8;
		const float f7 = e7 - e5;

		const float g0 = f0;
		const float g1 = f1;
		const float g2 = f2;
		const float g3 = f3;
		const float g4 = f4 + f7;
		const float g5 = f5 + f6;
		const float g6 = f5 - f6;
		const float g7 = f7 - f4;

		component[0 * 8 + i] = g0 * s0;
		component[4 * 8 + i] = g1 * s4;
		component[2 * 8 + i] = g2 * s2;
		component[6 * 8 + i] = g3 * s6;
		component[5 * 8 + i] = g4 * s5;
		component[1 * 8 + i] = g5 * s1;
		component[7 * 8 + i] = g6 * s7;
		component[3 * 8 + i] = g7 * s3;
	}
	for (uint i = 0; i < 8; ++i) {
		const float a0 = component[i * 8 + 0];
		const float a1 = component[i * 8 + 1];
		const float a2 = component[i * 8 + 2];
		const float a3 = component[i * 8 + 3];
		const float a4 = component[i * 8 + 4];
		const float a5 = component[i * 8 + 5];
		const float a6 = component[i * 8 + 6];
		const float a7 = component[i * 8 + 7];

		const float b0 = a0 + a7;
		const float b1 = a1 + a6;
		const float b2 = a2 + a5;
		const float b3 = a3 + a4;
		const float b4 = a3 - a4;
		const float b5 = a2 - a5;
		const float b6 = a1 - a6;
		const float b7 = a0 - a7;

		const float c0 = b0 + b3;
		const float c1 = b1 + b2;
		const float c2 = b1 - b2;
		const float c3 = b0 - b3;
		const float c4 = b4;
		const float c5 = b5 - b4;
		const float c6 = b6 - c5;
		const float c7 = b7 - b6;

		const float d0 = c0 + c1;
		const float d1 = c0 - c1;
		const float d2 = c2;
		const float d3 = c3 - c2;
		const float d4 = c4;
		const float d5 = c5;
		const float d6 = c6;
		const float d7 = c5 + c7;
		const float d8 = c4 - c6;

		const float e0 = d0;
		const float e1 = d1;
		const float e2 = d2 * m1;
		const float e3 = d3;
		const float e4 = d4 * m2;
		const float e5 = d5 * m3;
		const float e6 = d6 * m4;
		const float e7 = d7;
		const float e8 = d8 * m5;

		const float f0 = e0;
		const float f1 = e1;
		const float f2 = e2 + e3;
		const float f3 = e3 - e2;
		const float f4 = e4 + e8;
		const float f5 = e5 + e7;
		const float f6 = e6 + e8;
		const float f7 = e7 - e5;

		const float g0 = f0;
		const float g1 = f1;
		const float g2 = f2;
		const float g3 = f3;
		const float g4 = f4 + f7;
		const float g5 = f5 + f6;
		const float g6 = f5 - f6;
		const float g7 = f7 - f4;

		component[i * 8 + 0] = g0 * s0;
		component[i * 8 + 4] = g1 * s4;
		component[i * 8 + 2] = g2 * s2;
		component[i * 8 + 6] = g3 * s6;
		component[i * 8 + 5] = g4 * s5;
		component[i * 8 + 1] = g5 * s1;
		component[i * 8 + 7] = g6 * s7;
		component[i * 8 + 3] = g7 * s3;
	}
}

// perform FDCT on all MCUs
void forwardDCT(const BMPImage& image) {
	uint channelCount = 0;
	for (uint y = 0; y < image.blockHeight; ++y) {
		for (uint x = 0; x < image.blockWidth; ++x) {
			channelCount = image.blocks[y * image.blockWidth + x].isbase ? 3 : 1;
			for (uint i = 0; i < channelCount; ++i) {
				forwardDCTBlockComponent(image.blocks[y * image.blockWidth + x][i]);
			}
		}
	}
}

// quantize a block component based on a quantization table
void quantizeBlockComponent(const QuantizationTable& qTable, int* const component) {
	for (uint i = 0; i < 64; ++i) {
		component[i] /= (signed)qTable.table[i];
	}
}

// quantize all MCUs
void quantize(const BMPImage& image) {
	for (uint y = 0; y < image.blockHeight; ++y) {
		for (uint x = 0; x < image.blockWidth; ++x) {
			for (uint i = 0; i < 3; ++i) {
				quantizeBlockComponent(*qTables100[i], image.blocks[y * image.blockWidth + x][i]);
			}
		}
	}
}