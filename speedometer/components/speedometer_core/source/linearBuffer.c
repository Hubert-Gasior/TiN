#include "linearBuffer.h"

#ifdef SP_LINEAR_BUFFER_DEBUG
static const char *LINEAR_BUFFER_TAG = "Linear Buffer";
#endif

void LinearBufferInit(LinearBuffer *linBuffer, uint8 *buffer, uint32 size)
{
    memset(buffer, 0, size);
    linBuffer->beginning = buffer;
    linBuffer->write = buffer;
    linBuffer->end = buffer + size;
    linBuffer->size = size;
}

void ClearLinearBuffer(LinearBuffer *linBuffer)
{
    memset(linBuffer->beginning, 0, linBuffer->size);
    linBuffer->write = linBuffer->beginning;
}

bool InsertFloat(LinearBuffer *linBuffer, float32 value)
{
    char tmpBuff[64];
    int16 bytesWritten = snprintf(tmpBuff, sizeof(tmpBuff), "%.2f", value);

    if ((linBuffer->write + bytesWritten + 1) > linBuffer->end)
    {
        #ifdef SP_LINEAR_BUFFER_DEBUG
            ESP_LOGE(LINEAR_BUFFER_TAG, "Warning: buffer is full unable to save the value");
        #endif
        return false;
    }

    memcpy(linBuffer->write, tmpBuff, bytesWritten);
    linBuffer->write += bytesWritten;
    *(linBuffer->write) = ';';
    linBuffer->write++;

    return true;
}