#include <iostream>
#include <Magick++.h>
#include <vector>
#include <stdexcept>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;
using namespace Magick;
inline constexpr bool debug_enabled = true;

// Struttura per contenere le opzioni del programma
struct ProgramOptions
{
    std::string inputFile = "immagine.cr3";
    std::string outputFile = "output_negativo.png";
    double targetLuma = 0.45;
    double contrastThreshold = 0.20;
    double gamma = 1.225;
    double levelMax = 0.98;
    double levelGamma = 1.2;
    bool applyNegate = false;
    bool keepColor = false;
};

template <typename... Args>
void debug(Args &&...args)
{
    if constexpr (debug_enabled)
    {
        (std::cerr << ... << args) << '\n';
    }
}

double computeBrightnessModulate(
    double grayMean,
    double grayStdDev,
    double targetLuma = 0.45,
    double contrastThreshold = 0.20,
    double maxBoostPercent = 150.0)
{
    // Evita problemi se grayMean è troppo basso
    if (grayMean < 1e-5)
        return maxBoostPercent;

    // Se l'immagine ha già un buon contrasto, abbassa leggermente il target
    if (grayStdDev > contrastThreshold)
    {
        targetLuma -= 0.03; // oppure usa una formula più dinamica se vuoi
    }

    // Calcolo del fattore, con esponente per ammorbidire
    double factor = std::pow((targetLuma / grayMean), 0.75);

    // Conversione in percentuale e clamp
    return std::min(factor * 100.0, maxBoostPercent);
}

// Funzione per mostrare l'uso del programma
void showUsage(const char *programName)
{
    std::cerr << "Uso: " << programName << " [opzioni]\n"
              << "Opzioni:\n"
              << "  -i, --input=FILE       File di input (default: immagine.cr3)\n"
              << "  -o, --output=FILE      File di output (default: output_negativo.png)\n"
              << "  -n, --negate           Applica negazione dell'immagine\n"
              << "  -t, --target=VAL       Luminosità target (default: 0.45)\n"
              << "  -g, --gamma=VAL        Valore gamma (default: 1.225)\n"
              << "  -l, --level-max=VAL    Valore massimo per level (default: 0.98)\n"
              << "  -k, --level-gamma=VAL  Gamma per level (default: 1.2)\n"
              << "  -h, --help             Mostra questo messaggio\n";
}

// Funzione per il parsing degli argomenti
ProgramOptions parseArguments(int argc, char *argv[])
{
    ProgramOptions options;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            showUsage(argv[0]);
            exit(0);
        }
        else if (arg == "-n" || arg == "--negate")
        {
            options.applyNegate = true;
        }
        else if (arg.find("--input=") == 0)
        {
            options.inputFile = arg.substr(8);
        }
        else if (arg.find("-i=") == 0)
        {
            options.inputFile = arg.substr(3);
        }
        else if (arg.find("--output=") == 0)
        {
            options.outputFile = arg.substr(9);
        }
        else if (arg.find("-o=") == 0)
        {
            options.outputFile = arg.substr(3);
        }
        else if (arg.find("--target=") == 0)
        {
            options.targetLuma = std::stod(arg.substr(9));
        }
        else if (arg.find("-t=") == 0)
        {
            options.targetLuma = std::stod(arg.substr(3));
        }
        else if (arg.find("--gamma=") == 0)
        {
            options.gamma = std::stod(arg.substr(8));
        }
        else if (arg.find("-g=") == 0)
        {
            options.gamma = std::stod(arg.substr(3));
        }
        else if (arg.find("--level-max=") == 0)
        {
            options.levelMax = std::stod(arg.substr(12));
        }
        else if (arg.find("-l=") == 0)
        {
            options.levelMax = std::stod(arg.substr(3));
        }
        else if (arg.find("--level-gamma=") == 0)
        {
            options.levelGamma = std::stod(arg.substr(14));
        }
        else if (arg.find("-k=") == 0)
        {
            options.levelGamma = std::stod(arg.substr(3));
        }
        else if (arg == "-c" || arg == "--color")
        {
            options.keepColor = true;
        }
        else
        {
            // Se non è un'opzione riconosciuta, potrebbe essere un file di input/output posizionale
            if (options.inputFile == "immagine.cr3")
            {
                options.inputFile = arg;
            }
            else if (options.outputFile == "output_negativo.png")
            {
                options.outputFile = arg;
            }
            else
            {
                std::cerr << "Opzione non riconosciuta: " << arg << "\n";
                showUsage(argv[0]);
                exit(1);
            }
        }
    }
    // Informazioni sulle opzioni
    debug("File di input: ", options.inputFile);
    debug("File di output: ", options.outputFile);
    debug("Target luma: ", options.targetLuma);
    debug("Gamma: ", options.gamma);
    debug("Level max: ", options.levelMax);
    debug("Level gamma: ", options.levelGamma);
    debug("Applica negazione: ", options.applyNegate ? "si" : "no");
    debug("Mantieni i colori: ", options.keepColor ? "si" : "no");

    return options;
}

int main(int argc, char *argv[])
{
    try
    {
        InitializeMagick(nullptr);

        // Parsing degli argomenti da riga di comando
        ProgramOptions options = parseArguments(argc, argv);

        Image originalImage(options.inputFile);

        // Crea una copia in scala di grigi per le statistiche
        Image grayImage = originalImage;
        grayImage.colorSpace(GRAYColorspace);

        // Applica negazione se richiesto
        if (options.applyNegate)
        {
            grayImage.negate();
        }

        grayImage.level(0.0, options.levelMax * QuantumRange, options.levelGamma);
        grayImage.gamma(options.gamma);

        // Calcola le statistiche per l'auto-aggiustamento
        ImageStatistics stats = grayImage.statistics();
        ChannelStatistics grayStats = stats.channel(RedPixelChannel);

        double grayMean = grayStats.mean() / QuantumRange;
        double grayStd = grayStats.standardDeviation() / QuantumRange;

        debug("Gray mean (luma): ", grayMean);
        debug("Gray stdev (contrast): ", grayStd);

        // Calcola e applica brightness modulate
        double brightness = computeBrightnessModulate(grayMean, grayStd, options.targetLuma, options.contrastThreshold);
        debug("Brightness modulate: ", brightness);

        // Ora applica le trasformazioni all'immagine originale
        Image finalImage = originalImage;

        // Applica negazione all'immagine originale se richiesto
        if (options.applyNegate)
        {
            finalImage.negate();
        }

        // Applica le regolazioni all'immagine finale
        finalImage.level(0.0, options.levelMax * QuantumRange, options.levelGamma);
        finalImage.gamma(options.gamma);
        finalImage.modulate(brightness, 100.0, 100.0);
        finalImage.contrast(true);

        // Converti in scala di grigi l'immagine finale se non si vuole mantenere il colore
        if (!options.keepColor)
        {
            finalImage.colorSpace(GRAYColorspace);
        }

        // Salva l'immagine elaborata
        finalImage.write(options.outputFile);
        debug("Immagine elaborata salvata in: ", options.outputFile);
    }

    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
