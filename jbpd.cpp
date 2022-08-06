#include "jbpd.h"

#define Range(m, x) for(int x = 0; x < m; ++x)

//抽象迭代器
class Iterator
{
public:
	//下一个位置是否有元素
	virtual bool hasNext() = 0;
	//返回当前元素，并且走到下一个位置
	virtual int next() = 0;
};

uint BCUmap[6] = {0, 1, 2, 3, 0, 0};
uint BCUmap2[6] = { 0, 0, 0, 0, 1, 2 };

uint bitLength(int v) {
	uint length = 0;
	while (v > 0) {
		v >>= 1;
		length += 1;
	}
	return length;
}

class DCIterator : public Iterator
{
private:
	const JBPDImage& const _info;
	uint _curPos;				//当前访问的MCU位置
	uint _limit;				//这个图片总共有多少DC值
	uint _bcuX;					//当前游标所在的BCU横向位置
	uint _bcuY;					//当前游标所在的BCU纵向位置
	uint _innerbcuIdx;			//在当前BCU内遍历时的序号
	uint _bcuW;					//图片的BCU宽度
	uint _bcuH;					//图片的BCU高度

	uint _addW;
	uint _currentHcuW;
	uint _currentHcuH;
	uint _HcuPos;
public:
	DCIterator(const JBPDImage& info)
		:_info(info)
		, _curPos(0), _bcuX(0), _bcuY(0), _innerbcuIdx(0), _currentHcuH(0), _currentHcuW(0), _HcuPos(0), _addW(0)
	{
		_bcuH = _info._image.blockHeight / 2;
		_bcuW = _info._image.blockWidth / 2;
		_limit = (_bcuW * _bcuH) * 6;
	}

	virtual bool hasNext()
	{
		return _curPos < _limit;
	}

	virtual int next()
	{
		//uint v1 = _bcuY * (_bcuW * 4);
		//uint v2 = _bcuX * 2;
		//uint v3 = (BCUmap[_innerbcuIdx] > 1) * _bcuW * 2;
		//uint v4 = (BCUmap[_innerbcuIdx] % 2);
		_currentHcuW = _info.Cycles[_HcuPos].W;
		_currentHcuH = _info.Cycles[_HcuPos].H;

		int ret = _info._image.blocks[_addW + _bcuY * (_bcuW * 2 * 2) + _bcuX * 2 + (BCUmap[_innerbcuIdx] > 1) * _bcuW * 2 + (BCUmap[_innerbcuIdx] % 2)][BCUmap2[_innerbcuIdx]][0];
		//		所有MCU              |偏移|  |          行          |  |   列   |   |              在BCU中的列            |  |       在BCU中的行       ||     在BCU中的通道   ||DC|

		_curPos++;
		_innerbcuIdx++;
		if (_innerbcuIdx > 5)
		{
			_innerbcuIdx = 0;
			_bcuX++;
		}

		if (_bcuY >= _currentHcuH - 1 && _bcuX > _currentHcuW - 1)
		{
			_innerbcuIdx = 0;
			_bcuX = 0;
			_bcuY = 0;
			_HcuPos++;

			_addW += 32;
		}
		
		if (_bcuX > _currentHcuW - 1)
		{
			_bcuX = 0;
			_bcuY++;
		}

		if ((_HcuPos % _info.HCU_Width) == 0 && _HcuPos)
		{
			_addW = (_HcuPos / (_info.HCU_Width - 1)) * (_bcuW * _info.Cycles[_HcuPos - 1].H * 4);
		}

		return ret;
	}

	void DebugNext()
	{
		printf("Addw:%d, CurH:%d, CurW:%d, BCU_X:%d, BCU_Y:%d, Idx1:%d -> ", _addW, _currentHcuH, _currentHcuW, _bcuX, _bcuY, _innerbcuIdx);
		printf("%d\n", next());
	}
};

