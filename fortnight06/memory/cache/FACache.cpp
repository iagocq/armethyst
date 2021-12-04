/* ----------------------------------------------------------------------------

    (EN) armethyst - A simple ARM Simulator written in C++ for Computer Architecture
    teaching purposes. Free software licensed under the MIT License (see license
    below).

    (PT) armethyst - Um simulador ARM simples escrito em C++ para o ensino de
    Arquitetura de Computadores. Software livre licenciado pela MIT License
    (veja a licen�a, em ingl�s, abaixo).

    (EN) MIT LICENSE:

    Copyright 2020 Andr� Vital Sa�de

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

   ----------------------------------------------------------------------------
*/

#include "FACache.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

using namespace std;

void FACache::splitAddress(uint64_t address, uint64_t &tag, uint64_t &offset) {
	offset = address & offsetMask;
	tag = address & ~offsetMask;
}

bool FACache::findTag(uint64_t tag, unsigned int &idx) {
	for (int i = 0; i < associativity; i++) {
		// o bit menos significativo é utilizado para marcar a linha como suja,
		// portanto o desconsideramos quando comparamos com tag
		if ((directory[i] & ~1) == tag) {
			idx = i;
			return true;
		}
	}

	return false;
}

void FACache::setDirty(unsigned int idx) {
	// uint64_t firstTag, secondTag, offset;

	directory[idx] |= 1;

	// caso escrever `bytes` bytes começando do endereço ultrapassar uma linha,
	// então devemos marcar a próxima como suja também.

	// splitAddress(address, firstTag, offset);
	// splitAddress(address + bytes - 1, secondTag, offset);
	// if (firstTag < secondTag) {
	// 	if (!findTag(secondTag, idx)) {

	// 	}
	// 	directory[idx] |= 1;
	// }
}

bool FACache::isDirty(unsigned int idx) {
	return (directory[idx] & 1) == 1;
}

/**
 * Constructs a FACache of 'size' bytes organized in lines of 'lineSize' bytes.
 * 
 * The inherited 'associativity' attribute is set to 'size / lineSize' (integer division).
 * 
 * Constraints: 'size' must be a multiple of 'lineSize', and both must be a power of 2.
 */
FACache::FACache(unsigned int size, unsigned int lineSize) : Cache::Cache(size, lineSize, size/lineSize) {
	bool validArgs = true;

	// size é múltiplo de lineSize?
	validArgs = validArgs && associativity*lineSize == size;
	// size, lineSize e associativity são potências de 2?
	validArgs = validArgs && potencia2(size) && potencia2(lineSize) && potencia2(associativity);

	if (!validArgs) {
		throw "Bad FACache initialization. Invalid arguments.";
	}

	// se lineSize = 2^n, subtrair 1 faz com que se torne numa máscara para
	// selecionar os n primeiros bits do address, que é o offset.
	offsetMask = lineSize - 1;

	numSets = 1; // fully associative cache does not have sets
	
	data = new char[size];
	directory = new uint64_t[associativity];
}

FACache::~FACache() {
	delete[] data;
	delete[] directory;
}

/**
 * Reads the 32 bit value 'value' in address 'address'.
 * 
 * Returns
 * 		true, if cache hit
 * 		false, if cache miss
 */
bool FACache::read32(uint64_t address, uint32_t * value) {
	uint64_t tag, offset;
	unsigned int i;

	splitAddress(address, tag, offset);
	if (!findTag(tag, i)) {
		return false;
	} else {
		*value = *((uint32_t *) &data[i*lineSize + offset]);
		return true;
	}
}

/**
 * Reads the 64 bit value 'value' in address 'address'.
 * 
 * Returns
 * 		true, if cache hit
 * 		false, if cache miss
 */
bool FACache::read64(uint64_t address, uint64_t * value) {
	uint64_t tag, offset;
	unsigned int i;

	splitAddress(address, tag, offset);
	if (!findTag(tag, i)) {
		return false;
	} else {
		*value = *((uint64_t *) &data[i*lineSize + offset]);
		return true;
	}
}

/**
 * Overwrites the 32 bit value 'value' in address 'address'.
 * 
 * Returns
 * 		true, if cache hit and writing is successful
 * 		false, if cache miss
 */
bool FACache::write32(uint64_t address, uint32_t value) {
	uint64_t tag, offset;
	unsigned int i;

	splitAddress(address, tag, offset);
	if (!findTag(tag, i)) {
		return false;
	} else {
		*((uint32_t *) &data[i*lineSize + offset]) = value;
		setDirty(i);
		return true;
	}
}

/**
 * Overwrites the 64 bit value 'value' in address 'address'.
 * 
 * Returns
 * 		true, if cache hit and writing is successful
 * 		false, if cache miss
 */
bool FACache::write64(uint64_t address, uint64_t value) {
	uint64_t tag, offset;
	unsigned int i;

	splitAddress(address, tag, offset);
	if (!findTag(tag, i)) {
		return false;
	} else {
		*((uint64_t *) &data[i*lineSize + offset]) = value;
		setDirty(i);
		return true;
	}
}

/**
 * Fetches one line from slower memory and writes to this cache.
 * 
 * The bytes written are the bytes of the line that contains the byte in address
 * 'address'. The total number of bytes copied is exactly 'Cache::lineSize'.
 * 
 * The argument 'data' is a pointer to the whole data of the slower memory from
 * where the data is to be fetched.
 * 
 * Returns:
 * 		NULL, if the line is not set as modified
 * 		a pointer to a copy of the line, if the line is set as modified
 */
char * FACache::fetchLine(uint64_t address, char * data, uint64_t &oldLine) {
	char *removedLine = NULL;
	unsigned int localWriteIndex = writeIndex;

	uint64_t tag, offset;
	unsigned int i;

	splitAddress(address, tag, offset);
	if (findTag(tag, i)) {
		localWriteIndex = i;
	}

	if (isDirty(localWriteIndex)) {
		oldLine = directory[localWriteIndex] & ~1;
		removedLine = new char[lineSize];
		memcpy(removedLine, &this->data[localWriteIndex*lineSize], lineSize);
	}

	memcpy(&this->data[localWriteIndex*lineSize], &data[tag], lineSize);

	// atualiza o �ndice para o pr�ximo fetch (estrat�gia FIFO para escolha da linha)
	writeIndex = (writeIndex + 1) % associativity;
	return removedLine;
}
