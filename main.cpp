#include <iostream>
#include <png.h>
#include <libraw/libraw.h>
#include <vector>
#include <stdexcept>

inline constexpr bool debug_enabled = true;

template <typename... Args>
void debug(Args &&...args)
{
    if constexpr (debug_enabled)
    {
        (std::cerr << ... << args) << '\n';
    }
}

std::array<uint32_t, 3> compute_clip_max_per_channel(ushort (*img)[4], int width, int height, double clip_threshold = 0.90)
{
    std::array<std::vector<uint32_t>, 3> histograms;
    for (auto &h : histograms)
        h.resize(65536, 0);

    int total_pixels = width * height;

    // Costruisci gli istogrammi
    for (int i = 0; i < total_pixels; ++i)
    {
        ++histograms[0][img[i][0]];
        ++histograms[1][img[i][1]];
        ++histograms[2][img[i][2]];
    }

    std::array<uint32_t, 3> clip_max = {65535, 65535, 65535};
    uint64_t target = static_cast<uint64_t>(total_pixels * clip_threshold);

    for (int c = 0; c < 3; ++c)
    {
        // Calcola il percentile
        uint64_t cumulative = 0;
        for (int i = 0; i < 65536; ++i)
        {
            cumulative += histograms[c][i];
            if (cumulative >= target)
            {
                // clip_max[c] = std::clamp<uint32_t>(i, 1000, 20000); // Limite inferiore: 1000, limite superiore: 20000
                clip_max[c] = std::max<uint32_t>(i, 1000); // Limite inferiore: 1000
                break;
            }
        }
    }

    return clip_max;
}

float compute_exposure_shift(ushort (*img)[4], int width, int height)
{
    int total_pixels = width * height;

    // Calcola la media dei valori RGB per ogni canale
    std::array<double, 3> means = {0.0, 0.0, 0.0};
    std::array<uint64_t, 3> counts = {0, 0, 0};

    for (int i = 0; i < total_pixels; ++i)
    {
        for (int c = 0; c < 3; ++c)
        {
            if (img[i][c] > 0) // Escludi i neri assoluti
            {
                means[c] += img[i][c];
                counts[c]++;
            }
        }
    }

    for (int c = 0; c < 3; ++c)
    {
        means[c] = counts[c] > 0 ? means[c] / counts[c] : 0.0;
    }

    // Calcola la luminosità media (media dei canali RGB)
    double mean_luminance = (means[0] + means[1] + means[2]) / 3.0;
    if (mean_luminance < 1.0)
        mean_luminance = 1.0; // Evita divisioni per zero

    double target_luminance = 12000.0;
    double exposure_factor = target_luminance / mean_luminance;

    // Converti il fattore in stop (exp_shift è in stop)
    float exp_shift = std::log2(exposure_factor);

    // Limita exp_shift per evitare correzioni estreme
    exp_shift = std::clamp(exp_shift, 0.0f, 6.0f); // Tra 0 e 6 stop

    debug("Mean luminance: ", mean_luminance, ", Exposure factor: ", exposure_factor, ", Exp shift: ", exp_shift);

    return exp_shift;
}

std::array<double, 3> compute_pixel_percentage_below_threshold(ushort (*img)[4], int width, int height, uint32_t threshold)
{
    std::array<std::vector<uint32_t>, 3> histograms;
    for (auto &h : histograms)
        h.resize(65536, 0);

    int total_pixels = width * height;

    // Costruisci gli istogrammi
    for (int i = 0; i < total_pixels; ++i)
    {
        ++histograms[0][img[i][0]];
        ++histograms[1][img[i][1]];
        ++histograms[2][img[i][2]];
    }

    std::array<double, 3> percentages = {0.0, 0.0, 0.0};

    for (int c = 0; c < 3; ++c)
    {
        // Calcola il numero di pixel al di sotto della soglia
        uint64_t cumulative = 0;
        for (int i = 0; i <= threshold && i < 65536; ++i)
        {
            cumulative += histograms[c][i];
        }
        // Calcola la percentuale
        percentages[c] = (double)cumulative / total_pixels;
    }

    return percentages;
}

