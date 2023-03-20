//
// Created by Ziyi.Lu 2022/12/05
//

#include <misc/image.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <thirdparty/stb_image_write.h>
#pragma GCC diagnostic pop
#include <misc/utils.h>
#include <memory>
#include <cstring>

namespace tira {

    Image::Image(int w, int h)
        : width(w)
        , height(h) {
        int size = w * h * channel();
        data = new dataType[size];
        memset(data, 0, size * sizeof(dataType));
    }

    Image::~Image() {
        delete data;
    }

    void Image::fill(colorf const& color) {
        int size = width * height;
        for (int i = 0; i < size; ++i) {
            int offset = i * channel();
            data[offset + 0] = clamp(color.r, 0.0f, 1.0f) * 255;
            data[offset + 1] = clamp(color.g, 0.0f, 1.0f) * 255;
            data[offset + 2] = clamp(color.b, 0.0f, 1.0f) * 255;
        }
    }

    colorf Image::color_at(int x, int y, bool flip) {
        x = clamp(x, 0, width - 1);
        y = clamp(y, 0, height - 1);
        int offset;
        if (flip) {
            offset = (x + (height - 1 - y) * width) * channel();
        }
        else {
            offset = (x + y * width) * channel();
        }

        return {
            data[offset + 0] / 255.f,
            data[offset + 1] / 255.f,
            data[offset + 2] / 255.f,
        };
    }


    void Image::set_pixel(int x, int y, colorf const& color, bool flip) {
        if (x < 0 || x >= width) return;
        if (y < 0 || y >= height) return;
        int offset;
        if (flip) {
            offset = (x + (height - 1 - y) * width) * channel();
        }
        else {
            offset = (x + y * width) * channel();
        }
        data[offset + 0] = clamp(color.r, 0.0f, 1.0f) * 255;
        data[offset + 1] = clamp(color.g, 0.0f, 1.0f) * 255;
        data[offset + 2] = clamp(color.b, 0.0f, 1.0f) * 255;
    }

    void Image::write_PNG(std::string const& path) {
        stbi_flip_vertically_on_write(false);
        stbi_write_png(path.c_str(), width, height, channel(), data, 0);
    }

    // From https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm.

    const int CODE_INSIDE = 0; // 0000
    const int CODE_LEFT = 1; // 0001
    const int CODE_RIGHT = 2; // 0010
    const int CODE_BOTTOM = 4; // 0100
    const int CODE_TOP = 8; // 1000

    // Compute the bit code for a point (x, y) using the clip rectangle
    // bounded diagonally by (xmin, ymin), and (xmax, ymax)
    // ASSUME THAT xmax, xmin, ymax and ymin are global constants.
    static int compute_out_code(float2 const& v, float2 const& min, float2 const& max) {
        int code = CODE_INSIDE; // initialised as being inside of clip window

        if (v.x < min.x)        // to the left of clip window
            code |= CODE_LEFT;
        else if (v.x > max.x)   // to the right of clip window
            code |= CODE_RIGHT;
        if (v.y < min.y)        // below the clip window
            code |= CODE_BOTTOM;
        else if (v.y > max.y)   // above the clip window
            code |= CODE_TOP;

        return code;
    }