class ACIterator : public Iterator
{
private:
	const JBPDImage& const _info;
	uint _curPos;				//当前访问的MCU位置
	uint _limit;				//这个图片总共有多少DC值
	uint _bcuX;					//当前游标所在的BCU横向位置
	uint _bcuY;					//当前游标所在的BCU纵向位置
	uint _innerBcuIdx;			//在当前BCU内遍历时的序号
	uint _innerMcuIdx;			//在当前BCU内遍历时的MCU序号
	uint _bcuW;					//图片的BCU宽度
	uint _bcuH;					//图片的BCU高度

	uint _addW;
	uint _currentHcuW;
	uint _currentHcuH;
	uint _HcuPos;
public:
	ACIterator(const JBPDImage& info)
		:_info(info)
		, _curPos(0), _bcuX(0), _bcuY(0), _innerBcuIdx(0), _innerMcuIdx(1), _HcuPos(0), _addW(0), _currentHcuW(0), _currentHcuH(0)
	{
		_bcuH = _info._image.blockHeight / 2;
		_bcuW = _info._image.blockWidth / 2;
		_limit = (_bcuW * _bcuH) * 6 * 63;
	}

	virtual bool hasNext()
	{
		return _curPos < _limit;
	}

	virtual int next()
	{
		_currentHcuW = _info.Cycles[_HcuPos].W;
		_currentHcuH = _info.Cycles[_HcuPos].H;

		int ret = _info._image.blocks[_addW + _bcuY * (_bcuW * 4) + _bcuX * 2 + (BCUmap[_innerBcuIdx] > 1) * _bcuW * 2 + (BCUmap[_innerBcuIdx] % 2)][BCUmap2[_innerBcuIdx]][zigZagMap[_innerMcuIdx]];
		//		所有MCU               |偏移| |        行       |   |    列   |  |             在BCU中的列            |   |       在BCU中的行       ||     在BCU中的通道   || 在MCU中的序号(zigzag) |

		_curPos++;
		_innerMcuIdx++;
		if (_innerMcuIdx > 63)
		{
			_innerMcuIdx = 1;
			_innerBcuIdx++;
		}
		if (_innerBcuIdx > 5)
		{
			_innerBcuIdx = 0;
			_bcuX++;
		}

		if (_bcuY >= _currentHcuH - 1 && _bcuX > _currentHcuW - 1)
		{
			_innerBcuIdx = 0;
			_bcuX = 0;
			_bcuY = 0;
			_HcuPos++;

			_addW += 32;
		}

		if (_bcuX > _currentHcuW - 1)
		{
			_bcuX = 0;
			_bcuY++;
		}

		if ((_HcuPos % _info.HCU_Width) == 0 && _HcuPos)
		{
			_addW = (_HcuPos / (_info.HCU_Width - 1)) * (_bcuW * _info.Cycles[_HcuPos - 1].H * 4);
		}

		return ret;
	}

	void DebugNext()
	{
		//printf("AC: BCU_X:%d, BCU_Y:%d, Idx1:%d, Idx2:%d -> ", _bcuX, _bcuY, _innerBcuIdx, _innerMcuIdx);
		printf("AC: %d\n", next());
	}
};

void DebugPrintPixels(JBPDImage& image)
{
	FILE* stream1;
	freopen_s(&stream1, "E:\\vmvt\\op\\test1_8.txt", "w", stdout);
	DCIterator di(image);
	while (di.hasNext())
	{
		di.DebugNext();
	}

	ACIterator ai(image);
	while (ai.hasNext())
	{
		ai.DebugNext();
	}
	//fclose(stdin);
}

void countDCFrequent(JBPDImage& out)
{
	DCIterator di(out);
	int previousDC = 0;
	while (di.hasNext())
	{
		int currentdc = di.next();
		int coeff = currentdc - previousDC;
		previousDC = currentdc;

		uint coeffLength = bitLength(std::abs(coeff));
		if (coeffLength > 15)
		{
			throw("Error - DC Coefficient length is greater than 15\n");
		}
		else
		{
			out.DC_freq[coeffLength]++;
		}
	}
}

