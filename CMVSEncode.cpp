﻿// CMVSEncode.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "Image.h"
#include <io.h>

struct cmv_head {
	char magic[4]; //CMV
	int cmv_frame_start_offset;
	long long cmv_size;
	int cmv_frame_count;
	int unk2;
	int unk3;
	int frame_width;
	int frame_height;
	int unk4[2];
};

struct cmv_frame_index {
	int frame_index;
	int cmv_frame_size;
	int cmv_original_frame_size; // height * width * 3 (RGB)
	int frame_type; //2 == video, 0 == audio ?
	int offset; //in file : cmv_frame_start_offset + offset
};

unsigned char cmvhexData[24] = {
	0x18, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0xD0, 0x02, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00
};

char hexData[28] = {
	0x4A, 0x42, 0x50, 0x44, 0x2C, 0x00, 0x00, 0x00, 0x02, 0x00, 0xC0, 0x58, 0x50, 0x00, 0x00, 0x00, 0x00, 0x05, 0xD0, 0x02,
	0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

char hexData1[12] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x7F, 0x00, 0x00 };

char hexData2[164] = {
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x05, 0x0F, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0D, 0x10, 0x0D, 0x10, 0x0D, 0x10, 0x0D, 0x10,
	0x0D, 0x10, 0x00, 0x00
};

bool SortByFreq4(freqzero x, freqzero y) {
	return x.numofpadding < y.numofpadding;
}

size_t WriteSingleFrame(std::wstring& filename, std::ofstream& output)
{
	auto start = output.tellp();

	BMPImage in = readImage(filename);

    RGBToYCbCr(in);
	reSampleBlock(in);

    forwardDCT(in);
    quantize(in);

	JBPDImage image = BuildTrees(in);

	std::vector<byte> huffmanData;
    GetScanData(image, huffmanData);

	output.write(hexData, 28);
	output.write((char*)&image.DCScanSize, 4);
	output.write((char*)&image.ACScanSize, 4);
	output.write(hexData1, 12);

	output.write((char*)&image.DC_freq[0], 64);
	output.write((char*)&image.AC_freq[0], 64);

	std::sort(image.AC_zero, image.AC_zero + 16, SortByFreq4);
	for (uint i = 0; i < 16; ++i)
	{
		output.write((char*)&image.AC_zero[i].numofzero, 1);
	}
	output.write(hexData2, 164);
	output.write((char*)&huffmanData[0], huffmanData.size());
	
	if (in.blocks)
	{
		delete[] in.blocks;
	}
	if (in.Cycles)
	{
		delete in.Cycles;
	}

	if (image.AC)
	{
		delete image.AC;
	}
	if (image.DC)
	{
		delete image.DC;
	}

	return output.tellp() - start;
}

int getFileNum(const std::wstring& path) {
	struct _wfinddata_t fileinfo;
	std::wstring current = path + L"\\*.png";
	long long handle = _wfindfirst(current.c_str(), &fileinfo);
	int fileNum = 1;
	if (handle == -1)
		return 0;
	while (!_wfindnext(handle, &fileinfo))
		fileNum++;
	_findclose(handle);
	return fileNum;
}

static wchar_t pathtmp[260]{};
int wmain(int argc, wchar_t** argv)
{
	if (argc < 2)
	{
		std::cout << "Usage: Encoder.exe [PNG Path] [PNG NameLen] [OGG Path]\n\n";
		std::cout << "[PNG Path] 存放视频帧序列的文件夹路径\n";
		std::cout << "[PNG NameLen] PNG序列的文件名长度（例如00001.png长度是5）\n";
		std::cout << "[OGG Path] 视频音轨的路径" << std::endl;
		return 0;
	}

	if (argc == 4)
	{
		int nameLen = _wtoi(argv[2]);
		if (nameLen <= 0 || nameLen > 10)
		{
			std::cout << "PNG NameLen is invalid." << std::endl;
			return 0;
		}
		int fileNum = getFileNum(argv[1]);

		std::cout << "Total Frames: " << fileNum << std::endl;

		_snwprintf_s(pathtmp, _TRUNCATE, L"%s\\..\\output.cmv", argv[1]);
		std::ofstream outFile(pathtmp, std::ios::out | std::ios::binary);
		
		//TODO: SetUp CMV struct
		//outFile.write("CMV6", 4);
		cmv_head head{};
		memcpy_s(head.magic,4, "CMV6",4);
		head.cmv_frame_count = fileNum;
		head.unk2 = 24;
		head.unk3 = 1;
		head.frame_height = 720;
		head.frame_width = 1280;
		head.unk4[0] = 24;
		head.unk4[1] = 2;

		outFile.seekp(44, std::ios::beg);
		//outFile.write((char*)cmvhexData, 24);
		auto index = new cmv_frame_index[fileNum + 1];
		outFile.seekp((fileNum + 1) * sizeof(cmv_frame_index), std::ios::cur);
		head.cmv_frame_start_offset = outFile.tellp();

		for (int i = 0; i < fileNum; ++i)
		{
			_snwprintf_s(pathtmp, _TRUNCATE, L"%s\\%0*d.png", argv[1], nameLen, i);
			std::wstring inFile(pathtmp);
			index[i].frame_index = i;
			index[i].cmv_original_frame_size = 0x2A3000;
			index[i].frame_type = 2;
			index[i].offset = (size_t)outFile.tellp() - head.cmv_frame_start_offset;
			index[i].cmv_frame_size = WriteSingleFrame(inFile, outFile);
		}
		index[fileNum].frame_index = fileNum;
		index[fileNum].frame_type = 0;
		index[fileNum].offset = (size_t)outFile.tellp() - head.cmv_frame_start_offset;

		std::ifstream soundtrack(argv[3], std::ios::binary);
		soundtrack.seekg(0, std::ios::end);
		index[fileNum].cmv_original_frame_size = index[fileNum].cmv_frame_size = soundtrack.tellg();

		auto buffer = new byte[index[fileNum].cmv_frame_size];
		soundtrack.seekg(0, std::ios::beg);
		soundtrack.read((char*)buffer, index[fileNum].cmv_frame_size);
		soundtrack.close();

		outFile.write((char*)buffer, index[fileNum].cmv_frame_size);
		delete[] buffer;

		head.cmv_size = outFile.tellp();
		outFile.seekp(0, std::ios::beg);
		outFile.write((char*)&head, sizeof(cmv_head));
		outFile.seekp(44, std::ios::beg);
		outFile.write((char*)index, (fileNum + 1) * sizeof(cmv_frame_index));
		outFile.close();
		delete[] index;
	}

	
	
	return 0;
}
