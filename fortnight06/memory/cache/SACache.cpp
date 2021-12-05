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

#include "SACache.h"

#include <cstddef>
#include <cstdint>
#include <strings.h>

using namespace std;


/**
 * Constructs a SACache of 'size' bytes organized in sets of associativity 'associativity'
 * and line size of 'lineSize' bytes.
 * 
 * Constraints: 'size' must be equal to 'numSets*associativity*lineSize', and all the attributes
 * must be a power of 2.
 */
SACache::SACache(unsigned int size, unsigned int lineSize, unsigned int associativity) : Cache::Cache(size,lineSize,associativity) {
	bool validArgs = true;

	numSets = size / associativity / lineSize;

	validArgs = validArgs && numSets*associativity*lineSize == size;
	validArgs = validArgs && potencia2(size) && potencia2(numSets) && potencia2(lineSize) && potencia2(associativity);

	if (!validArgs) {
		throw "Bad FACache initialization. Invalid arguments.";
	}

	offsetMask = lineSize - 1;

	// ex: lineSize = 8 = 0b1000, ffs(lineSize) = 4.
	// ffs - 1 = 3 é o shift à direita necessário pra
	// mover o que vem depois de offset para o início de uma palavra
	lookupShift = ffs(lineSize) - 1;
	lookupMask = (numSets - 1) << lookupShift;

	sets = new FACache*[numSets];
	for (unsigned int i = 0; i < numSets; i++) {
		sets[i] = new FACache(size / numSets, lineSize);
	}
}

SACache::~SACache() {
	for (unsigned int i = 0; i < numSets; i++) {
		delete sets[i];
	}

	delete[] sets;
}

/**
 * Reads the 32 bit value 'value' in address 'address'.
 * 
 * Returns
 * 		true, if cache hit
 * 		false, if cache miss
 */
bool SACache::read32(uint64_t address, uint32_t * value) {
	uint64_t tag, lookup, offset;
	splitAddress(address, tag, lookup, offset);
	return sets[lookup]->read32(address, value);
}

void SACache::splitAddress(uint64_t address, uint64_t &tag, uint64_t &lookup, uint64_t &offset) {
	offset = address & offsetMask;
	lookup = (address & lookupMask) >> lookupShift;
	tag = address & ~(offsetMask | lookupMask);
}

/**
 * Reads the 64 bit value 'value' in address 'address'.
 * 
 * Returns
 * 		true, if cache hit
 * 		false, if cache miss
 */
bool SACache::read64(uint64_t address, uint64_t * value) {
	uint64_t tag, lookup, offset;
	splitAddress(address, tag, lookup, offset);
	return sets[lookup]->read64(address, value);
}

/**
 * Overwrites the 32 bit value 'value' in address 'address'.
 * 
 * Returns
 * 		true, if cache hit and writing is successful
 * 		false, if cache miss
 */
bool SACache::write32(uint64_t address, uint32_t value) {
	uint64_t tag, lookup, offset;
	splitAddress(address, tag, lookup, offset);
	return sets[lookup]->write32(address, value);
}

/**
 * Overwrites the 64 bit value 'value' in address 'address'.
 * 
 * Returns
 * 		true, if cache hit and writing is successful
 * 		false, if cache miss
 */
bool SACache::write64(uint64_t address, uint64_t value) {
	uint64_t tag, lookup, offset;
	splitAddress(address, tag, lookup, offset);
	return sets[lookup]->write64(address, value);
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
char * SACache::fetchLine(uint64_t address, char * data) {
	uint64_t tag, lookup, offset;
	splitAddress(address, tag, lookup, offset);
	return sets[lookup]->fetchLine(address, data);
}