void countACFrequent(JBPDImage& out)
{
	ACIterator ai(out);
	while (ai.hasNext())
	{
		for (int i = 0; i < 63; ++i)
		{
			byte numZeroes = 0;
			int coeff = ai.next();
			while (i < 63 && coeff == 0) {
				numZeroes += 1;
				i += 1;
				if (i < 63)
				{
					coeff = ai.next();
				}
			}

			if (i == 63) {
				out.AC_freq[15]++;
				break;
			}

			while (numZeroes >= 16) {
				out.AC_freq[0]++;
				out.AC_zero[15].freq++;
				numZeroes -= 16;
			}

			if (numZeroes > 0)
			{
				out.AC_freq[0]++;
				out.AC_zero[numZeroes - 1].freq++;
			}

			assert(coeff != 0);
			uint coeffLength = bitLength(std::abs(coeff));
			if (coeffLength > 10) {
				throw("Error - AC coefficient length greater than 10\n");
			}
			else 
			{
				out.AC_freq[coeffLength]++;
			}
		}
	}
}

bool SortByFreq2(freqzero x, freqzero y) {
	return x.freq > y.freq;
}

bool SortByFreq3(freqzero x, freqzero y) {
	return x.numofzero < y.numofzero;
}

JBPDImage BuildTrees(BMPImage& image)
{
	JBPDImage ret(image);

	ret.HCU_Height = (image.blockHeight + 31) / 32;
	ret.HCU_Width = (image.blockWidth + 31) / 32;
	ret.Cycles = new HCUCycle[ret.HCU_Height * ret.HCU_Width + 1]{};
	ret.HCU_Count = ret.HCU_Height * ret.HCU_Width;

	for (uint i = 0; i < ret.HCU_Count; ++i)
	{
		if (((i+1)%ret.HCU_Width) == 0)
		{
			ret.Cycles[i].W = (image.blockWidth / 2) % 16 == 0 ? 16 : (image.blockWidth / 2) % 16;
		}
		else
		{
			ret.Cycles[i].W = 16;
		}

		if ((i / ret.HCU_Width) == ret.HCU_Height - 1)
		{
			ret.Cycles[i].H = (image.blockHeight / 2) % 16 == 0 ? 16 : (image.blockHeight / 2) % 16;
		}
		else 
		{
			ret.Cycles[i].H = 16;
		}
	}

	//DebugPrintPixels(ret);

	countDCFrequent(ret);
	ret.DC->Encode();
	std::cout << std::endl;
	countACFrequent(ret);
	ret.AC->Encode();
	std::sort(ret.AC_zero, ret.AC_zero + 16, SortByFreq2);
	for (uint i = 0; i < 16; ++i)
	{
		ret.AC_zero[i].numofpadding = i;
	}
	std::sort(ret.AC_zero, ret.AC_zero + 16, SortByFreq3);
	return ret;
}

void HuffmanEncode::Encode()
{
	CalFreq();
	CreateTree();
	GenerateCode();
	Serialize(root);
	DestoryHuffmanTree(root);
}

void HuffmanEncode::CalFreq() {
	huffman_table.clear();
	huffman_code.clear();
	for (int i = 0; i < 16; ++i) {
		huffman_table.insert(std::make_pair(i, freq[i]));
	}
}

bool SortByFreq(huffman_node* x, huffman_node* y) {
	return x->freq < y->freq;
}

