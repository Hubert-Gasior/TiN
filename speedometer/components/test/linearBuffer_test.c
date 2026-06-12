#include "unity.h"
#include <string.h>
#include "linearBuffer.h" // Twój plik nagłówkowy

// Zakładam standardowe typy ESP-IDF, jeśli masz własne typedefy w nagłówku, 
// kompilator bez problemu je zinterpretuje.
typedef float float32;
typedef uint8_t uint8;

// --- Konfiguracja środowiska testowego ---

void setUp(void)
{
    // Nie potrzebujemy globalnej konfiguracji przed każdym testem,
    // inicjalizację buforów będziemy robić lokalnie wewnątrz testów.
}

void tearDown(void)
{
}

// --- Przypadki testowe ---

TEST_CASE("LinearBufferInit poprawnie przypisuje wskazniki i zeruje pamiec", "[linearBuffer]")
{
    uint8 rawBuffer[10];
    
    // Wypełniamy bufor "śmieciami", aby upewnić się, że Init go wyzeruje
    memset(rawBuffer, 0xFF, sizeof(rawBuffer));
    
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    // Weryfikacja wskaźników i rozmiaru
    TEST_ASSERT_EQUAL_PTR(rawBuffer, buf.beginning);
    TEST_ASSERT_EQUAL_PTR(rawBuffer, buf.write);
    TEST_ASSERT_EQUAL_PTR(rawBuffer + 10, buf.end);
    TEST_ASSERT_EQUAL_UINT32(10, buf.size);

    // Weryfikacja wyzerowania pamięci
    for (int i = 0; i < 10; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0, rawBuffer[i]);
    }
}

TEST_CASE("InsertFloat poprawnie formatuje wartosc, dodaje srednik i przesuwa wskaznik", "[linearBuffer]")
{
    uint8 rawBuffer[32];
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    // Akcja
    bool result = InsertFloat(&buf, 3.14f);

    // Weryfikacja
    TEST_ASSERT_TRUE(result);
    // 3.14 zajmuje 4 bajty + 1 bajt na średnik = 5 bajtów. Pamięć na końcu stringa w C to '\0',
    // ale Unity sprawdzi ciąg aż do wystąpienia nulla. Zwróć uwagę, że Twój kod nie dodaje
    // znaku '\0' po średniku, ale wyzerowana pamięć na starcie robi to za niego.
    TEST_ASSERT_EQUAL_STRING("3.14;", (char *)buf.beginning);
    
    // Wskaźnik write powinien przesunąć się o 5 bajtów
    TEST_ASSERT_EQUAL_PTR(rawBuffer + 5, buf.write);
}

TEST_CASE("InsertFloat dodaje wiele wartosci jedna po drugiej", "[linearBuffer]")
{
    uint8 rawBuffer[32];
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    // Akcja
    TEST_ASSERT_TRUE(InsertFloat(&buf, 1.23f));
    TEST_ASSERT_TRUE(InsertFloat(&buf, -4.5f)); // -4.50 (bo %.2f dopisuje zero)
    TEST_ASSERT_TRUE(InsertFloat(&buf, 0.0f));  // 0.00

    // Weryfikacja całego ciągu
    TEST_ASSERT_EQUAL_STRING("1.23;-4.50;0.00;", (char *)buf.beginning);
}

TEST_CASE("InsertFloat zwraca false i chroni przed zapelnieniem bufora", "[linearBuffer]")
{
    // Tworzymy celowo bardzo mały bufor
    uint8 rawBuffer[6]; 
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    // "12.34" = 5 znaków + ';' (1 znak) = 6 znaków. 
    // Ponieważ (write + 5 + 1) > end: (rawBuffer + 6) > (rawBuffer + 6) zwraca FALSE,
    // to akurat dokładnie zmieści się w buforze wg Twojej logiki.
    bool resultFits = InsertFloat(&buf, 3.14f); 
    TEST_ASSERT_TRUE(resultFits);

    // Reset i próba wstawienia wartości, która przekroczy 6 bajtów
    ClearLinearBuffer(&buf);
    
    // "123.45" = 6 znaków + ';' = 7 znaków. Zdecydowanie nie zmieści się w 6 bajtach.
    bool resultOverflow = InsertFloat(&buf, 123.45f);

    // Oczekujemy false
    TEST_ASSERT_FALSE(resultOverflow);
    // Wskaźnik zapisu nie powinien ulec zmianie w razie błędu!
    TEST_ASSERT_EQUAL_PTR(rawBuffer, buf.write);
}

TEST_CASE("ClearLinearBuffer calkowicie resetuje bufor", "[linearBuffer]")
{
    uint8 rawBuffer[32];
    LinearBuffer buf;
    LinearBufferInit(&buf, rawBuffer, sizeof(rawBuffer));

    // Wypełniamy jakimiś danymi
    InsertFloat(&buf, 99.99f);
    TEST_ASSERT_NOT_EQUAL(rawBuffer, buf.write); // Wskaźnik drgnął
    TEST_ASSERT_EQUAL_UINT8('9', rawBuffer[0]);  // Pierwszy znak to '9'

    // Akcja
    ClearLinearBuffer(&buf);

    // Weryfikacja: wskaźnik zapisu powrócił na początek
    TEST_ASSERT_EQUAL_PTR(rawBuffer, buf.write);
    // Weryfikacja: pierwsza pozycja to 0 (NULL)
    TEST_ASSERT_EQUAL_UINT8(0, rawBuffer[0]);
    // Weryfikacja: pamięć po poprzednich danych ('9', '.', ';') została usunięta
    TEST_ASSERT_EQUAL_UINT8(0, rawBuffer[4]); 
}