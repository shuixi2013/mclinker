/*****************************************************************************
 *   Test Suite of The MCLinker Project,                                     *
 *                                                                           *
 *   Copyright (C), 2011 -                                                   *
 *   Embedded and Web Computing Lab, National Taiwan University              *
 *   MediaTek, Inc.                                                          *
 *                                                                           *
 *   Luba Tang <lubatang@mediatek.com>                                       *
 ****************************************************************************/
#include <mcld/Support/Allocators.h>
#include "LinearAllocatorTest.h"
#ifdef MCLD_DEBUG
#include <iostream>

using namespace std;
#endif

using namespace mcld;
using namespace mcldtest;


// Constructor can do set-up work for all test here.
LinearAllocatorTest::LinearAllocatorTest()
{
	// create testee. modify it if need
	m_pTestee = new LinearAllocator<Data, CHUNK_SIZE>();
}

// Destructor can do clean-up work that doesn't throw exceptions here.
LinearAllocatorTest::~LinearAllocatorTest()
{
	delete m_pTestee;
}

// SetUp() will be called immediately before each test.
void LinearAllocatorTest::SetUp()
{
}

// TearDown() will be called immediately after each test.
void LinearAllocatorTest::TearDown()
{
}

//==========================================================================//
// Testcases
//
TEST_F( LinearAllocatorTest, allocateN ) {
	Data* pointer = m_pTestee->allocate(10);
	ASSERT_FALSE(0 == pointer);
	ASSERT_EQ(CHUNK_SIZE, m_pTestee->max_size());
	ASSERT_FALSE(m_pTestee->empty());
}

TEST_F( LinearAllocatorTest, allocate ) {
	Data* pointer = m_pTestee->allocate();
	ASSERT_FALSE(0 == pointer);
	ASSERT_EQ(CHUNK_SIZE, m_pTestee->max_size());
	ASSERT_FALSE(m_pTestee->empty());
}

TEST_F( LinearAllocatorTest, allocateOver ) {
	Data* pointer = m_pTestee->allocate(CHUNK_SIZE+1);
	ASSERT_TRUE(0 == pointer);
	ASSERT_EQ(0, m_pTestee->max_size());
	ASSERT_TRUE(m_pTestee->empty());
}

TEST_F( LinearAllocatorTest, alloc_construct ) {
	Data* pointer = m_pTestee->allocate();
	m_pTestee->construct(pointer);
	ASSERT_EQ(1, pointer->one);
	ASSERT_EQ(2, pointer->two);
	ASSERT_EQ(3, pointer->three);
	ASSERT_EQ(4, pointer->four);
}

TEST_F( LinearAllocatorTest, alloc_constructCopy ) {
	Data* pointer = m_pTestee->allocate();
	Data data(7, 7, 7, 7);
	m_pTestee->construct(pointer, data);
	
	ASSERT_EQ(7, pointer->one);
	ASSERT_EQ(7, pointer->two);
	ASSERT_EQ(7, pointer->three);
	ASSERT_EQ(7, pointer->four);
}

TEST_F( LinearAllocatorTest, allocN_construct ) {
	Data* pointer = m_pTestee->allocate(10);
	m_pTestee->construct(pointer);
	ASSERT_EQ(1, pointer->one);
	ASSERT_EQ(2, pointer->two);
	ASSERT_EQ(3, pointer->three);
	ASSERT_EQ(4, pointer->four);
}

TEST_F( LinearAllocatorTest, allocN_constructCopy ) {
	Data* pointer = m_pTestee->allocate(10);
	Data data(7, 7, 7, 7);
	m_pTestee->construct(pointer, data);
	
	ASSERT_EQ(7, pointer->one);
	ASSERT_EQ(7, pointer->two);
	ASSERT_EQ(7, pointer->three);
	ASSERT_EQ(7, pointer->four);
}

TEST_F( LinearAllocatorTest, multi_alloc_ctor_iterate ) {
	for (int i=0; i<101; ++i) {
		Data* pointer = m_pTestee->allocate();
		m_pTestee->construct(pointer);
		pointer->one = i;
	}
/**
	Alloc::iterator data, dEnd = m_pTestee->end();
	int counter = 0;
	for (data=m_pTestee->begin(); data!=dEnd; ++data) {
		ASSERT_EQ(counter, (*data).one);
		++counter;
	}
**/
}

TEST_F( LinearAllocatorTest, multi_allocN_ctor_iterate ) {
	int counter = 0;
	for (int i=0; i<10000; ++i) {
		Data* pointer = m_pTestee->allocate(10);
		for (int j=0; j<10; ++j) {
			m_pTestee->construct(pointer);
			pointer->one = counter;
			++pointer;
			++counter;
		}
	}
/**
	Alloc::iterator data, dEnd = m_pTestee->end();
	counter = 0;
	for (data=m_pTestee->begin(); data!=dEnd; ++data) {
		ASSERT_EQ(counter, (*data).one);
		++counter;
	}
**/
}

