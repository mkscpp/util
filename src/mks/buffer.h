//
// Created by Michal Němec on 28/01/2020.
// Modified https://github.com/Qihoo360/evpp/blob/master/evpp/buffer.h
//

#ifndef MKS_BUFFER_H
#define MKS_BUFFER_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

namespace mks {

class Buffer {
    char *buffer_;
    size_t capacity_;
    size_t read_index_;
    size_t write_index_;
    size_t reserved_prepend_size_;
    static const char kCRLF[];

public:
    static const size_t kCheapPrependSize;
    static const size_t kInitialSize;

    explicit Buffer(size_t initial_size = kInitialSize, size_t reserved_prepend_size = kCheapPrependSize);
    Buffer(const Buffer &rhs);
    Buffer(Buffer&& rhs);

    Buffer& operator=(const Buffer& other);
    Buffer& operator=(Buffer&& other);

    ~Buffer();

    void Swap(Buffer &rhs);

    // Skip advances the reading index of the buffer
    void Skip(size_t len);

    // Retrieve advances the reading index of the buffer
    // Retrieve it the same as Skip.
    void Retrieve(size_t len);

    // Truncate discards all but the first n unread bytes from the buffer
    // but continues to use the same allocated storage.
    // It does nothing if n is greater than the length of the buffer.
    void Truncate(size_t n);

    // Reset resets the buffer to be empty,
    // but it retains the underlying storage for use by future writes.
    // Reset is the same as Truncate(0).
    void Reset();

    // Increase the capacity of the container to a value that's greater
    // or equal to len. If len is greater than the current capacity(),
    // new storage is allocated, otherwise the method does nothing.
    void Reserve(size_t len);

    // Make sure there is enough memory space to append more data with length len
    void EnsureWritableBytes(size_t len);

    // ToText appends char '\0' to buffer to convert the underlying data to a c-style string text.
    // It will not change the length of buffer.
    void ToText();

    // Write
    void Write(const void * /*restrict*/ d, size_t len);
    void Append(const char * /*restrict*/ d, size_t len);
    void Append(const void * /*restrict*/ d, size_t len);
    void Append(const std::string& str) {
        Append(str.c_str(), str.size());
    }

    // Append int64_t/int32_t/int16_t with network endian
    void AppendInt64(int64_t x);
    void AppendInt32(int32_t x);
    void AppendInt16(int16_t x);
    void AppendInt8(int8_t x);
    // Prepend int64_t/int32_t/int16_t with network endian
    void PrependInt64(int64_t x);
    void PrependInt32(int32_t x);
    void PrependInt16(int16_t x);
    void PrependInt8(int8_t x);
    // Insert content, specified by the parameter, into the front of reading index
    void Prepend(const void * /*restrict*/ d, size_t len);
    void UnwriteBytes(size_t n);
    void WriteBytes(size_t n);

    // Read
    // Peek int64_t/int32_t/int16_t/int8_t with network endian
    int64_t ReadInt64();
    int32_t ReadInt32();
    int16_t ReadInt16();
    int8_t ReadInt8();

    void Shrink(size_t reserve);
    // ReadByte reads and returns the next byte from the buffer.
    // If no byte is available, it returns '\0'.
    char ReadByte();
    // UnreadBytes unreads the last n bytes returned
    // by the most recent read operation.
    void UnreadBytes(size_t n);
    // Peek
    // Peek int64_t/int32_t/int16_t/int8_t with network endian
    int64_t PeekInt64() const;
    int32_t PeekInt32() const;
    int16_t PeekInt16() const;
    int8_t PeekInt8() const;

    // data returns a pointer of length Buffer.length() holding the unread portion of the buffer.
    // The data is valid for use only until the next buffer modification (that is,
    // only until the next call to a method like Read, Write, Reset, or Truncate).
    // The data aliases the buffer content at least until the next buffer modification,
    // so immediate changes to the slice will affect the result of future reads.
    const char *data() const;
    char *WriteBegin();
    const char *WriteBegin() const;

    // length returns the number of bytes of the unread portion of the buffer
    size_t length() const;
    // size returns the number of bytes of the unread portion of the buffer.
    // It is the same as length().
    size_t size() const;

    // capacity returns the capacity of the buffer's underlying byte slice, that is, the
    // total space allocated for the buffer's data.
    size_t capacity() const;
    size_t WritableBytes() const;
    size_t PrependableBytes() const;

    // Helpers
    const char *FindCRLF() const;
    const char *FindCRLF(const char *start) const;
    const char *FindEOL() const;
    const char *FindEOL(const char *start) const;

private:
    char *begin();
    const char *begin() const;
    void grow(size_t len);
};

} // namespace mks


#endif // MKS_BUFFER_H
