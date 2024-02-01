/**
 * Created by Andrea Fiorito on 27/11/2020.
 * Copyright (c) 2020, BLOOM ENGINEERING LTD. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
**/

#ifndef TRIPLEBUFFER_H
#define TRIPLEBUFFER_H

#include <cstdint>
#include <atomic>

/**
 * Lib version:
 * 0.0.2 added management of move only objects
 */
static constexpr char VERSION[] = "0.0.2";

#define FORCEINLINE inline __attribute__ ((always_inline))

enum NoInit {
    NO_INIT
};

/**
 * Template for triple buffers.
 *
 * This template implements a lock-free triple buffer that can be used to exchange data
 * between two threads that are producing and consuming at different rates. Instead of
 * atomically exchanging pointers to the buffers, we atomically update a Flags register
 * that holds the indices into a 3-element buffer array.
 *
 * The three buffers are named as follows:
 * - Read buffer: This is where Read() will read the latest value from
 * - Write buffer: This is where Write() will write a new value to
 * - Temp buffer: This is the second back-buffer currently not used for reading or writing
 *
 * Please note that reading and writing to the buffer does not automatically swap the
 * back-buffers. Instead, two separate methods, SwapReadBuffers() and SwapWriteBuffers()
 * are provided. For convenience, we also provide SwapAndRead() and WriteAndSwap() to
 * update and swap the buffers using a single method call.
 *
 * A dirty flag indicates whether a new value has been written and swapped into the second
 * back-buffer and is available for reading. It can be checked using the IsDirtyMethod().
 * As an optimization, SwapReadBuffers() and SwapAndRead() will not perform a back-buffer
 * swap if no new data is available.
 *
 * This class is thread-safe in single-producer, single-consumer scenarios.
 *
 * Based on ideas and C code in "Triple Buffering as a Concurrency Mechanism" (Reddit.com)
 *
 * @param BufferType The type of buffered items.
 */
template<typename BufferType>
class TripleBuffer
{
    /** Enumerates human readable bit values for accessing the Flags field. */
    enum EBufferFlag
    {
        /** Indicates whether a new buffer is available for reading. */
        Dirty = 0x40,

        /** Initial flags value (0dttwwrr; dirty = false, temp index = 0, write index = 1, read index = 2) */
        Initial = 0x06,

        /** Bit mask for accessing the read buffer index (bit 0-1). */
        ReaderMask = 0x03,

        /** Bit mask for the index of the unused/clean/empty buffer (bit 4-5). */
        TempMask = 0x30,

        /** Bit shift for accessing the temp buffer index. */
        TempShift = 4,

        /** Bit mask for accessing the write buffer index (bit 2-3). */
        WriterMask = 0x0c,

        /** Bit shift for accessing the write buffer index. */
        WriterShift = 2,
    };

public:

    /** Default constructor. */
    TripleBuffer()
    {
        Initialize();
        Buffers[0] = Buffers[1] = Buffers[2] = BufferType();
    }

    /** Default constructor (no initialization). */
    explicit TripleBuffer(NoInit)
    {
        Initialize();
    }

    /**
     * Create and initialize a new instance with a given buffer value.
     *
     * @param InValue The initial value of all three buffers.
     */
    explicit TripleBuffer(const BufferType& InValue)
    {
        Initialize();
        Buffers[0] = Buffers[1] = Buffers[2] = InValue;
    }

    /**
     * Create and initialize a new instance using provided buffers.
     *
     * The elements of the provided items array are expected to have
     * the following initial contents:
     *     0 = Temp
     *     1 = Write
     *     2 = Read
     *
     * @param InBuffers The buffer memory to use.
     */
    TripleBuffer(BufferType (&InBuffers)[3])
    {
        Buffers = &InBuffers[0];
        Flags = EBufferFlag::Initial | EBufferFlag::Dirty;
        OwnsMemory = false;
    }

    /** Destructor. */
    ~TripleBuffer()
    {
        if (OwnsMemory)
        {
            delete[] Buffers;
        }
    }

public:

    /**
     * Check whether a new value is available for reading.
     *
     * @return true if a new value is available, false otherwise.
     */
    bool IsDirty() const
    {
        return ((Flags & EBufferFlag::Dirty) != 0);
    }

    /**
     * Read a value from the current read buffer.
     *
     * @return Reference to the read buffer's value.
     * @see SwapRead, Write
     */
    BufferType& Read()
    {
        return Buffers[Flags & EBufferFlag::ReaderMask];
    }

