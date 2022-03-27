//
// Created by Michal Němec on 28/01/2020.
//


#include "buffer.h"
#ifdef __linux__
#include <netinet/in.h> // htol
#elif defined(_WIN32)
#include <winsock.h>
#endif

// TODO XXX Little-Endian/Big-Endian problem.
#define ctlbase64(x)                          \
((((x) & 0xff00000000000000ull) >> 56)       \
| (((x) & 0x00ff000000000000ull) >> 40)     \
| (((x) & 0x0000ff0000000000ull) >> 24)     \
| (((x) & 0x000000ff00000000ull) >> 8)      \
| (((x) & 0x00000000ff000000ull) << 8)      \
| (((x) & 0x0000000000ff0000ull) << 24)     \
| (((x) & 0x000000000000ff00ull) << 40)     \
| (((x) & 0x00000000000000ffull) << 56))

using namespace mks;

const size_t mks::Buffer::kCheapPrependSize = 0;
const size_t mks::Buffer::kInitialSize = 0;
const char mks::Buffer::kCRLF[] = "\r\n";

Buffer::Buffer(size_t initial_size, size_t reserved_prepend_size)
: capacity_(reserved_prepend_size + initial_size)
, read_index_(reserved_prepend_size)
, write_index_(reserved_prepend_size)
, reserved_prepend_size_(reserved_prepend_size) {
    buffer_ = (capacity_ == 0) ? nullptr : new char[capacity_];
    assert(length() == 0);
    assert(WritableBytes() == initial_size);
    assert(PrependableBytes() == reserved_prepend_size);
}

Buffer::Buffer(const Buffer& rhs)
{
    if(rhs.buffer_ != nullptr && capacity_ > 0) {
        buffer_ = new char[rhs.capacity_];
        ::memcpy(buffer_, rhs.buffer_, rhs.capacity_);
    } else {
        buffer_ = nullptr;
    }
    capacity_ = rhs.capacity_;
    read_index_ = rhs.read_index_;
    write_index_ = rhs.write_index_;
    reserved_prepend_size_ = rhs.reserved_prepend_size_;
}

Buffer::Buffer(Buffer&& rhs) {
    size_t initial_size = 0;
    size_t reserved_prepend_size = 0;
    capacity_ = reserved_prepend_size + initial_size;
    read_index_ = reserved_prepend_size;
    write_index_ = reserved_prepend_size;
    reserved_prepend_size_ = reserved_prepend_size;
    buffer_ = (capacity_ == 0) ? nullptr : new char[capacity_];
    assert(length() == 0);
    assert(WritableBytes() == initial_size);
    assert(PrependableBytes() == reserved_prepend_size);
    Swap(rhs);
}

Buffer& Buffer::operator=(const Buffer& rhs) {
    if(rhs.buffer_ != nullptr && capacity_ > 0) {
        buffer_ = new char[rhs.capacity_];
        ::memcpy(buffer_, rhs.buffer_, rhs.capacity_);
    } else {
        buffer_ = nullptr;
    }
    capacity_ = rhs.capacity_;
    read_index_ = rhs.read_index_;
    write_index_ = rhs.write_index_;
    reserved_prepend_size_ = rhs.reserved_prepend_size_;
    return *this;
}

Buffer& Buffer::operator=(Buffer&& rhs) {
    size_t initial_size = 0;
    size_t reserved_prepend_size = 0;
    capacity_ = reserved_prepend_size + initial_size;
    read_index_ = reserved_prepend_size;
    write_index_ = reserved_prepend_size;
    reserved_prepend_size_ = reserved_prepend_size;
    buffer_ = (capacity_ == 0) ? nullptr : new char[capacity_];
    assert(length() == 0);
    assert(WritableBytes() == initial_size);
    assert(PrependableBytes() == reserved_prepend_size);
    Swap(rhs);
    return *this;
}

Buffer::~Buffer() {
    delete[] buffer_;
    buffer_ = nullptr;
    capacity_ = 0;
}