    // Cohen–Sutherland clipping algorithm clips a line from
    // P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with 
    // diagonal from (xmin, ymin) to (xmax, ymax).
    static bool cohen_sutherland_line_clip(float2& v0, float2& v1, float2 const& min, float2 const& max) {
        // compute outcodes for P0, P1, and whatever point lies outside the clip rectangle
        int outcode0 = compute_out_code(v0, min, max);
        int outcode1 = compute_out_code(v1, min, max);
        bool accept = false;

        while (true) {
            if (!(outcode0 | outcode1)) {
                // bitwise OR is 0: both points inside window; trivially accept and exit loop
                accept = true;
                break;
            }
            else if (outcode0 & outcode1) {
                // bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, TOP,
                // or BOTTOM), so both must be outside window; exit loop (accept is false)
                break;
            }
            else {
                // failed both tests, so calculate the line segment to clip
                // from an outside point to an intersection with clip edge
                float x, y;

                // At least one endpoint is outside the clip rectangle; pick it.
                int out_code_out = outcode1 > outcode0 ? outcode1 : outcode0;

                // Now find the intersection point;
                // use formulas:
                //   slope = (y1 - y0) / (x1 - x0)
                //   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
                //   y = y0 + slope * (xm - x0), where xm is xmin or xmax
                // No need to worry about divide-by-zero because, in each case, the
                // outcode bit being tested guarantees the denominator is non-zero
                if (out_code_out & CODE_TOP) {         // point is above the clip window
                    x = v0.x + (v1.x - v0.x) * (max.y - v0.y) / (v1.y - v0.y);
                    y = max.y;
                }
                else if (out_code_out & CODE_BOTTOM) { // point is below the clip window
                    x = v0.x + (v1.x - v0.x) * (min.y - v0.y) / (v1.y - v0.y);
                    y = min.y;
                }
                else if (out_code_out & CODE_RIGHT) {  // point is to the right of clip window
                    y = v0.y + (v1.y - v0.y) * (max.x - v0.x) / (v1.x - v0.x);
                    x = max.x;
                }
                else if (out_code_out & CODE_LEFT) {   // point is to the left of clip window
                    y = v0.y + (v1.y - v0.y) * (min.x - v0.x) / (v1.x - v0.x);
                    x = min.x;
                }

                // Now we move outside point to intersection point to clip
                // and get ready for next pass.
                if (out_code_out == outcode0) {
                    v0.x = x;
                    v0.y = y;
                    outcode0 = compute_out_code(v0, min, max);
                }
                else {
                    v1.x = x;
                    v1.y = y;
                    outcode1 = compute_out_code(v1, min, max);
                }
            }
        }
        return accept;
    }

    void Image::draw_line(int2 const& v0, int2 const& v1, colorf const& color) {

        auto min = float2{ itof(0), itof(0) };
        auto max = float2{ itof(width - 1), itof(height - 1) };
        auto v0f = float2{ itof(v0.x), itof(v0.y) };
        auto v1f = float2{ itof(v1.x), itof(v1.y) };

        if (!cohen_sutherland_line_clip(v0f, v1f, min, max)) return;

        auto x0 = ftoi(v0f.x);
        auto x1 = ftoi(v1f.x);
        auto y0 = ftoi(v0f.y);
        auto y1 = ftoi(v1f.y);

        int dx = abs(x1 - x0);
        int dy = -abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        int e2;

        while (true) {
            set_pixel(x0, y0, color);

            if (x0 == x1 && y0 == y1) break;

            e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    ImageFloat::ImageFloat(int w, int h)
        : width(w)
        , height(h) {
        int size = w * h * channel();
        data = new dataType[size];
        memset(data, 0, size * sizeof(dataType));
    }

    ImageFloat::~ImageFloat() {
        delete data;
    }

    void ImageFloat::fill(colorf const& color) {
        int size = width * height;
        for (int i = 0; i < size; ++i) {
            int offset = i * channel();
            data[offset + 0] = color.r;
            data[offset + 1] = color.g;
            data[offset + 2] = color.b;
        }
    }

    colorf ImageFloat::color_at(int x, int y, bool flip) {
        x = clamp(x, 0, width - 1);
        y = clamp(y, 0, height - 1);
        int offset;
        if (flip) {
            offset = (x + (height - 1 - y) * width) * channel();
        }
        else {
            offset = (x + y * width) * channel();
        }

        return {
            data[offset + 0],
            data[offset + 1],
            data[offset + 2],
        };
    }


    void ImageFloat::set_pixel(int x, int y, colorf const& color, bool flip) {
        if (x < 0 || x >= width) return;
        if (y < 0 || y >= height) return;
        int offset;
        if (flip) {
            offset = (x + (height - 1 - y) * width) * channel();
        }
        else {
            offset = (x + y * width) * channel();
        }
        data[offset + 0] = color.r;
        data[offset + 1] = color.g;
        data[offset + 2] = color.b;
    }

    void ImageFloat::increment_pixel(int x, int y, colorf const& color, bool flip) {
        if (x < 0 || x >= width) return;
        if (y < 0 || y >= height) return;
        int offset;
        if (flip) {
            offset = (x + (height - 1 - y) * width) * channel();
        }
        else {
            offset = (x + y * width) * channel();
        }
        data[offset + 0] += color.r;
        data[offset + 1] += color.g;
        data[offset + 2] += color.b;
    }

} // namespace tira
