/*
 * Carla Ring Buffer
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_RING_BUFFER_HPP_INCLUDED
#define CARLA_RING_BUFFER_HPP_INCLUDED

#include "CarlaMathUtils.hpp"

// -----------------------------------------------------------------------
// Buffer structs

/*
   head:
    current writing position, headmost position of the buffer.
    increments when writing.

   tail:
    current reading position, last used position of the buffer.
    increments when reading.
    head == tail means empty buffer

   wrtn:
    temporary position of head until a commitWrite() is called.
    if buffer writing fails, wrtn will be back to head position thus ignoring the last operation(s).
    if buffer writing succeeds, head will be set to this variable.

   invalidateCommit:
    boolean used to check if a write operation failed.
    this ensures we don't get incomplete writes.
  */

struct HeapBuffer {
    uint32_t    size;
    std::size_t head, tail, wrtn;
    bool        invalidateCommit;
    uint8_t*    buf;

    void copyDataFrom(const HeapBuffer& rb) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(size == rb.size,);

        size = rb.size;
        head = rb.head;
        tail = rb.tail;
        wrtn = rb.wrtn;
        invalidateCommit = rb.invalidateCommit;
        std::memcpy(buf, rb.buf, size);
    }
};

struct StackBuffer {
    static const uint32_t size = 4096;
    std::size_t head, tail, wrtn;
    bool        invalidateCommit;
    uint8_t     buf[size];
};

// -----------------------------------------------------------------------
// CarlaRingBuffer templated class

template <class BufferStruct>
class CarlaRingBuffer
{
public:
    CarlaRingBuffer() noexcept
        : fBuffer(nullptr) {}

    CarlaRingBuffer(BufferStruct* const ringBuf) noexcept
        : fBuffer(ringBuf)
    {
        if (ringBuf != nullptr)
            clear();
    }

    void clear() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        fBuffer->head = 0;
        fBuffer->tail = 0;
        fBuffer->wrtn = 0;
        fBuffer->invalidateCommit = false;

        carla_zeroBytes(fBuffer->buf, fBuffer->size);
    }

    // -------------------------------------------------------------------

    bool commitWrite() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);

        if (fBuffer->invalidateCommit)
        {
            fBuffer->wrtn = fBuffer->head;
            fBuffer->invalidateCommit = false;
            return false;
        }

        // nothing to commit?
        CARLA_SAFE_ASSERT_RETURN(fBuffer->head != fBuffer->wrtn, false);

        // all ok
        fBuffer->head = fBuffer->wrtn;
        return true;
    }

    bool isDataAvailableForReading() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);

        return (fBuffer->head != fBuffer->tail);
    }

    bool isEmpty() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);

        return (fBuffer->head == fBuffer->tail);
    }

    // -------------------------------------------------------------------

    bool readBool() noexcept
    {
        bool b = false;
        return tryRead(&b, sizeof(bool)) ? b : false;
    }

    int8_t readByte() noexcept
    {
        int8_t b = 0;
        return tryRead(&b, sizeof(int8_t)) ? b : 0;
    }

    uint8_t readUByte() noexcept
    {
        int8_t ub = -1;
        return (tryRead(&ub, sizeof(int8_t)) && ub >= 0 && ub <= INT_LEAST8_MAX) ? static_cast<uint8_t>(ub) : 0;
    }

    int16_t readShort() noexcept
    {
        int16_t s = 0;
        return tryRead(&s, sizeof(int16_t)) ? s : 0;
    }

    uint16_t readUShort() noexcept
    {
        int16_t us = -1;
        return (tryRead(&us, sizeof(int16_t)) && us >= 0 && us <= INT_LEAST16_MAX) ? static_cast<uint16_t>(us) : 0;
    }

    int32_t readInt() noexcept
    {
        int32_t i = 0;
        return tryRead(&i, sizeof(int32_t)) ? i : 0;
    }

    uint32_t readUInt() noexcept
    {
        int32_t ui = -1;
        return (tryRead(&ui, sizeof(int32_t)) && ui >= 0 && ui <= INT_LEAST32_MAX) ? static_cast<uint32_t>(ui) : 0;
    }

    int64_t readLong() noexcept
    {
        int64_t l = 0;
        return tryRead(&l, sizeof(int64_t)) ? l : 0;
    }

    uint64_t readULong() noexcept
    {
        int64_t ul = -1;
        return (tryRead(&ul, sizeof(int64_t)) && ul >= 0 && ul <= INT_LEAST64_MAX) ? static_cast<uint64_t>(ul) : 0;
    }

    float readFloat() noexcept
    {
        float f = 0.0f;
        return tryRead(&f, sizeof(float)) ? f : 0.0f;
    }

    double readDouble() noexcept
    {
        double d = 0.0;
        return tryRead(&d, sizeof(double)) ? d : 0.0;
    }

    void readCustomData(void* const data, const std::size_t size) noexcept
    {
        if (! tryRead(data, size))
            carla_zeroBytes(data, size);
    }

    template <typename T>
    void readCustomType(T& type) noexcept
    {
        if (! tryRead(&type, sizeof(T)))
            carla_zeroStruct(type);
    }

    // -------------------------------------------------------------------

    bool writeBool(const bool value) noexcept
    {
        return tryWrite(&value, sizeof(bool));
    }

    bool writeByte(const int8_t value) noexcept
    {
        return tryWrite(&value, sizeof(int8_t));
    }

    bool writeShort(const int16_t value) noexcept
    {
        return tryWrite(&value, sizeof(int16_t));
    }

    bool writeInt(const int32_t value) noexcept
    {
        return tryWrite(&value, sizeof(int32_t));
    }

    bool writeLong(const int64_t value) noexcept
    {
        return tryWrite(&value, sizeof(int64_t));
    }

    bool writeFloat(const float value) noexcept
    {
        return tryWrite(&value, sizeof(float));
    }

    bool writeDouble(const double value) noexcept
    {
        return tryWrite(&value, sizeof(double));
    }

    bool writeCustomData(const void* const value, const std::size_t size) noexcept
    {
        return tryWrite(value, size);
    }

    template <typename T>
    bool writeCustomType(const T& value) noexcept
    {
        return tryWrite(&value, sizeof(T));
    }

    // -------------------------------------------------------------------