    /**
     * Swap the latest read buffer, if available.
     *
     * Will not perform a back-buffer swap if no new data is available (dirty flag = false).
     *
     * @see SwapWrite, Read
     */
    void SwapReadBuffers()
    {
        if (!IsDirty())
        {
            return;
        }

        int32_t CurrentFlags;
        int32_t NewFlags;

        do
        {
            CurrentFlags = Flags;
            NewFlags = SwapReadWithTempFlags(CurrentFlags);
        }
        while (!Flags.compare_exchange_weak(CurrentFlags, NewFlags));
    }

public:

    /**
     * Get the current write buffer.
     *
     * @return Reference to write buffer.
     */
    BufferType& GetWriteBuffer()
    {
        return Buffers[(Flags & EBufferFlag::WriterMask) >> EBufferFlag::WriterShift];
    }

    /**
     * Swap a new write buffer (makes current write buffer available for reading).
     *
     * @see SwapRead, Write
     */
    void SwapWriteBuffers()
    {
        int32_t CurrentFlags;
        int32_t NewFlags;

        do
        {
            CurrentFlags = Flags;
            NewFlags = SwapWriteWithTempFlags(CurrentFlags);
        }
        while (!Flags.compare_exchange_weak(CurrentFlags, NewFlags));
    }

    /**
     * Write a value to the current write buffer.
     *
     * @param Value The value to write.
     * @see SwapWrite, Read
     */
    void Write(const BufferType &Value)
    {
        Buffers[(Flags & EBufferFlag::WriterMask) >> EBufferFlag::WriterShift] = Value;
    }

    /**
     * Moves a value to the current write buffer.
     *
     * @param Value The value to write.
     * @see SwapWrite, Read
     */
    void Write(BufferType &&Value)
    {
        Buffers[(Flags & EBufferFlag::WriterMask) >> EBufferFlag::WriterShift] = std::move(Value);
    }

public:

    /** Reset the buffer. */
    void Reset()
    {
        Flags = EBufferFlag::Initial;
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    /**
     * Convenience method for fetching and reading the latest buffer.
     *
     * @return Reference to the buffer.
     * @see SwapRead, Read, WriteAndSwap
     */
    const BufferType& SwapAndRead()
    {
        SwapReadBuffers();
        return Read();
    }

    /**
     * Convenience method for writing the latest buffer and fetching a new one.
     *
     * @param Value The value to write into the buffer.
     * @see SwapAndRead, SwapWrite, Write
     */
    void WriteAndSwap(const BufferType &Value)
    {
        Write(Value);
        SwapWriteBuffers();
    }

    /**
     * Convenience method for writing as move the latest buffer and fetching a new one.
     *
     * @param Value The value to write into the buffer.
     * @see SwapAndRead, SwapWrite, Write
     */
    void WriteAndSwap(BufferType &&Value)
    {
        Write(std::move(Value));
        SwapWriteBuffers();
    }
protected:

    /** Initialize the triple buffer. */
    void Initialize()
    {
        Buffers = new BufferType[3];
        Flags = EBufferFlag::Initial;
        OwnsMemory = true;
    }

private:

    /** Swaps the read and temp buffer indices in the Flags field. */
    static FORCEINLINE int32_t SwapReadWithTempFlags(int32_t Flags)
    {
        return ((Flags & EBufferFlag::ReaderMask) << 4) | ((Flags & EBufferFlag::TempMask) >> 4) | (Flags & EBufferFlag::WriterMask);
    }

    /** Swaps the write and temp buffer indices in the Flags field, and sets the dirty bit. */
    static FORCEINLINE int32_t SwapWriteWithTempFlags(int32_t Flags)
    {
        return ((Flags & EBufferFlag::TempMask) >> 2) | ((Flags & EBufferFlag::WriterMask) << 2) | (Flags & EBufferFlag::ReaderMask) | EBufferFlag::Dirty;
    }

private:

    /** Hidden copy constructor (triple buffers cannot be copied). */
    TripleBuffer(const TripleBuffer&);

    /** Hidden copy assignment (triple buffers cannot be copied). */
    TripleBuffer& operator=(const TripleBuffer&);

private:

    /** The three buffers. */
    BufferType* Buffers;

    /** Buffer access flags. */
    std::atomic_int32_t Flags;

    /** Whether this instance owns the buffer memory. */
    bool OwnsMemory;
};

#endif //TRIPLEBUFFER_H