void
Buffer::Swap(Buffer& rhs)
{
    std::swap(buffer_, rhs.buffer_);
    std::swap(capacity_, rhs.capacity_);
    std::swap(read_index_, rhs.read_index_);
    std::swap(write_index_, rhs.write_index_);
    std::swap(reserved_prepend_size_, rhs.reserved_prepend_size_);
}

// Skip advances the reading index of the buffer
void
Buffer::Skip(size_t len)
{
    if (len < length()) {
        read_index_ += len;
    } else {
        Reset();
    }
}

void
Buffer::Retrieve(size_t len)
{
    Skip(len);
}

void
Buffer::Truncate(size_t n)
{
    if (n == 0) {
        read_index_ = reserved_prepend_size_;
        write_index_ = reserved_prepend_size_;
    } else if (write_index_ > read_index_ + n) {
        write_index_ = read_index_ + n;
    }
}

void
Buffer::Reset()
{
    Truncate(0);
}

void
Buffer::Reserve(size_t len)
{
    if (capacity_ >= len + reserved_prepend_size_) {
        return;
    }

    // TODO add the implementation logic here
    grow(len + reserved_prepend_size_);
}

void
Buffer::EnsureWritableBytes(size_t len)
{
    if (WritableBytes() < len) {
        grow(len);
    }

    assert(WritableBytes() >= len);
}

void
Buffer::ToText()
{
    AppendInt8('\0');
    UnwriteBytes(1);
}

void
Buffer::Write(const void* /*restrict*/ d, size_t len)
{
    EnsureWritableBytes(len);
    memcpy(WriteBegin(), d, len);
    assert(write_index_ + len <= capacity_);
    write_index_ += len;
}

void
Buffer::Append(const char* /*restrict*/ d, size_t len)
{
    Write(d, len);
}

void
Buffer::Append(const void* /*restrict*/ d, size_t len)
{
    Write(d, len);
}

void
Buffer::AppendInt64(int64_t x)
{
    int64_t be = ctlbase64(x);
    Write(&be, sizeof be);
}

void
Buffer::AppendInt32(int32_t x)
{
    int32_t be32 = htonl(x);
    Write(&be32, sizeof be32);
}

void
Buffer::AppendInt16(int16_t x)
{
    int16_t be16 = htons(x);
    Write(&be16, sizeof be16);
}

void
Buffer::AppendInt8(int8_t x)
{
    Write(&x, sizeof x);
}

void
Buffer::PrependInt64(int64_t x)
{
    int64_t be = ctlbase64(x);
    Prepend(&be, sizeof be);
}

void
Buffer::PrependInt32(int32_t x)
{
    int32_t be32 = htonl(x);
    Prepend(&be32, sizeof be32);
}

void
Buffer::PrependInt16(int16_t x)
{
    int16_t be16 = htons(x);
    Prepend(&be16, sizeof be16);
}

void
Buffer::PrependInt8(int8_t x)
{
    Prepend(&x, sizeof x);
}

void
Buffer::Prepend(const void* /*restrict*/ d, size_t len)
{
    assert(len <= PrependableBytes());
    read_index_ -= len;
    const char* p = static_cast<const char*>(d);
    memcpy(begin() + read_index_, p, len);
}

void
Buffer::UnwriteBytes(size_t n)
{
    assert(n <= length());
    write_index_ -= n;
}

void
Buffer::WriteBytes(size_t n)
{
    assert(n <= WritableBytes());
    write_index_ += n;
}

int64_t
Buffer::ReadInt64()
{
    int64_t result = PeekInt64();
    Skip(sizeof result);
    return result;
}

int32_t
Buffer::ReadInt32()
{
    int32_t result = PeekInt32();
    Skip(sizeof result);
    return result;
}

int16_t
Buffer::ReadInt16()
{
    int16_t result = PeekInt16();
    Skip(sizeof result);
    return result;
}

int8_t
Buffer::ReadInt8()
{
    int8_t result = PeekInt8();
    Skip(sizeof result);
    return result;
}

void
Buffer::Shrink(size_t reserve)
{
    Buffer other(length() + reserve);
    other.Append(data(), length());
    Swap(other);
}

