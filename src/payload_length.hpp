#pragma once

template<class super>
class payload_length : public super
{
    typedef uint16_t len_type;
    static constexpr size_t len_size = sizeof(len_type);

    len_type *len_pointer(uint8_t *ptr)
    {
        return reinterpret_cast<len_type *>(ptr);
    }

    void set_len(uint8_t *ptr, size_t len)
    {
        *len_pointer(ptr) = len;
    }

    len_type get_len(uint8_t *ptr)
    {
        return *len_pointer(ptr);
    }

  protected:
    static size_t header_len()
    {
        return len_size + super::header_len();
    }

    size_t frame_len() const
    {
        return len_size + super::frame_len();
    }

    size_t payload_data_len()
    {
        return super::payload_data_len() - len_size;
    }

    void payload_data_add(uint8_t *data, size_t len)
    {
        set_len(data - len_size, len);
        super::payload_data_add(data - len_size, len + len_size);
    }

    size_t payload_data_get(uint8_t *data)
    {
        super::payload_data_get(data - len_size);
        return get_len(data - len_size);
    }

    size_t payload_data_read(uint8_t *data)
    {
        return super::payload_data_read(data - len_size);
    }

    uint8_t *payload_data_write(uint8_t *data)
    {
        return super::payload_data_write(data - len_size);
    }

  public:
    payload_length(const struct arguments &args) : super(args)
    {

    }
};
