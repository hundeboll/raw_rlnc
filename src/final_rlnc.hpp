#pragma once

#include <cstdlib>

class final_coder
{
  protected:
    size_t frame_len() const
    {
        return 0;
    }

    static size_t header_len()
    {
        return 0;
    }

  public:
    final_coder(const struct arguments &args)
    {}
};