void HuffmanEncode::CreateTree() {
	std::vector<huffman_node*> huffman_tree_node;

	//创建所有叶子节点
	for (auto it_t : huffman_table) {
		huffman_node* node = new huffman_node;
		node->c = it_t.first;
		node->freq = it_t.second;
		node->left_child = NULL;
		node->right_child = NULL;
		node->leaf = true;
		huffman_tree_node.push_back(node);
	}

	//构建哈夫曼树
	while (huffman_tree_node.size() > 0) {
		//按照频率升序排序
		std::sort(huffman_tree_node.begin(), huffman_tree_node.end(), SortByFreq);

		if (huffman_tree_node.size() == 1) {
			//哈夫曼树已经生成完成
			root = huffman_tree_node[0];
			huffman_tree_node.erase(huffman_tree_node.begin());
		}
		else {
			//取出前两个
			huffman_node* node_1 = huffman_tree_node[0];
			huffman_node* node_2 = huffman_tree_node[1];
			//删除
			huffman_tree_node.erase(huffman_tree_node.begin());
			huffman_tree_node.erase(huffman_tree_node.begin());
			//生成新的节点
			huffman_node* node = new huffman_node;
			node->leaf = false;
			node->freq = node_1->freq + node_2->freq;
			(node_1->freq <= node_2->freq) ? (node->left_child = node_1, node->right_child = node_2) : (node->left_child = node_2, node->right_child = node_1);
			huffman_tree_node.push_back(node);
		}
	}
}

void HuffmanEncode::GenerateCode() {
	if (root == NULL) return;
	//利用层次遍历，构造每一个节点的前缀码
	huffman_node* p = root;
	std::queue<huffman_node*> q;
	q.push(p);
	while (q.size() > 0) {
		p = q.front();
		q.pop();
		if (p->left_child != NULL) {
			q.push(p->left_child);
			strcpy_s((p->left_child)->huffman_code, p->huffman_code);
			char* ptr = (p->left_child)->huffman_code;
			while (*ptr != '\0') {
				ptr++;
			}
			*ptr = '0';
			*(ptr + 1) = '\0';
		}
		if (p->right_child != NULL) {
			q.push(p->right_child);
			strcpy_s((p->right_child)->huffman_code, p->huffman_code);
			char* ptr = (p->right_child)->huffman_code;
			while (*ptr != '\0') {
				ptr++;
			}
			*ptr = '1';
			*(ptr + 1) = '\0';
		}
	}
}

bool HuffmanEncode::GetCode(uint coefflen, uint& code, uint& length)
{
	code = 0;
	auto it = huffman_code.find(coefflen);
	if (it == huffman_code.end()) {
		printf("Error: HuffmanTable is missing element.");
		return false;
	}
	else 
	{
		length = it->second.size();
		for (int i = 0; i < length; i++) 
		{
			code <<= 1;
			//将以ASCII存储的前缀码转换为二进制
			switch (it->second.data()[i]) {
			case '0':
				break;
			case '1':
				code++;
				break;
			default:
				break;
			}
		}
		return true;
	}
}

void HuffmanEncode::Serialize(huffman_node* node) {
	//序列化哈夫曼树，下面注释掉的三句话可以打印 http://mshang.ca/syntree/ 能识别的语法树。
	if (node != NULL) {
		if (!node->leaf) {
			printf_s("[Node ");
			Serialize(node->left_child);
			Serialize(node->right_child);
			printf_s("]");
		}
		else {
			huffman_code.insert(std::make_pair(node->c, node->huffman_code));
			printf_s("[%d]", node->c);
		}
	}
}

void HuffmanEncode::DestoryHuffmanTree(huffman_node* node) {
	if (node != NULL) {
		DestoryHuffmanTree(node->left_child);
		DestoryHuffmanTree(node->right_child);
		delete[] node;
		node = NULL;
	}
}

byte BitWriter::reverseBits(byte n) {
	byte ret = 0;
	for (int i = 0; i < 8; i++) {
		ret = (ret << 1) + (n % 2); //左移1和 *2等效
		n = n >> 1;   //右移1 和 除以2 等效
	}
	return ret;
}

