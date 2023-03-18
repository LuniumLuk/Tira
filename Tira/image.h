//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef IMAGE_H
#define IMAGE_H

#include <utils.h>
#include <string>

namespace tira {

    struct Image {
        using dataType = unsigned char;
        dataType* data;
        constexpr int channel() const { return 3; }
        int width, height;

        Image(int w, int h);
        ~Image();

        void fill(colorf const& color);
        colorf color_at(int x, int y, bool flip = true);
        void set_pixel(int x, int y, colorf const& color, bool flip = true);
        void write_PNG(std::string const& path);

        /**
         * Rasterize a line using Bresenham algorithm.
         * - Input coordinates are in image space.
         */
        void draw_line(int2 const& v0, int2 const& v1, colorf const& color);
    };

    struct ImageFloat {
        using dataType = float;
        dataType* data;
        constexpr int channel() const { return 3; }
        int width, height;

        ImageFloat(int w, int h);
        ~ImageFloat();

        void fill(colorf const& color);
        colorf color_at(int x, int y, bool flip = true);
        void set_pixel(int x, int y, colorf const& color, bool flip = true);
        void increment_pixel(int x, int y, colorf const& color, bool flip = true);
    };

} // namespace tira

#endif
