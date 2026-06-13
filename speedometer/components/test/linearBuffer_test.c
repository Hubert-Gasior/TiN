#include "unity.h"
#include <string.h>
#include "linearBuffer.h"

typedef float float32;
typedef uint8_t uint8;

void setUp(void)
{
    ;
}

void tearDown(void)
{
    ;
}

TEST_CASE("LinearBufferInit correctly assigns pointers and zeros memory", "[linearBuffer]")
{
    uint8 rawBuffer[10];
    
    memset(rawBuffer, 0xFF, sizeof(rawBuffer));
    
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    TEST_ASSERT_EQUAL_PTR(rawBuffer, buf.beginning);
    TEST_ASSERT_EQUAL_PTR(rawBuffer, buf.write);
    TEST_ASSERT_EQUAL_PTR(rawBuffer + 10, buf.end);
    TEST_ASSERT_EQUAL_UINT32(10, buf.size);

    for (int i = 0; i < 10; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0, rawBuffer[i]);
    }
}

TEST_CASE("InsertFloat correctly formats the value, adds a semicolon, and shifts the pointer", "[linearBuffer]")
{
    uint8 rawBuffer[32];
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    bool result = InsertFloat(&buf, 3.14f);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("3.14;", (char *)buf.beginning);
    TEST_ASSERT_EQUAL_PTR(rawBuffer + 5, buf.write);
}

TEST_CASE("InsertFloat appends multiple values sequentially", "[linearBuffer]")
{
    uint8 rawBuffer[32];
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    TEST_ASSERT_TRUE(InsertFloat(&buf, 1.23f));
    TEST_ASSERT_TRUE(InsertFloat(&buf, -4.5f)); 
    TEST_ASSERT_TRUE(InsertFloat(&buf, 0.0f)); 

    TEST_ASSERT_EQUAL_STRING("1.23;-4.50;0.00;", (char *)buf.beginning);
}

TEST_CASE("InsertFloat returns false and protects against buffer overflow", "[linearBuffer]")
{
    uint8 rawBuffer[6]; 
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    bool resultFits = InsertFloat(&buf, 3.14f); 
    TEST_ASSERT_TRUE(resultFits);

    ClearLinearBuffer(&buf);
    
    bool resultOverflow = InsertFloat(&buf, 123.45f);
    TEST_ASSERT_FALSE(resultOverflow);
    TEST_ASSERT_EQUAL_PTR(rawBuffer, buf.write);
}

TEST_CASE("ClearLinearBuffer completely resets the buffer", "[linearBuffer]")
{
    uint8 rawBuffer[32];
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    InsertFloat(&buf, 99.99f);
    TEST_ASSERT_NOT_EQUAL(rawBuffer, buf.write); 
    TEST_ASSERT_EQUAL_UINT8('9', rawBuffer[0]); 

    ClearLinearBuffer(&buf);
    TEST_ASSERT_EQUAL_PTR(rawBuffer, buf.write);
    TEST_ASSERT_EQUAL_UINT8(0, rawBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0, rawBuffer[4]); 
}

TEST_CASE("Reusing the buffer after clearing (ClearLinearBuffer)", "[linearBuffer]")
{
    uint8 rawBuffer[32];
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    InsertFloat(&buf, 10.5f);
    InsertFloat(&buf, 20.1f);
    uint8* pointerAfterWrite = buf.write;
    TEST_ASSERT_NOT_EQUAL(buf.beginning, pointerAfterWrite);
    TEST_ASSERT_TRUE(pointerAfterWrite > buf.beginning);

    ClearLinearBuffer(&buf);
    TEST_ASSERT_EQUAL_PTR(buf.beginning, buf.write);
    for(uint32 i = 0; i < buf.size; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0, rawBuffer[i]);
    }

    bool result = InsertFloat(&buf, 3.33f);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("3.33;", (char *)buf.beginning);
}

TEST_CASE("Memory boundary protection after multiple buffer clears", "[linearBuffer]")
{
    uint8 rawBuffer[10];
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    for (int i = 0; i < 3; i++)
    {
        TEST_ASSERT_TRUE(InsertFloat(&buf, 9.99f));
        TEST_ASSERT_FALSE(InsertFloat(&buf, -8.88f)); 
        
        ClearLinearBuffer(&buf);
    }
}