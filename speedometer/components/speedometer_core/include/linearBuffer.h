#ifndef PARSER_H_
#define PARSER_H_

#include "types.h"
#include "string.h"
#include "esp_log.h"

typedef struct 
{
    uint8 *beginning;
    uint8 *write;
    uint8 *end;
    uint32 size;
} LinearBuffer;

/*******************************************************
 * @brief   Initializes a LinearBuffer structure.
 * 
 * @details Clears the provided memory region by setting it to zero
 *          and sets up buffer pointers and total size to manage a linear buffer.
 * 
 * @param   linBuffer - A pointer to the LinearBuffer structure to be initailized
 * @param   buffer - A pointer to the allocated memory block to be used as data storage
 * @param   size - a uint32 value representing the total size of the buffer in bytes
 * 
 * @return  none
 *******************************************************/
void LinearBufferInit(LinearBuffer *linBuffer, uint8 *buffer, uint32 size);

/*******************************************************
 * @brief   Clears the content of the LinearBuffer.
 * 
 * @details Resets the entire memory region associated with the buffer to zero and moves
 *          the write pointer back to the beginning of the buffer.
 * 
 * @param   linBuffer - A pointer to the LinearBuffer structure to be cleared.
 * 
 * @return  none
 *******************************************************/
void ClearLinearBuffer(LinearBuffer *linBuffer);

/*******************************************************
 * @brief  Inserts a float value into the LinearBuffer as a string. 
 * 
 * @details Converts a float32 value to a string with two decimal places precision
 *          and appends it to the buffer followed by a semicolon separator.
 * 
 * @param   linBuffer - A pointer to the LinearBuffer structure where the data will be inserted.
 * @param   vallue - The float32 value to be inserted.
 * 
 * @return   True if the value was successgully inserted, false if there was insufficient space in the buffer.
 *******************************************************/
bool InsertFloat(LinearBuffer *linBuffer, float32 value);

#endif /* PARSER_H_ */