auto curve_s = [](uint16_t value) -> uint16_t
{
    double x = value / 65535.0;
    double s = 0.5 + 0.5 * x; // curva morbida che apre il centro
    x = std::pow(x, s);
    return static_cast<uint16_t>(std::min(x * 65535.0, 65535.0));
};

void save_as_png(const char *filename, LibRaw &processor)
{
    int width = processor.imgdata.sizes.width;
    int height = processor.imgdata.sizes.height;
    ushort(*img)[4] = processor.imgdata.image;
    if (!img)
    {
        throw std::runtime_error("Errore: immagine non valida dopo dcraw_process");
    }

    // Calcola la percentuale di pixel al di sotto di una soglia
    // uint32_t threshold = 3000;
    // auto percentages = compute_pixel_percentage_below_threshold(img, width, height, threshold);
    // debug("Percentuale di pixel sotto la soglia ", threshold, ": R: ", percentages[0] * 100, "%, G: ", percentages[1] * 100, "%, B: ", percentages[2] * 100, "%");

    // double clip_threshold = (percentages[0] + percentages[1] + percentages[2]) / 3.0;
    // debug("Clip threshold calcolato: ", clip_threshold);

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png)
        throw std::runtime_error("Errore nella creazione della struttura PNG");

    png_infop info = png_create_info_struct(png);
    if (!info)
    {
        png_destroy_write_struct(&png, nullptr);
        throw std::runtime_error("Errore nella creazione della struttura info");
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        png_destroy_write_struct(&png, &info);
        throw std::runtime_error("Errore nell'aprire il file per la scrittura");
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        throw std::runtime_error("Errore durante la scrittura del PNG");
    }

    png_init_io(png, fp);

    png_set_filter(png, 0, PNG_FILTER_NONE);
    png_set_compression_level(png, PNG_COMPRESSION_TYPE_DEFAULT);
    png_set_IHDR(
        png, info,
        width, height,
        16, // 16 bit
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    debug("Calcolo clip_max...");

    std::vector<png_byte> row(width * 6); // 3 canali x 2 byte per canale
    auto clip_max = compute_clip_max_per_channel(img, width, height, 0.95);

    debug("Clip max R: ", clip_max[0], ", G: ", clip_max[1], ", B: ", clip_max[2]);
    debug("Scrittura PNG...");

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            auto &px = img[y * width + x]; // px è ushort[4]

            uint32_t norm_r_32 = (uint32_t(px[0]) * 65535) / clip_max[0];
            uint16_t norm_r = static_cast<uint16_t>(std::min(norm_r_32, 65535u));

            uint32_t norm_g_32 = (uint32_t(px[1]) * 65535) / clip_max[1];
            uint16_t norm_g = static_cast<uint16_t>(std::min(norm_g_32, 65535u));

            uint32_t norm_b_32 = (uint32_t(px[2]) * 65535) / clip_max[2];
            uint16_t norm_b = static_cast<uint16_t>(std::min(norm_b_32, 65535u));

            norm_r = curve_s(norm_r);
            norm_g = curve_s(norm_g);
            norm_b = curve_s(norm_b);

            row[x * 6 + 0] = norm_r >> 8;
            row[x * 6 + 1] = norm_r & 0xFF;
            row[x * 6 + 2] = norm_g >> 8;
            row[x * 6 + 3] = norm_g & 0xFF;
            row[x * 6 + 4] = norm_b >> 8;
            row[x * 6 + 5] = norm_b & 0xFF;

            // Normalizziamo px[n] su 16 bit, invertiamo, e lo riportiamo a ushort
            uint16_t inv_r = 65535 - px[0];
            uint16_t inv_g = 65535 - px[1];
            uint16_t inv_b = 65535 - px[2];
            row[x * 6 + 0] = inv_r >> 8;
            row[x * 6 + 1] = inv_r & 0xFF;
            row[x * 6 + 2] = inv_g >> 8;
            row[x * 6 + 3] = inv_g & 0xFF;
            row[x * 6 + 4] = inv_b >> 8;
            row[x * 6 + 5] = inv_b & 0xFF;
        }

        png_write_row(png, reinterpret_cast<png_bytep>(row.data()));
    }

    png_write_end(png, nullptr);
    fclose(fp);
    png_destroy_write_struct(&png, &info);
}

