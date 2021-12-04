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

#include "Corei7Memory.h"
#include <cstdint>
#include <cstring>

Corei7Memory::Corei7Memory(int size) {
	logger = new MemoryLogger("cacheLog.txt");
	initHierarchy();
	mainMemory = new BasicMemory(size);
}

Corei7Memory::~Corei7Memory() {
	delete[] mainMemory;

	delete l1i;
	delete l1d;
	delete l2;
}

/**
 * Inicializa a hierarquia de mem�ria. O ideal seria que este procedimento utilizasse um arquivo de configura��o,
 * que descreveria a organiza��o de toda a hierarquia de mem�ria. Mas, para facilitar, vamos fazer isso hard-coded,
 * ou seja, no c�digo mesmo!
 */
void Corei7Memory::initHierarchy() {
	// TODO
	// A instancia��o da mem�ria principal j� est� implementada no construtor.
	// 1. Instanciar as caches, conforme hierarquia definida no enunciado do trabalho:
	// 		l1i: cache de 2KB, 4 vias, linhas de 64B (escrita n�o se aplica)
	// 		l1d: cache de 2KB, 8 vias, linhas de 64B, estrat�gia write-through
	// 		l2: cache de 8KB, 8 vias, linhas de 64B, estrat�gia writeback

	l1i = new SACache(2*1024, 64, 2*1024/64/4);
	l1d = new SACache(2*1024, 64, 2*1024/64/8);
	l2 = new SACache(8*1024, 64, 8*1024/64/8);
}

/**
 * Retorna o ponteiro para o in�cio da mem�ria.
 */
char *Corei7Memory::getData() {
	return mainMemory->getData();
}

/**
 * L� uma instru��o de 32 bits considerando um endere�amento em bytes.
 *
 * BasicMemory.cpp implementa a arquitetura de Von Neumman, com apenas uma
 * mem�ria, que armazena instru��es e dados.
 */
uint32_t Corei7Memory::readInstruction32(uint64_t address)
{
	int hitLevel = 1;
	uint32_t value;

	if (l1i->read32(address, &value)) {
		hitLevel = 1;
	} else if (l2->read32(address, &value)) {
		hitLevel = 2;
	} else {
		value = mainMemory->readInstruction32(address);
		hitLevel = 3;
	}

	uint64_t oldLine;
	if (hitLevel > 1) {
		l1i->fetchLine(address, mainMemory->getData(), oldLine);
	}
	if (hitLevel > 2) {
		char *data;
		if ((data = l2->fetchLine(address, mainMemory->getData(), oldLine))) {
			memcpy(&mainMemory->getData()[address+oldLine], data, 64);
			delete[] data;
		}
	}

	// n�o mexa nas linhas a seguir, s�o necess�rias para a corre��o do trabalho
	logger->memlog(MemoryLogger::READI,address,hitLevel);
	return value;
}

/**
 * L� um dado de 32 bits considerando um endere�amento em bytes.
 */
uint32_t Corei7Memory::readData32(uint64_t address)
{
	int hitLevel = 1;
	uint32_t value;

	if (l1d->read32(address, &value)) {
		hitLevel = 1;
	} else if (l2->read32(address, &value)) {
		hitLevel = 2;
	} else {
		value = mainMemory->readData32(address);
		hitLevel = 3;
	}

	uint64_t oldLine;
	if (hitLevel > 1) {
		l1d->fetchLine(address, mainMemory->getData(), oldLine);
	}
	if (hitLevel > 2) {
		char *data;
		if ((data = l2->fetchLine(address, mainMemory->getData(), oldLine))) {
			memcpy(&mainMemory->getData()[address+oldLine], data, 64);
			delete[] data;
		}
	}
	
	// n�o mexa nas linhas a seguir, s�o necess�rias para a corre��o do trabalho
	logger->memlog(MemoryLogger::READ32,address,hitLevel);
	return value;
}

/**
 * L� um dado de 64 bits considerando um endere�amento em bytes.
 */
uint64_t Corei7Memory::readData64(uint64_t address)
{
	int hitLevel = 1;
	uint64_t value;
	
	if (l1d->read64(address, &value)) {
		return value;
	} else if (l2->read64(address, &value)) {
		hitLevel = 2;
	} else {
		value = mainMemory->readData64(address);
		hitLevel = 3;
	}

	uint64_t oldLine;
	if (hitLevel > 1) {
		l1d->fetchLine(address, mainMemory->getData(), oldLine);
	}
	if (hitLevel > 2) {
		char *data;
		if ((data = l2->fetchLine(address, mainMemory->getData(), oldLine))) {
			memcpy(&mainMemory->getData()[address+oldLine], data, 64);
			delete[] data;
		}
	}

	// n�o mexa nas linhas a seguir, s�o necess�rias para a corre��o do trabalho
	logger->memlog(MemoryLogger::READ64,address,hitLevel);
	return value;
}

/**
 * Escreve uma instru��o de 32 bits considerando um
 * endere�amento em bytes.
 */
void Corei7Memory::writeInstruction32(uint64_t address, uint32_t value)
{
	mainMemory->writeInstruction32(address,value);
}

/**
 * Escreve um dado (value) de 32 bits considerando um endere�amento em bytes.
 */
void Corei7Memory::writeData32(uint64_t address, uint32_t value)
{
	int hitLevel = 1;

	l1d->write32(address, value);

	if (l2->write32(address, value)) {
		hitLevel = 2;
	} else {
		hitLevel = 3;
	}

	uint64_t oldLine;
	if (hitLevel > 2) {
		char *data;
		if ((data = l2->fetchLine(address, mainMemory->getData(), oldLine))) {
			memcpy(&mainMemory->getData()[address+oldLine], data, 64);
			delete[] data;
		}

		l2->write32(address, value);
	}
	
	// n�o mexa nas linhas a seguir, s�o necess�rias para a corre��o do trabalho
	logger->memlog(MemoryLogger::WRITE32,address,hitLevel);
}

/**
 * Escreve um dado (value) de 64 bits considerando um endere�amento em bytes.
 */
void Corei7Memory::writeData64(uint64_t address, uint64_t value)
{
	int hitLevel = 1;

	l1d->write64(address, value);

	if (l2->write64(address, value)) {
		hitLevel = 2;
	} else {
		hitLevel = 3;
	}

	uint64_t oldLine;
	if (hitLevel > 2) {
		char *data;
		if ((data = l2->fetchLine(address, mainMemory->getData(), oldLine))) {
			memcpy(&mainMemory->getData()[address+oldLine], data, 64);
			delete[] data;
		}

		l2->write64(address, value);
	}

	// n�o mexa nas linhas a seguir, s�o necess�rias para a corre��o do trabalho
	logger->memlog(MemoryLogger::WRITE64,address,hitLevel);
}