void BitWriter::writeBits(uint b, uint len) {
	while (len--) {
		pack_tmp = pack_tmp << 1 | ((b >> len) & 1);
		bits_pos++;
		if (bits_pos == 8) {
			data.push_back(reverseBits(pack_tmp));
			bits_pos = 0;
			pack_tmp = 0;
		}
	}
}

void BitWriter::flush() {
	if (bits_pos == 0)
	{
		return;
	}
	if (bits_pos != 0)
	{
		writeBits(0, 8 - bits_pos);
	}
}

size_t BitWriter::GetDataSize()
{
	return data.size();
}

bool GetScanData(JBPDImage& input, std::vector<byte>& huffmanData)
{
	BitWriter bitWriter(huffmanData);
	DCIterator di(input);
	int previousDC = 0;
	uint coeffLength = 0;
	uint code = 0;
	uint codeLength = 0;
	while (di.hasNext())
	{
		int currentdc = di.next();
		int coeff = currentdc - previousDC;
		previousDC = currentdc;

		coeffLength = bitLength(std::abs(coeff));
		if (coeffLength > 15)
		{
			throw("Error - DC Coefficient length is greater than 15\n");
		}
		else
		{
			if (coeff < 0) {
				coeff += (1 << coeffLength) - 1;
			}

			code = 0;
			codeLength = 0;
			if (!input.DC->GetCode(coeffLength, code, codeLength)) {
				std::cout << "Error - Invalid DC value\n";
				return false;
			}
			bitWriter.writeBits(code, codeLength);
			bitWriter.writeBits(coeff, coeffLength);
		}
	}
	bitWriter.flush();
	input.DCScanSize = bitWriter.GetDataSize();

	ACIterator ai(input);
	while (ai.hasNext())
	{
		for (int i = 0; i < 63; ++i)
		{
			byte numZeroes = 0;
			int coeff = ai.next();
			while (i < 63 && coeff == 0) {
				numZeroes += 1;
				i += 1;
				if (i < 63)
				{
					coeff = ai.next();
				}
			}

			if (i == 63) {
				if (!input.AC->GetCode(0xF, code, codeLength)) {
					std::cout << "Error - Invalid AC value\n";
					return false;
				}
				bitWriter.writeBits(code, codeLength);
				//bitWriter.writeBits(coeff, coeffLength);
				break;
			}

			if (numZeroes > 0)
			{
				if (!input.AC->GetCode(0x0, code, codeLength)) {
					std::cout << "Error - Invalid AC value\n";
					return false;
				}

				while (numZeroes >= 16) {

					bitWriter.writeBits(code, codeLength);
					for (uint e = 0; e < input.AC_zero[15].numofpadding; ++e)
					{
						bitWriter.writeBits(1, 1);
					}
					bitWriter.writeBits(0, 1);
					numZeroes -= 16;
				}
			
				if (numZeroes > 0)
				{
					bitWriter.writeBits(code, codeLength);
					for (uint e = 0; e < input.AC_zero[numZeroes - 1].numofpadding; ++e)
					{
						bitWriter.writeBits(1, 1);
					}
					bitWriter.writeBits(0, 1);
				}

			}

			assert(coeff != 0);
			uint coeffLength = bitLength(std::abs(coeff));


			if (coeffLength > 10) {
				throw("Error - AC coefficient length greater than 10\n");
			}
			else
			{
				if (coeff < 0) {
					coeff += (1 << coeffLength) - 1;
				}

				code = 0;
				codeLength = 0;
				if (!input.AC->GetCode(coeffLength, code, codeLength)) {
					std::cout << "Error - Invalid AC value\n";
					return false;
				}
				bitWriter.writeBits(code, codeLength);
				bitWriter.writeBits(coeff, coeffLength);
			}
		}
	}
	bitWriter.flush();
	input.ACScanSize = bitWriter.GetDataSize() - input.DCScanSize;
}