int main()
{
    try
    {
        LibRaw processor;

        // Apri un file CR3
        if (processor.open_file("immagine.cr3") != LIBRAW_SUCCESS)
        {
            throw std::runtime_error("Errore nell'apertura del file CR3");
        }

        // Estrai i dati RAW
        if (processor.unpack() != LIBRAW_SUCCESS)
        {
            throw std::runtime_error("Errore nell'estrazione dei dati RAW");
        }

        debug("Larghezza: ", processor.imgdata.sizes.width, ", Altezza: ", processor.imgdata.sizes.height, ", Max colori: ", processor.imgdata.rawdata.color.maximum);
        // Imposta i parametri di elaborazione (prima passata, senza exp_shift)
        processor.imgdata.params.user_qual = 10;
        processor.imgdata.params.output_bps = 16;
        processor.imgdata.params.no_auto_bright = 1;
        processor.imgdata.params.use_camera_wb = 1;
        processor.imgdata.params.use_auto_wb = 0;
        processor.imgdata.params.user_flip = 0;
        processor.imgdata.params.gamm[0] = 1.0 / 2.2;
        processor.imgdata.params.gamm[1] = 1.0;
        processor.imgdata.params.highlight = 0;
        processor.imgdata.params.exp_correc = 1;
        processor.imgdata.params.exp_shift = 0.0f; // Nessuna correzione nella prima passata

        // Prima passata: elabora senza exp_shift per calcolare la luminosità
        if (processor.dcraw_process() != LIBRAW_SUCCESS)
        {
            throw std::runtime_error("Errore nella prima elaborazione dell'immagine raw");
        }

        // Calcola l'esposizione dinamica
        ushort(*img)[4] = processor.imgdata.image;
        if (!img)
        {
            throw std::runtime_error("Errore: immagine non valida dopo la prima dcraw_process");
        }

        float exp_shift = compute_exposure_shift(img, processor.imgdata.sizes.width, processor.imgdata.sizes.height);

        // Seconda passata: pulisci, riapri il file, riesegui unpack e applica exp_shift
        processor.recycle(); // Pulisce lo stato precedente

        // Riapri il file RAW
        if (processor.open_file("immagine.cr3") != LIBRAW_SUCCESS)
        {
            throw std::runtime_error("Errore nella riapertura del file CR3");
        }

        // Riesegui unpack
        if (processor.unpack() != LIBRAW_SUCCESS)
        {
            throw std::runtime_error("Errore nella seconda estrazione dei dati RAW");
        }

        // Reimposta i parametri con il nuovo exp_shift
        processor.imgdata.params.user_qual = 10;
        processor.imgdata.params.output_bps = 16;
        processor.imgdata.params.no_auto_bright = 1;
        processor.imgdata.params.use_camera_wb = 1;
        processor.imgdata.params.use_auto_wb = 0;
        processor.imgdata.params.user_flip = 0;
        processor.imgdata.params.gamm[0] = 1.0 / 2.2;
        processor.imgdata.params.gamm[1] = 1.0;
        processor.imgdata.params.highlight = 0;
        processor.imgdata.params.exp_correc = 1;
        processor.imgdata.params.exp_shift = exp_shift;

        // Seconda elaborazione con exp_shift
        if (processor.dcraw_process() != LIBRAW_SUCCESS)
        {
            throw std::runtime_error("Errore nella seconda elaborazione dell'immagine raw");
        }

        // Salva l'immagine negativa come PNG
        save_as_png("output_negativo.png", processor);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
