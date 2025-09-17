#include <catch2/catch_all.hpp>
#include "../documents/pdf/flx_pdf_sio.h"
#include "../documents/layout/flx_layout_vertex.h"
#include <filesystem>
#include <fstream>
#include <iostream>

SCENARIO("Real PDF generation with complex nested layout") {
    GIVEN("A complete business form with nested structure") {
        flx_pdf_sio pdf_doc;
        
        WHEN("Creating a professional invoice layout") {
            auto& page = pdf_doc.add_page();
            page.x = 0.0;
            page.y = 0.0;
            page.width = 595.0;  // A4 width
            page.height = 842.0; // A4 height
            
            // === HEADER SECTION (Container) ===
            flx_layout_geometry header_section;
            header_section.x = 50.0;
            header_section.y = 750.0;
            header_section.width = 495.0;
            header_section.height = 80.0;
            header_section.fill_color = "#E8F4F8";
            
            // Header background (light blue rectangle)
            flx_layout_vertex h1(50.0, 750.0);
            flx_layout_vertex h2(545.0, 750.0);
            flx_layout_vertex h3(545.0, 830.0);
            flx_layout_vertex h4(50.0, 830.0);
            header_section.vertices.push_back(h1);
            header_section.vertices.push_back(h2);
            header_section.vertices.push_back(h3);
            header_section.vertices.push_back(h4);
            
            // Company title in header
            flx_layout_text company_title;
            company_title.x = 70.0;
            company_title.y = 800.0;
            company_title.width = 200.0;
            company_title.height = 25.0;
            company_title.text = "ACME Corporation";
            company_title.font_family = "Arial";
            company_title.font_size = 18.0;
            company_title.bold = true;
            header_section.add_text(company_title);
            
            // Invoice number in header
            flx_layout_text invoice_number;
            invoice_number.x = 350.0;
            invoice_number.y = 800.0;
            invoice_number.width = 150.0;
            invoice_number.height = 20.0;
            invoice_number.text = "Invoice #INV-2024-001";
            invoice_number.font_size = 12.0;
            invoice_number.bold = true;
            header_section.add_text(invoice_number);
            
            // Company logo placeholder (we'll download a real image)
            flx_layout_image company_logo;
            company_logo.x = 450.0;
            company_logo.y = 770.0;
            company_logo.width = 80.0;
            company_logo.height = 40.0;
            company_logo.image_path = "/tmp/company_logo.png";
            company_logo.description = "Company Logo";
            company_logo.mime_type = "image/png";
            header_section.add_image(company_logo);
            
            page.add_sub_geometry(header_section);
            
            // === CUSTOMER INFO SECTION ===
            flx_layout_geometry customer_section;
            customer_section.x = 50.0;
            customer_section.y = 620.0;
            customer_section.width = 240.0;
            customer_section.height = 100.0;
            customer_section.fill_color = "#F5F5F5";
            
            // Customer section background
            flx_layout_vertex c1(50.0, 620.0);
            flx_layout_vertex c2(290.0, 620.0);
            flx_layout_vertex c3(290.0, 720.0);
            flx_layout_vertex c4(50.0, 720.0);
            customer_section.vertices.push_back(c1);
            customer_section.vertices.push_back(c2);
            customer_section.vertices.push_back(c3);
            customer_section.vertices.push_back(c4);
            
            // Bill To header
            flx_layout_text bill_to_header;
            bill_to_header.x = 60.0;
            bill_to_header.y = 700.0;
            bill_to_header.text = "BILL TO:";
            bill_to_header.font_size = 10.0;
            bill_to_header.bold = true;
            customer_section.add_text(bill_to_header);
            
            // Customer details
            flx_layout_text customer_name;
            customer_name.x = 60.0;
            customer_name.y = 680.0;
            customer_name.text = "John Smith";
            customer_name.font_size = 12.0;
            customer_section.add_text(customer_name);
            
            flx_layout_text customer_address;
            customer_address.x = 60.0;
            customer_address.y = 660.0;
            customer_address.text = "123 Main Street";
            customer_address.font_size = 10.0;
            customer_section.add_text(customer_address);
            
            flx_layout_text customer_city;
            customer_city.x = 60.0;
            customer_city.y = 645.0;
            customer_city.text = "New York, NY 10001";
            customer_city.font_size = 10.0;
            customer_section.add_text(customer_city);
            
            page.add_sub_geometry(customer_section);
            
            // === PAYMENT INFO SECTION ===
            flx_layout_geometry payment_section;
            payment_section.x = 320.0;
            payment_section.y = 620.0;
            payment_section.width = 225.0;
            payment_section.height = 100.0;
            
            // Payment info with decorative border (triangle pattern)
            flx_layout_vertex p1(320.0, 620.0);
            flx_layout_vertex p2(545.0, 620.0);
            flx_layout_vertex p3(545.0, 720.0);
            flx_layout_vertex p4(320.0, 720.0);
            payment_section.vertices.push_back(p1);
            payment_section.vertices.push_back(p2);
            payment_section.vertices.push_back(p3);
            payment_section.vertices.push_back(p4);
            payment_section.fill_color = "#FFF9E6";
            
            // Payment details
            flx_layout_text payment_header;
            payment_header.x = 330.0;
            payment_header.y = 700.0;
            payment_header.text = "PAYMENT INFO:";
            payment_header.font_size = 10.0;
            payment_header.bold = true;
            payment_section.add_text(payment_header);
            
            flx_layout_text due_date;
            due_date.x = 330.0;
            due_date.y = 680.0;
            due_date.text = "Due Date: 2024-02-15";
            due_date.font_size = 10.0;
            payment_section.add_text(due_date);
            
            flx_layout_text payment_terms;
            payment_terms.x = 330.0;
            payment_terms.y = 660.0;
            payment_terms.text = "Terms: Net 30";
            payment_terms.font_size = 10.0;
            payment_section.add_text(payment_terms);
            
            // Decorative corner triangle
            flx_layout_geometry corner_decoration;
            corner_decoration.fill_color = "#E0E0E0";
            
            flx_layout_vertex t1(520.0, 700.0);
            flx_layout_vertex t2(545.0, 700.0);
            flx_layout_vertex t3(545.0, 720.0);
            corner_decoration.vertices.push_back(t1);
            corner_decoration.vertices.push_back(t2);
            corner_decoration.vertices.push_back(t3);
            
            payment_section.add_sub_geometry(corner_decoration);
            page.add_sub_geometry(payment_section);
            
            // === ITEMS TABLE SECTION ===
            flx_layout_geometry table_section;
            table_section.x = 50.0;
            table_section.y = 400.0;
            table_section.width = 495.0;
            table_section.height = 180.0;
            
            // Table header background
            flx_layout_geometry table_header;
            table_header.fill_color = "#F0F0F0";
            
            flx_layout_vertex th1(50.0, 560.0);
            flx_layout_vertex th2(545.0, 560.0);
            flx_layout_vertex th3(545.0, 580.0);
            flx_layout_vertex th4(50.0, 580.0);
            table_header.vertices.push_back(th1);
            table_header.vertices.push_back(th2);
            table_header.vertices.push_back(th3);
            table_header.vertices.push_back(th4);
            
            // Table headers
            flx_layout_text header_item;
            header_item.x = 60.0;
            header_item.y = 567.0;
            header_item.text = "Item";
            header_item.font_size = 10.0;
            header_item.bold = true;
            table_header.add_text(header_item);
            
            flx_layout_text header_qty;
            header_qty.x = 300.0;
            header_qty.y = 567.0;
            header_qty.text = "Qty";
            header_qty.font_size = 10.0;
            header_qty.bold = true;
            table_header.add_text(header_qty);
            
            flx_layout_text header_price;
            header_price.x = 380.0;
            header_price.y = 567.0;
            header_price.text = "Price";
            header_price.font_size = 10.0;
            header_price.bold = true;
            table_header.add_text(header_price);
            
            flx_layout_text header_total;
            header_total.x = 480.0;
            header_total.y = 567.0;
            header_total.text = "Total";
            header_total.font_size = 10.0;
            header_total.bold = true;
            table_header.add_text(header_total);
            
            table_section.add_sub_geometry(table_header);
            
            // Table rows (nested containers for each row)
            for (int row = 0; row < 3; row++) {
                flx_layout_geometry table_row;
                double row_y = 530.0 - (row * 25.0);
                
                // Alternating row colors
                if (row % 2 == 0) {
                    table_row.fill_color = "#FFFFFF";
                } else {
                    table_row.fill_color = "#F8F9FA";
                }
                
                flx_layout_vertex r1(50.0, row_y);
                flx_layout_vertex r2(545.0, row_y);
                flx_layout_vertex r3(545.0, row_y + 20.0);
                flx_layout_vertex r4(50.0, row_y + 20.0);
                table_row.vertices.push_back(r1);
                table_row.vertices.push_back(r2);
                table_row.vertices.push_back(r3);
                table_row.vertices.push_back(r4);
                
                // Row content
                flx_layout_text item_name;
                item_name.x = 60.0;
                item_name.y = row_y + 7.0;
                item_name.text = (row == 0) ? "Software License" : 
                                (row == 1) ? "Support Package" : "Consulting Hours";
                item_name.font_size = 9.0;
                table_row.add_text(item_name);
                
                flx_layout_text item_qty;
                item_qty.x = 310.0;
                item_qty.y = row_y + 7.0;
                item_qty.text = (row == 0) ? "1" : (row == 1) ? "12" : "8";
                item_qty.font_size = 9.0;
                table_row.add_text(item_qty);
                
                flx_layout_text item_price;
                item_price.x = 380.0;
                item_price.y = row_y + 7.0;
                item_price.text = (row == 0) ? "$500.00" : (row == 1) ? "$50.00" : "$150.00";
                item_price.font_size = 9.0;
                table_row.add_text(item_price);
                
                flx_layout_text item_total;
                item_total.x = 480.0;
                item_total.y = row_y + 7.0;
                item_total.text = (row == 0) ? "$500.00" : (row == 1) ? "$600.00" : "$1200.00";
                item_total.font_size = 9.0;
                table_row.add_text(item_total);
                
                table_section.add_sub_geometry(table_row);
            }
            
            page.add_sub_geometry(table_section);
            
            // === TOTAL SECTION ===
            flx_layout_geometry total_section;
            total_section.x = 350.0;
            total_section.y = 300.0;
            total_section.width = 195.0;
            total_section.height = 80.0;
            total_section.fill_color = "#E8F6F3";
            
            flx_layout_vertex ts1(350.0, 300.0);
            flx_layout_vertex ts2(545.0, 300.0);
            flx_layout_vertex ts3(545.0, 380.0);
            flx_layout_vertex ts4(350.0, 380.0);
            total_section.vertices.push_back(ts1);
            total_section.vertices.push_back(ts2);
            total_section.vertices.push_back(ts3);
            total_section.vertices.push_back(ts4);
            
            // Total calculations
            flx_layout_text subtotal_label;
            subtotal_label.x = 360.0;
            subtotal_label.y = 360.0;
            subtotal_label.text = "Subtotal:";
            subtotal_label.font_size = 10.0;
            total_section.add_text(subtotal_label);
            
            flx_layout_text subtotal_amount;
            subtotal_amount.x = 480.0;
            subtotal_amount.y = 360.0;
            subtotal_amount.text = "$2300.00";
            subtotal_amount.font_size = 10.0;
            total_section.add_text(subtotal_amount);
            
            flx_layout_text tax_label;
            tax_label.x = 360.0;
            tax_label.y = 340.0;
            tax_label.text = "Tax (8.5%):";
            tax_label.font_size = 10.0;
            total_section.add_text(tax_label);
            
            flx_layout_text tax_amount;
            tax_amount.x = 480.0;
            tax_amount.y = 340.0;
            tax_amount.text = "$195.50";
            tax_amount.font_size = 10.0;
            total_section.add_text(tax_amount);
            
            flx_layout_text total_label;
            total_label.x = 360.0;
            total_label.y = 315.0;
            total_label.text = "TOTAL:";
            total_label.font_size = 12.0;
            total_label.bold = true;
            total_section.add_text(total_label);
            
            flx_layout_text total_amount;
            total_amount.x = 480.0;
            total_amount.y = 315.0;
            total_amount.text = "$2495.50";
            total_amount.font_size = 12.0;
            total_amount.bold = true;
            total_section.add_text(total_amount);
            
            page.add_sub_geometry(total_section);
            
            // === FOOTER WITH QR CODE PLACEHOLDER ===
            flx_layout_geometry footer_section;
            footer_section.x = 50.0;
            footer_section.y = 50.0;
            footer_section.width = 495.0;
            footer_section.height = 60.0;
            
            flx_layout_text footer_text;
            footer_text.x = 60.0;
            footer_text.y = 80.0;
            footer_text.text = "Thank you for your business! Payment can be made online at www.acme.com/pay";
            footer_text.font_size = 9.0;
            footer_text.italic = true;
            footer_section.add_text(footer_text);
            
            // QR Code placeholder
            flx_layout_image qr_code;
            qr_code.x = 480.0;
            qr_code.y = 55.0;
            qr_code.width = 50.0;
            qr_code.height = 50.0;
            qr_code.image_path = "/tmp/qr_code.png";
            qr_code.description = "Payment QR Code";
            qr_code.mime_type = "image/png";
            footer_section.add_image(qr_code);
            
            page.add_sub_geometry(footer_section);
            
            // === DEEP NESTED DECORATION SECTION ===
            flx_layout_geometry sidebar_decoration;
            sidebar_decoration.x = 20.0;
            sidebar_decoration.y = 200.0;
            sidebar_decoration.width = 25.0;
            sidebar_decoration.height = 400.0;
            sidebar_decoration.fill_color = "#E8E8E8";
            
            // Sidebar background
            flx_layout_vertex sb1(20.0, 200.0);
            flx_layout_vertex sb2(45.0, 200.0);
            flx_layout_vertex sb3(45.0, 600.0);
            flx_layout_vertex sb4(20.0, 600.0);
            sidebar_decoration.vertices.push_back(sb1);
            sidebar_decoration.vertices.push_back(sb2);
            sidebar_decoration.vertices.push_back(sb3);
            sidebar_decoration.vertices.push_back(sb4);
            
            // Level 2: Color bands within sidebar
            for (int i = 0; i < 4; ++i) {
                flx_layout_geometry color_band;
                double band_y = 220.0 + (i * 90.0);
                color_band.fill_color = (i % 2 == 0) ? "#FFE6E6" : "#E6F3FF";
                
                flx_layout_vertex cb1(22.0, band_y);
                flx_layout_vertex cb2(43.0, band_y);
                flx_layout_vertex cb3(43.0, band_y + 60.0);
                flx_layout_vertex cb4(22.0, band_y + 60.0);
                color_band.vertices.push_back(cb1);
                color_band.vertices.push_back(cb2);
                color_band.vertices.push_back(cb3);
                color_band.vertices.push_back(cb4);
                
                // Level 3: Small dots within each band
                for (int j = 0; j < 2; ++j) {
                    flx_layout_geometry dot;
                    double dot_y = band_y + 15.0 + (j * 30.0);
                    dot.fill_color = (j == 0) ? "#FF6B6B" : "#4ECDC4";
                    
                    // Small diamond shape
                    flx_layout_vertex d1(32.5, dot_y);
                    flx_layout_vertex d2(37.0, dot_y + 4.5);
                    flx_layout_vertex d3(32.5, dot_y + 9.0);
                    flx_layout_vertex d4(28.0, dot_y + 4.5);
                    dot.vertices.push_back(d1);
                    dot.vertices.push_back(d2);
                    dot.vertices.push_back(d3);
                    dot.vertices.push_back(d4);
                    
                    // Level 4: Tiny accent within dot
                    flx_layout_geometry accent;
                    accent.fill_color = "#FFFFFF";
                    
                    flx_layout_vertex a1(31.0, dot_y + 2.0);
                    flx_layout_vertex a2(34.0, dot_y + 2.0);
                    flx_layout_vertex a3(34.0, dot_y + 7.0);
                    flx_layout_vertex a4(31.0, dot_y + 7.0);
                    accent.vertices.push_back(a1);
                    accent.vertices.push_back(a2);
                    accent.vertices.push_back(a3);
                    accent.vertices.push_back(a4);
                    
                    dot.add_sub_geometry(accent);
                    color_band.add_sub_geometry(dot);
                }
                
                sidebar_decoration.add_sub_geometry(color_band);
            }
            
            page.add_sub_geometry(sidebar_decoration);
            
            THEN("Complex nested structure should be built correctly") {
                REQUIRE(pdf_doc.page_count() == 1);
                
                // Verify main page has all sections including new sidebar
                REQUIRE(pdf_doc.pages[0].sub_geometries.size() == 7); // header, customer, payment, table, total, footer, sidebar
                
                // Verify header section has nested content
                auto& header = pdf_doc.pages[0].sub_geometries[0];
                REQUIRE(header.texts.size() == 2); // company title + invoice number
                REQUIRE(header.images.size() == 1); // logo
                REQUIRE(header.vertices.size() == 4); // background rectangle
                
                // Verify customer section
                auto& customer = pdf_doc.pages[0].sub_geometries[1];
                REQUIRE(customer.texts.size() == 4); // bill to header + 3 address lines
                
                // Verify payment section with nested decoration
                auto& payment = pdf_doc.pages[0].sub_geometries[2];
                REQUIRE(payment.texts.size() == 3); // payment info texts
                REQUIRE(payment.sub_geometries.size() == 1); // corner triangle decoration
                
                // Verify table section with nested rows
                auto& table = pdf_doc.pages[0].sub_geometries[3];
                REQUIRE(table.sub_geometries.size() == 4); // header + 3 rows
                
                // Verify each table row has content
                for (size_t i = 1; i < table.sub_geometries.size(); ++i) {
                    REQUIRE(table.sub_geometries[i].texts.size() == 4); // item, qty, price, total
                }
                
                // Verify total section
                auto& total = pdf_doc.pages[0].sub_geometries[4];
                REQUIRE(total.texts.size() == 6); // subtotal, tax, total (labels + amounts)
                
                // Verify footer section
                auto& footer = pdf_doc.pages[0].sub_geometries[5];
                REQUIRE(footer.texts.size() == 1); // footer text
                REQUIRE(footer.images.size() == 1); // QR code
                
                std::cout << "✓ Complex nested layout structure verified!" << std::endl;
            }
            
            AND_WHEN("Downloading real images and generating PDF") {
                // Download a simple test image (placeholder) using picsum.photos
                std::string logo_command = "wget -q -O /tmp/company_logo.png 'https://picsum.photos/80/40' 2>/dev/null || echo 'Image download failed'";
                std::string qr_command = "wget -q -O /tmp/qr_code.png 'https://picsum.photos/50/50' 2>/dev/null || echo 'QR download failed'";
                
                system(logo_command.c_str());
                system(qr_command.c_str());
                
                // Generate the PDF
                std::string output_file = "/tmp/complex_invoice_test.pdf";
                bool success = pdf_doc.write(output_file);
                
                THEN("PDF should be generated successfully") {
                    REQUIRE(success == true);
                    REQUIRE(std::filesystem::exists(output_file));
                    
                    auto file_size = std::filesystem::file_size(output_file);
                    REQUIRE(file_size > 5000); // Should be substantial PDF
                    
                    std::cout << "✓ Complex PDF generated: " << output_file << std::endl;
                    std::cout << "  File size: " << file_size << " bytes" << std::endl;
                    std::cout << "  Contains: Text elements, Polygons, Images, Nested containers" << std::endl;
                    
                    // Open PDF viewer to show the result
                    std::string viewer_command = "which evince > /dev/null 2>&1 && evince '" + output_file + "' 2>/dev/null &";
                    std::string viewer_command2 = "which okular > /dev/null 2>&1 && okular '" + output_file + "' 2>/dev/null &"; 
                    std::string viewer_command3 = "which xdg-open > /dev/null 2>&1 && xdg-open '" + output_file + "' 2>/dev/null &";
                    
                    // Try different PDF viewers
                    if (system(viewer_command.c_str()) != 0) {
                        if (system(viewer_command2.c_str()) != 0) {
                            system(viewer_command3.c_str());
                        }
                    }
                    
                    std::cout << "→ PDF viewer should open showing the complex invoice layout" << std::endl;
                    std::cout << "→ Check for: Header with logo, Customer info, Payment info," << std::endl;
                    std::cout << "  Table with alternating rows, Total calculation, Footer with QR" << std::endl;
                }
            }
        }
    }
}