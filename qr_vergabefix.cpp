/**
 * QR Code Generator for Vergabefix with embedded logo
 */

#include "documents/qr/qrcodegen.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace qrcodegen;
using namespace cv;

int main() {
    try {
        // Generate QR code with HIGH error correction (allows up to 30% damage)
        std::string url = "https://www.vergabefix.de";
        QrCode qr = QrCode::encodeText(url.c_str(), QrCode::Ecc::HIGH);

        std::cout << "QR Code für Vergabefix generiert!" << std::endl;
        std::cout << "  URL: " << url << std::endl;
        std::cout << "  Size: " << qr.getSize() << "x" << qr.getSize() << " modules" << std::endl;

        // Settings
        int qr_size = qr.getSize();
        int margin = 4;
        int module_pixels = 20;
        int total_modules = qr_size + 2 * margin;
        int img_size = total_modules * module_pixels;

        // Create image
        Mat image(img_size, img_size, CV_8UC3);
        image.setTo(Scalar(255, 255, 255));  // White background

        // Helper function to check if position is in finder pattern
        auto is_finder_pattern = [qr_size](int x, int y) -> bool {
            // Top-left finder (0-6, 0-6)
            if (x <= 6 && y <= 6) return true;
            // Top-right finder (size-7 to size-1, 0-6)
            if (x >= qr_size - 7 && y <= 6) return true;
            // Bottom-left finder (0-6, size-7 to size-1)
            if (x <= 6 && y >= qr_size - 7) return true;
            return false;
        };

        // First pass: Draw finder patterns as rounded rectangles
        auto draw_rounded_rect = [&](int x, int y, int w, int h, int r) {
            int px = (x + margin) * module_pixels;
            int py = (y + margin) * module_pixels;
            int width = w * module_pixels;
            int height = h * module_pixels;

            // Draw rounded rectangle using circles and rectangles
            // Corners
            circle(image, Point(px + r, py + r), r, Scalar(0, 0, 0), FILLED);
            circle(image, Point(px + width - r, py + r), r, Scalar(0, 0, 0), FILLED);
            circle(image, Point(px + r, py + height - r), r, Scalar(0, 0, 0), FILLED);
            circle(image, Point(px + width - r, py + height - r), r, Scalar(0, 0, 0), FILLED);

            // Edges
            rectangle(image, Point(px + r, py), Point(px + width - r, py + height), Scalar(0, 0, 0), FILLED);
            rectangle(image, Point(px, py + r), Point(px + width, py + height - r), Scalar(0, 0, 0), FILLED);
        };

        // Draw the three finder patterns (7x7 each)
        int outer_corner_radius = 12;  // Larger radius for outer corners

        // Outer squares (7x7)
        draw_rounded_rect(0, 0, 7, 7, outer_corner_radius);                    // Top-left
        draw_rounded_rect(qr_size - 7, 0, 7, 7, outer_corner_radius);          // Top-right
        draw_rounded_rect(0, qr_size - 7, 7, 7, outer_corner_radius);          // Bottom-left

        // Inner white squares (5x5)
        auto draw_white_rounded_rect = [&](int x, int y, int w, int h, int r) {
            int px = (x + margin) * module_pixels;
            int py = (y + margin) * module_pixels;
            int width = w * module_pixels;
            int height = h * module_pixels;

            circle(image, Point(px + r, py + r), r, Scalar(255, 255, 255), FILLED);
            circle(image, Point(px + width - r, py + r), r, Scalar(255, 255, 255), FILLED);
            circle(image, Point(px + r, py + height - r), r, Scalar(255, 255, 255), FILLED);
            circle(image, Point(px + width - r, py + height - r), r, Scalar(255, 255, 255), FILLED);
            rectangle(image, Point(px + r, py), Point(px + width - r, py + height), Scalar(255, 255, 255), FILLED);
            rectangle(image, Point(px, py + r), Point(px + width, py + height - r), Scalar(255, 255, 255), FILLED);
        };

        int inner_corner_radius = 8;  // Medium radius for inner white squares
        draw_white_rounded_rect(1, 1, 5, 5, inner_corner_radius);
        draw_white_rounded_rect(qr_size - 6, 1, 5, 5, inner_corner_radius);
        draw_white_rounded_rect(1, qr_size - 6, 5, 5, inner_corner_radius);

        // Inner black squares (3x3)
        int innermost_corner_radius = 6;  // Smaller radius for innermost squares
        draw_rounded_rect(2, 2, 3, 3, innermost_corner_radius);
        draw_rounded_rect(qr_size - 5, 2, 3, 3, innermost_corner_radius);
        draw_rounded_rect(2, qr_size - 5, 3, 3, innermost_corner_radius);

        // Second pass: Draw normal modules as circles (skip finder patterns)
        int radius = module_pixels / 2;
        for (int y = 0; y < qr_size; y++) {
            for (int x = 0; x < qr_size; x++) {
                if (qr.getModule(x, y) && !is_finder_pattern(x, y)) {
                    int center_x = (x + margin) * module_pixels + radius;
                    int center_y = (y + margin) * module_pixels + radius;
                    circle(image,
                          Point(center_x, center_y),
                          radius - 2,  // Slightly smaller for clean look
                          Scalar(0, 0, 0),
                          FILLED);
                }
            }
        }

        // Load and embed logo
        std::string logo_path = "/home/fenno/Projects/vergabefix/docs/logo.png";
        Mat logo = imread(logo_path, IMREAD_UNCHANGED);

        if (!logo.empty()) {
            std::cout << "  Logo geladen: " << logo.cols << "x" << logo.rows << " Pixel" << std::endl;

            // Calculate logo size (20% of QR code size)
            double logo_scale = 0.20;
            int logo_size = static_cast<int>(img_size * logo_scale);

            // Resize logo
            Mat logo_resized;
            resize(logo, logo_resized, Size(logo_size, logo_size), 0, 0, INTER_AREA);

            // Add white padding around logo (smaller)
            int padding = 8;
            int logo_with_padding = logo_size + 2 * padding;

            // Calculate center position
            int logo_x = (img_size - logo_with_padding) / 2;
            int logo_y = (img_size - logo_with_padding) / 2;

            // Draw white circle background
            int radius = logo_with_padding / 2;
            int center_x = logo_x + radius;
            int center_y = logo_y + radius;
            circle(image, Point(center_x, center_y), radius, Scalar(255, 255, 255), FILLED);

            // Place logo in center
            int logo_pos_x = logo_x + padding;
            int logo_pos_y = logo_y + padding;

            // Create circular mask for logo
            Mat logo_mask = Mat::zeros(logo_size, logo_size, CV_8UC1);
            int logo_radius = logo_size / 2;
            circle(logo_mask, Point(logo_radius, logo_radius), logo_radius, Scalar(255), FILLED);

            // Blend logo with circular mask
            if (logo_resized.channels() == 4) {
                for (int y = 0; y < logo_size; y++) {
                    for (int x = 0; x < logo_size; x++) {
                        if (logo_mask.at<uchar>(y, x) > 0) {
                            Vec4b logo_pixel = logo_resized.at<Vec4b>(y, x);
                            float alpha = logo_pixel[3] / 255.0f;

                            if (alpha > 0) {
                                int img_x = logo_pos_x + x;
                                int img_y = logo_pos_y + y;

                                if (img_x >= 0 && img_x < img_size && img_y >= 0 && img_y < img_size) {
                                    Vec3b& img_pixel = image.at<Vec3b>(img_y, img_x);
                                    img_pixel[0] = static_cast<uchar>(logo_pixel[0] * alpha + img_pixel[0] * (1 - alpha));
                                    img_pixel[1] = static_cast<uchar>(logo_pixel[1] * alpha + img_pixel[1] * (1 - alpha));
                                    img_pixel[2] = static_cast<uchar>(logo_pixel[2] * alpha + img_pixel[2] * (1 - alpha));
                                }
                            }
                        }
                    }
                }
            } else {
                // No alpha channel - use mask only
                for (int y = 0; y < logo_size; y++) {
                    for (int x = 0; x < logo_size; x++) {
                        if (logo_mask.at<uchar>(y, x) > 0) {
                            int img_x = logo_pos_x + x;
                            int img_y = logo_pos_y + y;
                            if (img_x >= 0 && img_x < img_size && img_y >= 0 && img_y < img_size) {
                                Vec3b logo_pixel = logo_resized.at<Vec3b>(y, x);
                                image.at<Vec3b>(img_y, img_x) = logo_pixel;
                            }
                        }
                    }
                }
            }

            std::cout << "  Logo eingebettet: " << logo_size << "x" << logo_size << " Pixel (20% QR-Code-Größe)" << std::endl;
        } else {
            std::cout << "  Warnung: Logo nicht gefunden, QR-Code ohne Logo erstellt" << std::endl;
        }

        // Global anti-aliasing: stronger Gaussian blur for smooth edges
        Mat blurred;
        GaussianBlur(image, blurred, Size(5, 5), 1.0);
        image = blurred;

        // Save
        std::string output = "vergabefix_qr_with_logo.png";
        if (imwrite(output, image)) {
            std::cout << "\n✓ QR-Code gespeichert: " << output << std::endl;
            std::cout << "  Bildgröße: " << img_size << "x" << img_size << " Pixel" << std::endl;
            std::cout << "  Error Correction: HIGH (30% Toleranz für Logo-Überlagerung)" << std::endl;
            return 0;
        } else {
            std::cerr << "ERROR: Speichern fehlgeschlagen" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
