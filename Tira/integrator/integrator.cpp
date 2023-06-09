//
// Created by Ziyi.Lu 2023/03/20
//

#include <integrator/integrator.h>

#define POISSON_POINTS_NUM 32

namespace tira {

    Integrator::Integrator() {
        poisson_disk = generate_poisson_dist(POISSON_POINTS_NUM);
    }

    void Integrator::render(Image& image, Scene const& scene, int spp) {

#ifdef _OPENMP
        std::cout << "[Tira] OpenMP max threads: " << omp_get_max_threads() << "\n";
#endif
        std::cout << "[Tira] SPP: " << spp << " Width: " << scene.scr_w << " Height: " << scene.scr_h << "\n";

        ImageFloat buffer(scene.scr_w, scene.scr_h);

        timer.reset();
        for (int s = 0; s < spp; ++s) {
            for (int y = 0; y < image.height; ++y) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
                for (int x = 0; x < image.width; ++x) {
                    auto color = get_pixel_color(x, y, s, scene);
                    if (isfinite(color)) {
                        color = clamp(color, clamp_min, clamp_max);
                        buffer.increment_pixel(x, y, color);
                    }
                }
            }
            timer.update();
            print_progress2(s, spp, timer.delta_time(), timer.total_time());
        }
        print_progress2(spp - 1, spp, timer.delta_time(), timer.total_time());
        auto elapsed = timer.total_time();
        std::cout << "\n[Tira] Total time: " << elapsed << "s\n";

        for (int y = 0; y < image.height; ++y) for (int x = 0; x < image.width; ++x) {
            auto color = buffer.color_at(x, y);
            // color = reinhard_tone_mapping(color / spp);
            // color = ACES_tone_mapping(color / spp);
            color = gamma_correction(color / spp);
            color = saturate(color);
            image.set_pixel(x, y, color);
        }
    }

    void Integrator::render_N_samples(ImageFloat& image, Scene const& scene, int spp, int integrated_spp) {
        for (int y = 0; y < image.height; ++y) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int x = 0; x < image.width; ++x) {
                colorf color;
                for (int s = 0; s < spp; ++s) {
                    auto c = get_pixel_color(x, y, s, scene);
                    if (isfinite(c)) {
                        c = clamp(c, clamp_min, clamp_max);
                        color += c;
                    }
                }
                color = color / spp;

                auto color_prev = image.color_at(x, y);
                float alpha = (float)spp / (spp + integrated_spp);
                color = color_prev * (1 - alpha) + color * alpha;
                image.set_pixel(x, y, color);
            }
        }
    }
} // namespace tira