// ReadByte reads and returns the next byte from the buffer.
// If no byte is available, it returns '\0'.
char
Buffer::ReadByte()
{
    assert(length() >= 1);

    if (length() == 0) {
        return '\0';
    }

    return buffer_[read_index_++];
}

void
Buffer::UnreadBytes(size_t n)
{
    assert(n < read_index_);
    read_index_ -= n;
}

int64_t
Buffer::PeekInt64() const
{
    assert(length() >= sizeof(int64_t));
    int64_t be64 = 0;
    ::memcpy(&be64, data(), sizeof be64);
    return ctlbase64(be64);
}

int32_t
Buffer::PeekInt32() const
{
    assert(length() >= sizeof(int32_t));
    int32_t be32 = 0;
    ::memcpy(&be32, data(), sizeof be32);
    return ntohl(be32);
}

int16_t
Buffer::PeekInt16() const
{
    assert(length() >= sizeof(int16_t));
    int16_t be16 = 0;
    ::memcpy(&be16, data(), sizeof be16);
    return ntohs(be16);
}

int8_t
Buffer::PeekInt8() const
{
    assert(length() >= sizeof(int8_t));
    int8_t x = *data();
    return x;
}

const char*
Buffer::data() const
{
    return buffer_ + read_index_;
}

char*
Buffer::WriteBegin()
{
    return begin() + write_index_;
}

const char*
Buffer::WriteBegin() const
{
    return begin() + write_index_;
}

// length returns the number of bytes of the unread portion of the buffer
size_t
Buffer::length() const
{
    assert(write_index_ >= read_index_);
    return write_index_ - read_index_;
}

// size returns the number of bytes of the unread portion of the buffer.
// It is the same as length().
size_t
Buffer::size() const
{
    return length();
}

// capacity returns the capacity of the buffer's underlying byte slice, that is, the
// total space allocated for the buffer's data.
size_t
Buffer::capacity() const
{
    return capacity_;
}

size_t
Buffer::WritableBytes() const
{
    assert(capacity_ >= write_index_);
    return capacity_ - write_index_;
}

size_t
Buffer::PrependableBytes() const
{
    return read_index_;
}

// Helpers
const char*
Buffer::FindCRLF() const
{
    const char* crlf = std::search(data(), WriteBegin(), kCRLF, kCRLF + 2);
    return crlf == WriteBegin() ? nullptr : crlf;
}

const char*
Buffer::FindCRLF(const char* start) const
{
    assert(data() <= start);
    assert(start <= WriteBegin());
    const char* crlf = std::search(start, WriteBegin(), kCRLF, kCRLF + 2);
    return crlf == WriteBegin() ? nullptr : crlf;
}

const char*
Buffer::FindEOL() const
{
    const void* eol = memchr(data(), '\n', length());
    return static_cast<const char*>(eol);
}

const char*
Buffer::FindEOL(const char* start) const
{
    assert(data() <= start);
    assert(start <= WriteBegin());
    const void* eol = memchr(start, '\n', WriteBegin() - start);
    return static_cast<const char*>(eol);
}

char*
Buffer::begin()
{
    return buffer_;
}

const char*
Buffer::begin() const
{
    return buffer_;
}

void
Buffer::grow(size_t len)
{
    if (WritableBytes() + PrependableBytes() < len + reserved_prepend_size_) {
        //grow the capacity
        size_t n = (capacity_ << 1) + len;
        size_t m = length();
        char* d = new char[n];
        memcpy(d + reserved_prepend_size_, begin() + read_index_, m);
        write_index_ = m + reserved_prepend_size_;
        read_index_ = reserved_prepend_size_;
        capacity_ = n;
        delete[] buffer_;
        buffer_ = d;
    } else {
        // move readable data to the front, make space inside buffer
        assert(reserved_prepend_size_ < read_index_);
        size_t readable = length();
        memmove(begin() + reserved_prepend_size_, begin() + read_index_, length());
        read_index_ = reserved_prepend_size_;
        write_index_ = read_index_ + readable;
        assert(readable == length());
        assert(WritableBytes() >= len);
    }
}