protected:
    void setRingBuffer(BufferStruct* const ringBuf, const bool reset) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(ringBuf != fBuffer,);

        fBuffer = ringBuf;

        if (reset && ringBuf != nullptr)
            clear();
    }

    // -------------------------------------------------------------------

private:
    BufferStruct* fBuffer;

    bool tryRead(void* const buf, const std::size_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(size > 0, false);
        CARLA_SAFE_ASSERT_RETURN(size < fBuffer->size, false);

        // empty
        if (fBuffer->head == fBuffer->tail)
            return false;

        uint8_t* const bytebuf(static_cast<uint8_t*>(buf));

        const std::size_t head(fBuffer->head);
        const std::size_t tail(fBuffer->tail);
        const std::size_t wrap((head > tail) ? 0 : fBuffer->size);

        if (size > wrap + head - tail)
        {
            carla_stderr2("CarlaRingBuffer::tryRead(%p, " P_SIZE "): failed, not enough space", buf, size);
            return false;
        }

        std::size_t readto(tail + size);

        if (readto > fBuffer->size)
        {
            readto -= fBuffer->size;
            const std::size_t firstpart(fBuffer->size - tail);
            std::memcpy(bytebuf, fBuffer->buf + tail, firstpart);
            std::memcpy(bytebuf + firstpart, fBuffer->buf, readto);
        }
        else
        {
            std::memcpy(bytebuf, fBuffer->buf + tail, size);

            if (readto == fBuffer->size)
                readto = 0;
        }

        fBuffer->tail = readto;
        return true;
    }

    bool tryWrite(const void* const buf, const std::size_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(size > 0, false);
        CARLA_SAFE_ASSERT_RETURN(size < fBuffer->size, false);

        const uint8_t* const bytebuf(static_cast<const uint8_t*>(buf));

        const std::size_t tail(fBuffer->tail);
        const std::size_t wrtn(fBuffer->wrtn);
        const std::size_t wrap((tail > wrtn) ? 0 : fBuffer->size);

        if (size >= wrap + tail - wrtn)
        {
            carla_stderr2("CarlaRingBuffer::tryWrite(%p, " P_SIZE "): failed, not enough space", buf, size);
            fBuffer->invalidateCommit = true;
            return false;
        }

        std::size_t writeto(wrtn + size);

        if (writeto > fBuffer->size)
        {
            writeto -= fBuffer->size;
            const std::size_t firstpart(fBuffer->size - wrtn);
            std::memcpy(fBuffer->buf + wrtn, bytebuf, firstpart);
            std::memcpy(fBuffer->buf, bytebuf + firstpart, writeto);
        }
        else
        {
            std::memcpy(fBuffer->buf + wrtn, bytebuf, size);

            if (writeto == fBuffer->size)
                writeto = 0;
        }

        fBuffer->wrtn = writeto;
        return true;
    }

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaRingBuffer)
};

// -----------------------------------------------------------------------
// CarlaRingBuffer using heap space

class CarlaHeapRingBuffer : public CarlaRingBuffer<HeapBuffer>
{
public:
    CarlaHeapRingBuffer() noexcept
        : CarlaRingBuffer<HeapBuffer>() {}

    ~CarlaHeapRingBuffer() noexcept
    {
        if (fHeapBuffer.buf == nullptr)
            return;

        delete[] fHeapBuffer.buf;
        fHeapBuffer.buf = nullptr;
    }

    void createBuffer(const uint32_t size)
    {
        CARLA_SAFE_ASSERT_RETURN(size > 0,);

        fHeapBuffer.size = carla_nextPowerOf2(size);
        fHeapBuffer.buf  = new uint8_t[fHeapBuffer.size];

        setRingBuffer(&fHeapBuffer, true);
    }

    void deleteBuffer() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fHeapBuffer.buf != nullptr,);

        setRingBuffer(nullptr, false);

        delete[] fHeapBuffer.buf;
        fHeapBuffer.buf  = nullptr;
        fHeapBuffer.size = 0;
    }

private:
    HeapBuffer fHeapBuffer;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaHeapRingBuffer)
};

// -----------------------------------------------------------------------
// CarlaRingBuffer using stack space

class CarlaStackRingBuffer : public CarlaRingBuffer<StackBuffer>
{
public:
    CarlaStackRingBuffer() noexcept
        : CarlaRingBuffer<StackBuffer>(&fStackBuffer) {}

private:
    StackBuffer fStackBuffer;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaStackRingBuffer)
};

// -----------------------------------------------------------------------

#endif // CARLA_RING_BUFFER_HPP_INCLUDED
